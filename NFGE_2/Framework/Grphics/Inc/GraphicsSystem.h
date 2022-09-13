//====================================================================================================
// Filename:	GraphicsSystem.h
// Created by:	Mingzhuo Zhang
// Date:		2022/8
//====================================================================================================

#pragma once

#include "Colors.h"
#include "CommandQueue.h"

namespace NFGE::Graphics {
	using namespace Microsoft::WRL;
	
	enum class RenderType
	{
		Direct = D3D12_COMMAND_LIST_TYPE_DIRECT,
		Compute = D3D12_COMMAND_LIST_TYPE_COMPUTE,
		Copy = D3D12_COMMAND_LIST_TYPE_COPY
	};

	class GraphicsSystem
	{
	public:
		static void StaticInitialize(const NFGE::Core::Window& window, bool fullscreen, bool useWarp = false, SIZE_T dedicatedVideoMemory = 0);
		static void StaticTerminate();
		static GraphicsSystem* Get();

	public:
		GraphicsSystem() = default;
		~GraphicsSystem();

		GraphicsSystem(const GraphicsSystem&) = delete;
		GraphicsSystem& operator=(const GraphicsSystem&) = delete;

		void Initialize(const NFGE::Core::Window& window, bool fullscreen, bool useWarp, SIZE_T dedicatedVideoMemory);
		void Terminate();

		void BeginRender(RenderType type);
		void EndRender(RenderType type);

		void ToggleFullscreen(HWND WindowHandle);
		void Resize(uint32_t width, uint32_t height);

		void ResetRenderTarget();
		void ResetViewport();

		void SetClearColor(Color clearColor) { mClearColor = clearColor; }
		Color& GetClearColor() { return mClearColor; }
		void SetVSync(bool vSync) { mVSync = vSync ? 1 : 0, 0; }

		uint32_t GetBackBufferWidth() const;
		uint32_t GetBackBufferHeight() const;
		UINT GetCurrentBackBufferIndex() const { return mCurrentBackBufferIndex; };
		ComPtr<ID3D12Resource> GetCurrentBackBuffer() const { return mBackBuffers[mCurrentBackBufferIndex]; };
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;
		ComPtr<ID3D12GraphicsCommandList2> GetCurrentCommandList() const { return mCurrentCommandList; }

		// Transition a resource
		void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			Microsoft::WRL::ComPtr<ID3D12Resource> resource,
			D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

		// Clear a render target view.
		void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

		// Clear the depth of a depth-stencil view.
		void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

		// Create a GPU buffer.
		void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
			size_t numElements, size_t elementSize, const void* bufferData,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
		
		static const uint8_t sNumFrames = 3;
		uint64_t mFrameFenceValues[sNumFrames]{ 0 };
		ComPtr<IDXGISwapChain4> mSwapChain{ nullptr };
		UINT mCurrentBackBufferIndex{ 0 };
	private:
		//static const uint8_t sNumFrames = 3;

		friend LRESULT CALLBACK GraphicsSystemMessageHandler(HWND window, UINT message, WPARAM wPrarm, LPARAM lParam);
		friend ComPtr<ID3D12Device2> GetDevice();
		friend CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type);
		friend void Flush();

		// DirectX 12 Objects
		ComPtr<ID3D12Device2> mDevice{ nullptr };
		//ComPtr<ID3D12CommandAllocator> mCommandAllocators[sNumFrames]{ nullptr };
		ComPtr<ID3D12GraphicsCommandList2> mCurrentCommandList{ nullptr };
		//ComPtr<ID3D12CommandQueue> mCommandQueue{ nullptr };
		std::unique_ptr<CommandQueue> mDirectCommandQueue{ nullptr };
		std::unique_ptr<CommandQueue> mComputeCommandQueue{ nullptr };
		std::unique_ptr<CommandQueue> mCopyCommandQueue{ nullptr };
		//ComPtr<IDXGISwapChain4> mSwapChain{ nullptr };
		ComPtr<ID3D12Resource> mBackBuffers[sNumFrames]{ nullptr };
		ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap{ nullptr };
		UINT mRTVDescriptorSize{ 0 };
		//UINT mCurrentBackBufferIndex{ 0 };

		// Synchronization objects
		ComPtr<ID3D12Fence> mFence{ nullptr };
		uint64_t mFenceValue{ 0 };
		//uint64_t mFrameFenceValues[sNumFrames]{ 0 };
		HANDLE mFenceEvent{ nullptr };

		// Graphic controls
		Color mClearColor{ 0.23f,0.34f,0.17f,0.0f };
		uint32_t mWidth{ 0 };
		uint32_t mHeight{ 0 };
		bool mVSync{ true };
		bool mFullScreen{ false };
		RECT mWindowRect{};
		SIZE_T mMaxDedicatedVideoMemory{ 0 };

		ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp, SIZE_T dedicatedVideoMemory); // updates mMaxDedicatedVideoMemory
		ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter) const;
		ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type) const;
		ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount) const;
		ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors) const;
		void ShiftRTVDescriptorHandle(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& handleStart, int offsetInDescriptors, uint32_t descriptorSize) const;
		void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap); // updates mBackBuffers
		ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) const;
		ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type) const;
		D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) const;
		// Synchronization functions
		ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device) const;
		HANDLE CreateEventHandle() const;
		uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue) const;
		void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max()) const;
		void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent) const;
		void Flush();
	};

} // namespace NFGE::Graphics