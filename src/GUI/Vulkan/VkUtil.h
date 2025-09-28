#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <string>

#define C_ARRAY_SIZE(_x) (sizeof(_x) / sizeof((_x[0])))
#define VK_THROW_IF_FAILED(_expr) \
    do { \
        VkResult _vk_result = (_expr); \
        if (_vk_result != VK_SUCCESS) { \
            throw std::runtime_error( \
                std::string("[VULKAN] Vulkan error: ") + std::to_string(_vk_result) + \
                " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

struct VkUtil
{
    static uint32_t findMemoryTypeIdx(
        VkPhysicalDevice physicalDevice,
        const VkMemoryRequirements& requirements,
        VkMemoryPropertyFlags requiredFlags = 0);

    static void createImageView2D(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t iQueueFamily,
        VkFormat format,
        uint32_t width, uint32_t height,
        VkSampleCountFlagBits samples,
        VkImageUsageFlags usage,
        VkImageAspectFlags imageAspect,
        VkImage* image,
        VkImageView* imageView,
        VkDeviceMemory* memoryImage);
};

