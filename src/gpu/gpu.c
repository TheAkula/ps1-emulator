#include "gpu.h"

#include "shader.h"

#define SCR_WIDTH 640
#define SCR_HEIGHT 480

Shader *color_shader;
Shader *texture_blend_shader;

Gpu* initialize_gpu() {
	Gpu* gpu = malloc(sizeof(Gpu));
       
	gpu->gp1 = 0x14000000;
	gpu->gp0 = 0x00000000;
	gpu->gpu_mode = COMMAND;
	gpu->fifoc = 0;
	gpu->fifolen = 0;
	gpu->last_render = 0.0f;
    
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

	shader_create(
      &color_shader, 
      "shaders/color_vs.glsl", 
      "shaders/color_fs.glsl");
  shader_create(
      &texture_blend_shader, 
      "shaders/texture_blend_vs.glsl",
      "shaders/texture_blend_fs.glsl");

	gpu_render_clear(gpu);
	gpu_render_swap(gpu);

  uint32_t buffer_mode = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
	
	/* 16bit VRAM pixel buffer. */
	glGenBuffers(1, &gpu->pbo16);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gpu->pbo16);

	glBufferStorage(GL_PIXEL_UNPACK_BUFFER, 1024 * 512, NULL, buffer_mode);	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glGenTextures(1, &gpu->texture16);
	glBindTexture(GL_TEXTURE_2D, gpu->texture16);

	/* Set the texture wrapping parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* Set texture filtering parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* Allocate space on the GPU. */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, 1024, 512, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, gpu->texture16);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gpu->pbo16);

	gpu->ptr16 = (uint16_t*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 1024 * 512, buffer_mode);

/* 4bit VRAM pixel buffer. */
	glGenBuffers(1, &gpu->pbo4);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gpu->pbo4);

	glBufferStorage(GL_PIXEL_UNPACK_BUFFER, 1024 * 512 * 4, NULL, buffer_mode);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glGenTextures(1, &gpu->texture4);
	glBindTexture(GL_TEXTURE_2D, gpu->texture4);

	/* Set the texture wrapping parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* Set texture filtering parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* Allocate space on the GPU. */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 4096, 512, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	
	glBindTexture(GL_TEXTURE_2D, gpu->texture4);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gpu->pbo4);

	gpu->ptr4 = (uint8_t*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 1024 * 512 * 4, buffer_mode);
 /* 8bit VRAM pixel buffer. */
	glGenBuffers(1, &gpu->pbo8);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gpu->pbo8);

	glBufferStorage(GL_PIXEL_UNPACK_BUFFER, 1024 * 512 * 2, NULL, buffer_mode);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glGenTextures(1, &gpu->texture8);
	glBindTexture(GL_TEXTURE_2D, gpu->texture8);

	/* Set the texture wrapping parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* Set texture filtering parameters. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* Allocate space on the GPU. */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 2048, 512, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	
	glBindTexture(GL_TEXTURE_2D, gpu->texture8);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gpu->pbo8);

	gpu->ptr8 = (uint8_t*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 1024 * 512 * 2, buffer_mode);
	
	return gpu;
}

void gpu_destroy(Gpu* gpu) {
	glfwTerminate();
}

void gpu_upload_texture(Gpu *gpu) {
  /* Upload 16bit texture. */
  glBindTexture(GL_TEXTURE_2D, gpu->texture16);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gpu->pbo16);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 512, GL_RED, GL_UNSIGNED_BYTE, 0);

  /* Upload 4bit texture. */  
  glBindTexture(GL_TEXTURE_2D, gpu->texture4);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gpu->pbo4);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4096, 512, GL_RED, GL_UNSIGNED_BYTE, 0);
  /* Upload 8bit texture. */
  glBindTexture(GL_TEXTURE_2D, gpu->texture8);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gpu->pbo8);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2048, 512, GL_RED, GL_UNSIGNED_BYTE, 0);
}

float color_from_u8(uint8 color) {
	return (float)color / 255.0f;
}

