//====================================================================================================
// Filename:	TextureManager.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#include "Precompiled.h"
#include "TextureManager.h"

#include "d3dx12.h"
#include "D3DUtil.h"
#include "ResourceStateTracker.h"
#include "CommandList.h"
using namespace NFGE::Graphics;
using namespace DirectX;

namespace
{
	std::unique_ptr<TextureManager> sInstance = nullptr;

	void LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage, CommandList& commandlist)
	{
        auto device = NFGE::Graphics::GetDevice();

        TexMetadata metadata;
        ScratchImage scratchImage;
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
        
        std::filesystem::path filePath(fileName);
        if (filePath.extension() == ".dds")
        {
            // Use DDS texture loader.
            ThrowIfFailed(LoadFromDDSFile(fileName.c_str(), DDS_FLAGS_NONE, &metadata, scratchImage));
        }
        else if (filePath.extension() == ".hdr")
        {
            ThrowIfFailed(LoadFromHDRFile(fileName.c_str(), &metadata, scratchImage));
        }
        else if (filePath.extension() == ".tga")
        {
            ThrowIfFailed(LoadFromTGAFile(fileName.c_str(), &metadata, scratchImage));
        }
        else
        {
            ThrowIfFailed(LoadFromWICFile(fileName.c_str(), WIC_FLAGS_NONE, &metadata, scratchImage));
        }

        if (textureUsage == TextureUsage::Albedo)
        {
            metadata.format = MakeSRGB(metadata.format);
        }

        D3D12_RESOURCE_DESC textureDesc = {};
        switch (metadata.dimension)
        {
        case TEX_DIMENSION_TEXTURE1D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT16>(metadata.arraySize));
            break;
        case TEX_DIMENSION_TEXTURE2D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.arraySize));
            break;
        case TEX_DIMENSION_TEXTURE3D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.depth));
            break;
        default:
            throw std::exception("Invalid texture dimension.");
            break;
        }

        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&textureResource)));

        // Update the global state tracker.
        ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);

        texture.SetTextureUsage(textureUsage);
        texture.SetD3D12Resource(textureResource);
        texture.CreateViews();
        texture.SetName(fileName);

        std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
        const Image* pImages = scratchImage.GetImages();
        for (int i = 0; i < scratchImage.GetImageCount(); ++i)
        {
            auto& subresource = subresources[i];
            subresource.RowPitch = pImages[i].rowPitch;
            subresource.SlicePitch = pImages[i].slicePitch;
            subresource.pData = pImages[i].pixels;
        }

        commandlist.CopyTextureSubresource(texture, 0, static_cast<uint32_t>(subresources.size()), subresources.data());

        if (subresources.size() < textureResource->GetDesc().MipLevels)
        {
            commandlist.GenerateMips(texture);
        }
	}
}

void NFGE::Graphics::TextureManager::StaticInitialize(std::filesystem::path rootPath)
{
	ASSERT(sInstance == nullptr, "[TextureManager] System already initlizlized!");
	sInstance = std::make_unique<TextureManager>();
	sInstance->SetRootPath(std::move(rootPath));
}

void NFGE::Graphics::TextureManager::StaticTerminate()
{
	if (sInstance != nullptr)
	{
		sInstance.reset();
	}
}

TextureManager* NFGE::Graphics::TextureManager::Get()
{
	ASSERT(sInstance != nullptr, "[TextureManager] System already initlizlized!");
	return sInstance.get();
}

NFGE::Graphics::TextureManager::~TextureManager()
{
	// TODO Do some error checks here, but we need a way to unload sepecific textures first
	// Maybe add the concept of texture group
	for (auto& [key, value] : mInventory)
	{
		value->Reset();
	}
}

void NFGE::Graphics::TextureManager::SetRootPath(std::filesystem::path rootPath)
{
	ASSERT(std::filesystem::is_directory(rootPath), "[TextureManager] %s is not a directory!", (char*)rootPath.c_str());
	mRootPath = std::move(rootPath);
	//if (mRootPath.back() != L'/')
	//	mRootPath += L'/';
}

TextureId NFGE::Graphics::TextureManager::LoadTexture(std::filesystem::path filename, TextureUsage textureUsage, bool isUsingRootPath)
{

	auto hash = std::filesystem::hash_value(filename);
	auto [iter, success] = mInventory.insert({ hash, nullptr });
	if (success)
	{
		iter->second = std::make_unique<Texture>();
		if (isUsingRootPath)
			LoadTextureFromFile(*iter->second, mRootPath / filename, textureUsage, );
		else
			iter->second->Initialize(filename);
	}
	else
	{
		int a = 0;
	}
	return hash;
}

void TextureManager::Clear()
{
	for (auto& item : mInventory)
	{
		if (item.second)
		{
			item.second->Reset();
		}
	}
	mInventory.clear();
}

Texture* NFGE::Graphics::TextureManager::GetTexture(TextureId textureId)
{

	auto iter = mInventory.find(textureId);
	return iter != mInventory.end() ? iter->second.get() : nullptr;
}

uint32_t NFGE::Graphics::TextureManager::GetSpriteWidth(TextureId textureId)
{
	Texture* texture = TextureManager::Get()->GetTexture(textureId);
	return texture ? texture->GetWidth() : 0u;
}

uint32_t NFGE::Graphics::TextureManager::GetSpriteHeight(TextureId textureId)
{
	Texture* texture = TextureManager::Get()->GetTexture(textureId);
	return texture ? texture->GetHeight() : 0u;
}

void* NFGE::Graphics::TextureManager::GetSprite(TextureId textureId)
{
	Texture* texture = TextureManager::Get()->GetTexture(textureId);
	return texture ? texture->GetD3D12Resource().Get() : nullptr;
}
