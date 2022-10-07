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
#include "GenerateMipsPSO.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include "D3DUtil.h"

using namespace NFGE::Graphics;
using namespace DirectX;
using namespace Microsoft::WRL;

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
        //mResourceStateTracker->FlushResourceBarriers(*this);
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
    //mResourceStateTracker->FlushResourceBarriers(*this);
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

void NFGE::Graphics::CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
    mD3d12CommandList->IASetPrimitiveTopology(primitiveTopology);
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
        TrackObject(heap);

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
        TrackObject(aliasResource);

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
        TrackObject(uavResource);

        // Add an aliasing barrier for the alias resource.
        AliasingBarrier(nullptr, aliasResource.Get());

        // Copy the original resource to the alias resource.
        // This ensures GPU validation.
        CopyResource(aliasResource.Get(), d3d12Resource.Get());


        // Add an aliasing barrier for the UAV compatible resource.
        AliasingBarrier(aliasResource.Get(), uavResource.Get());
    }

    // Generate mips with the UAV compatible resource.
    GenerateMips_UAV(Texture(uavResource, texture.GetTextureUsage()), d3d12ResourceDesc.Format);

    if (aliasResource)
    {
        AliasingBarrier(uavResource.Get(), aliasResource.Get());
        // Copy the alias resource back to the original resource.
        CopyResource(d3d12Resource.Get(), aliasResource.Get());
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

void NFGE::Graphics::CommandList::SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* bufferData)
{
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = mUploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    mD3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void NFGE::Graphics::CommandList::SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
    mD3d12CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void NFGE::Graphics::CommandList::SetVertexBuffer(uint32_t slot, const VertexBuffer& vertexBuffer)
{
    TransitionBarrier(vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    auto vertexBufferView = vertexBuffer.GetVertexBufferView();

    mD3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);

    TrackResource(vertexBuffer);
}

void NFGE::Graphics::CommandList::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
    size_t bufferSize = numVertices * vertexSize;

    auto heapAllocation = mUploadBuffer->Allocate(bufferSize, vertexSize);
    memcpy(heapAllocation.CPU, vertexBufferData, bufferSize);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = heapAllocation.GPU;
    vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    vertexBufferView.StrideInBytes = static_cast<UINT>(vertexSize);

    mD3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void NFGE::Graphics::CommandList::SetIndexBuffer(const IndexBuffer& indexBuffer)
{
    TransitionBarrier(indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    auto indexBufferView = indexBuffer.GetIndexBufferView();

    mD3d12CommandList->IASetIndexBuffer(&indexBufferView);

    TrackResource(indexBuffer);
}

void NFGE::Graphics::CommandList::SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
    size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
    size_t bufferSize = numIndicies * indexSizeInBytes;

    auto heapAllocation = mUploadBuffer->Allocate(bufferSize, indexSizeInBytes);
    memcpy(heapAllocation.CPU, indexBufferData, bufferSize);

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation = heapAllocation.GPU;
    indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    indexBufferView.Format = indexFormat;

    mD3d12CommandList->IASetIndexBuffer(&indexBufferView);
}

void NFGE::Graphics::CommandList::SetGraphicsDynamicStructuredBuffer(uint32_t slot, size_t numElements, size_t elementSize, const void* bufferData)
{
    size_t bufferSize = numElements * elementSize;

    auto heapAllocation = mUploadBuffer->Allocate(bufferSize, elementSize);

    memcpy(heapAllocation.CPU, bufferData, bufferSize);

    mD3d12CommandList->SetGraphicsRootShaderResourceView(slot, heapAllocation.GPU);
}

void CommandList::SetViewport(const D3D12_VIEWPORT& viewport)
{
    SetViewports({ viewport });
}

void CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& viewports)
{
    ASSERT(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "Graphics pipeline setting too many viewports.");
    mD3d12CommandList->RSSetViewports(static_cast<UINT>(viewports.size()),
        viewports.data());
}

void CommandList::SetScissorRect(const D3D12_RECT& scissorRect)
{
    SetScissorRects({ scissorRect });
}

void CommandList::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
    ASSERT(scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "Graphics pipeline setting too many Scissors.");
    mD3d12CommandList->RSSetScissorRects(static_cast<UINT>(scissorRects.size()),
        scissorRects.data());
}

void NFGE::Graphics::CommandList::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
{
    mD3d12CommandList->SetPipelineState(pipelineState.Get());

    TrackObject(pipelineState);
}

void CommandList::SetGraphicsRootSignature(const RootSignature& rootSignature)
{
    auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
    if (mRootSignature != d3d12RootSignature)
    {
        mRootSignature = d3d12RootSignature;

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            mDynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
        }

        mD3d12CommandList->SetGraphicsRootSignature(mRootSignature);

        TrackObject(mRootSignature);
    }
}

void CommandList::SetComputeRootSignature(const RootSignature& rootSignature)
{
    auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
    if (mRootSignature != d3d12RootSignature)
    {
        mRootSignature = d3d12RootSignature;

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            mDynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
        }

        mD3d12CommandList->SetComputeRootSignature(mRootSignature);

        TrackObject(mRootSignature);
    }
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
        mDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw();
    }

    mD3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw();
    }

    mD3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch();
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

void NFGE::Graphics::CommandList::GenerateMips_UAV(const Texture& texture, DXGI_FORMAT format)
{
    if (!mGenerateMipsPSO)
    {
        mGenerateMipsPSO = std::make_unique<GenerateMipsPSO>();
    }

    mD3d12CommandList->SetPipelineState(mGenerateMipsPSO->GetPipelineState().Get());
    SetComputeRootSignature(mGenerateMipsPSO->GetRootSignature());

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

        SetCompute32BitConstants(GenerateMips::GenerateMipsCB, generateMipsCB);

        SetShaderResourceView(GenerateMips::SrcMip, 0, texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, srcMip, 1, &srvDesc);

        for (uint32_t mip = 0; mip < mipCount; ++mip)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = resourceDesc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = srcMip + mip + 1;

            SetUnorderedAccessView(GenerateMips::OutMip, mip, texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srcMip + mip + 1, 1, &uavDesc);
        }

        // Pad any unused mip levels with a default UAV. Doing this keeps the DX12 runtime happy.
        if (mipCount < 4)
        {
            mDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(GenerateMips::OutMip, mipCount, 4 - mipCount, mGenerateMipsPSO->GetDefaultUAV());
        }

        Dispatch(NFGE::Math::Memory::DivideByMultiple(dstWidth, 8), NFGE::Math::Memory::DivideByMultiple(dstHeight, 8));

        UAVBarrier(texture);

        srcMip += mipCount;
    }
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
