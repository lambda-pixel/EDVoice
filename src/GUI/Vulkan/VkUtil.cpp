#include "VkUtil.h"


uint32_t VkUtil::findMemoryTypeIdx(
    VkPhysicalDevice physicalDevice,
    const VkMemoryRequirements& requirements,
    VkMemoryPropertyFlags requiredFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (size_t i_bit = 0; i_bit < memoryProperties.memoryTypeCount; i_bit++) {
        if (requirements.memoryTypeBits & (1 << i_bit)) {
            if ((memoryProperties.memoryTypes[i_bit].propertyFlags & requiredFlags) == requiredFlags) {
                return i_bit;
            }
        }
    }

    return 0;
}


void VkUtil::createImageView2D(
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
    VkDeviceMemory* memoryImage)
{
    const VkImageCreateInfo imageCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, 0,    // sType, pNext, flags
        VK_IMAGE_TYPE_2D,                                   // imageType
        format,                                             // format
        {width, height, 1},                                 // extent
        1, 1,                                               // mips, layers
        samples,                                            // samples
        VK_IMAGE_TILING_OPTIMAL,                            // tiling
        usage,                                              // usage
        VK_SHARING_MODE_EXCLUSIVE,                          // sharingMode
        1, &iQueueFamily,                                   // queueFamilyInidices
        VK_IMAGE_LAYOUT_UNDEFINED
    };

    vkCreateImage(device, &imageCreateInfo, nullptr, image);

    // Allocate device memory
    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, *image, &imageMemoryRequirements);

    const uint32_t iImageRenderMemoryType = findMemoryTypeIdx(
        physicalDevice,
        imageMemoryRequirements);

    const VkMemoryAllocateInfo memoryImageAllocateInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,    // sType, pNext
        imageMemoryRequirements.size,                       // allocationSize
        iImageRenderMemoryType                              // memoryTypeIndex
    };

    vkAllocateMemory(device, &memoryImageAllocateInfo, nullptr, memoryImage);
    vkBindImageMemory(device, *image, *memoryImage, 0);

    // Create image view
    const VkImageViewCreateInfo imageViewCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0,   // sType, pNext, flags
        *image,                                                 // image
        VK_IMAGE_VIEW_TYPE_2D,                                  // viewType
        format,                                                 // format
        {                                                       // components
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        {
            imageAspect,                                         // aspectMask
            0, 1,                                                // baseMipLevel, levelCount
            0, 1                                                 // baseArrayLayer, layerCount
        }
    };

    vkCreateImageView(device, &imageViewCreateInfo, nullptr, imageView);
}