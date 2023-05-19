#ifndef GPU_H
#define GPU_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdint.h>

#include "range.h"

#define VRAM_SIZE 1024 * 1024

Range RENDERING_ATTRIBUTES = { 0xe1, 5 };
Range RENDER_LINES = { 0x40, 96 };
Range RENDER_POLYGONES = { 0x20, 32 };

typedef struct {
    GLFWwindow* window;
    char display_enable;
    
    uint8_t vram[VRAM_SIZE];

    uint64_t fifo;
    
    uint32_t gp0;
    uint32_t gp1;

    uint16_t viewport[4]; // top left[0:1], bottom right[2:3]
    int16_t offset[2];

    uint16_t draw_mode;

    uint32_t texture_window;
    uint8_t mask_bit;

    uint32_t* attrs;
    uint8_t attrsc;
    
    uint8_t next_attr;    
    uint8_t render_command;

    uint32_t da_start;

    uint16_t hdr;
    uint16_t vdr;
} Gpu;

Gpu* initialize_gpu();
void gpu_destroy(Gpu* gpu);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    
uint32_t gpu_offset(Gpu* gpu, uint16_t x, uint16_t y);
void gpu_gp0_command(Gpu* gpu, uint32_t command);
void gpu_gp1_command(Gpu* gpu, uint32_t command);
void gpu_proceed_arg(Gpu* gpu, uint32_t arg);
void gpu_render(Gpu* gpu);

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

void gpu_reset(Gpu* gpu);
void gpu_clear_fifo(Gpu* gpu);
void gpu_ack_irq(Gpu* gpu);
void gpu_set_d(Gpu* gpu, uint32_t v);
void gpu_set_dd(Gpu* gpu, uint32_t v);
void gpu_set_da(Gpu* gpu, uint32_t v);
void gpu_set_hdr(Gpu* gpu, uint32_t v);
void gpu_set_vdr(Gpu* gpu, uint32_t v);
void gpu_set_dm(Gpu* gpu, uint32_t v);

void gpu_store32(Gpu* gpu, uint32_t offset, uint32_t v);

void gpu_rendering_attributes(Gpu* gpu, uint8_t offset, uint32_t v);
void gpu_render_polygones(Gpu* gpu, uint8_t offset, uint32_t v);
void gpu_render_lines(Gpu* gpu, uint8_t offset, uint32_t v);

void set_area_top_left(Gpu* gpu, uint32_t v);
void set_area_bottom_right(Gpu* gpu, uint32_t v);
void set_drawing_offset(Gpu* gpu, uint32_t v);
void set_draw_mode(Gpu* gpu, uint32_t v);
void set_texture_window(Gpu* gpu, uint32_t v);
void set_mask_bit(Gpu* gpu, uint32_t v);

uint8_t* attr_color(Gpu* gpu, uint32_t v);
int16_t* attr_vertex(Gpu* gpu, uint32_t v);

void mon4pop(Gpu* gpu);

#endif
