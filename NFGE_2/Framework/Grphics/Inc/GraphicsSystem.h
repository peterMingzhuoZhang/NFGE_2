//====================================================================================================
// Filename:	GraphicsSystem.h
// Created by:	Mingzhuo Zhang
// Date:		2022/8
//====================================================================================================

#pragma once

#include "Colors.h"
#include "CommandQueue.h"
#include "DescriptorAllocation.h"
#include "DynamicDescriptorHeap.h" // TODO:: remove this dependicy 

namespace NFGE::Graphics {
	using namespace Microsoft::WRL;
	
	enum class RenderType
	{
		Direct = D3D12_COMMAND_LIST_TYPE_DIRECT,
		Compute = D3D12_COMMAND_LIST_TYPE_COMPUTE,
		Copy = D3D12_COMMAND_LIST_TYPE_COPY
	};

	class DescriptorAllocator;
	class DynamicDescriptorHeap;
	class UploadBuffer;
	class ResourceStateTracker;
	class Resource;
	class RootSignature;
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
		void SetCurrentCommandList(ComPtr<ID3D12GraphicsCommandList2> comandList) { mCurrentCommandList = comandList; }
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

		void SetGraphicsRootSignature(const RootSignature& rootSignature);
		void SetComputeRootSignature(const RootSignature& rootSignature);

		void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);

		void SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
		template<typename T>
		void SetCompute32BitConstants(uint32_t rootParameterIndex, const T& constants)
		{
			static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
			SetCompute32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
		}
		void SetShaderResourceView(
			uint32_t rootParameterIndex,
			uint32_t descriptorOffset,
			const Resource& resource,
			D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			UINT firstSubresource = 0,
			UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr
		);
		void SetUnorderedAccessView(
			uint32_t rootParameterIndex,
			uint32_t descrptorOffset,
			const Resource&,
			D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			UINT firstSubresource = 0,
			UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr
		);


		// Draw geometry.
		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0);
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);

		// Dispatch a compute shader.
		void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1);

		void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);
		void Reset();
		
		struct DynamicDescriptorHeapKit
		{
			std::unique_ptr<DynamicDescriptorHeap> mDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
			ID3D12DescriptorHeap* mDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
			DynamicDescriptorHeapKit() {
				for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
				{
					mDynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
					mDescriptorHeaps[i] = nullptr;
				}
			}
		};
		std::vector<DynamicDescriptorHeapKit> mDynamicDescriptorHeapKits;
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
		ID3D12RootSignature* mCurrentRootSignature;
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