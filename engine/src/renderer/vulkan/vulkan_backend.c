#include "vulkan_backend.h"

// #include "vulkan/vulkan_core.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_fence.h"
#include "vulkan_framebuffer.h"
#include "vulkan_platform.h"
#include "vulkan_renderpass.h"
#include "vulkan_swapchain.h"
#include "vulkan_type.inl"
#include "vulkan_utils.h"

#include "core/acmemory.h"
#include "core/app.h"
#include "core/astring.h"
#include "core/logger.h"

#include "container/dyn_array.h"

#include "platform/platform.h"

// static vulkan context related
static vulkan_context context;
static u32 cache_framebuffer_width = 0;
static u32 cache_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                 VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                 void* user_data);

i32 find_memory_index(u32 type_filter, u32 property_flags);
void create_command_buffers(renderer_backend* backend);
void regen_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass);
b8 recreate_swapchain(renderer_backend* backend);

b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* app_name, struct platform_state* plat_state)
{
    // function pointer
    context.find_memory_index = find_memory_index;

    // Custom allocator.
    context.allocator = 0;

    // BUG: somehow framebuffer size cannot setup correctly.
    application_get_framebuffer_size(&cache_framebuffer_width, &cache_framebuffer_height);
    context.framebuffer_width = (cache_framebuffer_width != 0) ? cache_framebuffer_width : 800;
    context.framebuffer_height = (cache_framebuffer_height != 0) ? cache_framebuffer_height : 600;
    cache_framebuffer_width = 0;
    cache_framebuffer_height = 0;

    // Setup Vulkan
    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.apiVersion = VK_API_VERSION_1_3;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Ance Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    create_info.pApplicationInfo = &app_info;

    // obtain requirement extension
    const char** required_extension = ac_dyn_array_create_t(const char*);
    ac_dyn_array_push_t(required_extension, &VK_KHR_SURFACE_EXTENSION_NAME); // Generic surface extension
    platform_get_required_extension_name(&required_extension);               // platform specific

#if defined(_DEBUG)
    ac_dyn_array_push_t(required_extension, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    ACDEBUG("Require Extensions:");
    u32 length = ac_dyn_array_length_t(required_extension);
    for (u32 i = 0; i < length; ++i)
    {
        ACDEBUG(required_extension[i]);
    }
#endif

    create_info.enabledExtensionCount = ac_dyn_array_length_t(required_extension);
    create_info.ppEnabledExtensionNames = required_extension;

    // check for validation layer
    const char** required_validation_layer_name = 0;
    u32 required_validation_layer_count = 0;

#if defined(_DEBUG)
    ACINFO("Validation layer enabled. Enumerating...");

    // list of validation layer required
    required_validation_layer_name = ac_dyn_array_create_t(const char*);
    ac_dyn_array_push_t(required_validation_layer_name, &"VK_LAYER_KHRONOS_validation");
    required_validation_layer_count = ac_dyn_array_length_t(required_validation_layer_name);

    u32 avail_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&avail_layer_count, 0));
    VkLayerProperties* available_layers = ac_dyn_array_reserved_t(VkLayerProperties, avail_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&avail_layer_count, available_layers));

    // Verify all required layers are available.
    for (u32 i = 0; i < required_validation_layer_count; ++i)
    {
        b8 found = FALSE;
        for (u32 j = 0; j < avail_layer_count; ++j)
        {
            if (string_equal(required_validation_layer_name[i], available_layers[j].layerName))
            {
                found = TRUE;
                ACINFO("Found");
                break;
            }
        }
        if (!found)
        {
            ACFATAL("Required validation layer is missing: %s", required_validation_layer_name[i]);
            // break;
            //  HACK: I'm cheating here!!!
            return TRUE;
        }
    }
    ACINFO("All required validation layers are present.");
#endif

    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_name;

    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
    ACINFO("Vulkan Instance success!");

#if defined(_DEBUG)
    ACINFO("Vulkan Debugger");
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    debug_create_info.pfnUserCallback = vk_debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    ACASSERT_MSG(func, "Failed to create Debug Messenger!");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
    ACDEBUG("Vulkan debug created");
