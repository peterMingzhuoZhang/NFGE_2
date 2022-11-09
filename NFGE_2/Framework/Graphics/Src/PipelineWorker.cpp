//====================================================================================================
// Filename:	PipelineWork.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================
#include "Precompiled.h"
#include "PipelineWorker.h"

#include "D3DUtil.h"

#include "GraphicsSystem.h"
#include "ResourceStateTracker.h"
#include "DynamicDescriptorHeap.h"
#include "Buffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "UploadBuffer.h"
#include "RootSignature.h"
#include "RenderTarget.h"
#include "Resource.h"
#include "CommandQueue.h"
#include "D3DUtil.h"

using namespace NFGE::Graphics;

NFGE::Graphics::PipelineWorker::PipelineWorker(WorkerType type)
	:mType(type)
{
    Initialize();
}

NFGE::Graphics::PipelineWorker::~PipelineWorker()
{
    Terminate();
}

void NFGE::Graphics::PipelineWorker::Initialize()
{
    auto device = NFGE::Graphics::GetDevice();
    mResourceStateTracker = std::make_unique<ResourceStateTracker>();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        mDescriptorHeaps[i] = nullptr;
    }

    mUploadBuffer = std::make_unique<UploadBuffer>();

    mCommandQueue = std::make_unique<CommandQueue>(device, static_cast<D3D12_COMMAND_LIST_TYPE>(mType));
    mResourceStateTracker = std::make_unique<ResourceStateTracker>();
}

void NFGE::Graphics::PipelineWorker::Terminate()
{
    Reset();
    mResourceStateTracker->Reset();
    mUploadBuffer->Reset();
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->Reset();
        mDescriptorHeaps[i] = nullptr;
    }
}

void NFGE::Graphics::PipelineWorker::RegisterComponent(PipelineComponent* component)
{
    mTrackedPipelineComponents.push_back(component);
}

void NFGE::Graphics::PipelineWorker::BeginWork()
{
    ASSERT(mCurrentCommandList == nullptr, "[PipelineWorker] Can not BeginWork() while doing work");
    mCurrentCommandList = mCommandQueue->GetCommandList();
    NFGE::Graphics::GraphicsSystem::Get()->SetCurrentCommandList(mCurrentCommandList);
}

void NFGE::Graphics::PipelineWorker::ProcessWork()
{
    for (auto& piplineComponent : mTrackedPipelineComponents)
    {
        piplineComponent->GetLoad(*this);
    }
}

void NFGE::Graphics::PipelineWorker::EndWork()
{
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");

    auto signal = mCommandQueue->ExecuteCommandList(mCurrentCommandList, *this);
    mCommandQueue->WaitForFenceValue(signal);
    mCurrentCommandList = nullptr;
    mTrackedPipelineComponents.clear();
    Reset();
    
}

void NFGE::Graphics::PipelineWorker::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource, bool flushBarriers)
{
	auto d3d12Resource = resource.GetD3D12Resource();
	TransitionBarrier(d3d12Resource.Get(), stateAfter, subresource, flushBarriers);
}

void NFGE::Graphics::PipelineWorker::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers)
{
	if (resource)
	{
		// The "before" state is not important. It will be resolved by the resource state tracker.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource);
		mResourceStateTracker->ResourceBarrier(barrier);
	}

	if (flushBarriers)
	{
        ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
		mResourceStateTracker->FlushResourceBarriers(mCurrentCommandList);
	}
}

void NFGE::Graphics::PipelineWorker::UAVBarrier(const Resource& resource, bool flushBarriers)
{
	auto d3d12Resource = resource.GetD3D12Resource();
	UAVBarrier(d3d12Resource.Get(), flushBarriers);
}

void NFGE::Graphics::PipelineWorker::UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, bool flushBarriers)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource.Get());

	mResourceStateTracker->ResourceBarrier(barrier);

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void NFGE::Graphics::PipelineWorker::AliasingBarrier(const Resource& beforeResource, const Resource& afterResource, bool flushBarriers)
{
	auto d3d12ResourceBefore = beforeResource.GetD3D12Resource();
	auto d3d12ResourceAfter = afterResource.GetD3D12Resource();
	AliasingBarrier(d3d12ResourceBefore.Get(), d3d12ResourceAfter.Get(), flushBarriers);
}

