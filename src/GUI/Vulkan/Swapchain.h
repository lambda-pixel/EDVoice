#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class Swapchain
{
public:
    Swapchain(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkSurfaceKHR surface,
        VkImageLayout optimalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    virtual void updateSwapchain();
    virtual void initSwapchainResources();

    virtual ~Swapchain();

    uint32_t nSwapchainImages() const { return _nSwapchainImages; }

    virtual VkResult acquireNextImage(
        uint32_t* iSwapchainImage,
        VkSemaphore* semaphoreImageAcquired,
        VkSemaphore* semaphoreRenderReady);

    virtual void present(
        VkQueue queue,
        VkFence fenceComandBuffer);

    VkImageView getImageView(size_t i) { return _swapchainImageViews[i]; }

    VkFormat getFormat() const { return _format; }
    VkImageLayout getOptimalLayout() const { return _optimalLayout; }

    uint32_t width() const { return _width; }
    uint32_t height() const { return _height; }

protected:
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;

    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    VkSurfaceFormatKHR _swapchainFormat;

    uint32_t _nSwapchainImages;
    uint32_t _iSwapchainImage;
    uint32_t _iCurrFrame = 0;

    std::vector<VkSemaphore> _semaphoreImageAcquired;
    std::vector<VkSemaphore> _semaphoreRenderReady;

    std::vector<VkFence> _fenceRendered;

    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkImageLayout _optimalLayout;
    VkFormat _format;
    uint32_t _width, _height;
};