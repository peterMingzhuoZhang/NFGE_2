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

#include "D3DUtil.h"

using namespace NFGE::Graphics;

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

void CommandList::UAVBarrier(ID3D12Resource* resource, bool flushBarriers)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource);

    mResourceStateTracker->ResourceBarrier(barrier);

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
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

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = mUploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    mD3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

//void CommandList::SetShaderResourceView(uint32_t rootParameterIndex,
//    uint32_t descriptorOffset,
//    const Resource& resource,
//    D3D12_RESOURCE_STATES stateAfter,
//    UINT firstSubresource,
//    UINT numSubresources,
//    const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
//{
//    if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
//    {
//        for (uint32_t i = 0; i < numSubresources; ++i)
//        {
//            TransitionBarrier(resource, stateAfter, firstSubresource + i);
//        }
//    }
//    else
//    {
//        TransitionBarrier(resource, stateAfter);
//    }
//
//    m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, resource.GetShaderResourceView(srv));
//
//    TrackResource(resource);
//}
//
//void CommandList::SetUnorderedAccessView(uint32_t rootParameterIndex,
//    uint32_t descrptorOffset,
//    const Resource& resource,
//    D3D12_RESOURCE_STATES stateAfter,
//    UINT firstSubresource,
//    UINT numSubresources,
//    const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav)
//{
//    if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
//    {
//        for (uint32_t i = 0; i < numSubresources; ++i)
//        {
//            TransitionBarrier(resource, stateAfter, firstSubresource + i);
//        }
//    }
//    else
//    {
//        TransitionBarrier(resource, stateAfter);
//    }
//
//    m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descrptorOffset, 1, resource.GetUnorderedAccessView(uav));
//
//    TrackResource(resource);
//}


//void CommandList::SetRenderTarget(const RenderTarget& renderTarget)
//{
//    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;
//    renderTargetDescriptors.reserve(AttachmentPoint::NumAttachmentPoints);
//
//    const auto& textures = renderTarget.GetTextures();
//
//    // Bind color targets (max of 8 render targets can be bound to the rendering pipeline.
//    for (int i = 0; i < 8; ++i)
//    {
//        auto& texture = textures[i];
//
//        if (texture.IsValid())
//        {
//            TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
//            renderTargetDescriptors.push_back(texture.GetRenderTargetView());
//
//            TrackResource(texture);
//        }
//    }
//
//    const auto& depthTexture = renderTarget.GetTexture(AttachmentPoint::DepthStencil);
//
//    CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor(D3D12_DEFAULT);
//    if (depthTexture.GetD3D12Resource())
//    {
//        TransitionBarrier(depthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
//        depthStencilDescriptor = depthTexture.GetDepthStencilView();
//
//        TrackResource(depthTexture);
//    }
//
//    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = depthStencilDescriptor.ptr != 0 ? &depthStencilDescriptor : nullptr;
//
//    m_d3d12CommandList->OMSetRenderTargets(static_cast<UINT>(renderTargetDescriptors.size()),
//        renderTargetDescriptors.data(), FALSE, pDSV);
//}

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
