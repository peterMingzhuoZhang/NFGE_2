//====================================================================================================
// Filename:	CommandList.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#include "Precompiled.h"
#include "CommandList.h"
#include "DynamicDescriptorHeap.h"
#include "RootSignature.h"
#include "ResourceStateTracker.h"
#include "UploadBuffer.h"
#include "RenderTarget.h"
#include "Resource.h"
#include "Texture.h"

#include "D3DUtil.h"

using namespace NFGE::Graphics;
using namespace DirectX;

NFGE::Graphics::CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
    : mD3d12CommandListType(type)
{
    auto device = NFGE::Graphics::GetDevice();

    ThrowIfFailed(device->CreateCommandAllocator(mD3d12CommandListType, IID_PPV_ARGS(&mD3d12CommandAllocator)));

    ThrowIfFailed(device->CreateCommandList(0, mD3d12CommandListType, mD3d12CommandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&mD3d12CommandList)));

    mUploadBuffer = std::make_unique<UploadBuffer>();
    mResourceStateTracker = std::make_unique<ResourceStateTracker>();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        mDescriptorHeaps[i] = nullptr;
    }
}

NFGE::Graphics::CommandList::~CommandList()
{}

void NFGE::Graphics::CommandList::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers)
{
    if (resource)
    {
        // The "before" state is not important. It will be resolved by the resource state tracker.
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource);
        mResourceStateTracker->ResourceBarrier(barrier);
    }

    if (flushBarriers)
    {
        mResourceStateTracker->FlushResourceBarriers(*this);
    }
}

void NFGE::Graphics::CommandList::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource, bool flushBarriers)
{
    auto d3d12Resource = resource.GetD3D12Resource();
    TransitionBarrier(d3d12Resource.Get(), stateAfter, subresource, flushBarriers);
}

void CommandList::UAVBarrier(ID3D12Resource* resource, bool flushBarriers)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource);

    mResourceStateTracker->ResourceBarrier(barrier);

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void NFGE::Graphics::CommandList::UAVBarrier(const Resource& resource, bool flushBarriers)
{
    auto d3d12Resource = resource.GetD3D12Resource();
    UAVBarrier(d3d12Resource.Get(), flushBarriers);
}

void CommandList::AliasingBarrier(ID3D12Resource* beforeResource, ID3D12Resource* afterResource, bool flushBarriers)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(beforeResource, afterResource);

    mResourceStateTracker->ResourceBarrier(barrier);

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void NFGE::Graphics::CommandList::AliasingBarrier(const Resource& beforeResource, const Resource& afterResource, bool flushBarriers)
{
    auto d3d12ResourceBefore = beforeResource.GetD3D12Resource();
    auto d3d12ResourceAfter = afterResource.GetD3D12Resource();
    AliasingBarrier(d3d12ResourceBefore.Get(), d3d12ResourceAfter.Get(), flushBarriers);
}

void CommandList::FlushResourceBarriers()
{
    mResourceStateTracker->FlushResourceBarriers(*this);
}

void CommandList::CopyResource(ID3D12Resource* dstRes, ID3D12Resource* srcRes)
{
    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

    FlushResourceBarriers();

    mD3d12CommandList->CopyResource(dstRes, srcRes);

    TrackResource(dstRes);
    TrackResource(srcRes);
}

void NFGE::Graphics::CommandList::CopyResource(Resource& dstRes, const Resource& srcRes)
{
    auto d3d12ResourceDst = dstRes.GetD3D12Resource();
    auto d3d12ResourceSrc = srcRes.GetD3D12Resource();
    CopyResource(d3d12ResourceDst.Get(), d3d12ResourceSrc.Get());
}

void NFGE::Graphics::CommandList::LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage)
{
    auto device = NFGE::Graphics::GetDevice();

    std::filesystem::path filePath(fileName);
    if (!std::filesystem::exists(filePath))
    {
        throw std::exception("File not found.");
    }

    auto iter = sTextureCache.find(fileName);
    if (iter != sTextureCache.end())
    {
        texture.SetTextureUsage(textureUsage);
        texture.SetD3D12Resource(iter->second);
        texture.CreateViews();
        texture.SetName(fileName);
    }
    else
    {
        TexMetadata metadata;
        ScratchImage scratchImage;
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

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

        CopyTextureSubresource(texture, 0, static_cast<uint32_t>(subresources.size()), subresources.data());

        if (subresources.size() < textureResource->GetDesc().MipLevels)
        {
            GenerateMips(texture);
        }

        // Add the texture resource to the texture cache.
        std::lock_guard<std::mutex> lock(sTextureCacheMutex);
        sTextureCache[fileName] = textureResource.Get();
    }
}

void NFGE::Graphics::CommandList::ClearTexture(const Texture& texture, const float clearColor[4])
{
    TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mD3d12CommandList->ClearRenderTargetView(texture.GetRenderTargetView(), clearColor, 0, nullptr);

    TrackResource(texture);
}

