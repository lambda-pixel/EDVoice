#pragma once

#include <vulkan/vulkan.h>
// WIN32 specific
#include <Windows.h>
#include <vulkan/vulkan_win32.h>

#include <vector>

#include "Swapchain.h"


struct CurrFrameInfo {
    uint32_t iSwapchainImage;
    VkSemaphore semaphoreImageAcquired;
    VkSemaphore semaphoreRenderReady;
    bool acquired;
};


class VkAdapter
{
public:
    VkAdapter(const std::vector<char*>& instanceExtensions = {});

    ~VkAdapter();

    // WIN32 specific
    void initDevice(
        HINSTANCE hInstance, HWND hwnd,
        const std::vector<char*>& deviceExtensions = {}
    );

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

private:
    void createFramebuffer();
    void createRenderPass();
    void createCommandBuffer();

    VkInstance _instance = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    uint32_t _iQueueFamily;
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

    CurrFrameInfo _currFrameInfo;

    VkSurfaceKHR _surface;

public:
    const uint32_t API_VERSION = VK_API_VERSION_1_0;
};