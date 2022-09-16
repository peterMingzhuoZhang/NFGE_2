//====================================================================================================
// Filename:	DescriptorAllocator.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#include "Precompiled.h"
#include "DescriptorAllocatorPage.h"
#include "DescriptorAllocation.h"
#include "D3DUtil.h"

using namespace NFGE::Graphics;

DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
	: mHeapType(type)
	, mNumDescriptorsInHeap(numDescriptors)
{
    auto device = NFGE::Graphics::GetDevice();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = mHeapType;
    heapDesc.NumDescriptors = mNumDescriptorsInHeap;

    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mD3d12DescriptorHeap)));

    mBaseDescriptor = mD3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    mDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(mHeapType);
    mNumFreeHandles = mNumDescriptorsInHeap;

    // Initialize the free lists
    AddNewBlock(0, mNumFreeHandles);
}

bool NFGE::Graphics::DescriptorAllocatorPage::HasSpace(uint32_t numDescriptors) const
{
    return mFreeListBySize.lower_bound(numDescriptors) != mFreeListBySize.end();
}

DescriptorAllocation NFGE::Graphics::DescriptorAllocatorPage::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(mAllocationMutex);

    // There are less than the requested number of descriptors left in the heap.
    // Return a NULL descriptor and try another heap.
    if (numDescriptors > mNumFreeHandles)
    {
        return DescriptorAllocation();
    }

    // Get the first block that is large enough to satisfy the request.
    auto smallestBlockIt = mFreeListBySize.lower_bound(numDescriptors);
    if (smallestBlockIt == mFreeListBySize.end())
    {
        // There was no free block that could satisfy the request.
        return DescriptorAllocation();
    }

    // The size of the smallest block that satisfies the request.
    auto blockSize = smallestBlockIt->first;

    // The pointer to the same entry in the FreeListByOffset map.
    auto offsetIt = smallestBlockIt->second;

    // The offset in the descriptor heap.
    auto offset = offsetIt->first;
    
    // Remove the existing free block from the free list.
    mFreeListBySize.erase(smallestBlockIt);
    mFreeListByOffset.erase(offsetIt);

    // Compute the new free block that results from splitting this block.
    auto newOffset = offset + numDescriptors;
    auto newSize = blockSize - numDescriptors;

    if (newSize > 0)
    {
        // If the allocation didn't exactly match the requested size,
        // return the left-over to the free list.
        AddNewBlock(newOffset, newSize);
    }

    // Decrement free handles.
    mNumFreeHandles -= numDescriptors;

    return DescriptorAllocation(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(mBaseDescriptor, offset, mDescriptorHandleIncrementSize),
        numDescriptors, mDescriptorHandleIncrementSize, nullptr);
}

void NFGE::Graphics::DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptor, uint64_t frameNumber)
{
    // Compute the offset of the descriptor within the descriptor heap.
    auto offset = ComputeOffset(descriptor.GetDescriptorHandle());

    std::lock_guard<std::mutex> lock(mAllocationMutex);

    // Don't add the block directly to the free list until the frame has completed.
    mStaleDescriptors.emplace(offset, descriptor.GetNumHandles(), frameNumber);
}

void NFGE::Graphics::DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(mAllocationMutex);

    while (!mStaleDescriptors.empty() && mStaleDescriptors.front().FrameNumber <= frameNumber)
    {
        auto& staleDescriptor = mStaleDescriptors.front();

        // The offset of the descriptor in the heap.
        auto offset = staleDescriptor.Offset;
        // The number of descriptors that were allocated.
        auto numDescriptors = staleDescriptor.Size;

        FreeBlock(offset, numDescriptors);

        mStaleDescriptors.pop();
    }
}

uint32_t NFGE::Graphics::DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    return static_cast<uint32_t>(handle.ptr - mBaseDescriptor.ptr) / mDescriptorHandleIncrementSize;
}

void NFGE::Graphics::DescriptorAllocatorPage::AddNewBlock(uint32_t offset, uint32_t numDescriptors)
{
    auto offsetIt = mFreeListByOffset.emplace(offset, numDescriptors);
    auto sizeIt = mFreeListBySize.emplace(numDescriptors, offsetIt.first);
    offsetIt.first->second.FreeListBySizeIt = sizeIt;
}

void NFGE::Graphics::DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
    // Find the first element whose offset is greater than the specified offset.
    // This is the block that should appear after the block that is being freed.
    auto nextBlockIt = mFreeListByOffset.upper_bound(offset);

    // Find the block that appears before the block being freed.
    auto prevBlockIt = nextBlockIt;
    // If it's not the first block in the list.
    if (prevBlockIt != mFreeListByOffset.begin())
    {
        // Go to the previous block in the list.
        --prevBlockIt;
    }
    else
    {
        // Otherwise, just set it to the end of the list to indicate that no
        // block comes before the one being freed.
        prevBlockIt = mFreeListByOffset.end();
    }

    // Add the number of free handles back to the heap.
    // This needs to be done before merging any blocks since merging
    // blocks modifies the numDescriptors variable.
    mNumFreeHandles += numDescriptors;

    if (prevBlockIt != mFreeListByOffset.end() &&
        offset == prevBlockIt->first + prevBlockIt->second.Size)
    {
        // The previous block is exactly behind the block that is to be freed.
        //
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //

        // Increase the block size by the size of merging with the previous block.
        offset = prevBlockIt->first;
        numDescriptors += prevBlockIt->second.Size;

        // Remove the previous block from the free list.
        mFreeListBySize.erase(prevBlockIt->second.FreeListBySizeIt);
        mFreeListByOffset.erase(prevBlockIt);
    }

    if (nextBlockIt != mFreeListByOffset.end() && offset + numDescriptors == nextBlockIt->first)
    {
        // The next block is exactly in front of the block that is to be freed.
        //
        // Offset               NextBlock.Offset 
        // |                    |
        // |<------Size-------->|<-----NextBlock.Size----->|

        // Increase the block size by the size of merging with the next block.
        numDescriptors += nextBlockIt->second.Size;

        // Remove the next block from the free list.
        mFreeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
        mFreeListByOffset.erase(nextBlockIt);
    }

    // Add the freed block to the free list.
    AddNewBlock(offset, numDescriptors);
}
