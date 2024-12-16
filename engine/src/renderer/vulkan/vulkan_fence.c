#include "vulkan_fence.h"

#include "core/logger.h"

void vulkan_fence_create(vulkan_context* context, b8 create_signal, vulkan_fence* out_fence)
{
    out_fence->is_signaled = create_signal;
    VkFenceCreateInfo fence_create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    if (out_fence->is_signaled)
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(context->device.logical_device, &fence_create_info, context->allocator, &out_fence->handle));
}

void vulkan_fence_destroy(vulkan_context* context, vulkan_fence* fence)
{
    if (fence->handle)
    {
        vkDestroyFence(context->device.logical_device, fence->handle, context->allocator);
        fence->handle = 0;
    }
    fence->is_signaled = FALSE;
}

b8 vulkan_fence_wait(vulkan_context* context, vulkan_fence* fence, u64 timeout)
{
    if (!fence->is_signaled)
    {
        VkResult result = vkWaitForFences(context->device.logical_device, 1, &fence->handle, TRUE, timeout);
        switch (result)
        {
        case VK_SUCCESS:
            fence->is_signaled = TRUE;
            return TRUE;
        case VK_TIMEOUT:
            ACWARN("vk_fence_wait - Time out");
            break;
        case VK_ERROR_DEVICE_LOST:
            ACERROR("vk_fence_wait - VK_ERROR_DEVICE_LOST");
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            ACERROR("vk_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY");
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            ACERROR("vk_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY");
            break;
        default:
            ACERROR("vk_fence_wait - An unknown error has occurred")
            break;
        }
    }
    else
    {
        return TRUE;
    }

    return FALSE;
}

void vulkan_fence_reset(vulkan_context* context, vulkan_fence* fence)
{
    if (fence->is_signaled)
    {
        VK_CHECK(vkResetFences(context->device.logical_device, 1, &fence->handle));
        fence->is_signaled = FALSE;
    }
}
