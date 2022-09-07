//====================================================================================================
// Filename:	GraphicsSystem.h
// Created by:	Mingzhuo Zhang
// Date:		2022/8
//====================================================================================================

#pragma once

#include "Colors.h"

namespace NFGE::Graphics {
	class GraphicsSystem
	{
	public:
		static void StaticInitialize(HWND window, bool fullscreen, bool useWarp = false, SIZE_T dedicatedVideoMemory = 0);
		static void StaticTerminate();
		static GraphicsSystem* Get();

	public:
		GraphicsSystem() = default;
		~GraphicsSystem();

		GraphicsSystem(const GraphicsSystem&) = delete;
		GraphicsSystem& operator=(const GraphicsSystem&) = delete;

		void Initialize(HWND window, bool fullscreen, bool useWarp, SIZE_T dedicatedVideoMemory);
		void Terminate();

		void BeginRender();
		void EndRender();

		void ToggleFullscreen();
		void Resize(uint32_t width, uint32_t height);

		void ResetRenderTarget();
		void ResetViewport();

		void SetClearColor(Color clearColor) { mClearColor = clearColor; }
		Color& GetClearColor() { return mClearColor; }
		void SetVSync(bool vSync) { mVSync = vSync ? 1 : 0, 0; }

		uint32_t GetBackBufferWidth() const;
		uint32_t GetBackBufferHeight() const;


	private:
		static const uint8_t sNumFrames = 3;

		friend LRESULT CALLBACK GraphicsSystemMessageHandler(HWND window, UINT message, WPARAM wPrarm, LPARAM lParam);
		//friend ID3D11Device* GetDevice();
		//friend ID3D11DeviceContext* GetContext();
		//
		//ID3D11Device* mD3ddDevice{ nullptr };
		//ID3D11DeviceContext* mImmediateContext{ nullptr };
		//
		//IDXGISwapChain* mSwapChain{ nullptr };
		//ID3D11RenderTargetView* mRenderTargetView{ nullptr };
		//
		//ID3D11Texture2D* mDepthStencilBuffer{ nullptr };
		//ID3D11DepthStencilView* mDepthStencilView{ nullptr };
		//
		//DXGI_SWAP_CHAIN_DESC mSwapChainDesc;
		//D3D11_VIEWPORT mViewport;

		// DirectX 12 Objects
		Microsoft::WRL::ComPtr<ID3D12Device2> mDevice{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocators[sNumFrames]{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue{ nullptr };
		Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12Resource> mBackBuffers[sNumFrames]{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap{ nullptr };
		UINT mRTVDescriptorSize{ 0 };
		UINT mCurrentBackBufferIndex{ 0 };

		// Synchronization objects
		Microsoft::WRL::ComPtr<ID3D12Fence> mFence{ nullptr };
		uint64_t mFenceValue{ 0 };
		uint64_t mFrameFenceValues[sNumFrames]{ 0 };
		HANDLE mFenceEvent{ nullptr };

		// Graphic controls
		Color mClearColor{ 0.0f,0.0f,0.0f,0.0f };
		bool mVSync{ true };
		bool mFullScreen{ false };
		SIZE_T mMaxDedicatedVideoMemory{ 0 };

		Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp, SIZE_T dedicatedVideoMemory);
	};

} // namespace NFGE::Graphics