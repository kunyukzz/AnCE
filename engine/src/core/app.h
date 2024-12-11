#pragma once

#include "define.h"

typedef struct 
{
i16 pos_x;
i16 pos_y;
i16 width;
i16 height;
char* title;
} application_config;

ACAPI b8 application_create(application_config* config);

ACAPI b8 application_run();
