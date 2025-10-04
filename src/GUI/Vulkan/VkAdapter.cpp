#include "VkAdapter.h"
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <string>

#include "VkUtil.h"


VkAdapter::VkAdapter(const std::vector<const char*>& instanceExtensions)
    : _instance(VK_NULL_HANDLE)
    , _physicalDevice(VK_NULL_HANDLE)
    , _device(VK_NULL_HANDLE)
    , _iQueueFamily(0)
    , _queue(VK_NULL_HANDLE)
    , _swapchain(nullptr)
    , _renderPass(VK_NULL_HANDLE)
    , _commandPool(VK_NULL_HANDLE)
    , _currFrameInfo{ 0, VK_NULL_HANDLE, VK_NULL_HANDLE, false }
    , _surface(VK_NULL_HANDLE)
{
    // Create Vulkan Instance
    const VkApplicationInfo applicationInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr,        // sType, pNext
        "EDVoice",                                          // pApplicationName
        VK_MAKE_VERSION(0, 1, 0),                           // applicationVersion
        "None",                                             // pEngineName
        VK_MAKE_VERSION(0, 1, 0),                           // engineVersion
        API_VERSION                                         // apiVersion
    };

    std::vector<const char*> enabledLayerNames;

#ifdef VULKAN_DEBUG_LAYER
    std::cout << "[VULKAN] Enabling Vulkan validation layer" << std::endl;
    enabledLayerNames.push_back("VK_LAYER_KHRONOS_validation");
#endif

    std::vector<const char*> extensions(instanceExtensions);
#ifdef _WIN32
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back("VK_KHR_win32_surface");
#else
    uint32_t nInstanceExt;
    const char* const* instanceExt = SDL_Vulkan_GetInstanceExtensions(&nInstanceExt);

    for (size_t i = 0; i < nInstanceExt; i++) {
        extensions.push_back(instanceExt[i]);
    }
#endif

    const VkInstanceCreateInfo instanceCreateInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, // sType, pNext, flags
        &applicationInfo,                                   // pApplicationInfo
        static_cast<uint32_t>(enabledLayerNames.size()),
        enabledLayerNames.data(),                           // ppEnabledLayerNames
        static_cast<uint32_t>(extensions.size()),
        extensions.data()                                   // ppEnabledExtensionNames
    };

    VK_THROW_IF_FAILED(vkCreateInstance(&instanceCreateInfo, nullptr, &_instance));
}


VkAdapter::~VkAdapter()
{
    if (_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);
        delete _swapchain;

        // createRenderPass
        vkDestroyRenderPass(_device, _renderPass, nullptr);

        // createFramebuffer
        for (size_t i = 0; i < _framebuffer.size(); i++) {
            vkDestroyFramebuffer(_device, _framebuffer[i], nullptr);
        }

        // createCommandBuffer
        vkDestroyCommandPool(_device, _commandPool, nullptr);

        for (const VkFence& fence : _fenceCommandBuffer) {
            vkDestroyFence(_device, fence, nullptr);
        }

        // initDevice
        vkDestroySurfaceKHR(_instance, _surface, nullptr);

        vkDestroyDevice(_device, nullptr);
        vkDestroyInstance(_instance, nullptr);
    }
}

void VkAdapter::initDevice(
#ifdef _WIN32
    HINSTANCE hInstance, HWND hwnd,
#else
    SDL_Window* window,
#endif
    const std::vector<const char*>& deviceExtensions)
{
    std::vector<const char*> extensions(deviceExtensions);
    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

#ifdef _WIN32
    // Create a Vulkan surface (WIN32 specific)
    const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, // sType, pNext
        0,                                                        // flags
        hInstance,                                                // hinstance
        hwnd                                                      // hwnd
    };

    vkCreateWin32SurfaceKHR(_instance, &surfaceCreateInfo, nullptr, &_surface);
