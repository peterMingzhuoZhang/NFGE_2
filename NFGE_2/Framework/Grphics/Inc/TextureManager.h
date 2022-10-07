//====================================================================================================
// Filename:	TextureManager.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================
#pragma once

#include "Texture.h"

namespace NFGE::Graphics
{
	using TextureId = size_t;

	class GenerateMipsPSO;
	class RootSignature;

	class TextureManager
	{
	public:
		static void StaticInitialize(std::filesystem::path rootPath);
		static void StaticTerminate();
		static TextureManager* Get();

	public:
		TextureManager() = default;
		~TextureManager();

		void SetRootPath(std::filesystem::path rootPath);

		TextureId LoadTexture(std::filesystem::path filename, TextureUsage textureUsage, bool isUsingRootPath = true); // Need PipelineWorker instance
		void Clear();
		Texture* GetTexture(TextureId textureId);

		uint32_t GetSpriteWidth(TextureId textureId);
		uint32_t GetSpriteHeight(TextureId textureId);
		void* GetSprite(TextureId textureId);
	private:
		std::filesystem::path mRootPath;
		std::unordered_map<TextureId, std::unique_ptr<Texture>> mInventory;

		// Pipeline objects for Mip map generation.
		std::unique_ptr<GenerateMipsPSO> mGenerateMipsPSO;
		ID3D12RootSignature* mRootSignature;

		void GenerateMips(Texture& texture, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);
		void GenerateMips_UAV(const Texture& texture, DXGI_FORMAT format, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);
		void SetComputeRootSignature(const RootSignature& rootSignature, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);
		void LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandlist);
		
	};
}
