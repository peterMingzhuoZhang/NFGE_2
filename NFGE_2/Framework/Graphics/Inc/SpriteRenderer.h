//====================================================================================================
// Filename:	SpriteRender.h
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#pragma once

#include "DrawCommand.h"
#include "DirectXTK12/Inc/DescriptorHeap.h"

namespace DirectX { class SpriteBatch; class CommonStates; class ResourceUploadBatch; class GraphicsMemory;  }

namespace NFGE::Graphics 
{
		class Texture;
		class PipelineWorker;

		class SpriteRenderer
		{
		public:
			static void StaticInitialize();
			static void StaticTerminate();
			static SpriteRenderer* Get();
		public:
			SpriteRenderer() = default;
			~SpriteRenderer();

			SpriteRenderer(const SpriteRenderer&) = delete;
			SpriteRenderer& operator=(const SpriteRenderer&) = delete;

			void Initialize();
			void Terminate();

			void RegisterSpriteTexture(Texture& texture);

			void PrepareRender();
			void BeginRender();
			void EndRender();
			void ReleaseGraphicsMemory();
			void Reset();

			void Draw(const Texture& texture, const Math::Vector2& pos, float rotation = 0.0f, Pivot pivot = Pivot::Center);
			void Draw(const Texture& texture, const Math::Rect& sourceRect, const Math::Vector2& pos, float rotation = 0.0f, Pivot pivot = Pivot::Center);
			void Draw(const Texture& texture, const Math::Vector2& pos, float rotation, float alpha);
			void Draw(const Texture& texture, const Math::Vector2& pos, float rotation, float alpha, float anchorX, float anchorY, float xScale, float yScale);
			void Draw(const Texture& texture, const Math::Rect& sourceRect, const Math::Vector2& pos, float rotation, float alpha, float anchorX, float anchorY, float xScale, float yScale);
		private:
			std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch = nullptr;
			std::unique_ptr<DirectX::CommonStates> mCommonStates = nullptr;
			std::unique_ptr<DirectX::GraphicsMemory> mGraphicsMemory = nullptr;
			PipelineWorker* mAssignedWorker = nullptr;

			std::vector<Texture*> mRegisterSprite;
			enum HeapType {
				RESOURCE = 0,
				STATE,
			};
			std::vector <Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> mSpriteTextureDescriptorHeap;
			uint32_t mSpriteTextureIndex{ 0 };
			uint32_t mSpriteTextureGeneration{ 0 };
			
			D3D12_GPU_DESCRIPTOR_HANDLE GetQuickTextureGPUHandle(const Texture& texture);

			// ...
			std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		};

} // namespace NFGE::Graphics

