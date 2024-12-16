#pragma once

#include "define.h"

struct game;

typedef struct application_config
{
    i16 pos_x;
    i16 pos_y;
    i16 width;
    i16 height;
    char* title;
} application_config;

ACAPI b8 application_create(struct game* game_inst);

ACAPI b8 application_run();

void application_get_framebuffer_size(u32* width, u32* height);
