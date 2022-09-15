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
	private:
		
		static const uint8_t sNumFrames = 3;

		friend LRESULT CALLBACK GraphicsSystemMessageHandler(HWND window, UINT message, WPARAM wPrarm, LPARAM lParam);
		friend ComPtr<ID3D12Device2> GetDevice();
		friend CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type);
		friend void Flush();

		void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
		void UpdateDepthStencilView(ComPtr<ID3D12Device2> device, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
		UINT GetCurrentBackBufferIndex() const { return mCurrentBackBufferIndex; };
		ComPtr<ID3D12Resource> GetCurrentBackBuffer() const { return mBackBuffers[mCurrentBackBufferIndex]; };
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStenciltView() const;

		// Synchronization functions
		void Flush();

		// DirectX 12 Objects
		ComPtr<ID3D12Device2> mDevice{ nullptr };
		ComPtr<ID3D12GraphicsCommandList2> mCurrentCommandList{ nullptr };
		std::unique_ptr<CommandQueue> mDirectCommandQueue{ nullptr };
		std::unique_ptr<CommandQueue> mComputeCommandQueue{ nullptr };
		std::unique_ptr<CommandQueue> mCopyCommandQueue{ nullptr };
		ComPtr<IDXGISwapChain4> mSwapChain{ nullptr };
		ComPtr<ID3D12Resource> mBackBuffers[sNumFrames]{ nullptr };
		UINT mCurrentBackBufferIndex{ 0 };
		ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12Resource> mDepthBuffer;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDSVHeap;
		UINT mRTVDescriptorSize{ 0 };
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
	};

} // namespace NFGE::Graphics