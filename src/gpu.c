#include "gpu.h"

#define SCR_WIDTH 800
#define SCR_HEIGHT 600

Gpu* initialize_gpu() {
    Gpu* gpu = malloc(sizeof(Gpu));

    gpu->attrsc = 0;
    gpu->next_attr = 0;
    gpu->render_command = 0;
    gpu->gp1 = 0x14000000;
    gpu->gp0 = 0x00000000;    
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    gpu->window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PS1 emulator", NULL, NULL);
    if (gpu->window == NULL) {
        printf("Failed to create GLFW window");
        glfwTerminate();
        exit(1);
    }
    
    glfwMakeContextCurrent(gpu->window);
    glfwSetFramebufferSizeCallback(gpu->window, framebuffer_size_callback);
    glfwSetInputMode(gpu->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        exit(1);
    }

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    glfwSwapBuffers(gpu->window);
    glfwPollEvents();
    
    return gpu;
}

void gpu_destroy(Gpu* gpu) {
    glfwTerminate();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void gpu_set_gp0(Gpu* gpu, uint32_t v) {
    gpu->gp0 = v;
}

void gpu_set_gp1(Gpu* gpu, uint32_t v) {
    gpu->gp1 = v;
}

uint32_t gpu_gpustat(Gpu* gpu) {
    return gpu->gp1;
}

uint32_t gpu_gpuread(Gpu* gpu) {
    return gpu->gp0;
}

void gpu_block_block(Gpu* gpu) {
    gpu->gp1 &= 0xefffffff;
}

void gpu_unblock_block(Gpu* gpu) {
    gpu->gp1 |= 0x10000000;
}

void gpu_block_cmd(Gpu* gpu) {
    gpu->gp1 &= 0xfbffffff;
}

void gpu_unblock_cmd(Gpu* gpu) {
    gpu->gp1 |= 0x04000000;
}

void gpu_block_vram(Gpu* gpu) {
    gpu->gp1 &= 0xf7ffffff;
}

void gpu_unblock_vram(Gpu* gpu) {
    gpu->gp1 |= 0x8000000;
}

void gpu_reset(Gpu* gpu) {    
    gpu_clear_fifo(gpu);
    gpu_ack_irq(gpu);
    gpu_set_d(gpu, 0x1);
    gpu_set_dd(gpu, 0x0);
    gpu_set_da(gpu, 0x0);
    gpu_set_hdr(gpu, 0x200 | ((0x200 + 256*10) << 12));
    gpu_set_vdr(gpu, 0x10 | ((0x10 + 240) << 10));
    gpu_set_dm(gpu, 0x0);    

    // TODO: also it should set rendering attributes (e.g. E1h...E6h)
    /* for(int i = 0; i < 5; ++i) { */
    /* 	gpu_rendering_attributes(gpu, 0xe1 + i, 0x0);	 */
    /* } */
}

void gpu_clear_fifo(Gpu* gpu) {
    gpu->fifo = 0;
}

void gpu_ack_irq(Gpu* gpu) {
    gpu->gp1 &= 0xfeffffff;
}

void gpu_set_d(Gpu* gpu, uint32_t v) {
    gpu->gp1 = ((gpu->gp1 & 0xff7fffff) | ((v & 0x1) << 23));
}

void gpu_set_dd(Gpu* gpu, uint32_t v) {
    uint8_t dir = v & 0x3;
    gpu->gp1 = (gpu->gp1 & 0x9fffffff) | (dir << 29);

    switch(dir) {
    case 0:
	gpu->gp1 &= 0xfdffffff;
	break;
    case 1:
	// TODO: implement fifo
	gpu->gp1 &= 0xfdffffff;
	break;
    case 2:
	gpu->gp1 = (gpu->gp1 & 0xfdffffff) | ((gpu->gp1 >> 3) & 0x2000000);
	break;
    case 3:
	gpu->gp1 = (gpu->gp1 & 0xfdffffff) | ((gpu->gp1 >> 2) & 0x2000000);
	break;
    default:
	printf("invalid dma direction: %x\n", dir);
	exit(1);
    }
}

void gpu_set_da(Gpu* gpu, uint32_t v) {
    uint16_t x = v & 0x3ff;
    uint16_t y = (v >> 10) & 0x1ff;

    gpu->da_start = gpu_offset(gpu, x, y);
}

void gpu_set_hdr(Gpu* gpu, uint32_t v) {
    uint16_t x1 = v & 0xfff;
    uint16_t x2 = (v >> 12) & 0xfff;

    gpu->hdr = x2 - x1;    
}

void gpu_set_vdr(Gpu* gpu, uint32_t v) {
    uint16_t y1 = v & 0x3ff;
    uint16_t y2 = (v >> 10) & 0x3ff;

    gpu->vdr = y2 - y1;
}

void gpu_set_dm(Gpu* gpu, uint32_t v) {
    gpu->gp1 = (gpu->gp1 & 0xff81ffff) | ((v & 0x3f) << 17);
    gpu->gp1 = (gpu->gp1 & 0xfffeffff) | (((v >> 6) & 0x1) << 16);
    gpu->gp1 = (gpu->gp1 & 0xffffbfff) | (((v >> 7) & 0x1) << 14);    
    
    if(((v >> 5) & 0x1) == 0) {	
	gpu->gp1 |= 0x2000;	
    }
}

void gpu_store32(Gpu* gpu, uint32_t offset, uint32_t v) {
    gpu->vram[offset + 0] = v;
    gpu->vram[offset + 1] = v >> 8;
    gpu->vram[offset + 2] = v >> 16;
    gpu->vram[offset + 3] = v >> 24;
}

uint32_t gpu_offset(Gpu* gpu, uint16_t x, uint16_t y) {    
    return y * 2048 + x;
}

void gpu_gp0_command(Gpu* gpu, uint32_t command) {
    if(gpu->attrsc != 0) {
	printf("proceed attr\n");
	gpu_proceed_arg(gpu, command);
	return;
    }    

    /* gpu_unblock_vram(gpu); */
    gpu_unblock_block(gpu);
    gpu_block_cmd(gpu);
    uint8_t cmd = command >> 24;
    uint32_t v = command & 0xffffff;
    printf("gp0 command: %x %x\n", cmd, v);

    if(cmd == 0) {
	printf("GPU NOP\n");
	gpu_unblock_cmd(gpu);
	return;	
    }
    
    if(range_contains(RENDERING_ATTRIBUTES, cmd) == 1) {	
	gpu_unblock_cmd(gpu);
	gpu_rendering_attributes(gpu, cmd, v);	
	return;
    }

    if(range_contains(RENDER_POLYGONES, cmd) == 1) {	
	gpu_render_polygones(gpu, cmd, v);
	return;
    }

    if(range_contains(RENDER_LINES, cmd) == 1) {	
	gpu_render_lines(gpu, cmd, v);
	return;
    }

    printf("unhandled gpu rendering: %x %x\n", cmd, command);
    exit(1);
}

void gpu_gp1_command(Gpu* gpu, uint32_t command) {    
    uint8_t cmd = command >> 24;
    uint32_t packet = command & 0xffffff;
    printf("gp1 command: %x %x\n", cmd, packet);
    switch(cmd) {
    case 0x0:
	gpu_reset(gpu);
	break;
    case 0x4:
	gpu_set_dd(gpu, packet);
	break;
    case 0x5:
	gpu_set_da(gpu, packet);
	break;
    case 0x6:
	gpu_set_hdr(gpu, packet);
	break;
    case 0x7:
	gpu_set_vdr(gpu, packet);
	break;
    case 0x8:
	gpu_set_dm(gpu, packet);
	break;
    default:
	printf("unhandled gp1 cmd: %x %x\n", cmd, command);
	exit(1);
    }
}

void gpu_proceed_arg(Gpu* gpu, uint32_t arg) {
    gpu->attrs[gpu->next_attr] = arg;
    gpu->next_attr++;
    gpu->attrsc--;

    if(gpu->attrsc == 0) {
	gpu->next_attr = 0;
	gpu_block_block(gpu);
	gpu_unblock_cmd(gpu);
	
	gpu_render(gpu);
    }
}

void gpu_render(Gpu* gpu) {
    
    switch(gpu->render_command) {
    case 0x28:
	mon4pop(gpu);
	break;
    default:
	printf("invalid render command: %x\n", gpu->render_command);
	exit(1);
    }

    /* free(gpu->attrs); */
    gpu->render_command = 0;
}

void gpu_rendering_attributes(Gpu* gpu, uint8_t command, uint32_t v) {
    switch(command) {
    case 0xe1:
	set_draw_mode(gpu, v);
	break;
    case 0xe2:
	set_texture_window(gpu, v);
	break;
    case 0xe3:
	set_area_top_left(gpu, v);
	break;
    case 0xe4:
	set_area_bottom_right(gpu, v);
	break;
    case 0xe5:
	set_drawing_offset(gpu, v);
	break;
    case 0xe6:
	set_mask_bit(gpu, v);
	break;
    default:
	printf("invalid redering attribute command: %x\n", command);
	exit(1);
    }
}

void gpu_render_polygones(Gpu* gpu, uint8_t command, uint32_t v) {
    gpu_block_block(gpu);
    switch(command) {
    case 0x28:
	gpu->attrsc = 4;
	gpu->attrs = realloc(gpu->attrs, sizeof(uint32_t) * 4);
	gpu->render_command = command;
	break;
    default:
	printf("invalid render polygones command: %x\n", command);
	exit(1);
    }
}

void gpu_render_lines(Gpu* gpu, uint8_t command, uint32_t v) {
    gpu_block_block(gpu);
    switch(command) {
	
    default:
	printf("invalid render lines command: %x\n", command);
	exit(1);
    }
}

void set_area_top_left(Gpu* gpu, uint32_t v) {
    uint16_t x = v & 0x3ff;
    uint16_t y = (v >> 10) & 0x1ff;

    gpu->viewport[0] = x;
    gpu->viewport[1] = y;
}

void set_area_bottom_right(Gpu* gpu, uint32_t v) {
    uint16_t x = v & 0x3ff;
    uint16_t y = (v >> 10) & 0x1ff;

    gpu->viewport[2] = x;
    gpu->viewport[3] = y;
}

void set_drawing_offset(Gpu* gpu, uint32_t v) {
    int16_t x = v & 0x7ff;
    int16_t y = (v >> 11) & 0xfff;

    gpu->offset[0] = x;
    gpu->offset[1] = y;
}

void set_draw_mode(Gpu* gpu, uint32_t v) {
    /* gpu->draw_mode = v & 0x3fff; */
    gpu->gp1 = (gpu->gp1 & 0xfffff800) | (v & 0x7ff);
    gpu->gp1 = (gpu->gp1 & 0xffff7fff) | (((v >> 11) & 0x1) << 15);
    gpu->gp1 = (gpu->gp1 & 0xffffdfff) | (((v >> 13) & 0x1) << 13);
}

void set_texture_window(Gpu* gpu, uint32_t v) {
    gpu->texture_window = v & 0xfffff;
}

void set_mask_bit(Gpu* gpu, uint32_t v) {
    gpu->gp1 = (gpu->gp1 & 0xffffe7ff) | ((v & 0x3) << 11);
}

uint8_t* attr_color(Gpu* gpu, uint32_t v) {
    // r g b
    uint8_t* color = malloc(sizeof(uint8_t) * 3);
    color[0] = v;
    color[1] = v >> 8;
    color[2] = v >> 16;
    return color;
}

int16_t* attr_vertex(Gpu* gpu, uint32_t v) {
    int16_t* vertex = malloc(sizeof(uint16_t) * 2);
    vertex[0] = v & 0x7ff;
    vertex[1] = (v >> 16) & 0xfff;
    return vertex;
}

void mon4pop(Gpu* gpu) {
    
}

