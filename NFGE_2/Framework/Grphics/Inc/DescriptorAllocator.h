//====================================================================================================
// Filename:	DescriptorAllocator.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#include "DescriptorAllocation.h"

#pragma once

namespace NFGE::Graphics 
{
    class DescriptorAllocatorPage;

    class DescriptorAllocator
    {
    public:
        DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
        virtual ~DescriptorAllocator() {};


        // Allocate a number of contiguous descriptors from a CPU visible descriptor heap.
        // @param numDescriptors The number of contiguous descriptors to allocate.
        // Cannot be more than the number of descriptors per descriptor heap.
        DescriptorAllocation Allocate(uint32_t numDescriptors = 1);

        //When the frame has completed, the stale descriptors can be released.
        void ReleaseStaleDescriptors(uint64_t frameNumber);
    private:
        using DescriptorHeapPool = std::vector< std::unique_ptr<DescriptorAllocatorPage> >;

        // Create a new heap with a specific number of descriptors.
        DescriptorAllocatorPage* CreateAllocatorPage();

        D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;
        uint32_t mNumDescriptorsPerHeap;

        DescriptorHeapPool mHeapPool;
        // Indices of available heaps in the heap pool.
        std::set<size_t> mAvailableHeaps;

        std::mutex mAllocationMutex;
    };
}