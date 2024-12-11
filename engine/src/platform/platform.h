#pragma once

#include "define.h"

typedef struct
{
    void* internal_state;
} platform_state;

b8 platform_startup(platform_state* plat_state, const char* app_name, i32 x, i32 y, i32 width, i32 height);
void platform_shutdown(platform_state* plat_state);
b8 platform_push_msg(platform_state* plat_state);

// function for memory allocation
void* platform_allocated(u64 size, b8 aligned);
void platform_free(void* block, b8 aligned);
void* platform_zero_mem(void* block, u64 size);
void* platform_copy_mem(void* dest, const void* source, u64 size);
void* platform_set_mem(void* dest, i32 value, u64 size);

void platform_console_write(const char* msg, u8 color);
void platform_console_write_error(const char* msg, u8 color);

f64 platform_get_absolute_time();
void platform_sleep(u64 ms);