#endif

    // Surface
    ACDEBUG("Creating Vulkan surface...");
    if (!platform_create_vulkan_surface(plat_state, &context))
    {
        ACERROR("Failed to create platform surface");
        return FALSE;
    }
    ACDEBUG("Vulkan surface created");

    // Create device
    if (!vulkan_device_create(&context))
    {
        ACERROR("Failed to create device");
        return FALSE;
    }

    // Swapchain
    vulkan_swapchain_create(&context, context.framebuffer_width, context.framebuffer_height, &context.swapchain);
    vulkan_renderpass_create(
      &context, &context.main_renderpass, 0, 0, context.framebuffer_width, context.framebuffer_height, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0);

    // Swapchain frame buffers
    context.swapchain.framebuffers = ac_dyn_array_reserved_t(vulkan_framebuffer, context.swapchain.image_count);
    regen_framebuffers(backend, &context.swapchain, &context.main_renderpass);

    // Command buffers
    create_command_buffers(backend);

    // Create sync object
    context.image_available_semaphores = ac_dyn_array_reserved_t(VkSemaphore, context.swapchain.max_frame_in_flight);
    context.queue_complete_semaphores = ac_dyn_array_reserved_t(VkSemaphore, context.swapchain.max_frame_in_flight);
    context.in_flight_fences = ac_dyn_array_reserved_t(vulkan_fence, context.swapchain.max_frame_in_flight);
    for (u8 i = 0; i < context.swapchain.max_frame_in_flight; ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]);
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]);

        // this was indicator that first time frame already been "rendered"
        // to prevent application from waiting
        vulkan_fence_create(&context, TRUE, &context.in_flight_fences[i]);
    }

    // NOTE: in flight fence SHOULD NOT EXIST yet!!!. These are stored in pointer because initial state should be 0.
    context.images_in_flight = ac_dyn_array_reserved_t(vulkan_fence, context.swapchain.image_count);
    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        context.images_in_flight[i] = 0;
    }

    ACINFO("Vulkan renderer initialized");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend)
{
    vkDeviceWaitIdle(context.device.logical_device);

    // Sync object
    for (u8 i = 0; i < context.swapchain.max_frame_in_flight; ++i)
    {
        if (context.image_available_semaphores[i])
        {
            vkDestroySemaphore(context.device.logical_device, context.image_available_semaphores[i], context.allocator);
            context.image_available_semaphores[i] = 0;
        }
        if (context.queue_complete_semaphores[i])
        {
            vkDestroySemaphore(context.device.logical_device, context.queue_complete_semaphores[i], context.allocator);
            context.queue_complete_semaphores[i] = 0;
        }
        vulkan_fence_destroy(&context, &context.in_flight_fences[i]);
    }
    ac_dyn_array_destroy_t(context.image_available_semaphores);
    context.image_available_semaphores = 0;
    ac_dyn_array_destroy_t(context.queue_complete_semaphores);
    context.queue_complete_semaphores = 0;
    ac_dyn_array_destroy_t(context.in_flight_fences);
    context.in_flight_fences = 0;
    ac_dyn_array_destroy_t(context.images_in_flight);
    context.images_in_flight = 0;

    // command buffer
    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        if (context.graphics_command_buffers[i].handle)
        {
            vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
            context.graphics_command_buffers[i].handle = 0;
        }
    }
    ac_dyn_array_destroy_t(context.graphics_command_buffers);
    context.graphics_command_buffers = 0;

    // framebuffer
    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
    }

    vulkan_renderpass_destroy(&context, &context.main_renderpass);
    vulkan_swapchain_destroy(&context, &context.swapchain);

    ACDEBUG("Destroy Vulkan Device...");
    vulkan_device_destroy(&context);

    ACDEBUG("Destroy Vulkan Surface...");
    if (context.surface)
    {
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = 0;
    }

    ACDEBUG("Destroy Vulkan Debugger...");
    if (context.debug_messenger)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
          (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debug_messenger, context.allocator);
    }

    ACDEBUG("Destroy Vulkan Instance...");
    vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_on_resize(renderer_backend* backend, u16 width, u16 height)
{
    cache_framebuffer_width = width;
    cache_framebuffer_height = height;
    context.framebuffer_size_generate++;

    ACINFO("Vulkan renderer backend->resized: w/h/gen: %i/%i/%11u", width, height, context.framebuffer_size_generate);
}

