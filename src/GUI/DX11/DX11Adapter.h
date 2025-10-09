#pragma once

#include <d3d11.h>
#include <cstdint>

class WindowSystem;
class Window;


class DX11Adapter
{
public:
    DX11Adapter();

    virtual ~DX11Adapter();

    void initDevice(Window* window);

    void startNewFrame();
    void renderFrame();
    void presentFrame();

    void resized(uint32_t width, uint32_t height);

    ID3D11Device* device() const { return _pDevice; }
    ID3D11DeviceContext* deviceContext() const { return _pDeviceContext; }

private:
    void createRenderTarget();
    void destroyRenderTarget();

private:
    ID3D11Device* _pDevice = nullptr;
    ID3D11DeviceContext* _pDeviceContext = nullptr;
    IDXGISwapChain* _pSwapchain = nullptr;
    ID3D11RenderTargetView* _pMainRenderTargetView = nullptr;
};