#include "gpu.h"

#define SCR_WIDTH 800
#define SCR_HEIGHT 600

Gpu* initialize_gpu() {
	Gpu* gpu = malloc(sizeof(Gpu));
       
	gpu->gp1 = 0x14000000;
	gpu->gp0 = 0x00000000;
	gpu->gpu_mode = COMMAND;
	gpu->fifoc = 0;
	gpu->fifolen = 0;
    
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
	// TODO: temporary hack (see 31 and 19 bits), implement timers
	return gpu->gp1 & 0xfff7ffff;
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

void gpu_reset(Gpu* gpu, uint32_t v) {    
	gpu_clear_fifo(gpu, v);
	gpu_ack_irq(gpu, v);
	gpu_set_d(gpu, 0x1);
	gpu_set_dd(gpu, 0x0);
	gpu_set_da(gpu, 0x0);
	gpu_set_hdr(gpu, 0x200 | ((0x200 + 256*10) << 12));
	gpu_set_vdr(gpu, 0x10 | ((0x10 + 240) << 10));
	gpu_set_dm(gpu, 0x0);    
}

void gpu_clear_fifo(Gpu* gpu, uint32_t v) {
	gpu->fifoc = 0;
	gpu->fifolen = 0;
}

void gpu_ack_irq(Gpu* gpu, uint32_t v) {
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
		gpu->gp1 |= (gpu->fifoc != 0 && gpu->fifolen == 0) << 25;
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

void gpu_store16(Gpu* gpu, uint16_t x, uint16_t y, uint16_t v) {
	uint32_t offset = y * 2048 + x * 2;

	gpu->vram[offset + 0] = v;
	gpu->vram[offset + 1] = v >> 8;    
}

void gpu_store32(Gpu* gpu, uint16_t x, uint16_t y, uint16_t v) {
	uint32_t offset = y * 2048 + x * 2;

	gpu->vram[offset + 0] = v;
	gpu->vram[offset + 1] = v >> 8;
	gpu->vram[offset + 2] = v >> 16;
	gpu->vram[offset + 3] = v >> 24;
}

uint32_t gpu_load32(Gpu* gpu, uint16_t x, uint16_t y) {
	uint32_t offset = y * 2048 + x * 2;

	uint32_t b0 = gpu->vram[offset + 0];
	uint32_t b1 = gpu->vram[offset + 1];
	uint32_t b2 = gpu->vram[offset + 2];
	uint32_t b3 = gpu->vram[offset + 3];

	return b0 | ( b1 << 8 ) | ( b2 << 16 ) | ( b3 << 24);
}

uint32_t gpu_offset(Gpu* gpu, uint16_t x, uint16_t y) {    
	return y * 2048 + x;
}

void gpu_gp0_command(Gpu* gpu, uint32_t command) {    
	if(gpu->fifoc == 0) {	
		uint8_t cmd = command >> 24;
		uint8_t len;
		printf("fifo cmd: %x\n", cmd);
		gpu_unblock_block(gpu);
		gpu_block_cmd(gpu);
		switch(cmd) {
		case 0x0:
	    len = 1;
	    break;
		case 0x1:
	    len = 1;
	    break;
		case 0x2:
	    len = 3;
	    break;
		case 0x3:
	    len = 1;
	    break;
		case 0x8:
	    len = 1;
	    break;
		case 0xe1:	    
	    len = 1;	    
	    break;
		case 0xe2:
	    len = 1;
	    break;
		case 0xe3:
	    len = 1;
	    break;
		case 0xe4:
	    len = 1;
	    break;
		case 0xe5:
	    len = 1;
	    break;
		case 0xe6:
	    len = 1;
	    break;
		case 0x28:
	    len = 5;	    	    
	    break;
		case 0xa0:	    
	    len = 3;	    	    
	    break;
		case 0xc0:
	    len = 3;
	    break;
		case 0x38:
			len = 8;
			break;
		case 0x30:
			len = 6;
			break;
		case 0x2c:
			len = 9;
			break;
		default:
	    printf("unhandled fifo cmd: %x %x\n", cmd, command);
	    exit(1);
		}

		gpu->fifolen = len;
		gpu->fifoc = len;
	}
    
	gpu->fifoc--;
    
	switch(gpu->gpu_mode) {
	case COMMAND: {
		printf("push word: %x\n", command);
		gpu->fifo[gpu->fifolen - gpu->fifoc - 1] = command;
		if(gpu->fifoc == 0) {	       
	    uint8_t cmd = gpu->fifo[0] >> 24;
	    gpu->fifolen = 0;
			printf("run gp0 cmd: %x\n", cmd);
	    gpu_unblock_block(gpu);
	    switch(cmd) {
	    case 0x0:
				break;
	    case 0x1:				
				break;
	    case 0x2:
				fill_rec(gpu);
				break;
	    case 0xe1:
				set_draw_mode(gpu);
				break;
	    case 0xe2:
				set_texture_window(gpu);
				break;
	    case 0xe3:
				set_area_top_left(gpu);
				break;
	    case 0xe4:
				set_area_bottom_right(gpu);
				break;
	    case 0xe5:
				set_drawing_offset(gpu);
				break;
	    case 0xe6:
				set_mask_bit(gpu);
				break;
	    case 0x28:	    
				mon4pop(gpu);	    
				break;
	    case 0xa0:		
				cpr_cv(gpu);		
				break;
	    case 0xc0:
				cpr_vc(gpu);
				break;
			case 0x38:
				sh4pop(gpu);
				break;
			case 0x30:
				sh3pop(gpu);
				break;
			case 0x2c:
				tex4popblend(gpu);
				break;
	    default:
				printf("unhandled gp0 command: %x\n", cmd);
				exit(1);
	    }
	    gpu_unblock_cmd(gpu);	
		}
	}
		break;
	case CPU_VRAM: {	    
		vramcv(gpu, command);

		if(gpu->fifoc == 0) {
	    gpu->gpu_mode = COMMAND;	    	
	    gpu->fifolen = 0;	    
		}
	}
		break;
	case VRAM_CPU: {
		vramvc(gpu, command);

		if(gpu->fifoc == 0) {
	    gpu->gpu_mode = COMMAND;
	    gpu->fifolen = 0;
	    /* gpu_block_vram(gpu); */
		}
	}	
		break;
	default:
		printf("unhandled gpu mode\n");
		exit(1);
	}	
                      
}

void gpu_gp1_command(Gpu* gpu, uint32_t command) {    
	uint8_t cmd = command >> 24;
	uint32_t packet = command & 0xffffff;
	switch(cmd) {
	case 0x0:
		gpu_reset(gpu, packet);
		break;
	case 0x1:
		gpu_clear_fifo(gpu, packet);
		break;
	case 0x2:
		gpu_ack_irq(gpu, packet);
		break;
	case 0x3:
		gpu_set_d(gpu, packet);
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

void set_area_top_left(Gpu* gpu) {
	uint16_t x = gpu->fifo[0] & 0x3ff;
	uint16_t y = (gpu->fifo[0] >> 10) & 0x1ff;

	gpu->viewport[0] = x;
	gpu->viewport[1] = y;
}

void set_area_bottom_right(Gpu* gpu) {
	uint16_t x = gpu->fifo[0] & 0x3ff;
	uint16_t y = (gpu->fifo[0] >> 10) & 0x1ff;

	gpu->viewport[2] = x;
	gpu->viewport[3] = y;
}

void set_drawing_offset(Gpu* gpu) {
	int16_t x = gpu->fifo[0] & 0x7ff;
	int16_t y = (gpu->fifo[0] >> 11) & 0xfff;

	gpu->offset[0] = x;
	gpu->offset[1] = y;
}

void set_draw_mode(Gpu* gpu) {    
	gpu->gp1 = (gpu->gp1 & 0xfffff800) | (gpu->fifo[0] & 0x7ff);
	gpu->gp1 = (gpu->gp1 & 0xffff7fff) | (((gpu->fifo[0] >> 11) & 0x1) << 15);
	gpu->gp1 = (gpu->gp1 & 0xffffdfff) | (((gpu->fifo[0] >> 13) & 0x1) << 13);
}

void set_texture_window(Gpu* gpu) {
	gpu->texture_window = gpu->fifo[0] & 0xfffff;
}

void set_mask_bit(Gpu* gpu) {
	gpu->gp1 = (gpu->gp1 & 0xffffe7ff) | ((gpu->fifo[0] & 0x3) << 11);
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

void tex4popblend(Gpu* gpu) {}

void sh4pop(Gpu* gpu) {
	
}

void sh3pop(Gpu* gpu) {

}

void mon4pop(Gpu* gpu) {
    
}

void cpr_cv(Gpu* gpu) {   
	gpu->gpu_mode = CPU_VRAM;    

	uint16_t w = gpu->fifo[2] & 0xffff;
	uint16_t h = gpu->fifo[2] >> 16;

	uint32_t size = w * h;
	size = (size + 1) & ~1;
	gpu->fifolen = size / 2;
	gpu->fifoc = size / 2;	
}

void cpr_vc(Gpu* gpu) {
	gpu_unblock_vram(gpu);
	gpu->gpu_mode = VRAM_CPU;    

	uint16_t w = gpu->fifo[2] & 0xffff;
	uint16_t h = gpu->fifo[2] >> 16;

	uint32_t size = w * h;    
	size = (size + 1) & ~1;
	gpu->fifolen = size / 2;
	gpu->fifoc = size / 2;	
}

void fill_rec(Gpu* gpu) {
	uint32_t color = gpu->fifo[0] & 0x7fff;
	uint16_t x = gpu->fifo[1];
	uint16_t y = gpu->fifo[1] >> 16;
	uint16_t w = gpu->fifo[2];
	uint16_t h = gpu->fifo[2] >> 16;    
    
	for(int i = 0; i < h; ++i) {
		for(int j = 0; j < w; ++j) {
	    gpu_store16(gpu, x + j, y + i, color);
		}
	}
}

void vramcv(Gpu* gpu, uint32_t v) {	
	uint16_t x = gpu->fifo[1] & 0xffff;
	uint16_t y = gpu->fifo[1] >> 16;
	uint16_t w = gpu->fifo[2] & 0xffff;
	uint16_t h = gpu->fifo[2] >> 16; 

	uint32_t transferx = x + (gpu->fifolen - gpu->fifoc) % w;
	uint32_t transfery = y + ceil((gpu->fifolen - gpu->fifoc) / w);	    

	if(transferx == x && gpu->fifoc != gpu->fifolen) {		
		transfery++;
	}
    
	gpu_store32(gpu, transferx, transfery, v);    
}

void vramvc(Gpu* gpu, uint32_t v) {
	uint16_t x = gpu->fifo[1] & 0xffff;
	uint16_t y = gpu->fifo[1] >> 16;
	uint16_t w = gpu->fifo[2] & 0xffff;
	uint16_t h = gpu->fifo[2] >> 16; 

	uint32_t transferx = x + (gpu->fifolen - gpu->fifoc) % w;
	uint32_t transfery = y + ceil((gpu->fifolen - gpu->fifoc) / w);	
	
	if(transferx == x && gpu->fifoc != gpu->fifolen) {		
		transfery++;
	}
    
	uint32_t word = gpu_load32(gpu, transferx, transfery);

	gpu_set_gp0(gpu, word);
}
