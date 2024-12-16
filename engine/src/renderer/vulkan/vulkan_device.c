#include "vulkan_device.h"
#include "container/dyn_array.h"
#include "core/acmemory.h"
#include "core/astring.h"
#include "core/logger.h"

typedef struct vulkan_physical_device_requirement
{
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;

    const char** device_extension_name;
    b8 sampler_anisotrophy;
    b8 discrete_gpu;
} vulkan_physical_device_requirement;

typedef struct vulkan_physical_device_queue_family_info
{
    u32 graphics_family_index;
    u32 present_family_index;
    u32 compute_family_index;
    u32 transfer_family_index;
} vulkan_physical_device_queue_family_info;

b8 select_physical_device(vulkan_context* context);

b8 physical_device_meet_requirement(VkPhysicalDevice device,
                                    VkSurfaceKHR surface,
                                    const VkPhysicalDeviceProperties* properties,
                                    const VkPhysicalDeviceFeatures* feature,
                                    const vulkan_physical_device_requirement* requirement,
                                    vulkan_physical_device_queue_family_info* out_queue_family_info,
                                    vulkan_swapchain_support_info* out_swapchain_support);

b8 vulkan_device_create(vulkan_context* context)
{
    if (!select_physical_device(context))
        return FALSE;

    ACINFO("Creating logical devices...");

    // return TRUE;

    b8 present_share_graphic_queue = context->device.graphics_queue_index == context->device.present_queue_index;
    b8 transfer_share_graphic_queue = context->device.graphics_queue_index == context->device.transfer_queue_index;
    u32 index_count = 1;
    if (!present_share_graphic_queue)
        index_count++;
    if (!transfer_share_graphic_queue)
        index_count++;
    u32 indices[index_count];
    u8 index = 0;
    indices[index++] = context->device.graphics_queue_index;

    if (!present_share_graphic_queue)
        indices[index++] = context->device.present_queue_index;
    if (!transfer_share_graphic_queue)
        indices[index++] = context->device.transfer_queue_index;

    VkDeviceQueueCreateInfo queue_create_infos[index_count];
    for (u32 i = 0; i < index_count; ++i)
    {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = indices[i];
        queue_create_infos[i].queueCount = 1;

        // NOTE: this was for multithreading
        // if (indices[i] == context->device.graphics_queue_index)
        //     queue_create_infos[i].queueCount = 2;

        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = 0;
        f32 queue_priority = 1.0f;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
    }

    // requesting device feature
    VkPhysicalDeviceFeatures device_features = {};
    // device_features.samplerAnisotropy = VK_TRUE; // TEST: idk if i needed this.

    VkDeviceCreateInfo device_create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = 1;
    const char* extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    device_create_info.ppEnabledExtensionNames = &extension_names;

    // ignored. so pass nothing
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;

    // create device
    VK_CHECK(vkCreateDevice(context->device.physical_device, &device_create_info, context->allocator, &context->device.logical_device));
    ACINFO("Logical Device Created");

    // get queue
    vkGetDeviceQueue(context->device.logical_device, context->device.graphics_queue_index, 0, &context->device.graphics_queue);
    vkGetDeviceQueue(context->device.logical_device, context->device.present_queue_index, 0, &context->device.present_queue);
    vkGetDeviceQueue(context->device.logical_device, context->device.transfer_queue_index, 0, &context->device.transfer_queue);
    ACINFO("Queues obtained");

    // create command pool for graphics queue
    VkCommandPoolCreateInfo pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_create_info.queueFamilyIndex = context->device.graphics_queue_index;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(context->device.logical_device, &pool_create_info, context->allocator, &context->device.graphics_command_pool));
    ACINFO("Graphics command pool created");

    return TRUE;
}

void vulkan_device_destroy(vulkan_context* context)
{
    // unset queueueueueueeu
    context->device.graphics_queue = 0;
    context->device.present_queue = 0;
    context->device.transfer_queue = 0;

    ACINFO("Destroy command pool...");
    vkDestroyCommandPool(context->device.logical_device, context->device.graphics_command_pool, context->allocator);

    // destroy logical device
    ACINFO("Destroy logical device...");
    if (context->device.logical_device)
    {
        vkDestroyDevice(context->device.logical_device, context->allocator);
        context->device.logical_device = 0;
    }

    // of course physical device not destroyed, right........
    ACINFO("Releasing physical device resources...");
    context->device.physical_device = 0;

    if (context->device.swapchain_support.formats)
    {
        ac_free_t(context->device.swapchain_support.formats,
                  sizeof(VkSurfaceFormatKHR) * context->device.swapchain_support.format_count,
                  MEMTAG_RENDERER);
        context->device.swapchain_support.formats = 0;
        context->device.swapchain_support.format_count = 0;
    }

    if (context->device.swapchain_support.present_modes)
    {
        ac_free_t(context->device.swapchain_support.present_modes,
                  sizeof(VkPresentModeKHR) * context->device.swapchain_support.present_mode_count,
                  MEMTAG_RENDERER);
        context->device.swapchain_support.present_modes = 0;
        context->device.swapchain_support.present_mode_count = 0;
    }

    ac_zero_memory_t(&context->device.swapchain_support.capabilities, sizeof(context->device.swapchain_support.capabilities));

    context->device.graphics_queue_index = -1;
    context->device.present_queue_index = -1;
    context->device.transfer_queue_index = -1;
}

