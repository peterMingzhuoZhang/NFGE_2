//====================================================================================================
// Filename:	GraphicsSystem.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/8
//====================================================================================================

#include "Precompiled.h"
#include "GraphicsSystem.h"

using namespace NFGE;
using namespace NFGE::Graphics;

// Internal linkage
namespace 
{
	std::unique_ptr<GraphicsSystem> sGraphicsSystem;
	Core::WindowMessageHandler sWindowMessageHandler;

	void EnableDebugLayer()
	{
#if defined(_DEBUG)
		// Always enable the debug layer before doing anything DX12 related
		// so all possible errors generated while creating DX12 objects
		// are caught by the debug layer.
		Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
		debugInterface->EnableDebugLayer();
#endif
	}
}

LRESULT CALLBACK Graphics::GraphicsSystemMessageHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (sGraphicsSystem)
	{
		switch (message)
		{
		case WM_SIZE:
		{
			const uint32_t width = static_cast<uint32_t>(LOWORD(lParam));
			const uint32_t height = static_cast<uint32_t>(HIWORD(lParam));
			sGraphicsSystem->Resize(width, height);
			break;
		}
		}
	}
	return sWindowMessageHandler.ForwardMessage(window, message, wParam, lParam);
}

void GraphicsSystem::StaticInitialize(HWND window, bool fullscreen, bool useWarp, SIZE_T dedicatedVideoMemory)
{
	ASSERT(sGraphicsSystem == nullptr, "[Graphics::GraphicsSystem] System already initialized!");
	sGraphicsSystem = std::make_unique<GraphicsSystem>();
	sGraphicsSystem->Initialize(window, fullscreen, useWarp, dedicatedVideoMemory);
}

void GraphicsSystem::StaticTerminate()
{
	if (sGraphicsSystem != nullptr)
	{
		sGraphicsSystem->Terminate();
		sGraphicsSystem.reset();
	}
}

GraphicsSystem* GraphicsSystem::Get()
{
	ASSERT(sGraphicsSystem != nullptr, "[Graphics::graphicsSystem] No system registered.");
	return sGraphicsSystem.get();
}

GraphicsSystem::~GraphicsSystem()
{
	//ASSERT(mD3ddDevice == nullptr, "[Graphics::GraphicsSystem] Terminate() must be called to clean up!");
}

void GraphicsSystem::Initialize(HWND window, bool fullscreen, bool useWarp, SIZE_T dedicatedVideoMemory)
{
	EnableDebugLayer();

	Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(useWarp, dedicatedVideoMemory);
	
	mFullScreen = fullscreen;

	//TODO
}

void GraphicsSystem::Terminate()
{
	// Restore original windows procedure
	sWindowMessageHandler.Unhook();

	//TODO
}

void GraphicsSystem::BeginRender()
{
	//TODO
}

void GraphicsSystem::EndRender()
{
	//TODO
}

void GraphicsSystem::ToggleFullscreen()
{
	//TODO
}

void NFGE::Graphics::GraphicsSystem::Resize(uint32_t width, uint32_t height)
{
	//TODO
}

void NFGE::Graphics::GraphicsSystem::ResetRenderTarget()
{
	//TODO
}

void NFGE::Graphics::GraphicsSystem::ResetViewport()
{
	//TODO
}

uint32_t NFGE::Graphics::GraphicsSystem::GetBackBufferWidth() const
{
	//TODO
}

uint32_t NFGE::Graphics::GraphicsSystem::GetBackBufferHeight() const
{
	//TODO
}

Microsoft::WRL::ComPtr<IDXGIAdapter4> NFGE::Graphics::GraphicsSystem::GetAdapter(bool useWarp, SIZE_T dedicatedVideoMemory)
{
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (useWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > dedicatedVideoMemory)
			{
				mMaxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}