//====================================================================================================
// Filename:	CommandQueue.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-2/
//====================================================================================================

#pragma once

#include <queue>

namespace NFGE::Graphics {
    class PipelineWorker;
    class CommandQueue
    {
    public:
        CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
        virtual ~CommandQueue();

        // Get an available command list from the command queue.
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

        // Execute a command list.
        // Returns the fence value to wait for for this command list.
        uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, PipelineWorker& worker);

        uint64_t Signal();
        bool IsFenceComplete(uint64_t fenceValue);
        void WaitForFenceValue(uint64_t fenceValue);
        void Flush();

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;
    protected:

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);

    private:
        // Keep track of command allocators that are "in-flight"
        struct CommandAllocatorEntry
        {
            uint64_t fenceValue;
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
        };

        using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
        using CommandListQueue = std::queue< Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> >;

        D3D12_COMMAND_LIST_TYPE                     mCommandListType{};
        Microsoft::WRL::ComPtr<ID3D12Device2>       mD3d12Device{ nullptr };
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>  mD3d12CommandQueue{ nullptr };
        Microsoft::WRL::ComPtr<ID3D12Fence>         mD3d12Fence{ nullptr };
        HANDLE                                      mFenceEvent{ nullptr };
        uint64_t                                    mFenceValue{ 0 };

        CommandAllocatorQueue                       mCommandAllocatorQueue{};
        CommandListQueue                            mCommandListQueue{};
    };
}