#else
    SDL_Vulkan_CreateSurface(window, _instance, nullptr, &_surface);
    
    // WTF... I have to call GetWindowSize first and then GetWindowSizeInPixel just for
    // lord Wayland. Damn
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
#endif

    // Find a suitable physical device
    uint32_t nPhysicalDevices;
    std::vector<VkPhysicalDevice> physicalDevices;
    bool physicalDeviceFound = false;

    vkEnumeratePhysicalDevices(_instance, &nPhysicalDevices, nullptr);
    physicalDevices.resize(nPhysicalDevices);
    vkEnumeratePhysicalDevices(_instance, &nPhysicalDevices, physicalDevices.data());

    for (const VkPhysicalDevice& device : physicalDevices) {
        VkPhysicalDeviceProperties deviceProperties;
        std::vector<VkQueueFamilyProperties> queueProperties;
        uint32_t nQueues;

        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueues, nullptr);
        queueProperties.resize(nQueues);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueues, queueProperties.data());

        for (uint32_t iQueue = 0; iQueue < nQueues; iQueue++) {
            const VkQueueFamilyProperties& currentQueue = queueProperties[iQueue];

            // TODO: safer !
            VkBool32 supported = VK_FALSE;

            vkGetPhysicalDeviceSurfaceSupportKHR(
                device,
                iQueue,
                _surface,
                &supported);

            if (supported &&
                (currentQueue.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT &&
                (currentQueue.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT) {
                physicalDeviceFound = true;
                _physicalDevice = device;
                _iQueueFamily = iQueue;

                std::cout << "[VULKAN] Selected device: " << deviceProperties.deviceName << std::endl;

                break;
            }
        }

        // Suitable device found, no need to search further...
        if (physicalDeviceFound) {
            break;
        }
    }

    if (!physicalDeviceFound) {
        throw std::runtime_error("Could not find a suitable Vulkan device");
    }

    // Create device
    const float queuePriorities[] = { 1.f };

    const VkDeviceQueueCreateInfo queueCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, // sType, pNext, flags
        _iQueueFamily, 1, queuePriorities                       // queueFamilyIndex, queueCount, pQueuePriorities
    };

    const VkDeviceCreateInfo deviceCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, nullptr, 0,       // sType, pNext, flags
        1, &queueCreateInfo,                                    // pQueueCreateInfos
        0, nullptr,                                             // ppEnabledLayerNames
        (uint32_t)extensions.size(), extensions.data(),         // ppEnabledExtensionNames
        nullptr                                                 // VkPhysicalDeviceFeatures
    };

    VK_THROW_IF_FAILED(vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device));
    vkGetDeviceQueue(_device, _iQueueFamily, 0, &_queue);

    _swapchain = new Swapchain(_physicalDevice, _device, _surface, width, height);

    createRenderPass();

    if (_swapchain->valid()) {
        createFramebuffer();
        createCommandBuffer();
    }
}


VkCommandBuffer VkAdapter::startNewFrame()
{
    _currFrameInfo.acquired = false;

    uint32_t iSwapchainImage;

    const VkResult acquired = _swapchain->acquireNextImage(
        &iSwapchainImage,
        &_currFrameInfo.semaphoreImageAcquired,
        &_currFrameInfo.semaphoreRenderReady);

    if (acquired == VK_SUCCESS) {
        vkWaitForFences(_device, 1, &_fenceCommandBuffer[iSwapchainImage], VK_TRUE, UINT64_MAX);
        vkResetFences(_device, 1, &_fenceCommandBuffer[iSwapchainImage]);

        _currFrameInfo.iSwapchainImage = iSwapchainImage;
        _currFrameInfo.acquired = true;

        // Start recording
        const VkCommandBufferBeginInfo beginInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0,    // sType, pNext, flags
            nullptr
        };

        vkBeginCommandBuffer(_commandBuffer[iSwapchainImage], &beginInfo);

        // Rendering
        const VkClearValue clearValue[] = { {0, 0, 0, 0} };
        const VkRenderPassBeginInfo renderPassBeginInfo = {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr,  // sType, pNext
            _renderPass,                                        // renderpass
            _framebuffer[iSwapchainImage],                      // framebuffer
            {                                                   // renderArea
                {0, 0},                                         // offset
                {_swapchain->width(), _swapchain->height()},    // extent
            },
            1, clearValue                                       // clearValues
        };

        vkCmdBeginRenderPass(_commandBuffer[iSwapchainImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        return _commandBuffer[iSwapchainImage];
    }

    return VK_NULL_HANDLE;
}


void VkAdapter::renderFrame()
{
    if (_currFrameInfo.acquired) {
        const uint32_t iSwapchainImage = _currFrameInfo.iSwapchainImage;

        vkCmdEndRenderPass(_commandBuffer[iSwapchainImage]);
        vkEndCommandBuffer(_commandBuffer[iSwapchainImage]);

        const VkPipelineStageFlags waitStage = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        const VkSubmitInfo submitInfo = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr,
            (uint32_t)((_currFrameInfo.semaphoreImageAcquired != VK_NULL_HANDLE) ? 1 : 0),
            &_currFrameInfo.semaphoreImageAcquired, &waitStage,
            1, &_commandBuffer[iSwapchainImage],    // commandBuffers
            1, &_currFrameInfo.semaphoreRenderReady // signalSemaphores
        };

        VK_THROW_IF_FAILED(vkQueueSubmit(_queue, 1, &submitInfo, _fenceCommandBuffer[iSwapchainImage]));
    }
}


