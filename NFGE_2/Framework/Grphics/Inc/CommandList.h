//====================================================================================================
// Filename:	CommandList.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#pragma once

namespace NFGE::Graphics {

    class DynamicDescriptorHeap;
    class RenderTarget;
    class Resource;
    class ResourceStateTracker;
    class UploadBuffer;

    class CommandList 
    {
    public:
        CommandList(D3D12_COMMAND_LIST_TYPE type);
        virtual ~CommandList();

        D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return mD3d12CommandListType; }

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const { return mD3d12CommandList; }

        void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
        void UAVBarrier(ID3D12Resource* resource, bool flushBarriers = false);
        void AliasingBarrier(ID3D12Resource* beforeResource, ID3D12Resource* afterResource, bool flushBarriers = false);
        void FlushResourceBarriers();
        void CopyResource(ID3D12Resource* dstRes, ID3D12Resource* srcRes);

        void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);

        void SetShaderResourceView(
            uint32_t rootParameterIndex,
            uint32_t descriptorOffset,
            const Resource& resource,
            D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            UINT firstSubresource = 0,
            UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr
        );

        void SetUnorderedAccessView(
            uint32_t rootParameterIndex,
            uint32_t descrptorOffset,
            const Resource&,
            D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            UINT firstSubresource = 0,
            UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr
        );

        void SetRenderTarget(const RenderTarget& renderTarget);

        // Draw geometry.
        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0);
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);

        // Dispatch a compute shader.
        void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1);

        void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);
    private:
        D3D12_COMMAND_LIST_TYPE mD3d12CommandListType;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> mD3d12CommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mD3d12CommandAllocator;

        ID3D12RootSignature* mRootSignature;
        // Resource created in an upload heap. Useful for drawing of dynamic geometry
        // or for uploading constant buffer data that changes every draw call.
        std::unique_ptr<UploadBuffer> mUploadBuffer;
        std::unique_ptr<ResourceStateTracker> mResourceStateTracker;

        std::unique_ptr<DynamicDescriptorHeap> mDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        ID3D12DescriptorHeap* mDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        void TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object);
        void TrackResource(ID3D12Resource* res);
        // Binds the current descriptor heaps to the command list.
        void BindDescriptorHeaps();

        using TrackedObjects = std::vector < Microsoft::WRL::ComPtr<ID3D12Object> >;
        TrackedObjects mTrackedObjects;
    };
}
