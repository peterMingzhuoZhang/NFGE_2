//====================================================================================================
// Filename:	IndexBuffer.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#pragma once

#include "Buffer.h"
namespace NFGE::Graphics 
{

    class IndexBuffer : public Buffer
    {
    public:
        IndexBuffer(const std::wstring& name = L"");
        virtual ~IndexBuffer();

        // Inherited from Buffer
        virtual void CreateViews(size_t numElements, size_t elementSize) override;

        size_t GetNumIndicies() const
        {
            return mNumIndicies;
        }

        DXGI_FORMAT GetIndexFormat() const
        {
            return mIndexFormat;
        }

        /**
         * Get the index buffer view for biding to the Input Assembler stage.
         */
        D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
        {
            return mIndexBufferView;
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
        size_t mNumIndicies;
        DXGI_FORMAT mIndexFormat;

        D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    };
}