float x_from_u16(Gpu* gpu, uint16 x) {
	return (((float)x - gpu->offset[0]) - (float)gpu_hdr(gpu) / 2) / ((float)gpu_hdr(gpu) / 2);
}

float x_from_u8(Gpu* gpu, uint8 x) {
	return (((float)x - gpu->offset[0]) - (float)gpu_hdr(gpu) / 2) / ((float)gpu_hdr(gpu) / 2);
}

float y_from_u16(Gpu* gpu, uint16 y) {
	return (((float)y - gpu->offset[1]) - (float)gpu_vdr(gpu) / 2) / ((float)gpu_vdr(gpu) / 2);
}

float y_from_u8(Gpu* gpu, uint8 y) {
	return (((float)y - gpu->offset[1]) - (float)gpu_vdr(gpu) / 2) / ((float)gpu_vdr(gpu) / 2);
}

uint16 gpu_hdr(Gpu* gpu) {
	switch((gpu->gp1 >> 17) & 0x3) {
	case 0:
		return 256;
	case 1:
		return 320;
	case 2:
		return 512;
	case 3:
		return 640;
	default:
		printf("unknown horizontal display resolution %x\n", (gpu->gp1 >> 17) & 0x3);
		exit(1);
	}
}

uint16 gpu_vdr(Gpu* gpu) {
	switch((gpu->gp1 >> 19) & 0x1) {
	case 0:
		return 240;
	case 1:
		// TODO: check 5 bit for interlace
		return 480;
	default:
		printf("unknown vertical display resolution %x\n", (gpu->gp1 >> 19) & 0x1);
	}
}

