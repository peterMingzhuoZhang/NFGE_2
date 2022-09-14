//====================================================================================================
// Filename:	GraphicsSystem.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/8
//====================================================================================================

#include "Precompiled.h"
#include "GraphicsSystem.h"
#include "d3dx12.h"
#include "D3DUtil.h"

using namespace NFGE;
using namespace NFGE::Graphics;
using namespace Microsoft::WRL;

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
		case WM_KEYDOWN:
		{
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

			switch (wParam)
			{
			case 'V':
				sGraphicsSystem->mVSync = !sGraphicsSystem->mVSync;
				break;
			case VK_RETURN:
				if (alt)
				{
			case VK_F11:
				sGraphicsSystem->ToggleFullscreen(window);
				}
				break;
			}
		}
		}
	}
	return sWindowMessageHandler.ForwardMessage(window, message, wParam, lParam);
}

void GraphicsSystem::StaticInitialize(const NFGE::Core::Window& window, bool fullscreen, bool useWarp, SIZE_T dedicatedVideoMemory)
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

void GraphicsSystem::Initialize(const NFGE::Core::Window& window, bool fullscreen, bool useWarp, SIZE_T dedicatedVideoMemory)
{
	EnableDebugLayer();

	ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(useWarp, dedicatedVideoMemory);
	mDevice = CreateDevice(dxgiAdapter4);
	if (mDevice)
	{
		mDirectCommandQueue = std::make_unique<CommandQueue>(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		mComputeCommandQueue = std::make_unique<CommandQueue>(mDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		mCopyCommandQueue = std::make_unique<CommandQueue>(mDevice, D3D12_COMMAND_LIST_TYPE_COPY);
	}
	
	mWindowRect = window.GetWindowRECT();
	mHeight = (uint32_t)(mWindowRect.bottom - mWindowRect.top);
	mWidth = (uint32_t)(mWindowRect.right - mWindowRect.left);
	HWND windowHandle = window.GetWindowHandle();
	mSwapChain = CreateSwapChain(windowHandle, mDirectCommandQueue->GetD3D12CommandQueue(), mWidth, mHeight, sNumFrames);

	mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

	mRTVDescriptorHeap = CreateDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, sNumFrames);
	mRTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRenderTargetViews(mDevice, mSwapChain, mRTVDescriptorHeap);

	// Create the descriptor heap for the depth-stencil view.
	DSVHeap = CreateDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1);
	UpdateDepthStencilView(mDevice, DSVHeap);

	viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight));
	scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

	mFence = CreateFence(mDevice);
	mFenceEvent = CreateEventHandle();

	mFullScreen = fullscreen;

	// Hook application to windows procedure
	sWindowMessageHandler.Hook(windowHandle, GraphicsSystemMessageHandler);
}

void GraphicsSystem::Terminate()
{
	// Make sure the command queue has finished all commands before closing.
	Flush();

	mDevice.Reset();
	mSwapChain.Reset();
	mRTVDescriptorHeap.Reset();
	mFence.Reset();
	CloseHandle(mFenceEvent);

	for (size_t i = 0; i < sNumFrames; i++)
	{
		mBackBuffers[i].Reset();
	}

	// Restore original windows procedure
	sWindowMessageHandler.Unhook();
}

