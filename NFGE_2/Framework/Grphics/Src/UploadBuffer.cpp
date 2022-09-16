//====================================================================================================
// Filename:	UploadBuffer.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#include "Precompiled.h"
#include "UploadBuffer.h"
#include "D3DUtil.h"
#include "d3dx12.h"

using namespace NFGE::Graphics;

NFGE::Graphics::UploadBuffer::UploadBuffer(size_t pageSize)
	: mPageSize(pageSize)
{}

UploadBuffer::Allocation NFGE::Graphics::UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (sizeInBytes > mPageSize)
    {
        throw std::bad_alloc();
    }

    // If there is no current page, or the requested allocation exceeds the
    // remaining space in the current page, request a new page.
    if (!mCurrentPage || !mCurrentPage->HasSpace(sizeInBytes, alignment))
    {
        mCurrentPage = RequestPage();
    }

    return mCurrentPage->Allocate(sizeInBytes, alignment);
}

void NFGE::Graphics::UploadBuffer::Reset()
{
    mCurrentPage = nullptr;
    // Reset all available pages.
    size_t totalPage = mPagePool.size();
    mAvailablePages.resize(totalPage, nullptr);
    
    for (size_t i = 0; i < totalPage; ++i)
    {
        mAvailablePages[i] = mPagePool[i].get();
        // Reset the page for new allocations.
        mAvailablePages[i]->Reset();
    }
}

UploadBuffer::Page* NFGE::Graphics::UploadBuffer::RequestPage()
{
    Page* page;

    if (!mAvailablePages.empty())
    {
        page = mAvailablePages.front();
        mAvailablePages.pop_front();
    }
    else
    {
        mPagePool.emplace_back(std::make_unique<Page>(mPageSize));
        page = mPagePool.back().get();
    }

    return page;
}

NFGE::Graphics::UploadBuffer::Page::Page(size_t sizeInBytes)
	: mPageSize(sizeInBytes)
	, mOffset(0)
	, mCPUPtr(nullptr)
	, mGPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
    auto device = NFGE::Graphics::GetDevice();

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mPageSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mD3d12Resource)
    ));

    mGPUPtr = mD3d12Resource->GetGPUVirtualAddress();
    mD3d12Resource->Map(0, nullptr, &mCPUPtr);
}

NFGE::Graphics::UploadBuffer::Page::~Page()
{
    mD3d12Resource->Unmap(0, nullptr);
    mCPUPtr = nullptr;
    mGPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool NFGE::Graphics::UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    size_t alignedSize = Math::Memory::AlignUp(sizeInBytes, alignment);
    size_t alignedOffset = Math::Memory::AlignUp(mOffset, alignment);

    return alignedOffset + alignedSize <= mPageSize;
}

UploadBuffer::Allocation NFGE::Graphics::UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (!HasSpace(sizeInBytes, alignment))
    {
        // Can't allocate space from page.
        throw std::bad_alloc();
    }

    size_t alignedSize = Math::Memory::AlignUp(sizeInBytes, alignment);
    mOffset = Math::Memory::AlignUp(mOffset, alignment);

    Allocation allocation;
    allocation.CPU = static_cast<uint8_t*>(mCPUPtr) + mOffset;
    allocation.GPU = mGPUPtr + mOffset;

    mOffset += alignedSize;

    return allocation;
}

void NFGE::Graphics::UploadBuffer::Page::Reset()
{
    mOffset = 0;
}