void gpu_render_clear(Gpu* gpu) {	
	if (glfwGetTime() - gpu->last_render > fps) {		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

void gpu_render_swap(Gpu* gpu) {
	float now = glfwGetTime();
	if (now - gpu->last_render > fps) {		

		glfwSwapBuffers(gpu->window);
		glfwPollEvents();
		gpu->last_render = now;
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
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

	gpu->da_start = y * 1024 + x;
}

void gpu_set_hdr(Gpu* gpu, uint32_t v) {
	uint16_t x1 = v & 0xfff;
	uint16_t x2 = (v >> 12) & 0xfff;

	gpu->hdr[0] = x1;
	gpu->hdr[1] = x2;
}

void gpu_set_vdr(Gpu* gpu, uint32_t v) {
	uint16_t y1 = v & 0x3ff;
	uint16_t y2 = (v >> 10) & 0x3ff;

	gpu->vdr[0] = y1;
	gpu->vdr[1] = y2;
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
  uint32 index = (y * 1024) + x;
  /* Write data as 16bit. */ 	
  gpu->ptr16[index] = v;
	
	/* Write data as 8bit. */
	gpu->ptr8[index * 2 + 0] = (uint8_t)v;
	gpu->ptr8[index * 2 + 1] = (uint8_t)(v >> 8);
        
  /* Write data as 4bit. */ 
	gpu->ptr4[index * 4 + 0] = (uint8_t)v & 0xf;
	gpu->ptr4[index * 4 + 1] = (uint8_t)(v >> 4) & 0xf;
	gpu->ptr4[index * 4 + 2] = (uint8_t)(v >> 8) & 0xf;
	gpu->ptr4[index * 4 + 3] = (uint8_t)(v >> 12) & 0xf;
}

void gpu_store32(Gpu* gpu, uint16_t x, uint16_t y, uint32_t v) {
	uint32 index = (y * 1024) + x;
  /* Write data as 16bit. */ 	
  gpu->ptr16[index] = (uint16)v;
  gpu->ptr16[index] = (uint16)(v >> 16) & 0xffff;
	
	/* Write data as 8bit. */
	gpu->ptr8[index * 2 + 0] = (uint8)v;
	gpu->ptr8[index * 2 + 1] = (uint8)(v >> 8) & 0xff;
  gpu->ptr8[index * 2 + 2] = (uint8)(v >> 16) & 0xff;
  gpu->ptr8[index * 2 + 3] = (uint8)(v >> 24) & 0xff;
        
  /* Write data as 4bit. */ 
	gpu->ptr4[index * 4 + 0] = (uint8)v & 0xf;
	gpu->ptr4[index * 4 + 1] = (uint8)(v >> 4) & 0xf;
	gpu->ptr4[index * 4 + 2] = (uint8)(v >> 8) & 0xf;
	gpu->ptr4[index * 4 + 3] = (uint8)(v >> 12) & 0xf;
  gpu->ptr4[index * 4 + 4] = (uint8)(v >> 16) & 0xf;
	gpu->ptr4[index * 4 + 5] = (uint8)(v >> 20) & 0xf;
	gpu->ptr4[index * 4 + 6] = (uint8)(v >> 24) & 0xf;
	gpu->ptr4[index * 4 + 7] = (uint8)(v >> 28) & 0xf;
}

uint32 gpu_load32(Gpu* gpu, uint16 x, uint16 y) {
	uint32 offset = y * 1024 + x;

	uint32 b0 = gpu->ptr16[offset + 0];
	uint32 b1 = gpu->ptr16[offset + 1];

	return b0 | ( b1 << 16 );
}

uint16 gpu_load16(Gpu *gpu, uint16 x, uint16 y) {
  uint32 offset = y * 1024 + x;

  return gpu->ptr16[offset];
}

uint8 gpu_load8(Gpu *gpu, uint16 x, uint16 y) {
  uint32 offset = y * 1024 + x;

  return (uint8)gpu->ptr16[offset];
}

void gpu_gp0_command(Gpu* gpu, uint32_t command) {    
	if(gpu->fifoc == 0) {	
		uint8_t cmd = command >> 24;
		uint8_t len;
		/* printf("fifo cmd: %x\n", cmd); */
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
		/* printf("push word: %x\n", command); */
		gpu->fifo[gpu->fifolen - gpu->fifoc - 1] = command;
		if(gpu->fifoc == 0) {	       
	    uint8_t cmd = gpu->fifo[0] >> 24;
	    gpu->fifolen = 0;
			/* printf("run gp0 cmd: %x\n", cmd); */
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
      gpu_upload_texture(gpu);
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
  gpu->texture_window_mask_x = (gpu->fifo[0] >> 0) & 0x1f;
  gpu->texture_window_mask_y = (gpu->fifo[0] >> 5) & 0x1f;
  gpu->texture_window_offset_x = (gpu->fifo[0] >> 10) & 0x1f;
  gpu->texture_window_offset_y = (gpu->fifo[0] >> 15) >> 0x1f;
}

void set_mask_bit(Gpu* gpu) {
	gpu->gp1 = (gpu->gp1 & 0xffffe7ff) | ((gpu->fifo[0] & 0x3) << 11);
}

void attr_color(Gpu* gpu, uint32_t v, float* r, float* g, float* b) {
	*r = color_from_u8(v);
	*g = color_from_u8(v >> 8);
	*b = color_from_u8(v >> 16);	
}

void attr_vertex(Gpu* gpu, uint32_t v, float* x, float* y) {	
	*x = x_from_u16(gpu, v & 0x7ff);
	*y = -y_from_u16(gpu, (v >> 16) & 0xfff);	
}

void attr_clut(Gpu* gpu, uint32_t v, uint16* x, uint16* y) {
	*x = (v >> 16) & 0x3f;
  *y = (v >> 22) & 0x1ff;
}

void attr_tex_coord(Gpu* gpu, uint32 v, float* x, float* y, uint8 tex_depth, uint16 tpx, uint16 tpy) {
  float r;
  if (tex_depth == 0) {
    r = 4.0f;
  } else if (tex_depth == 1) {
    r = 2.0f;
  } else {
    r = 1.0f;
  }
  *x = (tpx * r + (v & 0xff)) / (1024.0f * r);
  *y = (tpy + (v >> 8) & 0xff) / 512.0f;
	//*x = (v & 0xff & (~(gpu->texture_window_mask_x * 8))
  //  | ((gpu->texture_window_offset_x & gpu->texture_window_mask_x) * 8)) / 255.0f;
	//*y = ((v >> 8) & 0xff & (~(gpu->texture_window_mask_y * 8))
  //  | ((gpu->texture_window_offset_y & gpu->texture_window_mask_y) * 8)) / 255.0f;	
  //*x = (v & 0xff) / 255.0f;
  //*y = ((v >> 8) & 0xff) / 255.0f;
}

void attr_texpage(Gpu* gpu, uint32 v, uint8* x, uint8* y, uint8* st, uint8* texmode, uint8* disable) {
	*x = v & 0xf;
  *y = (v >> 4) & 0x1;
  *st = (v >> 5) & 0x3;
	*texmode = (v >> 7) & 0x3;
	*disable = (v >> 11) & 0x1;
}

void tex4popblend(Gpu* gpu) {
  uint16 cx, cy;

	uint8 tpx, tpy, tpst, tpmode, tpdis;
	
	float
		r, g, b,			
		
		x1, y1,
		tx1, ty1,

		x2, y2,
		tx2, ty2,

		x3, y3,
		tx3, ty3,

		x4, y4,
		tx4, ty4;

  attr_color(gpu, gpu->fifo[0], &r, &g, &b);
	attr_vertex(gpu, gpu->fifo[1], &x1, &y1);
	attr_clut(gpu, gpu->fifo[2], &cx, &cy);
	attr_vertex(gpu, gpu->fifo[3], &x2, &y2);
	attr_texpage(gpu, gpu->fifo[4], &tpx, &tpy, &tpst, &tpmode, &tpdis);
  
  uint16 tx = tpx * 64;
  uint16 ty = tpy * 256;
  
  attr_tex_coord(gpu, gpu->fifo[2], &tx1, &ty1, tpmode, tx, ty);
  attr_tex_coord(gpu, gpu->fifo[4], &tx2, &ty2, tpmode, tx, ty);
	attr_vertex(gpu, gpu->fifo[5], &x3, &y3);
	attr_tex_coord(gpu, gpu->fifo[6], &tx3, &ty3, tpmode, tx, ty);
	attr_vertex(gpu, gpu->fifo[7], &x4, &y4);
	attr_tex_coord(gpu, gpu->fifo[8], &tx4, &ty4, tpmode, tx, ty);

  float *color = malloc(sizeof(float) * 3 * 6);
	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = r;
	color[4] = g;
	color[5] = b;
	color[6] = r;
	color[7] = g;
	color[8] = b;
	color[9] = r;
	color[10] = g;
	color[11] = b;
	color[12] = r;
	color[13] = g;
	color[14] = b;
	color[15] = r;
	color[16] = g;
	color[17] = b;	

	float *vertices = malloc(sizeof(float) * 12);

	vertices[0] = x1;
	vertices[1] = y1;
	vertices[2] = x2;
	vertices[3] = y2;	
	vertices[4] = x4;
	vertices[5] = y4;	
	vertices[6] = x4;
	vertices[7] = y4;
	vertices[8] = x1;
	vertices[9] = y1;
	vertices[10] = x3;
	vertices[11] = y3;

  float *tex_coords = malloc(sizeof(float) * 12);

  tex_coords[0] = tx1;
  tex_coords[1] = ty1;
  tex_coords[2] = tx2;
  tex_coords[3] = ty2;
  tex_coords[4] = tx4;
  tex_coords[5] = ty4;
  tex_coords[6] = tx4;
  tex_coords[7] = ty4;
  tex_coords[8] = tx1;
  tex_coords[9] = ty1;
  tex_coords[10] = tx3;
  tex_coords[11] = ty3;

  cx = cx * 16;
  
  GLuint vbo[3], vao;

  glGenBuffers(3, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 18, color, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, vertices, GL_STATIC_DRAW);
	
  glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, tex_coords, GL_STATIC_DRAW);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
 	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);

	shader_bind(texture_blend_shader);	

  shader_seti(texture_blend_shader, "texture_depth", tpmode);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gpu->texture4);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gpu->texture8);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, gpu->texture16);

  shader_seti(texture_blend_shader, "tex4", 0);
  shader_seti(texture_blend_shader, "tex8", 1);
  shader_seti(texture_blend_shader, "tex16", 2);
    
  char buf[20];
  switch (tpmode) {
    // 4 bit texture
    case 0:
      for (int i = 0; i < 16; i++) {
        snprintf(buf, 20, "clut4[%d]", i);
        shader_seti(texture_blend_shader, buf, gpu_load16(gpu, cx + i, cy));
      }
      break;
    case 1:
      for (int i = 0; i < 256; i++) {
        snprintf(buf, 20, "clut8[%d]", i);
        shader_seti(texture_blend_shader, buf, gpu_load16(gpu, cx + i, cy));
      }
      break;
  }

  gpu_render_swap(gpu);	
	gpu_render_clear(gpu);

  glDrawArrays(GL_TRIANGLES, 0, 6);
	
	shader_unbind();

	free(vertices);
	free(color);
  free(tex_coords);

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(3, vbo);
}

