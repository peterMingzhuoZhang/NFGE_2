//====================================================================================================
// Filename:	DescriptorAllocator.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/10
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#include "Precompiled.h"
#include "DescriptorAllocator.h"
#include "DescriptorAllocatorPage.h"

using namespace NFGE::Graphics;

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap)
	: mHeapType(type)
	, mNumDescriptorsPerHeap(numDescriptorsPerHeap)
{}

DescriptorAllocation NFGE::Graphics::DescriptorAllocator::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(mAllocationMutex);

    DescriptorAllocation allocation;

    for (auto iter = mAvailableHeaps.begin(); iter != mAvailableHeaps.end(); ++iter)
    {
        auto& allocatorPage = mHeapPool[*iter];

        allocation = allocatorPage->Allocate(numDescriptors);

        if (allocatorPage->NumFreeHandles() == 0)
        {
            iter = mAvailableHeaps.erase(iter);
        }

        // A valid allocation has been found.
        if (!allocation.IsNull())
        {
            break;
        }
    }

    // No available heap could satisfy the requested number of descriptors.
    if (allocation.IsNull())
    {
        mNumDescriptorsPerHeap = std::max(mNumDescriptorsPerHeap, numDescriptors);
        auto newPage = CreateAllocatorPage();

        allocation = newPage->Allocate(numDescriptors);
    }

    return allocation;
}

void NFGE::Graphics::DescriptorAllocator::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(mAllocationMutex);

    for (size_t i = 0; i < mHeapPool.size(); ++i)
    {
        auto& page = mHeapPool[i];

        page->ReleaseStaleDescriptors(frameNumber);

        if (page->NumFreeHandles() > 0)
        {
            mAvailableHeaps.insert(i);
        }
    }
}

DescriptorAllocatorPage* NFGE::Graphics::DescriptorAllocator::CreateAllocatorPage()
{
	mHeapPool.emplace_back(std::make_unique<DescriptorAllocatorPage>(mHeapType, mNumDescriptorsPerHeap));
	mAvailableHeaps.insert(mHeapPool.size() - 1);

	return mHeapPool.back().get();
}