void NFGE::Graphics::PipelineWorker::AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> beforeResource, Microsoft::WRL::ComPtr<ID3D12Resource> afterResource, bool flushBarriers)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(beforeResource.Get(), afterResource.Get());

	mResourceStateTracker->ResourceBarrier(barrier);

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void NFGE::Graphics::PipelineWorker::CopyResource(Resource& dstRes, const Resource& srcRes)
{
}

void NFGE::Graphics::PipelineWorker::CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes)
{
	TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

	FlushResourceBarriers();

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
	mCurrentCommandList->CopyResource(dstRes.Get(), srcRes.Get());

	TrackObject(dstRes);
	TrackObject(srcRes);
}

void NFGE::Graphics::PipelineWorker::ResolveSubresource(Resource& dstRes, const Resource& srcRes, uint32_t dstSubresource, uint32_t srcSubresource)
{
	TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_RESOLVE_DEST, dstSubresource);
	TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, srcSubresource);

	FlushResourceBarriers();

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
	mCurrentCommandList->ResolveSubresource(dstRes.GetD3D12Resource().Get(), dstSubresource, srcRes.GetD3D12Resource().Get(), srcSubresource, dstRes.GetD3D12ResourceDesc().Format);

	TrackResource(srcRes);
	TrackResource(dstRes);
}

void NFGE::Graphics::PipelineWorker::CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData)
{
    CopyBuffer(vertexBuffer, numVertices, vertexStride, vertexBufferData);
}

void NFGE::Graphics::PipelineWorker::CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
    size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
    CopyBuffer(indexBuffer, numIndicies, indexSizeInBytes, indexBufferData);
}

void NFGE::Graphics::PipelineWorker::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->IASetPrimitiveTopology(primitiveTopology);
}

void NFGE::Graphics::PipelineWorker::ClearTexture(const Texture& texture, const float clearColor[4])
{
    TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->ClearRenderTargetView(texture.GetRenderTargetView(), clearColor, 0, nullptr);

    TrackResource(texture);
}

void NFGE::Graphics::PipelineWorker::ClearDepthStencilTexture(const Texture& texture, D3D12_CLEAR_FLAGS clearFlags, float depth, uint8_t stencil)
{
    TransitionBarrier(texture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->ClearDepthStencilView(texture.GetDepthStencilView(), clearFlags, depth, stencil, 0, nullptr);

    TrackResource(texture);
}

void NFGE::Graphics::PipelineWorker::CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
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
        ComPtr<ID3D12Resource> intermediateResource;
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

        ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
        UpdateSubresources(mCurrentCommandList.Get(), destinationResource.Get(), intermediateResource.Get(), 0, firstSubresource, numSubresources, subresourceData);

        TrackObject(intermediateResource);
        TrackObject(destinationResource);
    }
}

void NFGE::Graphics::PipelineWorker::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = mUploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void NFGE::Graphics::PipelineWorker::SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void NFGE::Graphics::PipelineWorker::SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void NFGE::Graphics::PipelineWorker::SetVertexBuffer(uint32_t slot, const VertexBuffer& vertexBuffer)
{
    TransitionBarrier(vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    auto vertexBufferView = vertexBuffer.GetVertexBufferView();

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);

    TrackResource(vertexBuffer);
}

void NFGE::Graphics::PipelineWorker::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
    size_t bufferSize = numVertices * vertexSize;

    auto heapAllocation = mUploadBuffer->Allocate(bufferSize, vertexSize);
    memcpy(heapAllocation.CPU, vertexBufferData, bufferSize);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = heapAllocation.GPU;
    vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    vertexBufferView.StrideInBytes = static_cast<UINT>(vertexSize);

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void NFGE::Graphics::PipelineWorker::SetIndexBuffer(const IndexBuffer& indexBuffer)
{
    TransitionBarrier(indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    auto indexBufferView = indexBuffer.GetIndexBufferView();

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->IASetIndexBuffer(&indexBufferView);

    TrackResource(indexBuffer);
}

void NFGE::Graphics::PipelineWorker::SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
    size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
    size_t bufferSize = numIndicies * indexSizeInBytes;

    auto heapAllocation = mUploadBuffer->Allocate(bufferSize, indexSizeInBytes);
    memcpy(heapAllocation.CPU, indexBufferData, bufferSize);

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation = heapAllocation.GPU;
    indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    indexBufferView.Format = indexFormat;

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->IASetIndexBuffer(&indexBufferView);
}

