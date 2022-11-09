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
//#include "CommandList.h"
#include "CommandQueue.h"
#include "GraphicsSystem.h"
#include "GenerateMipsPSO.h"
#include "DynamicDescriptorHeap.h"
#include "RootSignature.h"
#include "PipelineWorker.h"
#include "SpriteRenderer.h"

using namespace NFGE::Graphics;
using namespace DirectX;
using namespace Microsoft::WRL;

namespace
{
	std::unique_ptr<TextureManager> sInstance = nullptr;
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
		auto worker = NFGE::Graphics::GetWorker(WorkerType::Compute);
		iter->second = std::make_unique<Texture>();
		if (isUsingRootPath)
			LoadTextureFromFile(*iter->second.get(), mRootPath / filename, textureUsage, *worker);
		else
			LoadTextureFromFile(*iter->second.get(), filename, textureUsage, *worker);

        if (textureUsage == TextureUsage::Sprite)
        {
            Graphics::SpriteRenderer::Get()->RegisterSpriteTexture(*iter->second.get());
        }

	}
	else
	{
        ASSERT(false, "[TextureManager] Reload on same texture ID not is not allowed. Must clear first.");
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

void NFGE::Graphics::TextureManager::GenerateMips(Texture& texture, PipelineWorker& pipelineWorker)
{
    auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();

    ASSERT(pipelineWorker.GetCommandListType() != D3D12_COMMAND_LIST_TYPE_COPY, "[TextureManager] require a non-copy type commandList");

    auto d3d12Resource = texture.GetD3D12Resource();

    // If the texture doesn't have a valid resource, do nothing.
    if (!d3d12Resource) return;
    auto d3d12ResourceDesc = d3d12Resource->GetDesc();

    // If the texture only has a single mip level (level 0)
    // do nothing.
    if (d3d12ResourceDesc.MipLevels == 1) return;
    // Currently, only 2D textures are supported.
    if (d3d12ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || d3d12ResourceDesc.DepthOrArraySize != 1 || d3d12ResourceDesc.SampleDesc.Count > 1)
    {
        throw std::exception("Generate Mips only supports non-multiSampled 2D Textures.");
    }

    ComPtr<ID3D12Resource> uavResource = d3d12Resource;
    // Create an alias of the original resource.
    // This is done to perform a GPU copy of resources with different formats.
    // BGR -> RGB texture copies will fail GPU validation unless performed 
    // through an alias of the BRG resource in a placed heap.
    ComPtr<ID3D12Resource> aliasResource;

    // If the passed-in resource does not allow for UAV access
    // then create a staging resource that is used to generate
    // the mipmap chain.
    if (!texture.CheckUAVSupport() ||
        (d3d12ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
    {
        auto device = NFGE::Graphics::GetDevice();

        // Describe an alias resource that is used to copy the original texture.
        auto aliasDesc = d3d12ResourceDesc;
        // Placed resources can't be render targets or depth-stencil views.
        aliasDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        aliasDesc.Flags &= ~(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        // Describe a UAV compatible resource that is used to perform
        // mipmapping of the original texture.
        auto uavDesc = aliasDesc;   // The flags for the UAV description must match that of the alias description.
        uavDesc.Format = Texture::GetUAVCompatableFormat(d3d12ResourceDesc.Format);

        D3D12_RESOURCE_DESC resourceDescs[] = {
            aliasDesc,
            uavDesc
        };

        // Create a heap that is large enough to store a copy of the original resource.
        auto allocationInfo = device->GetResourceAllocationInfo(0, _countof(resourceDescs), resourceDescs);

        D3D12_HEAP_DESC heapDesc = {};
        heapDesc.SizeInBytes = allocationInfo.SizeInBytes;
        heapDesc.Alignment = allocationInfo.Alignment;
        heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
        heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        ComPtr<ID3D12Heap> heap;
        ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));

        // Make sure the heap does not go out of scope until the command list
        // is finished executing on the command queue.
        pipelineWorker.TrackObject(heap);

        // Create a placed resource that matches the description of the 
        // original resource. This resource is used to copy the original 
        // texture to the UAV compatible resource.
        ThrowIfFailed(device->CreatePlacedResource(
            heap.Get(),
            0,
            &aliasDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&aliasResource)
        ));

        ResourceStateTracker::AddGlobalResourceState(aliasResource.Get(), D3D12_RESOURCE_STATE_COMMON);
        // Ensure the scope of the alias resource.
        pipelineWorker.TrackObject(aliasResource);

        // Create a UAV compatible resource in the same heap as the alias
        // resource.
        ThrowIfFailed(device->CreatePlacedResource(
            heap.Get(),
            0,
            &uavDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&uavResource)
        ));

        ResourceStateTracker::AddGlobalResourceState(uavResource.Get(), D3D12_RESOURCE_STATE_COMMON);
        // Ensure the scope of the UAV compatible resource.
        pipelineWorker.TrackObject(uavResource);

        // Add an aliasing barrier for the alias resource.
        pipelineWorker.AliasingBarrier(nullptr, aliasResource.Get());

        // Copy the original resource to the alias resource.
        // This ensures GPU validation.
        pipelineWorker.CopyResource(aliasResource.Get(), d3d12Resource.Get());


        // Add an aliasing barrier for the UAV compatible resource.
        pipelineWorker.AliasingBarrier(aliasResource.Get(), uavResource.Get());
    }

    // Generate mips with the UAV compatible resource.
    GenerateMips_UAV(Texture(uavResource, texture.GetTextureUsage()), d3d12ResourceDesc.Format, pipelineWorker);

    if (aliasResource)
    {
        pipelineWorker.AliasingBarrier(uavResource.Get(), aliasResource.Get());
        // Copy the alias resource back to the original resource.
        pipelineWorker.CopyResource(d3d12Resource.Get(), aliasResource.Get());
    }
}
void NFGE::Graphics::TextureManager::GenerateMips_UAV(const Texture& texture, DXGI_FORMAT format, PipelineWorker& pipelineWorker)
{
    auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
    if (!mGenerateMipsPSO)
    {
        mGenerateMipsPSO = std::make_unique<GenerateMipsPSO>();
    }

    pipelineWorker.SetPipelineState(mGenerateMipsPSO->GetPipelineState().Get());
    pipelineWorker.SetComputeRootSignature(mGenerateMipsPSO->GetRootSignature());

    GenerateMipsCB generateMipsCB;
    generateMipsCB.IsSRGB = Texture::IsSRGBFormat(format);

    auto resource = texture.GetD3D12Resource();
    auto resourceDesc = resource->GetDesc();

    // Create an SRV that uses the format of the original texture.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
    srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

    for (uint32_t srcMip = 0; srcMip < resourceDesc.MipLevels - 1u; )
    {
        uint64_t srcWidth = resourceDesc.Width >> srcMip;
        uint32_t srcHeight = resourceDesc.Height >> srcMip;
        uint32_t dstWidth = static_cast<uint32_t>(srcWidth >> 1);
        uint32_t dstHeight = srcHeight >> 1;

        // 0b00(0): Both width and height are even.
        // 0b01(1): Width is odd, height is even.
        // 0b10(2): Width is even, height is odd.
        // 0b11(3): Both width and height are odd.
        generateMipsCB.SrcDimension = (srcHeight & 1) << 1 | (srcWidth & 1);

        // How many mipmap levels to compute this pass (max 4 mips per pass)
        DWORD mipCount;

        // The number of times we can half the size of the texture and get
        // exactly a 50% reduction in size.
        // A 1 bit in the width or height indicates an odd dimension.
        // The case where either the width or the height is exactly 1 is handled
        // as a special case (as the dimension does not require reduction).
        _BitScanForward(&mipCount, (dstWidth == 1 ? dstHeight : dstWidth) |
            (dstHeight == 1 ? dstWidth : dstHeight));
        // Maximum number of mips to generate is 4.
        mipCount = std::min<DWORD>(4, mipCount + 1);
        // Clamp to total number of mips left over.
        mipCount = (srcMip + mipCount) >= resourceDesc.MipLevels ?
            resourceDesc.MipLevels - srcMip - 1 : mipCount;

        // Dimensions should not reduce to 0.
        // This can happen if the width and height are not the same.
        dstWidth = std::max<DWORD>(1, dstWidth);
        dstHeight = std::max<DWORD>(1, dstHeight);

        generateMipsCB.SrcMipLevel = srcMip;
        generateMipsCB.NumMipLevels = mipCount;
        generateMipsCB.TexelSize.x = 1.0f / (float)dstWidth;
        generateMipsCB.TexelSize.y = 1.0f / (float)dstHeight;

        pipelineWorker.SetCompute32BitConstants(GenerateMips::GenerateMipsCB, generateMipsCB);

        pipelineWorker.SetShaderResourceView(GenerateMips::SrcMip, 0, texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, srcMip, 1, &srvDesc);

        for (uint32_t mip = 0; mip < mipCount; ++mip)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = resourceDesc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = srcMip + mip + 1;

            pipelineWorker.SetUnorderedAccessView(GenerateMips::OutMip, mip, texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srcMip + mip + 1, 1, &uavDesc);
        }

        // Pad any unused mip levels with a default UAV. Doing this keeps the DX12 runtime happy.
        if (mipCount < 4)
        {
            pipelineWorker.mDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(GenerateMips::OutMip, mipCount, 4 - mipCount, mGenerateMipsPSO->GetDefaultUAV());
        }

        pipelineWorker.Dispatch(NFGE::Math::Memory::DivideByMultiple(dstWidth, 8), NFGE::Math::Memory::DivideByMultiple(dstHeight, 8));

        pipelineWorker.UAVBarrier(texture);

        srcMip += mipCount;
    }
}

