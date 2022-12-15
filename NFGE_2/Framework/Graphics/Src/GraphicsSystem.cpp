//====================================================================================================
// Filename:	GraphicsSystem.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/8
//====================================================================================================

#include "Precompiled.h"
#include "GraphicsSystem.h"
#include "d3dx12.h"
#include "D3DUtil.h"
#include "DescriptorAllocator.h"
#include "DescriptorAllocatorPage.h"
#include "DynamicDescriptorHeap.h"
#include "Resource.h"
#include "ResourceStateTracker.h"
#include "UploadBuffer.h"
#include "RootSignature.h"
#include "PipelineWorker.h"
#include "SpriteRenderer.h"
#include "PipelineComponent.h"

using namespace NFGE;
using namespace NFGE::Graphics;
using namespace Microsoft::WRL;

// Internal linkage
namespace 
{
	std::unique_ptr<GraphicsSystem> sGraphicsSystem;
	Core::WindowMessageHandler sWindowMessageHandler;

	void EnableDebugLayer();

	// D3d12 object creation
	ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp, SIZE_T dedicatedVideoMemory, SIZE_T& videoMemoryRecord);
	ComPtr<ID3D12Device5> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
	ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
	D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
	ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device);
	HANDLE CreateEventHandle();
	// D3d12 object Synchronization
	uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);
	void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
	void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent);

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
					sGraphicsSystem->ToggleFullscreen(window);
				}
				break;
			case VK_F11:
				sGraphicsSystem->ToggleFullscreen(window);
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
	}
}

GraphicsSystem* GraphicsSystem::Get()
{
	ASSERT(sGraphicsSystem != nullptr, "[Graphics::graphicsSystem] No system registered.");
	return sGraphicsSystem.get();
}

void GraphicsSystem::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	commandList->ResourceBarrier(1, &barrier);
}

void GraphicsSystem::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void GraphicsSystem::ClearDSV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void GraphicsSystem::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	auto device = GetDevice();

	size_t bufferSize = numElements * elementSize;
	// Create a committed resource for the GPU resource in a default heap.
	{
		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(pDestinationResource)));
	}

	// Create an committed resource for the upload.
	if (bufferData)
	{
		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}

void NFGE::Graphics::GraphicsSystem::AllocateUAVBuffer(ID3D12Device* pDevice, UINT64 bufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialResourceState, const wchar_t* resourceName)
{
	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		initialResourceState,
		nullptr,
		IID_PPV_ARGS(ppResource)));
	if (resourceName)
	{
		(*ppResource)->SetName(resourceName);
	}
}

void NFGE::Graphics::GraphicsSystem::AllocateUploadBuffer(ID3D12Device* pDevice, void* pData, UINT64 datasize, ID3D12Resource** ppResource, const wchar_t* resourceName)
{
	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(ppResource)));
	if (resourceName)
	{
		(*ppResource)->SetName(resourceName);
	}
	void* pMappedData;
	(*ppResource)->Map(0, nullptr, &pMappedData);
	memcpy(pMappedData, pData, datasize);
	(*ppResource)->Unmap(0, nullptr);
}

GraphicsSystem::~GraphicsSystem()
{
	//ASSERT(mD3ddDevice == nullptr, "[Graphics::GraphicsSystem] Terminate() must be called to clean up!");
}

void GraphicsSystem::Initialize(const NFGE::Core::Window& window, bool fullscreen, bool useWarp, SIZE_T dedicatedVideoMemory)
{
	EnableDebugLayer();

	ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(useWarp, dedicatedVideoMemory, mMaxVideoMemory);
	mDevice = CreateDevice(dxgiAdapter4);
	if (mDevice)
	{
		mDirectWorker = std::make_unique<PipelineWorker>(WorkerType::Direct);
		mComputeWorker = std::make_unique<PipelineWorker>(WorkerType::Compute);
		mCopyWorker = std::make_unique<PipelineWorker>(WorkerType::Copy);
		mRaytracingWorker = std::make_unique<PipelineWorker>(WorkerType::Direct);

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
		mIsRayTracingSupport = SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
			&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
	}
	
	mWindowRect = window.GetWindowRECT();
	mHeight = (uint32_t)(mWindowRect.bottom - mWindowRect.top);
	mWidth = (uint32_t)(mWindowRect.right - mWindowRect.left);
	HWND windowHandle = window.GetWindowHandle();
	mSwapChain = CreateSwapChain(windowHandle, mDirectWorker->GetCommandQueue()->GetD3D12CommandQueue(), mWidth, mHeight, sNumFrames);

	mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

	for (int i = 0; i < sNumFrames; ++i)
	{
		mBackBuffers[i].SetName(L"Backbuffer[" + std::to_wstring(i) + L"]");
	}
	
	mFullScreen = fullscreen;

	// Hook application to windows procedure
	sWindowMessageHandler.Hook(windowHandle, GraphicsSystemMessageHandler);

	// Create descriptor allocators
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		mDescriptorAllocators[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
	}

	UpdateRenderTargetViews();
	UpdateDepthStencilView();
	BindMasterRenderTarget();
	mScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
}

