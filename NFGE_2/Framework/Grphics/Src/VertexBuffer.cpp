//====================================================================================================
// Filename:	VertexBuffer.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#include "Precompiled.h"
#include "VertexBuffer.h"

using namespace NFGE::Graphics;

VertexBuffer::VertexBuffer(const std::wstring& name)
    : Buffer(name)
    , mNumVertices(0)
    , mVertexStride(0)
    , mVertexBufferView({})
{}

VertexBuffer::~VertexBuffer()
{}

void VertexBuffer::CreateViews(size_t numElements, size_t elementSize)
{
    mNumVertices = numElements;
    mVertexStride = elementSize;

    mVertexBufferView.BufferLocation = mD3d12Resource->GetGPUVirtualAddress();
    mVertexBufferView.SizeInBytes = static_cast<UINT>(mNumVertices * mVertexStride);
    mVertexBufferView.StrideInBytes = static_cast<UINT>(mVertexStride);
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    ASSERT(false, "VertexBuffer::GetShaderResourceView should not be called.");
    return D3D12_CPU_DESCRIPTOR_HANDLE{};
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    ASSERT(false, "VertexBuffer::GetUnorderedAccessView should not be called.");
    return D3D12_CPU_DESCRIPTOR_HANDLE{};
}