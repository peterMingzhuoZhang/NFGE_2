//====================================================================================================
// Filename:	IndexBuffer.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================
#include "Precompiled.h"
#include "IndexBuffer.h"

using namespace NFGE::Graphics;

IndexBuffer::IndexBuffer(const std::wstring& name)
    : Buffer(name)
    , mNumIndicies(0)
    , mIndexFormat(DXGI_FORMAT_UNKNOWN)
    , mIndexBufferView({})
{}

IndexBuffer::~IndexBuffer()
{}

void IndexBuffer::CreateViews(size_t numElements, size_t elementSize)
{
    ASSERT(elementSize == 2 || elementSize == 4, "Indices must be 16, or 32-bit integers.");

    mNumIndicies = numElements;
    mIndexFormat = (elementSize == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    mIndexBufferView.BufferLocation = mD3d12Resource->GetGPUVirtualAddress();
    mIndexBufferView.SizeInBytes = static_cast<UINT>(numElements * elementSize);
    mIndexBufferView.Format = mIndexFormat;
}

D3D12_CPU_DESCRIPTOR_HANDLE IndexBuffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    ASSERT(false, "IndexBuffer::GetShaderResourceView should not be called.");
    return D3D12_CPU_DESCRIPTOR_HANDLE{};
}

D3D12_CPU_DESCRIPTOR_HANDLE IndexBuffer::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    ASSERT(false, "IndexBuffer::GetUnorderedAccessView should not be called.");
    return D3D12_CPU_DESCRIPTOR_HANDLE{};
}