void GraphicsSystem::BeginRender(RenderType type)
{
	auto currentBackbuffer = mBackBuffers[mCurrentBackBufferIndex];
	auto commandQueue = NFGE::Graphics::GetCommandQueue(static_cast<D3D12_COMMAND_LIST_TYPE>(type));
	mCurrentCommandList = commandQueue->GetCommandList();

	// Clear the render target.
	{
		TransitionResource(mCurrentCommandList, currentBackbuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		//ShiftRTVDescriptorHandle(rtvHandle, mCurrentBackBufferIndex, mRTVDescriptorSize);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			mCurrentBackBufferIndex, mRTVDescriptorSize);
		auto dsv = DSVHeap->GetCPUDescriptorHandleForHeapStart();

		mCurrentCommandList->ClearRenderTargetView(rtvHandle, &mClearColor.x, 0, nullptr);
		mCurrentCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	auto rtv = GetCurrentRenderTargetView();
	auto dsv = GetDepthStenciltView();

	mCurrentCommandList->RSSetViewports(1, &viewport);
	mCurrentCommandList->RSSetScissorRects(1, &scissorRect);
	mCurrentCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}

void GraphicsSystem::EndRender(RenderType type)
{
	auto commandQueue = NFGE::Graphics::GetCommandQueue(static_cast<D3D12_COMMAND_LIST_TYPE>(type));
	auto currentBackbuffer = mBackBuffers[mCurrentBackBufferIndex];

	//D3D12_RESOURCE_BARRIER barrier = CreateTransitionBarrier(currentBackbuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	//mCurrentCommandList->ResourceBarrier(1, &barrier);
	TransitionResource(mCurrentCommandList, currentBackbuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	//ID3D12CommandList* const commandLists[] = { mCurrentCommandList.Get()};
	mFrameFenceValues[mCurrentBackBufferIndex] = commandQueue->ExecuteCommandList(mCurrentCommandList);

	uint32_t syncInterval = mVSync ? 1 : 0;
	uint32_t presentFlag = 0; // set up present flags if needed
	ThrowIfFailed(mSwapChain->Present(syncInterval, presentFlag));

	mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

	commandQueue->WaitForFenceValue(mFrameFenceValues[mCurrentBackBufferIndex]);
}

void GraphicsSystem::ToggleFullscreen(HWND windowHandle)
{
	mFullScreen = !mFullScreen;


	if (mFullScreen) // Switching to fullscreen.
	{
		// Store the current window dimensions so they can be restored 
		// when switching out of fullscreen state.
		GetWindowRect(windowHandle, &mWindowRect);

		// Set the window style to a borderless window so the client area fills
		// the entire screen.
		UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

		SetWindowLongW(windowHandle, GWL_STYLE, windowStyle);

		// Query the name of the nearest display device for the window.
		// This is required to set the fullscreen dimensions of the window
		// when using a multi-monitor setup.
		HMONITOR hMonitor = ::MonitorFromWindow(windowHandle, MONITOR_DEFAULTTONEAREST);
		MONITORINFOEX monitorInfo = {};
		monitorInfo.cbSize = sizeof(MONITORINFOEX);
		::GetMonitorInfo(hMonitor, &monitorInfo);

		::SetWindowPos(windowHandle, HWND_TOPMOST,
			monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.top,
			monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		::ShowWindow(windowHandle, SW_MAXIMIZE);
	}
	else
	{
		// Restore all the window decorators.
		::SetWindowLong(windowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);

		::SetWindowPos(windowHandle, HWND_NOTOPMOST,
			mWindowRect.left,
			mWindowRect.top,
			mWindowRect.right - mWindowRect.left,
			mWindowRect.bottom - mWindowRect.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		::ShowWindow(windowHandle, SW_NORMAL);
	}

}

void NFGE::Graphics::GraphicsSystem::Resize(uint32_t width, uint32_t height)
{
	if (mWidth != width || mHeight != height)
	{
		// Don't allow 0 size swap chain back buffers.
		mWidth = std::max(1u, width);
		mHeight = std::max(1u, height);

		// Flush the GPU queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
		Flush();

		for (int i = 0; i < sNumFrames; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			mBackBuffers[i].Reset();
			mFrameFenceValues[i] = mFrameFenceValues[mCurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		ThrowIfFailed(mSwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(mSwapChain->ResizeBuffers(sNumFrames, mWidth, mHeight,
			swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

		UpdateRenderTargetViews(mDevice, mSwapChain, mRTVDescriptorHeap);

		UpdateDepthStencilView(mDevice, DSVHeap);

		viewport.Width = (float)mWidth ;
		viewport.Height = (float)mHeight;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
	}
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
	return mWidth;
}

uint32_t NFGE::Graphics::GraphicsSystem::GetBackBufferHeight() const
{
	return mHeight;
}

D3D12_CPU_DESCRIPTOR_HANDLE NFGE::Graphics::GraphicsSystem::GetCurrentRenderTargetView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrentBackBufferIndex, mRTVDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE NFGE::Graphics::GraphicsSystem::GetDepthStenciltView() const
{
	return DSVHeap->GetCPUDescriptorHandleForHeapStart();
}

// Private functions
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
				// It is neither safe nor reliable to perform a static_cast on COM objects.
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}

Microsoft::WRL::ComPtr<ID3D12Device2> NFGE::Graphics::GraphicsSystem::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter) const
{
	ComPtr<ID3D12Device2> d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return d3d12Device2;
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> NFGE::Graphics::GraphicsSystem::CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type) const
{
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

	return d3d12CommandQueue;
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> NFGE::Graphics::GraphicsSystem::CreateSwapChain(HWND hWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount) const
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	//swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	return dxgiSwapChain4;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> NFGE::Graphics::GraphicsSystem::CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors) const
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

void NFGE::Graphics::GraphicsSystem::ShiftRTVDescriptorHandle(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& handleStart, int offsetInDescriptors, uint32_t descriptorSize) const
{
	handleStart.ptr += offsetInDescriptors * descriptorSize;
}

void NFGE::Graphics::GraphicsSystem::UpdateRenderTargetViews(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < sNumFrames; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i])));

		device->CreateRenderTargetView(mBackBuffers[i].Get(), nullptr, rtvHandle);

		// set offset
		ASSERT(mRTVDescriptorSize != 0, "Unexpect RTVDescriptorSize.");
		rtvHandle.ptr += mRTVDescriptorSize;
	}
}

void NFGE::Graphics::GraphicsSystem::UpdateDepthStencilView(ComPtr<ID3D12Device2> device, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
	// Create a depth buffer.
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0f, 0 };

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, mWidth, mHeight,
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&depthBuffer)
	));

	// Update the depth-stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	dsv.Format = DXGI_FORMAT_D32_FLOAT;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice = 0;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthBuffer.Get(), &dsv,
		DSVHeap->GetCPUDescriptorHandleForHeapStart());
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> NFGE::Graphics::GraphicsSystem::CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) const
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> NFGE::Graphics::GraphicsSystem::CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type) const
{
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	ThrowIfFailed(commandList->Close());

	return commandList;
}