void GraphicsSystem::Terminate()
{
	// Make sure the command queue has finished all commands before closing.
	Flush();

	Reset();

	// Restore original windows procedure
	sWindowMessageHandler.Unhook();
}

void NFGE::Graphics::GraphicsSystem::BeginUpload()
{
	mCopyWorker->BeginWork();
	mComputeWorker->BeginWork();
	mRaytracingWorker->BeginWork();
	mDirectWorker->BeginWork();
}

void NFGE::Graphics::GraphicsSystem::EndUpload()
{
	mCopyWorker->ProcessWork();
	mCopyWorker->EndWork();
	mComputeWorker->ProcessWork();
	mComputeWorker->EndWork();
	mRaytracingWorker->ProcessWork();
	mRaytracingWorker->EndWork();
	mDirectWorker->ProcessWork();
	mDirectWorker->EndWork();
}

void NFGE::Graphics::GraphicsSystem::BeginUpdate()
{
	mCopyWorker->BeginWork();
	mComputeWorker->BeginWork();
	mRaytracingWorker->BeginWork();
}

void NFGE::Graphics::GraphicsSystem::EndUpdate()
{
	mCopyWorker->ProcessWork();
	mCopyWorker->EndWork();
	mComputeWorker->ProcessWork();
	mComputeWorker->EndWork();
	mRaytracingWorker->ProcessWork();
	mRaytracingWorker->EndWork();
}

