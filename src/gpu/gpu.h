#ifndef GPU_H
#define GPU_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <math.h>

#include "../range.h"

#define VRAM_SIZE 2048 * 512

typedef enum {
	VRAM_VRAM,
	CPU_VRAM,
	VRAM_CPU,
	COMMAND,
} GPU_Mode;

typedef struct {
	GLFWwindow* window;
    
	uint32_t fifo[16];    
	uint32_t fifoc;
	uint32_t fifolen;
    
	uint32_t gp0;
	uint32_t gp1;

	uint16_t viewport[4]; // top left[0:1], bottom right[2:3]
	int16_t offset[2];

  uint8 texture_window_mask_x;
  uint8 texture_window_mask_y;
  uint8 texture_window_offset_x;
  uint8 texture_window_offset_y; 
	
	uint32_t da_start;	

	uint16 hdr[2];
	uint16 vdr[2];	
    
	GPU_Mode gpu_mode;

	float last_render;
  
  uint32 pbo4, pbo8, pbo16; 
        
  /* The pixel arrays connected to the PBOs. */
  uint8* ptr4;
  uint8* ptr8;
  uint16* ptr16;
    
  /* The OpenGL textures. */ 
	uint32 texture4, texture8, texture16;
} Gpu;

const float fps = 1.0f / 60.0f;


Gpu* initialize_gpu();
void gpu_destroy(Gpu* gpu);
void gpu_upload_texture(Gpu *gpu);

float color_from_u8(uint8 color);
float x_from_u16(Gpu* gpu, uint16 x);
float y_from_u16(Gpu* gpu, uint16 y);
float x_from_u8(Gpu* gpu, uint8 x);
float y_from_u(Gpu* gpu, uint8 y);

uint16 gpu_hdr(Gpu* gpu);
uint16 gpu_vdr(Gpu* gpu);

void gpu_render_clear(Gpu* gpu);
void gpu_render_swap(Gpu* gpu);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    
uint32_t gpu_offset(Gpu* gpu, uint16_t x, uint16_t y);
void gpu_gp0_command(Gpu* gpu, uint32_t command);
void gpu_gp1_command(Gpu* gpu, uint32_t command);
void gpu_transfer_command(Gpu* gpu, uint8_t command);
void gpu_transfer(Gpu* gpu);

void gpu_set_gp0(Gpu* gpu, uint32_t v);
void gpu_set_gp1(Gpu* gpu, uint32_t v);
uint32_t gpu_gpustat(Gpu* gpu);
uint32_t gpu_gpuread(Gpu* gpu);

void gpu_block_block(Gpu* gpu);
void gpu_unblock_block(Gpu* gpu);
void gpu_block_cmd(Gpu* gpu);
void gpu_unblock_cmd(Gpu* gpu);
void gpu_block_vram(Gpu* gpu);
void gpu_unblock_vram(Gpu* gpu);

void gpu_reset(Gpu* gpu, uint32_t v);
void gpu_clear_fifo(Gpu* gpu, uint32_t v);
void gpu_ack_irq(Gpu* gpu, uint32_t v);
void gpu_set_d(Gpu* gpu, uint32_t v);
void gpu_set_dd(Gpu* gpu, uint32_t v);
void gpu_set_da(Gpu* gpu, uint32_t v);
void gpu_set_hdr(Gpu* gpu, uint32_t v);
void gpu_set_vdr(Gpu* gpu, uint32_t v);
void gpu_set_dm(Gpu* gpu, uint32_t v);

void gpu_store16(Gpu* gpu, uint16_t x, uint16_t y, uint16_t v);
void gpu_store32(Gpu* gpu, uint16_t x, uint16_t y, uint32_t v);

uint32_t gpu_load32(Gpu* gpu, uint16_t x, uint16_t y);

void set_area_top_left(Gpu* gpu);
void set_area_bottom_right(Gpu* gpu);
void set_drawing_offset(Gpu* gpu);
void set_draw_mode(Gpu* gpu);
void set_texture_window(Gpu* gpu);
void set_mask_bit(Gpu* gpu);

void attr_color(Gpu* gpu, uint32 v, float* r, float* g, float* b);
void attr_vertex(Gpu* gpu, uint32 v, float* x, float* y);
void attr_clut(Gpu* gpu, uint32 v, uint16* x, uint16* y);
void attr_tex_coord(Gpu* gpu, uint32 v, float* x, float* y, uint8 tex_depth, uint16 tpx, uint16 tpy);
void attr_texpage(Gpu* gpu, uint32 v, uint8* x, uint8* y, uint8* st, uint8* texmode, uint8* disable);

void tex4popblend(Gpu* gpu);

void sh4pop(Gpu* gpu);
void sh3pop(Gpu* gpu);

void mon4pop(Gpu* gpu);

void cpr_cv(Gpu* gpu);
void cpr_vc(Gpu* gpu);
void fill_rec(Gpu* gpu);

void vramcv(Gpu* gpu, uint32_t v);
void vramvc(Gpu* gpu, uint32_t v);

#endif
