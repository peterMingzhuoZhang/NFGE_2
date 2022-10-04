//====================================================================================================
// Filename:	GraphicsSystem.h
// Created by:	Mingzhuo Zhang
// Date:		2022/8
//====================================================================================================

#pragma once

#include "Colors.h"
#include "CommandQueue.h"
#include "DescriptorAllocation.h"

namespace NFGE::Graphics {
	using namespace Microsoft::WRL;
	
	enum class RenderType
	{
		Direct = D3D12_COMMAND_LIST_TYPE_DIRECT,
		Compute = D3D12_COMMAND_LIST_TYPE_COMPUTE,
		Copy = D3D12_COMMAND_LIST_TYPE_COPY
	};

	class DescriptorAllocator;
	class UploadBuffer;
	class ResourceStateTracker;
	class Resource;
	class GraphicsSystem
	{
	public:
		static void StaticInitialize(const NFGE::Core::Window& window, bool fullscreen, bool useWarp = false, SIZE_T dedicatedVideoMemory = 0);
		static void StaticTerminate();
		static GraphicsSystem* Get();

		// Transition a resource
		static void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			Microsoft::WRL::ComPtr<ID3D12Resource> resource,
			D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

		// Clear a render target view.
		static void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

		// Clear the depth of a depth-stencil view.
		static void ClearDSV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

		// Create a GPU buffer.
		static void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
			size_t numElements, size_t elementSize, const void* bufferData,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	public:
		GraphicsSystem() = default;
		~GraphicsSystem();

		GraphicsSystem(const GraphicsSystem&) = delete;
		GraphicsSystem& operator=(const GraphicsSystem&) = delete;

		void Initialize(const NFGE::Core::Window& window, bool fullscreen, bool useWarp, SIZE_T dedicatedVideoMemory);
		void Terminate();

		void BeginRender(RenderType type);
		ComPtr<ID3D12GraphicsCommandList2> GetCurrentCommandList() const { return mCurrentCommandList; }
		void EndRender(RenderType type);

		void ToggleFullscreen(HWND WindowHandle);
		void Resize(uint32_t width, uint32_t height);

		void SetClearColor(Color clearColor) { mClearColor = clearColor; }
		Color& GetClearColor() { return mClearColor; }
		void SetVSync(bool vSync) { mVSync = vSync ? 1 : 0, 0; }

		uint32_t GetBackBufferWidth() const;
		uint32_t GetBackBufferHeight() const;

	//private:
		void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
		void TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);

		void UAVBarrier(ID3D12Resource* resource, bool flushBarriers = false);
		void UAVBarrier(const Resource& resource, bool flushBarriers = false);

		void AliasingBarrier(ID3D12Resource* beforeResource, ID3D12Resource* afterResource, bool flushBarriers = false);
		void AliasingBarrier(const Resource& beforeResource, const Resource& afterResource, bool flushBarriers = false);

		void FlushResourceBarriers();

		void CopyResource(ID3D12Resource* dstRes, ID3D12Resource* srcRes);
		void CopyResource(Resource& dstRes, const Resource& srcRes);

		void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);
	private:
		
		static const uint8_t sNumFrames = 3;

		friend LRESULT CALLBACK GraphicsSystemMessageHandler(HWND window, UINT message, WPARAM wPrarm, LPARAM lParam);
		friend ComPtr<ID3D12Device2> GetDevice();
		friend CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type);
		friend uint8_t GetFrameCount();
		friend void Flush();

		friend DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
		// Release stale descriptors. This should only be called with a completed frame counter.
		friend void ReleaseStaleDescriptors(uint64_t finishedFrame);
		friend UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type);
		
		friend void TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object);

		void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
		void UpdateDepthStencilView(ComPtr<ID3D12Device2> device, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
		UINT GetCurrentBackBufferIndex() const { return mCurrentBackBufferIndex; };
		ComPtr<ID3D12Resource> GetCurrentBackBuffer() const { return mBackBuffers[mCurrentBackBufferIndex]; };
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStenciltView() const;

		void TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object);
		void TrackResource(ID3D12Resource* res);
		void TrackResource(const Resource& res);

		// Synchronization functions
		void Flush();

		// DirectX 12 Objects
		// Allocation and Control
		ComPtr<ID3D12Device2> mDevice{ nullptr };
		ComPtr<ID3D12GraphicsCommandList2> mCurrentCommandList{ nullptr };
		std::unique_ptr<CommandQueue> mDirectCommandQueue{ nullptr };
		std::unique_ptr<CommandQueue> mComputeCommandQueue{ nullptr };
		std::unique_ptr<CommandQueue> mCopyCommandQueue{ nullptr };

		// SwapChian
		ComPtr<IDXGISwapChain4> mSwapChain{ nullptr };

		// Memory
		ComPtr<ID3D12Resource> mBackBuffers[sNumFrames]{ nullptr };
		UINT mCurrentBackBufferIndex{ 0 };
		ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap{ nullptr };
		UINT mRTVDescriptorSize{ 0 };
		Microsoft::WRL::ComPtr<ID3D12Resource> mDepthBuffer;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDSVHeap;
		// Resource created in an upload heap. Useful for drawing of dynamic geometry
		// or for uploading constant buffer data that changes every draw call.
		std::unique_ptr<UploadBuffer> mUploadBuffer;

		std::unique_ptr<DescriptorAllocator> mDescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		D3D12_VIEWPORT mViewport{};
		D3D12_RECT mScissorRect{};

		// Synchronization
		uint64_t mFrameFenceValues[sNumFrames]{ 0 };

		// Graphic controls
		Color mClearColor{ 0.23f,0.34f,0.17f,0.0f };
		uint32_t mWidth{ 0 };
		uint32_t mHeight{ 0 };
		bool mVSync{ true };
		bool mFullScreen{ false };
		RECT mWindowRect{};
		SIZE_T mMaxVideoMemory{ 0 };

		
		std::unique_ptr<ResourceStateTracker> mResourceStateTracker;
		using TrackedObjects = std::vector < Microsoft::WRL::ComPtr<ID3D12Object> >;
		TrackedObjects mTrackedObjects;
	};

} // namespace NFGE::Graphics