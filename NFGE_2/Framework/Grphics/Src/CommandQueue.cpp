//====================================================================================================
// Filename:	CommandQueue.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-2/
//====================================================================================================

#include "Precompiled.h"
#include "CommandQueue.h"

using namespace NFGE::Graphic;
using namespace Microsoft::WRL;

NFGE::Graphic::CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
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

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> NFGE::Graphic::CommandQueue::GetCommandList()
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

uint64_t NFGE::Graphic::CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	commandList->Close();

	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	ID3D12CommandList* const ppCommandLists[] = {
		commandList.Get()
	};

	mD3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	mCommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
	mCommandListQueue.push(commandList);

	// The ownership of the command allocator has been transferred to the ComPtr
	// in the command allocator queue. It is safe to release the reference 
	// in this temporary COM pointer here.
	commandAllocator->Release();

	return fenceValue;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> NFGE::Graphic::CommandQueue::CreateCommandAllocator()
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(mD3d12Device->CreateCommandAllocator(mCommandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> NFGE::Graphic::CommandQueue::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
	ThrowIfFailed(mD3d12Device->CreateCommandList(0, mCommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;
}
