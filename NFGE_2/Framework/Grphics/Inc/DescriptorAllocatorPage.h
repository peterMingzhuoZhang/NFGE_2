//====================================================================================================
// Filename:	DescriptorAllocatorPage.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#pragma once
#include "DescriptorAllocation.h"
#include "d3dx12.h"

namespace NFGE::Graphics
{
    class DescriptorAllocatorPage
    {
    public:
        DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

        D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return mHeapType; }

        // Check to see if this descriptor page has a contiguous block of descriptors
        // large enough to satisfy the request.
        bool HasSpace(uint32_t numDescriptors) const;

        // Get the number of available handles in the heap.
        uint32_t NumFreeHandles() const { return mNumFreeHandles; }
        
        // Allocate a number of descriptors from this descriptor heap.
        // If the allocation cannot be satisfied, then a NULL descriptor is returned.
        DescriptorAllocation Allocate(uint32_t numDescriptors);

        // Return a descriptor back to the heap.
        // @param frameNumber Stale descriptors are not freed directly, but put
        // on a stale allocations queue. Stale allocations are returned to the heap
        // using the DescriptorAllocatorPage::ReleaseStaleAllocations method.
        void Free(DescriptorAllocation&& descriptorHandle, uint64_t frameNumber);

        // Returned the stale descriptors back to the descriptor heap.
        void ReleaseStaleDescriptors(uint64_t frameNumber);
    protected:

        // Compute the offset of the descriptor handle from the start of the heap.
        uint32_t ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);

        // Adds a new block to the free list.
        void AddNewBlock(uint32_t offset, uint32_t numDescriptors);

        // Free a block of descriptors.
        // This will also merge free blocks in the free list to form larger blocks
        // that can be reused.
        void FreeBlock(uint32_t offset, uint32_t numDescriptors);
    private:
        // The offset (in descriptors) within the descriptor heap.
        using OffsetType = uint32_t;
        // The number of descriptors that are available.
        using SizeType = uint32_t;
        struct FreeBlockInfo;
        // A map that lists the free blocks by the offset within the descriptor heap.
        using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;

        // A map that lists the free blocks by size.
        // Needs to be a multimap since multiple blocks can have the same size.
        using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

        struct FreeBlockInfo
        {
            FreeBlockInfo(SizeType size)
                : Size(size)
            {}

            SizeType Size;
            FreeListBySize::iterator FreeListBySizeIt;
        };

        struct StaleDescriptorInfo
        {
            StaleDescriptorInfo(OffsetType offset, SizeType size, uint64_t frame)
                : Offset(offset)
                , Size(size)
                , FrameNumber(frame)
            {}

            // The offset within the descriptor heap.
            OffsetType Offset;
            // The number of descriptors
            SizeType Size;
            // The frame number that the descriptor was freed.
            uint64_t FrameNumber;
        };
        
        // Stale descriptors are queued for release until the frame that they were freed
        // has completed.
        using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

        FreeListByOffset mFreeListByOffset;
        FreeListBySize mFreeListBySize;
        StaleDescriptorQueue mStaleDescriptors;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mD3d12DescriptorHeap;
        D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;
        CD3DX12_CPU_DESCRIPTOR_HANDLE mBaseDescriptor;
        uint32_t mDescriptorHandleIncrementSize;
        uint32_t mNumDescriptorsInHeap;
        uint32_t mNumFreeHandles;

        std::mutex mAllocationMutex;
    };
}