void sh4pop(Gpu* gpu) {
	float
		r1, g1, b1,
		x1, y1,

		r2, g2, b2,
		x2, y2,

		r3, g3, b3,
		x3, y3,

		r4, g4, b4,
		x4, y4;
	
	attr_color(gpu, gpu->fifo[0], &r1, &g1, &b1);
	attr_vertex(gpu, gpu->fifo[1], &x1, &y1);
	attr_color(gpu, gpu->fifo[2], &r2, &g2, &b2);
	attr_vertex(gpu, gpu->fifo[3], &x2, &y2);
	attr_color(gpu, gpu->fifo[4], &r3, &g3, &b3);
	attr_vertex(gpu, gpu->fifo[5], &x3, &y3);
	attr_color(gpu, gpu->fifo[6], &r4, &g4, &b4);
	attr_vertex(gpu, gpu->fifo[7], &x4, &y4);

	float *color = malloc(sizeof(float) * 18);
	color[0] = r1;
	color[1] = g1;
	color[2] = b1;
	color[3] = r2;
	color[4] = g2;
	color[5] = b2;
	color[6] = r4;
	color[7] = g4;
	color[8] = b4;
	color[9] = r4;
	color[10] = g4;
	color[11] = b4;
	color[12] = r1;
	color[13] = g1;
	color[14] = b1;
	color[15] = r3;
	color[16] = g3;
	color[17] = b3;	

	float *vertices = malloc(sizeof(float) * 12);

	vertices[0] = x1;
	vertices[1] = y1;
	vertices[2] = x2;
	vertices[3] = y2;	
	vertices[4] = x4;
	vertices[5] = y4;	
	vertices[6] = x4;
	vertices[7] = y4;
	vertices[8] = x1;
	vertices[9] = y1;
	vertices[10] = x3;
	vertices[11] = y3;
	
	uint32 vbo[2], vao;
	
	glGenBuffers(2, vbo);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 18, color, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, vertices, GL_STATIC_DRAW);
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	gpu_render_swap(gpu);	
	gpu_render_clear(gpu);
	
	shader_bind(color_shader);	
	
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	shader_unbind();

	free(vertices);
	free(color);

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(2, vbo);
}

