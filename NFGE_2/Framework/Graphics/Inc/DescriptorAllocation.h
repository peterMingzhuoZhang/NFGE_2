//====================================================================================================
// Filename:	DescriptorAllocation.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#pragma once

namespace NFGE::Graphics
{
	class DescriptorAllocatorPage;

    class DescriptorAllocation
    {
    public:
        // Creates a NULL descriptor.
        DescriptorAllocation();

        DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numHandles, uint32_t descriptorSize, DescriptorAllocatorPage* page);

        // The destructor will automatically free the allocation.
        ~DescriptorAllocation();

        // Copies are not allowed.
        DescriptorAllocation(const DescriptorAllocation&) = delete;
        DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

        // Move is allowed.
        DescriptorAllocation(DescriptorAllocation&& allocation) noexcept;
        DescriptorAllocation& operator=(DescriptorAllocation&& other) noexcept;

        // Check if this a valid descriptor.
        bool IsNull() const;

        // Get a descriptor at a particular offset in the allocation.
        D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0) const;

        // Get the number of (consecutive) handles for this allocation.
        uint32_t GetNumHandles() const { return mNumHandles; }

        // Get the heap that this allocation came from.
        // (For internal use only).
        DescriptorAllocatorPage* GetDescriptorAllocatorPage() const { return mPage; }

    private:
        // Free the descriptor back to the heap it came from.
        void Free();

        // The base descriptor.
        D3D12_CPU_DESCRIPTOR_HANDLE mDescriptor;
        // The number of descriptors in this allocation.
        uint32_t mNumHandles;
        // The offset to the next descriptor.
        uint32_t mDescriptorSize;

        // A pointer back to the original page where this allocation came from.
        DescriptorAllocatorPage* mPage;
    };
}