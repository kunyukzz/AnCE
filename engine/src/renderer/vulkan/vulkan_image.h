#pragma once

#include "vulkan_type.inl"

void vulkan_image_create(vulkan_context* context,
                         VkImageType image_type,
                         u32 width,
                         u32 height,
                         VkFormat format,
                         VkImageTiling tiling,
                         VkImageUsageFlags usage,
                         VkMemoryPropertyFlags mem_flags,
                         b32 create_view,
                         VkImageAspectFlags view_aspect_flag,
                         vulkan_image* out_image);

void vulkan_image_view_create(vulkan_context* context, VkFormat format, vulkan_image* image, VkImageAspectFlags aspect_flags);

void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);