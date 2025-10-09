#include "DX11Adapter.h"

#include <stdexcept>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../Window/Window.h"


DX11Adapter::DX11Adapter()
{

}


DX11Adapter::~DX11Adapter()
{
    destroyRenderTarget();

    if (_pSwapchain) { _pSwapchain->Release(); _pSwapchain = nullptr; }
    if (_pDeviceContext) { _pDeviceContext->Release(); _pDeviceContext = nullptr; }
    if (_pDevice) { _pDevice->Release(); _pDevice = nullptr; }
}


void DX11Adapter::initDevice(Window* window)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window->handle();
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &_pSwapchain, &_pDevice, &featureLevel, &_pDeviceContext);
    
    if (res == DXGI_ERROR_UNSUPPORTED) {
        // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &_pSwapchain, &_pDevice, &featureLevel, &_pDeviceContext);
    }
    
    if (res != S_OK) {
        throw std::runtime_error("[DX11  ] Could not initialize device");
    }

    createRenderTarget();
}


void DX11Adapter::startNewFrame()
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 0.00f);
    const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

    _pDeviceContext->OMSetRenderTargets(1, &_pMainRenderTargetView, nullptr);
    _pDeviceContext->ClearRenderTargetView(_pMainRenderTargetView, clear_color_with_alpha);
}


void DX11Adapter::renderFrame()
{

}


void DX11Adapter::presentFrame()
{
    //HRESULT hr = _pSwapchain->Present(1, 0); // Present with vsync
    HRESULT hr = _pSwapchain->Present(0, 0);
    //(hr == DXGI_STATUS_OCCLUDED);
}


void DX11Adapter::resized(uint32_t width, uint32_t height)
{
    destroyRenderTarget();
    _pSwapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    createRenderTarget();
}


void DX11Adapter::createRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    _pSwapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    _pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pMainRenderTargetView);
    pBackBuffer->Release();
}


void DX11Adapter::destroyRenderTarget()
{
    if (_pMainRenderTargetView) { _pMainRenderTargetView->Release(); _pMainRenderTargetView = nullptr; }
}