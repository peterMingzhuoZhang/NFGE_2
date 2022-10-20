//====================================================================================================
// Filename:	PipelineWork.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#pragma once

#include "Geometry.h"
#include "VertexType.h"

namespace NFGE::Graphics 
{
	class CommandQueue;
	class ResourceStateTracker;
	class Resoure;
    class RenderTarget;
    class VertexBuffer;
    class IndexBuffer;
    class Buffer;

	enum class WokerType
	{
		Direct = D3D12_COMMAND_LIST_TYPE_DIRECT,
		Compute = D3D12_COMMAND_LIST_TYPE_COMPUTE,
		Copy = D3D12_COMMAND_LIST_TYPE_COPY
	};

	class PipelineWorker 
	{
	public:
        PipelineWorker(WokerType type);

        void Initialize();
        void Terminate();

        void BeginWork();
        void EndWork();

        D3D12_COMMAND_LIST_TYPE GetCommandListType() const
        {
            return static_cast<D3D12_COMMAND_LIST_TYPE>(mType);
        }

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const
        {
            return mCurrentCommandList;
        }

        void TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
        void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);

        void UAVBarrier(const Resource& resource, bool flushBarriers = false);
        void UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, bool flushBarriers = false);


        void AliasingBarrier(const Resource& beforeResource, const Resource& afterResource, bool flushBarriers = false);
        void AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> beforeResource, Microsoft::WRL::ComPtr<ID3D12Resource> afterResource, bool flushBarriers = false);

        void CopyResource(Resource& dstRes, const Resource& srcRes);
        void CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes);

        void ResolveSubresource(Resource& dstRes, const Resource& srcRes, uint32_t dstSubresource = 0, uint32_t srcSubresource = 0);


        void CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData);
        template<typename T>
        void CopyVertexBuffer(VertexBuffer& vertexBuffer, const std::vector<T>& vertexBufferData)
        {
            CopyVertexBuffer(vertexBuffer, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
        }

        void CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData);
        template<typename T>
        void CopyIndexBuffer(IndexBuffer& indexBuffer, const std::vector<T>& indexBufferData)
        {
            assert(sizeof(T) == 2 || sizeof(T) == 4);

            DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            CopyIndexBuffer(indexBuffer, indexBufferData.size(), indexFormat, indexBufferData.data());
        }


        void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);

        void ClearTexture(const Texture& texture, const float clearColor[4]);


        void ClearDepthStencilTexture(const Texture& texture, D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f, uint8_t stencil = 0);


        void PanoToCubemap(Texture& cubemap, const Texture& pano);


        void CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);


        void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
        template<typename T>
        void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, const T& data)
        {
            SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
        }


        void SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
        template<typename T>
        void SetGraphics32BitConstants(uint32_t rootParameterIndex, const T& constants)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
            SetGraphics32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
        }


        void SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
        template<typename T>
        void SetCompute32BitConstants(uint32_t rootParameterIndex, const T& constants)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
            SetCompute32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
        }

        void SetVertexBuffer(uint32_t slot, const VertexBuffer& vertexBuffer);

        void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData);
        template<typename T>
        void SetDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData)
        {
            SetDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
        }

        void SetIndexBuffer(const IndexBuffer& indexBuffer);

        void SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData);
        template<typename T>
        void SetDynamicIndexBuffer(const std::vector<T>& indexBufferData)
        {
            static_assert(sizeof(T) == 2 || sizeof(T) == 4);

            DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            SetDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.data());
        }

        void SetGraphicsDynamicStructuredBuffer(uint32_t slot, size_t numElements, size_t elementSize, const void* bufferData);
        template<typename T>
        void SetGraphicsDynamicStructuredBuffer(uint32_t slot, const std::vector<T>& bufferData)
        {
            SetGraphicsDynamicStructuredBuffer(slot, bufferData.size(), sizeof(T), bufferData.data());
        }

        void SetViewport(const D3D12_VIEWPORT& viewport);
        void SetViewports(const std::vector<D3D12_VIEWPORT>& viewports);

        void SetScissorRect(const D3D12_RECT& scissorRect);
        void SetScissorRects(const std::vector<D3D12_RECT>& scissorRects);

        void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);

        void SetGraphicsRootSignature(const RootSignature& rootSignature);
        void SetComputeRootSignature(const RootSignature& rootSignature);

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
            const Resource& resource,
            D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            UINT firstSubresource = 0,
            UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr
        );

        void SetRenderTarget(const RenderTarget& renderTarget);

		void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

		// Draw geometry.
		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0);
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);
		// Dispatch a compute shader.
		void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1);

		void FlushResourceBarriers();
		void ReleaseTrackedObjects();
		void Reset();
		void Close();
	private:
		void TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object);
		void TrackResource(const Resource& res);
        void TrackResource(ID3D12Resource* res);

        // Copy the contents of a CPU buffer to a GPU buffer (possibly replacing the previous buffer contents).
        void CopyBuffer(Buffer& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

        // Binds the current descriptor heaps to the command list.
        void BindDescriptorHeaps();

		WokerType mType;
		std::unique_ptr<CommandQueue> mCommandQueue{ nullptr };
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> mCurrentCommandList{ nullptr };
        ID3D12RootSignature* mCurrentRootSignature{ nullptr };

		std::unique_ptr<DynamicDescriptorHeap> mDynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
		ID3D12DescriptorHeap* mDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        // Resource created in an upload heap. Useful for drawing of dynamic geometry
        // or for uploading constant buffer data that changes every draw call.
        std::unique_ptr<UploadBuffer> mUploadBuffer;

		std::unique_ptr<ResourceStateTracker> mResourceStateTracker;
		using TrackedObjects = std::vector < Microsoft::WRL::ComPtr<ID3D12Object> >;
		TrackedObjects mTrackedObjects;
	};
	
}