void NFGE::Graphics::CommandList::ClearDepthStencilTexture(const Texture& texture, D3D12_CLEAR_FLAGS clearFlags, float depth, uint8_t stencil)
{
    TransitionBarrier(texture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mD3d12CommandList->ClearDepthStencilView(texture.GetDepthStencilView(), clearFlags, depth, stencil, 0, nullptr);

    TrackResource(texture);
}

void NFGE::Graphics::CommandList::GenerateMips(Texture& texture)
{
    if (mD3d12CommandListType == D3D12_COMMAND_LIST_TYPE_COPY)
    {
        // TODO:: grab Compute type command list to do this logic
        /*if (!mComputeCommandList)
        {
            mComputeCommandList = NFGE::Graphics::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE)->GetCommandList().Get();
        }
        m_ComputeCommandList->GenerateMips(texture);*/
        return;
    }

    auto d3d12Resource = texture.GetD3D12Resource();

    // If the texture doesn't have a valid resource, do nothing.
    if (!d3d12Resource) return;
    auto d3d12ResourceDesc = d3d12Resource->GetDesc();

    // If the texture only has a single mip level (level 0)
    // do nothing.
    if (d3d12ResourceDesc.MipLevels == 1) return;
    // Currently, only 2D textures are supported.
    if (d3d12ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || d3d12ResourceDesc.DepthOrArraySize != 1)
    {
        throw std::exception("Generate Mips only supports 2D Textures.");
    }

    if (Texture::IsUAVCompatibleFormat(d3d12ResourceDesc.Format))
    {
        //GenerateMips_UAV(texture);
    }
    else if (Texture::IsBGRFormat(d3d12ResourceDesc.Format))
    {
        //GenerateMips_BGR(texture);
    }
    else if (Texture::IsSRGBFormat(d3d12ResourceDesc.Format))
    {
        //GenerateMips_sRGB(texture);
    }
    else
    {
        throw std::exception("Unsupported texture format for mipmap generation.");
    }
}

void NFGE::Graphics::CommandList::CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
{
    auto device = NFGE::Graphics::GetDevice();
    auto destinationResource = texture.GetD3D12Resource();
    if (destinationResource)
    {
        // Resource must be in the copy-destination state.
        TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();

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

        UpdateSubresources(mD3d12CommandList.Get(), destinationResource.Get(), intermediateResource.Get(), 0, firstSubresource, numSubresources, subresourceData);

        TrackObject(intermediateResource);
        TrackObject(destinationResource);
    }
}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = mUploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    mD3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void CommandList::SetShaderResourceView(uint32_t rootParameterIndex,
    uint32_t descriptorOffset,
    const Resource& resource,
    D3D12_RESOURCE_STATES stateAfter,
    UINT firstSubresource,
    UINT numSubresources,
    const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
    if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        for (uint32_t i = 0; i < numSubresources; ++i)
        {
            TransitionBarrier(resource, stateAfter, firstSubresource + i);
        }
    }
    else
    {
        TransitionBarrier(resource, stateAfter);
    }

    mDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, resource.GetShaderResourceView(srv));

    TrackResource(resource.GetD3D12Resource().Get());
}

void CommandList::SetUnorderedAccessView(uint32_t rootParameterIndex,
    uint32_t descrptorOffset,
    const Resource& resource,
    D3D12_RESOURCE_STATES stateAfter,
    UINT firstSubresource,
    UINT numSubresources,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav)
{
    if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        for (uint32_t i = 0; i < numSubresources; ++i)
        {
            TransitionBarrier(resource, stateAfter, firstSubresource + i);
        }
    }
    else
    {
        TransitionBarrier(resource, stateAfter);
    }

    mDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descrptorOffset, 1, resource.GetUnorderedAccessView(uav));

    TrackResource(resource.GetD3D12Resource().Get());
}

void CommandList::SetRenderTarget(const RenderTarget& renderTarget)
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;
    renderTargetDescriptors.reserve(AttachmentPoint::NumAttachmentPoints);

    const auto& textures = renderTarget.GetTextures();

    // Bind color targets (max of 8 render targets can be bound to the rendering pipeline.
    for (int i = 0; i < 8; ++i)
    {
        auto& texture = textures[i];

        if (texture.IsValid())
        {
            TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
            renderTargetDescriptors.push_back(texture.GetRenderTargetView());

            TrackResource(texture.GetD3D12Resource().Get());
        }
    }

    const auto& depthTexture = renderTarget.GetTexture(AttachmentPoint::DepthStencil);

    CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor(D3D12_DEFAULT);
    if (depthTexture.GetD3D12Resource())
    {
        TransitionBarrier(depthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        depthStencilDescriptor = depthTexture.GetDepthStencilView();

        TrackResource(depthTexture.GetD3D12Resource().Get());
    }

    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = depthStencilDescriptor.ptr != 0 ? &depthStencilDescriptor : nullptr;

    mD3d12CommandList->OMSetRenderTargets(static_cast<UINT>(renderTargetDescriptors.size()),
        renderTargetDescriptors.data(), FALSE, pDSV);
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
    }

    mD3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
    }

    mD3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch(*this);
    }

    mD3d12CommandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
}

void NFGE::Graphics::CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
{
    if (mDescriptorHeaps[heapType] != heap)
    {
        mDescriptorHeaps[heapType] = heap;
        BindDescriptorHeaps();
    }
}

void CommandList::TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object)
{
    mTrackedObjects.push_back(object);
}

void CommandList::TrackResource(ID3D12Resource* res)
{
    TrackObject(res);
}

void NFGE::Graphics::CommandList::TrackResource(const Resource& res)
{
    TrackObject(res.GetD3D12Resource());
}

void NFGE::Graphics::CommandList::BindDescriptorHeaps()
{
    UINT numDescriptorHeaps = 0;
    ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

    for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        ID3D12DescriptorHeap* descriptorHeap = mDescriptorHeaps[i];
        if (descriptorHeap)
        {
            descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
        }
    }

    mD3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}
