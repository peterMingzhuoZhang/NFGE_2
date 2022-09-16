//====================================================================================================
// Filename:	UploadBuffer.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
// Description: Class for D3d12 GPU allocation reuse for each frame.
//====================================================================================================

#pragma once

#define _KB(x) (x * 1024)
#define _MB(x) (x * 1024 * 1024)

#define _64KB _KB(64)
#define _1MB _MB(1)
#define _2MB _MB(2)
#define _4MB _MB(4)
#define _8MB _MB(8)
#define _16MB _MB(16)
#define _32MB _MB(32)
#define _64MB _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)

namespace NFGE::Graphics 
{
    class UploadBuffer
    {
    public:
        // Use to upload data to the GPU
        struct Allocation
        {
            void* CPU;
            D3D12_GPU_VIRTUAL_ADDRESS GPU;
        };

        // @param pageSize The size to use to allocate new pages in GPU memory.
        explicit UploadBuffer(size_t pageSize = _2MB);
        
        // The maximum size of an allocation is the size of a single page.
        size_t GetPageSize() const { return mPageSize; }

        
        // Allocate memory in an Upload heap.
        // An allocation must not exceed the size of a page.
        // Use a memcpy or similar method to copy the
        // buffer data to CPU pointer in the Allocation structure returned from
        // this function.
        Allocation Allocate(size_t sizeInBytes, size_t alignment);

        
        //Release all allocated pages. This should only be done when the command list
        //is finished executing on the CommandQueue.
        void Reset();

    private:
        // A single page for the allocator.
        struct Page
        {
            Page(size_t sizeInBytes);
            ~Page();
            
            // Check to see if the page has room to satisfy the requested allocation.
            bool HasSpace(size_t sizeInBytes, size_t alignment) const;

            // Allocate memory from the page.
            // Throws std::bad_alloc if the the allocation size is larger
            // that the page size or the size of the allocation exceeds the 
            // remaining space in the page.
            Allocation Allocate(size_t sizeInBytes, size_t alignment);
          
            // Reset the page for reuse.
            void Reset();
        private:

            Microsoft::WRL::ComPtr<ID3D12Resource> mD3d12Resource;

            // Base pointer.
            void* mCPUPtr;
            D3D12_GPU_VIRTUAL_ADDRESS mGPUPtr;

            // Allocated page size.
            size_t mPageSize;
            // Current allocation offset in bytes.
            size_t mOffset;
        };

        // A pool of memory pages.
        using PagePool = std::deque< std::unique_ptr<Page> >;
        using PagePoolRef = std::deque<Page*>;

        Page* RequestPage();

        PagePool mPagePool{};
        PagePoolRef mAvailablePages{};

        Page* mCurrentPage{ nullptr };

        // The size of each page of memory.
        size_t mPageSize{ 0 };
    };
}