#pragma once

#include "vulkan_type.inl"

void vulkan_command_buffer_allocate(vulkan_context* context, VkCommandPool pool, b8 is_primary, vulkan_command_buffer* out_command_buffer);

void vulkan_command_buffer_free(vulkan_context* context, VkCommandPool pool, vulkan_command_buffer* command_buffer);

void vulkan_command_buffer_begin(vulkan_command_buffer* command_buffer, b8 is_single_use, b8 is_renderpass_continue, b8 is_simultan_use);

void vulkan_command_buffer_end(vulkan_command_buffer* command_buffer);

void vulkan_command_buffer_update_submitted(vulkan_command_buffer* command_buffer);

void vulkan_command_buffer_reset(vulkan_command_buffer* command_buffer);

/*
 * Allocate and begin recording to out command buffer
 */
void vulkan_command_buffer_allocate_and_begin_single_use(vulkan_context* context,
                                                         VkCommandPool pool,
                                                         vulkan_command_buffer* out_command_buffer);

/*
 * End recording. submit 'to' and 'wait' for queue operation and frees the command buffer
 */
void vulkan_command_buffer_allocate_end_single_use(vulkan_context* context,
                                                   VkCommandPool pool,
                                                   vulkan_command_buffer* command_buffer,
                                                   VkQueue queue);