void vulkan_device_query_swapchain_support(VkPhysicalDevice physical_device,
                                           VkSurfaceKHR surface,
                                           vulkan_swapchain_support_info* out_support_info)
{
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &out_support_info->capabilities));
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, 0));
    if (out_support_info->format_count != 0)
    {
        if (!out_support_info->formats)
        {
            out_support_info->formats = ac_allocate_t(sizeof(VkSurfaceFormatKHR) * out_support_info->format_count, MEMTAG_RENDERER);
        }
        VK_CHECK(
          vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, out_support_info->formats));
    }

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_support_info->present_mode_count, 0));
    if (out_support_info->present_mode_count != 0)
    {
        if (!out_support_info->present_modes)
        {
            out_support_info->present_modes =
              ac_allocate_t(sizeof(VkPresentModeKHR) * out_support_info->present_mode_count, MEMTAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
          physical_device, surface, &out_support_info->present_mode_count, out_support_info->present_modes));
    }
}

b8 vulkan_device_detect_depth_format(vulkan_device* device)
{
    const u64 candidate_count = 3;
    VkFormat candidates[3] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < candidate_count; ++i)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physical_device, candidates[i], &properties);
        if ((properties.linearTilingFeatures & flags) == flags)
        {
            device->depth_format = candidates[i];
            return TRUE;
        }
        else if ((properties.optimalTilingFeatures & flags) == flags)
        {
            device->depth_format = candidates[i];
            return TRUE;
        }
    }

    return FALSE;
}

