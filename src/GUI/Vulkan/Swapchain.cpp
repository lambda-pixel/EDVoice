#include "Swapchain.h"
#include "VkUtil.h"

#include <stdexcept>

Swapchain::Swapchain(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkSurfaceKHR surface,
    uint32_t width, uint32_t height,
    VkImageLayout optimalLayout)
    : _physicalDevice(physicalDevice)
    , _device(device)
    , _optimalLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    , _surface(surface)
    , _swapchain(VK_NULL_HANDLE)
{
    updateSwapchain(width, height);
}


Swapchain::~Swapchain()
{
    //vkWaitForFences(_device, _fencePresent.size(), _fencePresent.data(), VK_TRUE, UINT64_MAX);
    vkDeviceWaitIdle(_device);

    for (uint32_t i = 0; i < _nSwapchainImages; i++) {
        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
        vkDestroySemaphore(_device, _semaphoreImageAcquired[i], nullptr);
        vkDestroySemaphore(_device, _semaphoreRenderReady[i], nullptr);
    }

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
}

VkCompositeAlphaFlagBitsKHR chooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& caps) {
    if (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
        return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    if (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
        return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    if (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

void Swapchain::updateSwapchain(uint32_t width, uint32_t height)
{
    // TODO: optimize using fences?
    vkDeviceWaitIdle(_device);

    // Free resources
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    // Find surface capabilties
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &surfaceCapabilities);

    uint32_t nFormats;
    std::vector<VkSurfaceFormatKHR> supportedFormats;

    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &nFormats, nullptr);
    supportedFormats.resize(nFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &nFormats, supportedFormats.data());

    bool formatFound = false;

    const std::vector<VkFormat> preferred = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        //VK_FORMAT_B8G8R8A8_SRGB,
        //VK_FORMAT_R8G8B8A8_SRGB
    };

    for (const VkSurfaceFormatKHR& format : supportedFormats) {
        for (const auto& f: preferred) {
            if ((format.colorSpace & VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
                format.format == f)
            {
                _swapchainFormat = format;
                formatFound = true;
                break;
            }
        }

        if (formatFound) {
            break;
        }
    }

    if (!formatFound) {
        throw std::runtime_error("[VULKAN] Failed to create swapchain: could not find a suitable format");
    }

    // Great... worked seamlessly with WIN32 and X with
    // surfaceCapabilities.currentExtent.width & height but somehow
    // broken on this damn Wayland...
    _format = _swapchainFormat.format;
    _width  = width;
    _height = height;

    if (_width == UINT32_MAX || _height == UINT32_MAX) {
        // Wayland seems to return crazy value when creating the window
        return;
    }

    const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr, 0,// sType, pNext, flags
        _surface,                                               // surface
        surfaceCapabilities.minImageCount,                      // minImageCount
        _format,                                                // imageFormat
        _swapchainFormat.colorSpace,                            // imageColorSpace
        { _width, _height },//surfaceCapabilities.currentExtent,                      // imageExtent
        1,                                                      // imageArrayLayers
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,                    // imageUsage
        VK_SHARING_MODE_EXCLUSIVE,                              // imageSharingMode
        0, nullptr,                                             // sharingQueues
        surfaceCapabilities.currentTransform,                   // preTransform
        chooseCompositeAlpha(surfaceCapabilities),                      // compositeAlpha
        VK_PRESENT_MODE_FIFO_KHR,                               // presentMode
        VK_FALSE,                                               // clipped
        VK_NULL_HANDLE//_swapchain                                              // oldSwapchain
    };

    VK_THROW_IF_FAILED(vkCreateSwapchainKHR(_device, &swapchainCreateInfo, nullptr, &_swapchain));

    // Get images
    vkGetSwapchainImagesKHR(_device, _swapchain, &_nSwapchainImages, nullptr);
    _swapchainImages.resize(_nSwapchainImages);
    vkGetSwapchainImagesKHR(_device, _swapchain, &_nSwapchainImages, _swapchainImages.data());

    initSwapchainResources();

    //const VkFenceCreateInfo fenceCreateInfo = {
    //    VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr,
    //    VK_FENCE_CREATE_SIGNALED_BIT
    //};

    _fenceRendered.resize(_nSwapchainImages);

    for (size_t i = 0; i < _nSwapchainImages; i++) {
        _fenceRendered[i] = VK_NULL_HANDLE;
    }

    _iCurrFrame = 0;
    _valid = true;
}


void Swapchain::initSwapchainResources()
{
    // Free resources
    for (const VkImageView& imageView : _swapchainImageViews) {
        vkDestroyImageView(_device, imageView, nullptr);
    }

    for (const VkSemaphore& semaphore : _semaphoreImageAcquired) {
        vkDestroySemaphore(_device, semaphore, nullptr);
    }

    for (const VkSemaphore& semaphore : _semaphoreRenderReady) {
        vkDestroySemaphore(_device, semaphore, nullptr);
    }

    // Create imageviews
    _swapchainImageViews.resize(_nSwapchainImages);

    for (uint32_t i = 0; i < _nSwapchainImages; i++) {
        const VkImageViewCreateInfo imageViewCreateInfo = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0,   // sType, pNext, flags
            _swapchainImages[i],                                    // image
            VK_IMAGE_VIEW_TYPE_2D,                                  // viewType
            _format,                                                // format
            {   // components
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            {   // subresourceRange
                VK_IMAGE_ASPECT_COLOR_BIT,                          // aspect
                0, 1,                                               // mips
                0, 1,                                               // layers
            }
        };

        VK_THROW_IF_FAILED(vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &_swapchainImageViews[i]));
    }

    // Create synchronization tools
    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
    };

    _semaphoreImageAcquired.resize(_nSwapchainImages);
    _semaphoreRenderReady.resize(_nSwapchainImages);

    for (uint32_t i = 0; i < _nSwapchainImages; i++) {
        VK_THROW_IF_FAILED(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_semaphoreImageAcquired[i]));
        VK_THROW_IF_FAILED(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_semaphoreRenderReady[i]));
    }
}


