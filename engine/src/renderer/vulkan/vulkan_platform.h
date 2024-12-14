#pragma once

#include "define.h"

struct platform_state;
struct vulkan_context;

b8 platform_create_vulkan_surface(struct platform_state* plat_state, struct vulkan_context* context);

// Append names of required extension to names dynamic array
void platform_get_required_extension_name(const char*** names_dyn_array);