void NFGE::Graphics::PipelineWorker::SetGraphicsDynamicStructuredBuffer(uint32_t slot, size_t numElements, size_t elementSize, const void* bufferData)
{
    size_t bufferSize = numElements * elementSize;

    auto heapAllocation = mUploadBuffer->Allocate(bufferSize, elementSize);

    memcpy(heapAllocation.CPU, bufferData, bufferSize);

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->SetGraphicsRootShaderResourceView(slot, heapAllocation.GPU);
}

void NFGE::Graphics::PipelineWorker::SetViewport(const D3D12_VIEWPORT& viewport)
{
    SetViewports({ viewport });
}

void NFGE::Graphics::PipelineWorker::SetViewports(const std::vector<D3D12_VIEWPORT>& viewports)
{
    ASSERT(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "Too many viewports");
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->RSSetViewports(static_cast<UINT>(viewports.size()),
        viewports.data());
}

void NFGE::Graphics::PipelineWorker::SetScissorRect(const D3D12_RECT& scissorRect)
{
    SetScissorRects({ scissorRect });
}

void NFGE::Graphics::PipelineWorker::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
    ASSERT(scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, "Too many scissorRects");
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->RSSetScissorRects(static_cast<UINT>(scissorRects.size()),
        scissorRects.data());
}

void NFGE::Graphics::PipelineWorker::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
{
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->SetPipelineState(pipelineState.Get());

    TrackObject(pipelineState);
}

void NFGE::Graphics::PipelineWorker::SetGraphicsRootSignature(const RootSignature& rootSignature)
{
    auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
    if (mCurrentRootSignature != d3d12RootSignature)
    {
        mCurrentRootSignature = d3d12RootSignature;

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            mDynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
        }

        mCurrentCommandList->SetGraphicsRootSignature(mCurrentRootSignature);

        TrackObject(mCurrentRootSignature);
    }
}

void NFGE::Graphics::PipelineWorker::SetComputeRootSignature(const RootSignature& rootSignature)
{
    auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
    if (mCurrentRootSignature != d3d12RootSignature)
    {
        mCurrentRootSignature = d3d12RootSignature;

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            mDynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
        }

        ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
        mCurrentCommandList->SetComputeRootSignature(mCurrentRootSignature);

        TrackObject(mCurrentRootSignature);
    }
}

void NFGE::Graphics::PipelineWorker::SetShaderResourceView(uint32_t rootParameterIndex, uint32_t descriptorOffset, const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubresource, UINT numSubresources, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
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

    TrackResource(resource);
}

void NFGE::Graphics::PipelineWorker::SetUnorderedAccessView(uint32_t rootParameterIndex, uint32_t descrptorOffset, const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubresource, UINT numSubresources, const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav)
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

    TrackResource(resource);
}