void sh3pop(Gpu* gpu) {
	float
		r1, g1, b1,
		x1, y1,

		r2, g2, b2,
		x2, y2,

		r3, g3, b3,
		x3, y3;
	
	attr_color(gpu, gpu->fifo[0], &r1, &g1, &b1);
	attr_vertex(gpu, gpu->fifo[1], &x1, &y1);
	attr_color(gpu, gpu->fifo[2], &r2, &g2, &b2);
	attr_vertex(gpu, gpu->fifo[3], &x2, &y2);
	attr_color(gpu, gpu->fifo[4], &r3, &g3, &b3);
	attr_vertex(gpu, gpu->fifo[5], &x3, &y3);

	float *color = malloc(sizeof(float) * 9);
	color[0] = r1;
	color[1] = g1;
	color[2] = b1;
	color[3] = r2;
	color[4] = g2;
	color[5] = b2;
	color[6] = r3;
	color[7] = g3;
	color[8] = b3;	 

	float *vertices = malloc(sizeof(float) * 6);

	vertices[0] = x1;
	vertices[1] = y1;
	vertices[2] = x2;
	vertices[3] = y2;	
	vertices[4] = x3;
	vertices[5] = y3;
	
	uint32 vbo[2], vao;
	
	glGenBuffers(2, vbo);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, color, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, vertices, GL_STATIC_DRAW);
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	
	shader_bind(color_shader);

	gpu_render_swap(gpu);	
	gpu_render_clear(gpu);
	
	glDrawArrays(GL_TRIANGLES, 0, 3);

	shader_unbind();

	free(vertices);
	free(color);

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(2, vbo);
}

