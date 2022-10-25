//====================================================================================================
// Filename:	CommandQueue.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-2/
//====================================================================================================

#include "Precompiled.h"
#include "CommandQueue.h"
#include "PipelineWorker.h"
#include "ResourceStateTracker.h"

using namespace NFGE::Graphics;
using namespace Microsoft::WRL;

NFGE::Graphics::CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
	: mFenceValue(0)
	, mCommandListType(type)
	, mD3d12Device(device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(mD3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&mD3d12CommandQueue)));
	ThrowIfFailed(mD3d12Device->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mD3d12Fence)));

	mFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ASSERT(mFenceEvent, "Failed to create fence event handle.");
}

NFGE::Graphics::CommandQueue::~CommandQueue()
{
	mD3d12Device.Reset();
	mD3d12CommandQueue.Reset();
	mD3d12Fence.Reset();
	CloseHandle(mFenceEvent);
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> NFGE::Graphics::CommandQueue::GetCommandList()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList2> commandList;

	if (!mCommandAllocatorQueue.empty() && IsFenceComplete(mCommandAllocatorQueue.front().fenceValue))
	{
		commandAllocator = mCommandAllocatorQueue.front().commandAllocator;
		mCommandAllocatorQueue.pop();

		ThrowIfFailed(commandAllocator->Reset());
	}
	else
	{
		commandAllocator = CreateCommandAllocator();
	}

	if (!mCommandListQueue.empty())
	{
		commandList = mCommandListQueue.front();
		mCommandListQueue.pop();

		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	}
	else
	{
		commandList = CreateCommandList(commandAllocator);
	}

	// Associate the command allocator with the command list so that it can be
	// retrieved when the command list is executed.
	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	return commandList;
}

uint64_t NFGE::Graphics::CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, PipelineWorker& worker)
{
	// Dealing with pending barriers
	ResourceStateTracker::Lock();
	auto pendingBarriersCommandList = GetCommandList();
	ID3D12CommandAllocator* commandAllocator_pending;
	UINT dataSize_pending = sizeof(commandAllocator_pending);
	ThrowIfFailed(pendingBarriersCommandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize_pending, &commandAllocator_pending));
	bool hasPendingBarriers = worker.Close(pendingBarriersCommandList);
	pendingBarriersCommandList->Close();
	if (hasPendingBarriers)
	{
		ID3D12CommandList* const commandLists[] = { pendingBarriersCommandList.Get() };
		mD3d12CommandQueue->ExecuteCommandLists(1, commandLists);
		uint64_t fenceValue_pending = Signal();

		mCommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue_pending, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>(commandAllocator_pending) });
		mCommandListQueue.push(pendingBarriersCommandList);
	}
	commandAllocator_pending->Release();
	ResourceStateTracker::Unlock();

	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	ID3D12CommandList* const commandLists[] = {
		commandList.Get()
	};

	mD3d12CommandQueue->ExecuteCommandLists(1, commandLists);
	uint64_t fenceValue = Signal();

	mCommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>(commandAllocator) });
	mCommandListQueue.push(commandList);

	// The ownership of the command allocator has been transferred to the ComPtr
	// in the command allocator queue. It is safe to release the reference 
	// in this temporary COM pointer here.
	commandAllocator->Release();

	return fenceValue;
}

uint64_t NFGE::Graphics::CommandQueue::Signal()
{
	uint64_t fenceValue = ++mFenceValue;
	mD3d12CommandQueue->Signal(mD3d12Fence.Get(), fenceValue);
	return fenceValue;
}

bool NFGE::Graphics::CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
	return mD3d12Fence->GetCompletedValue() >= fenceValue;
}

void NFGE::Graphics::CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
	if (!IsFenceComplete(fenceValue))
	{
		mD3d12Fence->SetEventOnCompletion(fenceValue, mFenceEvent);
		::WaitForSingleObject(mFenceEvent, DWORD_MAX);
	}
}

void NFGE::Graphics::CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> NFGE::Graphics::CommandQueue::GetD3D12CommandQueue() const
{
	return mD3d12CommandQueue;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> NFGE::Graphics::CommandQueue::CreateCommandAllocator()
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(mD3d12Device->CreateCommandAllocator(mCommandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> NFGE::Graphics::CommandQueue::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
	ThrowIfFailed(mD3d12Device->CreateCommandList(0, mCommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;
}