b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time)
{
    vulkan_device* device = &context.device;

    // check if recreating swapchain and boot.
    if (context.recreate_swapchain)
    {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if (!vulkan_result_is_success(result))
        {
            ACERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) failed: '%s'", vulkan_result_string(result, TRUE));
            return FALSE;
        }
        ACINFO("Recreating swapchain, booting.");
        return FALSE;
    }

    // check if framebuffer has been resized
    if (context.framebuffer_size_generate != context.framebuffer_size_last_generate)
    {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if (!vulkan_result_is_success(result))
        {
            ACERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed: '%s'", vulkan_result_string(result, TRUE));
            return FALSE;
        }

        if (!recreate_swapchain(backend))
            return FALSE;

        ACINFO("Resized. booting.");
        return FALSE;
    }

    // wait for the execute of the current frame complete.
    if (!vulkan_fence_wait(&context, &context.in_flight_fences[context.current_frame], UINT64_MAX))
    {
        ACWARN("In-flight fence wait failure!");
        return FALSE;
    }

    // acquire next image from swapchain. pass along semaphore that should signaled when this complete.
    if (!vulkan_swapchain_acquire_next_image_index(
          &context, &context.swapchain, UINT64_MAX, context.image_available_semaphores[context.current_frame], 0, &context.image_index))
        return FALSE;

    // begin recording command
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_command_buffer_reset(command_buffer);
    vulkan_command_buffer_begin(command_buffer, FALSE, FALSE, FALSE);

    // dynamic state
    // setup the way openGL works.
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context.framebuffer_height;
    viewport.width = (f32)context.framebuffer_width;
    viewport.height = -(f32)context.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context.framebuffer_width;
    scissor.extent.height = context.framebuffer_height;

    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

    // begin renderpass
    vulkan_renderpass_begin(command_buffer, &context.main_renderpass, context.swapchain.framebuffers[context.image_index].handle);

    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time)
{
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    // end renderpass
    vulkan_renderpass_end(command_buffer, &context.main_renderpass);
    vulkan_command_buffer_end(command_buffer);

    if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE)
        vulkan_fence_wait(&context, context.images_in_flight[context.image_index], UINT64_MAX);

    // mark image fence as in-use by this frame
    context.images_in_flight[context.image_index] = &context.in_flight_fences[context.current_frame];

    // reset fence for use the next frame
    vulkan_fence_reset(&context, &context.in_flight_fences[context.current_frame]);

    // submit queue and wait for operation to complete, then executed
    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.queue_complete_semaphores[context.current_frame];

    // wait semaphore
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores[context.current_frame];

    VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, context.in_flight_fences[context.current_frame].handle);
    if (result != VK_SUCCESS)
    {
        ACERROR("vkQueueSubmit failed with result: %s", vulkan_result_string(result, TRUE));
        return FALSE;
    }

    vulkan_command_buffer_update_submitted(command_buffer);
    // end of submitting

    // send image back to swapchain
    vulkan_swapchain_present(&context,
                             &context.swapchain,
                             context.device.graphics_queue,
                             context.device.present_queue,
                             context.queue_complete_semaphores[context.current_frame],
                             context.image_index);
    return TRUE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                 VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                 void* user_data)
{
    switch (message_severity)
    {
    default:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        ACERROR(callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        ACWARN(callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        ACINFO(callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        ACTRACE(callback_data->pMessage);
        break;
    }
    return VK_FALSE;
}

i32 find_memory_index(u32 type_filter, u32 property_flags)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &mem_properties);

    for (u32 i = 0; i < mem_properties.memoryTypeCount; ++i)
    {
        if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags)
            return i;
    }

    ACWARN("Unable to find suitable memory type!");
    return -1;
}

void create_command_buffers(renderer_backend* backend)
{
    if (!context.graphics_command_buffers)
    {
        context.graphics_command_buffers = ac_dyn_array_reserved_t(vulkan_command_buffer, context.swapchain.image_count);
        for (u32 i = 0; i < context.swapchain.image_count; ++i)
        {
            ac_zero_memory_t(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        }
    }

    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        if (context.graphics_command_buffers[i].handle)
        {
            vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
        }
        ac_zero_memory_t(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        vulkan_command_buffer_allocate(&context, context.device.graphics_command_pool, TRUE, &context.graphics_command_buffers[i]);
    }

    ACINFO("Vulkan command buffer created");
}

void regen_framebuffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass)
{
    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        u32 attachment_count = 2;
        VkImageView attachments[] = { swapchain->views[i], swapchain->depth_attachment.view };

        vulkan_framebuffer_create(&context,
                                  renderpass,
                                  context.framebuffer_width,
                                  context.framebuffer_height,
                                  attachment_count,
                                  attachments,
                                  &context.swapchain.framebuffers[i]);
    }
}

b8 recreate_swapchain(renderer_backend* backend)
{
    if (context.recreate_swapchain)
    {
        ACDEBUG("recreate_swapchain called when already recreate. boot.");
        return FALSE;
    }

    if (context.framebuffer_width == 0 || context.framebuffer_height == 0)
    {
        ACDEBUG("recreate_swapchain called when window is < 1 in a dimension. boot.");
        return FALSE;
    }

    context.recreate_swapchain = TRUE;
    vkDeviceWaitIdle(context.device.logical_device);
    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        context.images_in_flight[i] = 0;
    }

    // requeery swapchain support
    vulkan_device_query_swapchain_support(context.device.physical_device, context.surface, &context.device.swapchain_support);
    vulkan_device_detect_depth_format(&context.device);
    vulkan_swapchain_recreate(&context, cache_framebuffer_width, cache_framebuffer_height, &context.swapchain);

    // sync framebuffer size with cache size
    context.framebuffer_width = cache_framebuffer_width;
    context.framebuffer_height = cache_framebuffer_height;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    cache_framebuffer_width = 0;
    cache_framebuffer_height = 0;

    // context.framebuffer_size_generate = context.framebuffer_size_last_generate;
    context.framebuffer_size_last_generate = context.framebuffer_size_generate;
    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
    }

    for (u32 i = 0; i < context.swapchain.image_count; ++i)
    {
        vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
    }

    context.main_renderpass.x = 0;
    context.main_renderpass.y = 0;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

    regen_framebuffers(backend, &context.swapchain, &context.main_renderpass);
    create_command_buffers(backend);
    context.recreate_swapchain = FALSE;

    return TRUE;
}