void GraphicsSystem::BeginMasterRender()
{
	mDirectWorker->BeginWork();

	// Clear the render target.
	{
		BindMasterRenderTarget();
		mDirectWorker->ClearTexture(mMasterRenderTarget.GetTexture(AttachmentPoint::Color0), &mClearColor.x);
		mDirectWorker->ClearDepthStencilTexture(mMasterRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
	}

	mDirectWorker->SetRenderTarget(mMasterRenderTarget);
	mDirectWorker->SetViewport(mMasterRenderTarget.GetViewport());
	mDirectWorker->SetScissorRect(mScissorRect);
}

void GraphicsSystem::EndMasterRender()
{
	auto& currentBackbuffer = mBackBuffers[mCurrentBackBufferIndex];
	mDirectWorker->TransitionBarrier(currentBackbuffer, D3D12_RESOURCE_STATE_PRESENT);
	mDirectWorker->EndWork();
}

void NFGE::Graphics::GraphicsSystem::Present()
{
	uint32_t syncInterval = mVSync ? 1 : 0;
	uint32_t presentFlag = 0; // set up present flags if needed
	ThrowIfFailed(mSwapChain->Present(syncInterval, presentFlag));

	mFrameFenceValues[mCurrentBackBufferIndex] = mDirectWorker->Signal();
	mFrameCountValues[mCurrentBackBufferIndex] = mFrameCount;

	mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

	ReleaseStaleDescriptors(mFrameCountValues[mCurrentBackBufferIndex]);
	SpriteRenderer::Get()->ReleaseGraphicsMemory();
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

		mMasterRenderTarget.AttachTexture(Color0, Texture());
		for (int i = 0; i < sNumFrames; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			ResourceStateTracker::RemoveGlobalResourceState(mBackBuffers[i].GetD3D12Resource().Get());
			mBackBuffers[i].Reset();
			mFrameFenceValues[i] = mFrameFenceValues[mCurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		ThrowIfFailed(mSwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(mSwapChain->ResizeBuffers(sNumFrames, mWidth, mHeight,
			swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();

		UpdateRenderTargetViews();
		UpdateDepthStencilView();

		for(auto RTcomponent : mRTComponent)
		{
			RTcomponent->OnSizeChanged(width, height);
		}
	}
}

void NFGE::Graphics::GraphicsSystem::RegisterResizeComponent(PipelineComponent_RayTracing* RTComponent)
{
	mRTComponent.push_back(RTComponent);
}

uint32_t NFGE::Graphics::GraphicsSystem::GetBackBufferWidth() const
{
	return mWidth;
}

uint32_t NFGE::Graphics::GraphicsSystem::GetBackBufferHeight() const
{
	return mHeight;
}

D3D12_GPU_DESCRIPTOR_HANDLE NFGE::Graphics::GraphicsSystem::GetGpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, Texture& texture)
{
	return D3D12_GPU_DESCRIPTOR_HANDLE();
	//return mDescriptorAllocators[type]->CopyDescriptor(*this, cpuHandle);
}

const RenderTarget& NFGE::Graphics::GraphicsSystem::GetRenderTarget()
{
	return mMasterRenderTarget;
}

void NFGE::Graphics::GraphicsSystem::BindMasterRenderTarget()
{
	mMasterRenderTarget.AttachTexture(AttachmentPoint::Color0, mBackBuffers[mCurrentBackBufferIndex]);
	mMasterRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, mDepthStencil);
}

void NFGE::Graphics::GraphicsSystem::Reset()
{
	mDevice.Reset();
	mSwapChain.Reset();
	mMasterRenderTarget.Reset();
	//mFence.Reset();
	//CloseHandle(mFenceEvent);

	for (size_t i = 0; i < sNumFrames; i++)
	{
		mBackBuffers[i].Reset();
	}
	mDepthStencil.Reset();
}

void NFGE::Graphics::GraphicsSystem::UpdateRenderTargetViews()
{
	for (int i = 0; i < sNumFrames; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		ResourceStateTracker::AddGlobalResourceState(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

		mBackBuffers[i].SetD3D12Resource(backBuffer);
		mBackBuffers[i].CreateViews();
	}
}

void NFGE::Graphics::GraphicsSystem::UpdateDepthStencilView()
{
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, mWidth, mHeight);
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	mDepthStencil = Texture(depthDesc, &depthClearValue,
		TextureUsage::Depth,
		L"Depth Render Target");
}

void NFGE::Graphics::GraphicsSystem::Flush()
{
	mCopyWorker->GetCommandQueue()->Flush();
	mComputeWorker->GetCommandQueue()->Flush();
	mDirectWorker->GetCommandQueue()->Flush();
	mRaytracingWorker->GetCommandQueue()->Flush();
}

// Internal linkage functions defination
namespace 
{
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

	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp, SIZE_T dedicatedVideoMemory, SIZE_T& videoMemoryRecord)
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
					videoMemoryRecord = dxgiAdapterDesc1.DedicatedVideoMemory;
					// It is neither safe nor reliable to perform a static_cast on COM objects.
					ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
				}
			}
		}

		return dxgiAdapter4;
	}

	Microsoft::WRL::ComPtr<ID3D12Device5> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
	{
		ComPtr<ID3D12Device5> d3d12Device5;
		ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device5)));

		// Enable debug messages in debug mode.
#if defined(_DEBUG)
		ComPtr<ID3D12InfoQueue> pInfoQueue;
		if (SUCCEEDED(d3d12Device5.As(&pInfoQueue)))
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
		return d3d12Device5;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type)
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

	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount)
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

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
	{
		ComPtr<ID3D12DescriptorHeap> descriptorHeap;

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = numDescriptors;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.Type = type;

		ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

		return descriptorHeap;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
	{
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

		return commandAllocator;
	}

	ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
	{
		ComPtr<ID3D12GraphicsCommandList> commandList;
		ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

		ThrowIfFailed(commandList->Close());

		return commandList;
	}

	D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, UINT subresource, D3D12_RESOURCE_BARRIER_FLAGS flags)
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

	ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device)
	{
		ComPtr<ID3D12Fence> fence;

		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

		return fence;
	}

	HANDLE CreateEventHandle()
	{
		HANDLE fenceEvent;

		fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		ASSERT(fenceEvent, "Failed to create fence event.");

		return fenceEvent;
	}

	uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue)
	{
		uint64_t fenceValueForSignal = ++fenceValue;
		ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

		return fenceValueForSignal;
	}

	void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration)
	{
		if (fence->GetCompletedValue() < fenceValue)
		{
			ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
			::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
		}
	}

	void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent)
	{
		uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
		WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
	}
} // Nameless namespace


