#pragma once

#include <vulkan/vulkan.h>

#define C_ARRAY_SIZE(_x) (sizeof(_x) / sizeof((_x[0])))

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