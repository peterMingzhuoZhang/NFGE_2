//====================================================================================================
// Filename:	VertexBuffer.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================
#pragma once
#include "Buffer.h"

namespace NFGE::Graphics 
{

    class VertexBuffer : public Buffer
    {
    public:
        VertexBuffer(const std::wstring& name = L"");
        virtual ~VertexBuffer();

        // Inherited from Buffer
        virtual void CreateViews(size_t numElements, size_t elementSize) override;

        /**
         * Get the vertex buffer view for binding to the Input Assembler stage.
         */
        D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
        {
            return mVertexBufferView;
        }

        size_t GetNumVertices() const
        {
            return mNumVertices;
        }

        size_t GetVertexStride() const
        {
            return mVertexStride;
        }

        /**
        * Get the SRV for a resource.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

        /**
        * Get the UAV for a (sub)resource.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

    protected:

    private:
        size_t mNumVertices;
        size_t mVertexStride;

        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    };
}