VkResult Swapchain::acquireNextImage(
    uint32_t* iSwapchainImage,
    VkSemaphore* semaphoreImageAcquired,
    VkSemaphore* semaphoreRenderReady)
{
    if (!_valid) { 
        return VK_ERROR_OUT_OF_DATE_KHR;
    }

    if (_fenceRendered[_iCurrFrame] != VK_NULL_HANDLE) {
        vkWaitForFences(_device, 1, &_fenceRendered[_iCurrFrame], VK_TRUE, UINT64_MAX);
    }

    const VkResult result = vkAcquireNextImageKHR(
        _device,
        _swapchain,
        0,
        _semaphoreImageAcquired[_iCurrFrame],
        VK_NULL_HANDLE,//_fencePresent[_iCurrFrame],
        &_iSwapchainImage
    );

    switch (result) {
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        // updateSwapchain();
        break;

    case VK_SUCCESS:
        *iSwapchainImage = _iSwapchainImage;
        *semaphoreImageAcquired = _semaphoreImageAcquired[_iCurrFrame];
        *semaphoreRenderReady = _semaphoreRenderReady[_iSwapchainImage];
        break;

        // TODO:
        //VK_NOT_READY
        //VK_SUBOPTIMAL_KHR
        //VK_SUCCESS
        //VK_TIMEOUT

    default:
        break;
    }

    return result;
}


void Swapchain::present(VkQueue queue, VkFence fenceComandBuffer)
{
    const VkPresentInfoKHR presentInfo = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr,    // sType, pNext
        1, &_semaphoreRenderReady[_iSwapchainImage],    // waitSemaphore
        1, &_swapchain, &_iSwapchainImage,              // swapchains
        nullptr                                         // result
    };

    vkQueuePresentKHR(queue, &presentInfo);

    _fenceRendered[_iCurrFrame] = fenceComandBuffer;

    _iCurrFrame = (_iCurrFrame + 1) % _nSwapchainImages;
}