b8 select_physical_device(vulkan_context* context)
{
    u32 physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
    if (physical_device_count == 0)
    {
        ACFATAL("No devices support Vulkan were found!");
        return FALSE;
    }

    VkPhysicalDevice physical_device[physical_device_count];
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_device));
    for (u32 i = 0; i < physical_device_count; ++i)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_device[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_device[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_device[i], &memory);

        // TODO: requirement should be driven by engine configuration
        vulkan_physical_device_requirement requirement = {};
        requirement.graphics = TRUE;
        requirement.present = TRUE;
        requirement.transfer = TRUE;

        // NOTE: enable this if compute will be required
        // requirement.compute = TRUE;
        requirement.sampler_anisotrophy = TRUE;

        requirement.discrete_gpu = FALSE; // HACK: I'm cheating in this line. the correct value was TRUE
        requirement.device_extension_name = ac_dyn_array_create_t(const char*);
        ac_dyn_array_push_t(requirement.device_extension_name, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        vulkan_physical_device_queue_family_info queue_info = {};
        b8 result = physical_device_meet_requirement(
          physical_device[i], context->surface, &properties, &features, &requirement, &queue_info, &context->device.swapchain_support);
        if (result)
        {
            ACINFO("Selected device: '%s'.", properties.deviceName);
            switch (properties.deviceType)
            {
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                ACINFO("GPU is unknown.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                ACINFO("GPU type is Integrated.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                ACINFO("GPU type is Discrete.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                ACINFO("GPU type is Virtual.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                ACINFO("GPU type is CPU.");
                break;
            }
            ACINFO("GPU Driver Version: %d.%d.%d",
                   VK_VERSION_MAJOR(properties.driverVersion),
                   VK_VERSION_MINOR(properties.driverVersion),
                   VK_VERSION_PATCH(properties.driverVersion));

            ACINFO("Vulkan API Version: %d.%d.%d",
                   VK_VERSION_MAJOR(properties.apiVersion),
                   VK_VERSION_MINOR(properties.apiVersion),
                   VK_VERSION_PATCH(properties.apiVersion));

            for (u32 j = 0; j < memory.memoryHeapCount; ++j)
            {
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    ACINFO("Local GPU memory: %.2f GiB", memory_size_gib);
                }
                else
                {
                    ACINFO("Shared system memory: %.2f GiB", memory_size_gib);
                }
            }

            context->device.physical_device = physical_device[i];
            context->device.graphics_queue_index = queue_info.graphics_family_index;
            context->device.present_queue_index = queue_info.present_family_index;
            context->device.transfer_queue_index = queue_info.transfer_family_index;

            context->device.properties = properties;
            context->device.features = features;
            context->device.memory = memory;
            break;
        }
    }

    if (!context->device.physical_device)
    {
        ACERROR("No physical device found to meet the requirement.");
        return FALSE; // correct version
    }

    ACINFO("Physical Device Selected");
    return TRUE;
}

b8 physical_device_meet_requirement(VkPhysicalDevice device,
                                    VkSurfaceKHR surface,
                                    const VkPhysicalDeviceProperties* properties,
                                    const VkPhysicalDeviceFeatures* feature,
                                    const vulkan_physical_device_requirement* requirement,
                                    vulkan_physical_device_queue_family_info* out_queue_family_info,
                                    vulkan_swapchain_support_info* out_swapchain_support)
{
    out_queue_family_info->graphics_family_index = -1;
    out_queue_family_info->present_family_index = -1;
    out_queue_family_info->compute_family_index = -1;
    out_queue_family_info->transfer_family_index = -1;

    if (requirement->discrete_gpu)
    {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            ACINFO("Device is not a discrete GPU, and one is required. Skip.");
            return FALSE;
        }
    }
    // else
    // {
    //     ACINFO("Integrated GPU found. Proceeding with the check...");
    //     return TRUE;
    // }

    u32 queue_fam_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_fam_count, 0);
    VkQueueFamilyProperties queue_fam[queue_fam_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_fam_count, queue_fam);

    ACTRACE("Graphics | Present | Compute | Transfer | Name");
    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queue_fam_count; ++i)
    {
        // graohic queue
        u8 current_transfer_score = 0;
        if (queue_fam[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            out_queue_family_info->graphics_family_index = i;
            ++current_transfer_score;
        }

        // compute queue
        if (queue_fam[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            out_queue_family_info->compute_family_index = i;
            ++current_transfer_score;
        }

        // transfer queue
        if (queue_fam[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            if (current_transfer_score <= min_transfer_score)
            {
                min_transfer_score = current_transfer_score;
                out_queue_family_info->transfer_family_index = i;
            }
        }

        VkBool32 support_present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &support_present));
        if (support_present)
        {
            out_queue_family_info->present_family_index = i;
        }
    }

    ACTRACE("        %d |        %d |        %d |        %d | %s",
            out_queue_family_info->graphics_family_index != -1,
            out_queue_family_info->present_family_index != -1,
            out_queue_family_info->compute_family_index != -1,
            out_queue_family_info->transfer_family_index != -1,
            properties->deviceName);

    if ((!requirement->graphics || (requirement->graphics && out_queue_family_info->graphics_family_index != -1)) &&
        (!requirement->present || (requirement->present && out_queue_family_info->present_family_index != -1)) &&
        (!requirement->compute || (requirement->compute && out_queue_family_info->compute_family_index != -1)) &&
        (!requirement->transfer || (requirement->transfer && out_queue_family_info->transfer_family_index != -1)))
    {
        ACINFO("Device meets queue requirement");
        ACTRACE("Graphics Family Index: %i", out_queue_family_info->graphics_family_index);
        ACTRACE("Present Family Index: %i", out_queue_family_info->present_family_index);
        ACTRACE("Compute Family Index: %i", out_queue_family_info->compute_family_index);
        ACTRACE("Transfer Family Index: %i", out_queue_family_info->transfer_family_index);

        vulkan_device_query_swapchain_support(device, surface, out_swapchain_support);

        if (out_swapchain_support->format_count < 1 || out_swapchain_support->present_mode_count < 1)
        {
            if (out_swapchain_support->formats)
            {
                ac_free_t(out_swapchain_support->formats,
                          sizeof(VkSurfaceFormatKHR) * out_swapchain_support->format_count,
                          MEMTAG_RENDERER);
            }
            if (out_swapchain_support->present_modes)
            {
                ac_free_t(out_swapchain_support->present_modes,
                          sizeof(VkPresentModeKHR) * out_swapchain_support->present_mode_count,
                          MEMTAG_RENDERER);
            }
            ACINFO("Required swapchain support not present. Skip device");
            return FALSE;
        }

        if (requirement->device_extension_name)
        {
            u32 available_extension_count = 0;
            VkExtensionProperties* available_extension = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, 0));
            if (available_extension_count != 0)
            {
                available_extension = ac_allocate_t(sizeof(VkExtensionProperties) * available_extension_count, MEMTAG_RENDERER);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, available_extension));

                u32 required_extension_count = ac_dyn_array_length_t(requirement->device_extension_name);
                for (u32 i = 0; i < required_extension_count; ++i)
                {
                    b8 found = FALSE;
                    for (u32 j = 0; j < available_extension_count; ++j)
                    {
                        if (string_equal(requirement->device_extension_name[i], available_extension[j].extensionName))
                        {
                            found = TRUE;
                            break;
                        }
                    }
                    if (!found)
                    {
                        ACINFO("Required extension not found: '%s'. Skip device", requirement->device_extension_name[i]);
                        ac_free_t(available_extension, sizeof(VkExtensionProperties) * available_extension_count, MEMTAG_RENDERER);
                        return FALSE;
                    }
                }
            }
            ac_free_t(available_extension, sizeof(VkExtensionProperties) * available_extension_count, MEMTAG_RENDERER);
        }
        if (requirement->sampler_anisotrophy && !feature->samplerAnisotropy)
        {
            ACINFO("Device does not support samplerAnisothropy. Skip");
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}
