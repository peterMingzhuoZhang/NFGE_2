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
    class Texture;
    enum class TextureUsage;

    class CommandList 
    {
    public:
        CommandList(D3D12_COMMAND_LIST_TYPE type);
        virtual ~CommandList();

        D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return mD3d12CommandListType; }

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const { return mD3d12CommandList; }

        void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
        void TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);

        void UAVBarrier(ID3D12Resource* resource, bool flushBarriers = false);
        void UAVBarrier(const Resource& resource, bool flushBarriers = false);

        void AliasingBarrier(ID3D12Resource* beforeResource, ID3D12Resource* afterResource, bool flushBarriers = false);
        void AliasingBarrier(const Resource& beforeResource, const Resource& afterResource, bool flushBarriers = false);

        void FlushResourceBarriers();

        void CopyResource(ID3D12Resource* dstRes, ID3D12Resource* srcRes);
        void CopyResource(Resource& dstRes, const Resource& srcRes);

        void LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage);
        void ClearTexture(const Texture& texture, const float clearColor[4]);
        void ClearDepthStencilTexture(const Texture& texture, D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f, uint8_t stencil = 0);
        void GenerateMips(Texture& texture);
        // Generate a cubemap texture from a panoramic (equirectangular) texture.
        //void PanoToCubemap(Texture& cubemap, const Texture& pano);
        //Copy subresource data to a texture.
        void CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);

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
        // Keep track of loaded textures to avoid loading the same texture multiple times.
        static std::map<std::wstring, ID3D12Resource* > sTextureCache;
        static std::mutex sTextureCacheMutex;
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

        CommandList* mComputeCommandList;

        void TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object);
        void TrackResource(ID3D12Resource* res);
        void TrackResource(const Resource& res);
        // Binds the current descriptor heaps to the command list.
        void BindDescriptorHeaps();

        using TrackedObjects = std::vector < Microsoft::WRL::ComPtr<ID3D12Object> >;
        TrackedObjects mTrackedObjects;
    };
}

/*
 *  Copyright(c) 2018 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

 /**
  *  @file CommandList.h
  *  @date October 22, 2018
  *  @author Jeremiah van Oosten
  *
  *  @brief CommandList class encapsulates a ID3D12GraphicsCommandList2 interface.
  *  The CommandList class provides additional functionality that makes working with
  *  DirectX 12 applications easier.
  */