D3D12_RESOURCE_BARRIER NFGE::Graphics::GraphicsSystem::CreateTransitionBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, UINT subresource, D3D12_RESOURCE_BARRIER_FLAGS flags) const
{
	D3D12_RESOURCE_BARRIER result;
	ZeroMemory(&result, sizeof(result));
	result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	result.Flags = flags;
	result.Transition.pResource = resource.Get();
	result.Transition.StateBefore = stateBefore;
	result.Transition.StateAfter = stateAfter;
	result.Transition.Subresource = subresource;
	return result;
}

ComPtr<ID3D12Fence> NFGE::Graphics::GraphicsSystem::CreateFence(ComPtr<ID3D12Device2> device) const
{
	ComPtr<ID3D12Fence> fence;

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	return fence;
}

HANDLE NFGE::Graphics::GraphicsSystem::CreateEventHandle() const
{
	HANDLE fenceEvent;

	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ASSERT(fenceEvent, "Failed to create fence event.");

	return fenceEvent;
}

uint64_t NFGE::Graphics::GraphicsSystem::Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue) const
{
	uint64_t fenceValueForSignal = ++fenceValue;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}

void NFGE::Graphics::GraphicsSystem::WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration) const
{
	if (fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
	}
}

void NFGE::Graphics::GraphicsSystem::Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent) const
{
	uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
	WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}

void NFGE::Graphics::GraphicsSystem::Flush()
{
	mDirectCommandQueue->Flush();
	mComputeCommandQueue->Flush();
	mCopyCommandQueue->Flush();
}