void mon4pop(Gpu* gpu) {
	float
		r, g, b,

		x1, y1,

		x2, y2,

		x3, y3,

		x4, y4;
	
	attr_color(gpu, gpu->fifo[0], &r, &g, &b);
	
	float *color = malloc(sizeof(float) * 3 * 6);
	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = r;
	color[4] = g;
	color[5] = b;
	color[6] = r;
	color[7] = g;
	color[8] = b;
	color[9] = r;
	color[10] = g;
	color[11] = b;
	color[12] = r;
	color[13] = g;
	color[14] = b;
	color[15] = r;
	color[16] = g;
	color[17] = b;	

	float *vertices = malloc(sizeof(float) * 12);

	attr_vertex(gpu, gpu->fifo[1], &x1, &y1);
	attr_vertex(gpu, gpu->fifo[2], &x2, &y2);
	attr_vertex(gpu, gpu->fifo[3], &x3, &y3);
	attr_vertex(gpu, gpu->fifo[4], &x4, &y4); 

	vertices[0] = x1;
	vertices[1] = y1;
	vertices[2] = x2;
	vertices[3] = y2;	
	vertices[4] = x4;
	vertices[5] = y4;	
	vertices[6] = x4;
	vertices[7] = y4;
	vertices[8] = x1;
	vertices[9] = y1;
	vertices[10] = x3;
	vertices[11] = y3;
		
	uint32 vbo[2], vao;
	
	glGenBuffers(2, vbo);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 18, color, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, vertices, GL_STATIC_DRAW);
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);	
	
	shader_bind(color_shader);

	gpu_render_swap(gpu);
	gpu_render_clear(gpu);
	
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	shader_unbind();

	free(color);
	free(vertices);	

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(2, vbo);
}

void cpr_cv(Gpu* gpu) {   
	gpu->gpu_mode = CPU_VRAM;    

	uint16_t w = gpu->fifo[2] & 0xffff;
	uint16_t h = gpu->fifo[2] >> 16;

	uint32_t size = w * h;
	size = (size + 1) & ~1;
	gpu->fifolen = size / 2;
	gpu->fifoc = size / 2;	

  uint16_t x = gpu->fifo[1] & 0xffff;
	uint16_t y = gpu->fifo[1] >> 16;

  printf("store w h %d %d to %x %x\n", w, h, x, y);
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
    
  printf("fill rec %x w h %d %d to %x %x\n", color, w, h, x, h);

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

	uint16_t transferx = x + ((gpu->fifolen - gpu->fifoc - 1) * 2) % w;
	uint16_t transfery = y + ceil((gpu->fifolen - gpu->fifoc - 1) * 2 / w); 

	gpu_store32(gpu, transferx, transfery, v);    
}

void vramvc(Gpu* gpu, uint32_t v) {
	uint16_t x = gpu->fifo[1] & 0xffff;
	uint16_t y = gpu->fifo[1] >> 16;
	uint16_t w = gpu->fifo[2] & 0xffff;
	uint16_t h = gpu->fifo[2] >> 16; 

	uint32_t transferx = x + (gpu->fifolen - gpu->fifoc - 1) * 2 % w;
	uint32_t transfery = y + ceil((gpu->fifolen - gpu->fifoc - 1) * 2 / w);	
	 
	uint32_t word = gpu_load32(gpu, transferx, transfery);

	gpu_set_gp0(gpu, word);
}
