#pragma once

#include "core/assertion.h"
#include "define.h"
// #include "core/astring.h"

#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                                     \
    {                                                                                                                                      \
        ACASSERT(expr == VK_SUCCESS);                                                                                                      \
    }

typedef struct vulkan_swapchain_support_info
{
    VkSurfaceCapabilitiesKHR capabilities;
    u32 format_count;
    VkSurfaceFormatKHR* formats;
    u32 present_mode_count;
    VkPresentModeKHR* present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_device
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    vulkan_swapchain_support_info swapchain_support;

    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;

    VkCommandPool graphics_command_pool;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depth_format;
} vulkan_device;

typedef struct vulkan_image
{
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
} vulkan_image;

typedef enum vulkan_renderpass_state
{
    READY = 0,
    RECORDING,
    IN_RENDER_PASS,
    RECORD_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
} vulkan_renderpass_state;

typedef struct vulkan_renderpass
{
    VkRenderPass handle;
    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    vulkan_renderpass_state state;
} vulkan_renderpass;

typedef struct vulkan_framebuffer
{
    VkFramebuffer handle;
    u32 attachment_count;
    VkImageView* attachments;
    vulkan_renderpass* renderpass;
} vulkan_framebuffer;

typedef struct vulkan_swapchain
{
    VkSurfaceFormatKHR image_format;
    u8 max_frame_in_flight;
    VkSwapchainKHR handle;
    u32 image_count;
    VkImage* images;
    VkImageView* views;
    vulkan_image depth_attachment;
    vulkan_framebuffer* framebuffers;
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state
{
    COM_BUFFER_STATE_READY,
    COM_BUFFER_STATE_RECORDING,
    COM_BUFFER_STATE_IN_RENDERPASS,
    COM_BUFFER_STATE_RECORD_END,
    COM_BUFFER_STATE_SUBMITTED,
    COM_BUFFER_STATE_NOT_ALLOCATED
} vulkan_command_buffer_state;

typedef struct vulkan_command_buffer
{
    VkCommandBuffer handle;
    vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_fence
{
    VkFence handle;
    b8 is_signaled;
} vulkan_fence;

typedef struct vulkan_context
{
    u32 framebuffer_width;
    u32 framebuffer_height;

    u64 framebuffer_size_generate;
    u64 framebuffer_size_last_generate;

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    vulkan_device device;
    vulkan_swapchain swapchain;
    vulkan_renderpass main_renderpass;

    vulkan_command_buffer* graphics_command_buffers; // dynamic array

    VkSemaphore* image_available_semaphores; // dynamic array
    VkSemaphore* queue_complete_semaphores;  // dynamic array

    u32 in_flight_fence_count;
    vulkan_fence* in_flight_fences;
    vulkan_fence** images_in_flight; // hold pointer to fences

    u32 image_index;
    i32 current_frame;
    b8 recreate_swapchain;
    i32 (*find_memory_index)(u32 type_filter, u32 property_flags);
} vulkan_context;
