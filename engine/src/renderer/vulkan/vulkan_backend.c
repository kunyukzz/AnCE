#include "vulkan_backend.h"

#include "vulkan_device.h"
#include "vulkan_platform.h"
#include "vulkan_type.inl"

#include "container/dyn_array.h"
// #include "core/assertion.h"
#include "core/astring.h"
#include "core/logger.h"
#include "platform/platform.h"

static vulkan_context context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                 VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                 void* user_data);

b8 vulkan_renderer_backend_initialize(renderer_backend* backend, const char* app_name, struct platform_state* plat_state)
{
    // FIXME: custom allocator.
    context.allocator = 0;

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
            break;
            // HACK: I'm cheating here!!!
            // return TRUE;
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

    ACINFO("Vulkan renderer initialized");
    return TRUE;
}

void vulkan_renderer_backend_shutdown(renderer_backend* backend)
{
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

void vulkan_renderer_backend_on_resize(renderer_backend* backend, u16 width, u16 height) {}

b8 vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time)
{
    return TRUE;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time)
{
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