void VkAdapter::presentFrame()
{
    if (_currFrameInfo.acquired) {
        _swapchain->present(_queue, _fenceCommandBuffer[_currFrameInfo.iSwapchainImage]);
    }
}


void VkAdapter::resized(uint32_t width, uint32_t height)
{
    vkDeviceWaitIdle(_device);

    _swapchain->updateSwapchain(width, height);

    // Swapchain was updated, we need to update other resources
    if (_swapchain->valid()) {
        createRenderPass();
        createFramebuffer();
        createCommandBuffer();
    }
}


void VkAdapter::createFramebuffer()
{
    assert(_swapchain != nullptr);

    // Free resource in case replacement of swapchain
    const size_t nPrevSwapchainImages = _framebuffer.size();

    for (size_t i = 0; i < nPrevSwapchainImages; i++) {
        vkDestroyFramebuffer(_device, _framebuffer[i], nullptr);
    }

    // Allocate images for render, resolved render, depth/stencil buffer
    const size_t nCurrSwapchainImages = _swapchain->nSwapchainImages();

    _framebuffer.resize(nCurrSwapchainImages);

    for (size_t i = 0; i < nCurrSwapchainImages; i++) {
        const VkImageView attachments[] = {
            _swapchain->getImageView(i)    // Back buffer
        };

        const VkFramebufferCreateInfo framebufferCreateInfo = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0,  // sType, pNext, flags
            _renderPass,                                            // renderpass
            C_ARRAY_SIZE(attachments), attachments,                 // attachments
            _swapchain->width(), _swapchain->height(), 1            // width, height, layers
        };

        VK_THROW_IF_FAILED(vkCreateFramebuffer(_device, &framebufferCreateInfo, nullptr, &_framebuffer[i]));
    }
}


void VkAdapter::createRenderPass()
{
    // Free previously allocated resources in case of a swapchain change
    vkDestroyRenderPass(_device, _renderPass, nullptr);

    const VkAttachmentDescription attachmentDescription[] = {
        {       // resolve
            0,                                          // flags
            _swapchain->getFormat(),                    // format
            VK_SAMPLE_COUNT_1_BIT,                      // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,                // loadOp
            VK_ATTACHMENT_STORE_OP_STORE,               // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,            // loadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,           // storeOp
            VK_IMAGE_LAYOUT_UNDEFINED,                  // initialLayout
            _swapchain->getOptimalLayout(),             // finalLayout
        }
    };

    const VkAttachmentReference attachmentReference[] = {
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },        // render
    };

    const VkSubpassDescription subpassDescription = {
        0,                                          // flags
        VK_PIPELINE_BIND_POINT_GRAPHICS,            // pipelineBindPoint
        0, nullptr,                                 // inputAttachments
        1, &attachmentReference[0],                 // colorAttachments
        nullptr,                                    // resolveAttachments
        nullptr,                                    // depthStencilAttachment
        0, nullptr,                                 // preserveAttachments
    };

    const VkRenderPassCreateInfo renderPassCreateInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0,  // sType, pNext, flags
        C_ARRAY_SIZE(attachmentDescription), attachmentDescription, // attachments
        1, &subpassDescription,                                 // subpasses
        0, nullptr                                              // dependencies
    };

    VK_THROW_IF_FAILED(vkCreateRenderPass(_device, &renderPassCreateInfo, nullptr, &_renderPass));
}


void VkAdapter::createCommandBuffer()
{
    // Free previously allocated resources in case of a swapchain change
    vkDestroyCommandPool(_device, _commandPool, nullptr);

    for (const VkFence& fence : _fenceCommandBuffer) {
        vkDestroyFence(_device, fence, nullptr);
    }

    const VkCommandPoolCreateInfo commandPoolCreateInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,    // sType, pNext
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |                  // flags
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        _iQueueFamily                                           // queueFamilyIdx
    };

    VK_THROW_IF_FAILED(vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_commandPool));

    const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,    // sType, pNext
        _commandPool,                                               // commandPool
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,                            // level
        _swapchain->nSwapchainImages()
    };

    _commandBuffer.resize(_swapchain->nSwapchainImages());
    VK_THROW_IF_FAILED(vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, _commandBuffer.data()));

    const VkFenceCreateInfo fenceCreateInfo = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT
    };

    _fenceCommandBuffer.resize(_swapchain->nSwapchainImages());

    for (size_t i = 0; i < _swapchain->nSwapchainImages(); i++) {
        VK_THROW_IF_FAILED(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_fenceCommandBuffer[i]));
    }
}