void NFGE::Graphics::TextureManager::LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage, PipelineWorker& pipelineWorker)
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

    pipelineWorker.CopyTextureSubresource(texture, 0, static_cast<uint32_t>(subresources.size()), subresources.data());

    if (subresources.size() < textureResource->GetDesc().MipLevels)
    {
        GenerateMips(texture, pipelineWorker);
    }
}


void NFGE::Graphics::TextureManager::CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData, PipelineWorker& pipelineWorker)
{
	auto device = NFGE::Graphics::GetDevice();
	auto destinationResource = texture.GetD3D12Resource();
	if (destinationResource)
	{
		auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
		// Resource must be in the copy-destination state.
		pipelineWorker.TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST);
		pipelineWorker.FlushResourceBarriers();

		UINT64 requiredSize = GetRequiredIntermediateSize(destinationResource.Get(), firstSubresource, numSubresources);

		// Create a temporary (intermediate) resource for uploading the subresources
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateResource)
		));

		UpdateSubresources(graphicSystem->GetCurrentCommandList().Get(), destinationResource.Get(), intermediateResource.Get(), 0, firstSubresource, numSubresources, subresourceData);

		pipelineWorker.TrackObject(intermediateResource);
		pipelineWorker.TrackObject(destinationResource);
	}
}
    