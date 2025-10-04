#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Swapchain.h"

class WindowSystem;

struct CurrFrameInfo {
    uint32_t iSwapchainImage;
    VkSemaphore semaphoreImageAcquired;
    VkSemaphore semaphoreRenderReady;
    bool acquired;
};


class VkAdapter
{
public:
    VkAdapter(
        WindowSystem* windowSystem,
        const std::vector<const char*>& instanceExtensions = {});

    ~VkAdapter();

    void initDevice(const std::vector<const char*>& deviceExtensions = {});

    VkInstance getInstance() const { return _instance; }
    VkPhysicalDevice getPhysicalDevice() const { return _physicalDevice; }
    VkDevice getDevice() const { return _device;  }

    uint32_t getQueueFamily() const { return _iQueueFamily; }
    VkQueue getQueue() const { return _queue; }
    VkRenderPass getRenderPass() const { return _renderPass; }

    uint32_t nImageCount() const { return _swapchain->nSwapchainImages();  }

    VkCommandBuffer startNewFrame();
    void renderFrame();
    void presentFrame();

    void resized(uint32_t width, uint32_t height);

private:
    void createFramebuffer();
    void createRenderPass();
    void createCommandBuffer();

    WindowSystem* _windowSystem;

    VkInstance _instance = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    uint32_t _iQueueFamily = 0;
    VkQueue _queue = VK_NULL_HANDLE;

    Swapchain* _swapchain = nullptr;

    // createFramebuffer
    std::vector<VkFramebuffer> _framebuffer;

    // createRenderPass
    VkRenderPass _renderPass = VK_NULL_HANDLE;

    // createCommandBuffer
    VkCommandPool _commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> _commandBuffer;
    std::vector<VkFence> _fenceCommandBuffer;

    CurrFrameInfo _currFrameInfo = { 0, VK_NULL_HANDLE, VK_NULL_HANDLE, false };
    VkSurfaceKHR _surface = VK_NULL_HANDLE;

public:
    const uint32_t API_VERSION = VK_API_VERSION_1_0;
};