void NFGE::Graphics::PipelineWorker::SetRenderTarget(const RenderTarget& renderTarget)
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;
    renderTargetDescriptors.reserve(AttachmentPoint::NumAttachmentPoints);

    const auto& textures = renderTarget.GetTextures();

    // Bind color targets (max of 8 render targets can be bound to the rendering pipeline.
    for (int i = 0; i < RenderTarget::sMaxRendertarget; ++i)
    {
        auto& texture = textures[i];

        if (texture.IsValid())
        {
            TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
            renderTargetDescriptors.push_back(texture.GetRenderTargetView());

            TrackResource(texture);
        }
    }

    const auto& depthTexture = renderTarget.GetTexture(AttachmentPoint::DepthStencil);

    CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor(D3D12_DEFAULT);
    if (depthTexture.GetD3D12Resource())
    {
        TransitionBarrier(depthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        depthStencilDescriptor = depthTexture.GetDepthStencilView();

        TrackResource(depthTexture);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = depthStencilDescriptor.ptr != 0 ? &depthStencilDescriptor : nullptr;

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->OMSetRenderTargets(static_cast<UINT>(renderTargetDescriptors.size()),
        renderTargetDescriptors.data(), FALSE, pDSV);
}

void NFGE::Graphics::PipelineWorker::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
{
    if (mDescriptorHeaps[heapType] != heap)
    {
        mDescriptorHeaps[heapType] = heap;
        BindDescriptorHeaps();
    }
}

void NFGE::Graphics::PipelineWorker::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
    }

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void NFGE::Graphics::PipelineWorker::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
    }

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void NFGE::Graphics::PipelineWorker::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch(*this);
    }

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
}

uint64_t NFGE::Graphics::PipelineWorker::Signal()
{
    return mCommandQueue->Signal();
}

void NFGE::Graphics::PipelineWorker::FlushResourceBarriers()
{
    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mResourceStateTracker->FlushResourceBarriers(mCurrentCommandList);
}

void NFGE::Graphics::PipelineWorker::ReleaseTrackedObjects()
{
    mTrackedObjects.clear();
}

void NFGE::Graphics::PipelineWorker::Reset()
{
    mResourceStateTracker->Reset();
    mUploadBuffer->Reset();

    ReleaseTrackedObjects();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        mDynamicDescriptorHeap[i]->Reset();
        mDescriptorHeaps[i] = nullptr;
    }

    mCurrentRootSignature = nullptr;
}

bool NFGE::Graphics::PipelineWorker::Close(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> pendingBarrierCommandList)
{
    FlushResourceBarriers();

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->Close();

    // Flush pending resource barriers.
    uint32_t numPendingBarriers = mResourceStateTracker->FlushPendingResourceBarriers(pendingBarrierCommandList);
    // Commit the final resource state to the global state.
    mResourceStateTracker->CommitFinalResourceStates();

    return numPendingBarriers > 0;
}

void NFGE::Graphics::PipelineWorker::TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object)
{
    mTrackedObjects.push_back(object);
}

void NFGE::Graphics::PipelineWorker::TrackResource(const Resource& res)
{
    TrackResource(res.GetD3D12Resource().Get());
}

void NFGE::Graphics::PipelineWorker::TrackResource(ID3D12Resource* res)
{
    mTrackedObjects.push_back(res);
}

void NFGE::Graphics::PipelineWorker::CopyBuffer(Buffer& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
    auto device = NFGE::Graphics::GetDevice();

    size_t bufferSize = numElements * elementSize;

    ComPtr<ID3D12Resource> d3d12Resource;
    if (bufferSize == 0)
    {
        // This will result in a NULL resource (which may be desired to define a default null resource).
    }
    else
    {
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&d3d12Resource)));

        // Add the resource to the global resource state tracker.
        ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

        if (bufferData != nullptr)
        {
            // Create an upload resource to use as an intermediate buffer to copy the buffer resource 
            ComPtr<ID3D12Resource> uploadResource;
            auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
            ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&uploadResource)));

            D3D12_SUBRESOURCE_DATA subresourceData = {};
            subresourceData.pData = bufferData;
            subresourceData.RowPitch = bufferSize;
            subresourceData.SlicePitch = subresourceData.RowPitch;

            mResourceStateTracker->TransitionResource(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
            FlushResourceBarriers();

            ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
            UpdateSubresources(mCurrentCommandList.Get(), d3d12Resource.Get(),
                uploadResource.Get(), 0, 0, 1, &subresourceData);

            // Add references to resources so they stay in scope until the command list is reset.
            TrackObject(uploadResource);
        }
        TrackObject(d3d12Resource);
    }

    buffer.SetD3D12Resource(d3d12Resource);
    buffer.CreateViews(numElements, elementSize);
}

void NFGE::Graphics::PipelineWorker::BindDescriptorHeaps()
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

    ASSERT(mCurrentCommandList, "Current PipleineWorker is not Begin work.");
    mCurrentCommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}
