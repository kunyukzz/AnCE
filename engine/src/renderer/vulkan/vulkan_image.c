#include "vulkan_image.h"

#include "vulkan_device.h"

#include "core/acmemory.h"
#include "core/logger.h"

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
                         vulkan_image* out_image)
{
    out_image->width = width;
    out_image->height = height;

    VkImageCreateInfo image_create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1; // NOTE: support configure depth
    image_create_info.mipLevels = 4;    // NOTE: support mip-mapping
    image_create_info.arrayLayers = 1;  // NOTE: support number of layer in the image
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;         // NOTE: configurable sample count
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // NOTE: configurable sharing mode

    VK_CHECK(vkCreateImage(context->device.logical_device, &image_create_info, context->allocator, &out_image->handle));

    VkMemoryRequirements memory_require;
    vkGetImageMemoryRequirements(context->device.logical_device, out_image->handle, &memory_require);

    i32 mem_type = context->find_memory_index(memory_require.memoryTypeBits, mem_flags);
    if (mem_type == -1)
        ACERROR("Require memory type not found!. Image not valid");

    // allocate memory
    VkMemoryAllocateInfo mem_allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    mem_allocate_info.allocationSize = memory_require.size;
    mem_allocate_info.memoryTypeIndex = mem_type;
    VK_CHECK(vkAllocateMemory(context->device.logical_device, &mem_allocate_info, context->allocator, &out_image->memory));

    // bind memory
    VK_CHECK(
      vkBindImageMemory(context->device.logical_device, out_image->handle, out_image->memory, 0)); // NOTE: configerable memory offset

    // create view
    if (create_view)
    {
        out_image->view = 0;
        vulkan_image_view_create(context, format, out_image, view_aspect_flag);
    }
}

void vulkan_image_view_create(vulkan_context* context, VkFormat format, vulkan_image* image, VkImageAspectFlags aspect_flags)
{
    VkImageViewCreateInfo view_create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view_create_info.image = image->handle;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D; // NOTE: configurable
    view_create_info.format = format;
    view_create_info.subresourceRange.aspectMask = aspect_flags;

    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(context->device.logical_device, &view_create_info, context->allocator, &image->view));
}

void vulkan_image_destroy(vulkan_context* context, vulkan_image* image)
{
    if (image->view)
    {
        vkDestroyImageView(context->device.logical_device, image->view, context->allocator);
        image->view = 0;
    }
    if (image->memory)
    {
        vkFreeMemory(context->device.logical_device, image->memory, context->allocator);
        image->memory = 0;
    }
    if (image->handle)
    {
        vkDestroyImage(context->device.logical_device, image->handle, context->allocator);
        image->handle = 0;
    }
}
