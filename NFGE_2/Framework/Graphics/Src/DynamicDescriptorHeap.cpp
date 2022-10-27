//====================================================================================================
// Filename:	DynamicDescriptorHeap.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#include "Precompiled.h"
#include "DynamicDescriptorHeap.h"
#include "D3DUtil.h"

#include "GraphicsSystem.h"
#include "RootSignature.h"
#include "PipelineWorker.h"

using namespace NFGE::Graphics;

NFGE::Graphics::DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptorsPerHeap)
	: mDescriptorHeapType(heapType)
	, mNumDescriptorsPerHeap(numDescriptorsPerHeap)
	, mDescriptorTableBitMask(0)
	, mStaleDescriptorTableBitMask(0)
	, mCurrentCPUDescriptorHandle(D3D12_DEFAULT)
	, mCurrentGPUDescriptorHandle(D3D12_DEFAULT)
	, mNumFreeHandles(0)
{
	mDescriptorHandleIncrementSize = NFGE::Graphics::GetDevice()->GetDescriptorHandleIncrementSize(heapType);

	// Allocate space for staging CPU visible descriptors.
	mDescriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(mNumDescriptorsPerHeap);
}

NFGE::Graphics::DynamicDescriptorHeap::~DynamicDescriptorHeap()
{
}

void NFGE::Graphics::DynamicDescriptorHeap::StageDescriptors(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor)
{
	// Cannot stage more than the maximum number of descriptors per heap.
	// Cannot stage more than MaxDescriptorTables root parameters.
	if (numDescriptors > mNumDescriptorsPerHeap || rootParameterIndex >= sMaxDescriptorTables)
	{
		throw std::bad_alloc();
	}

	DescriptorTableCache& descriptorTableCache = mDescriptorTableCache[rootParameterIndex];

	// Check that the number of descriptors to copy does not exceed the number
	// of descriptors expected in the descriptor table.
	if ((offset + numDescriptors) > descriptorTableCache.mNumDescriptors)
	{
		throw std::length_error("Number of descriptors exceeds the number of descriptors in the descriptor table.");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = (descriptorTableCache.mBaseDescriptor + offset);
	for (uint32_t i = 0; i < numDescriptors; ++i)
	{
		dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(srcDescriptor, i, mDescriptorHandleIncrementSize);
	}

	// Set the root parameter index bit to make sure the descriptor table 
	// at that index is bound to the command list.
	mStaleDescriptorTableBitMask |= (1 << rootParameterIndex);
}

void NFGE::Graphics::DynamicDescriptorHeap::CommitStagedDescriptors(PipelineWorker& worker, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
{
	// Compute the number of descriptors that need to be copied 
	uint32_t numDescriptorsToCommit = ComputeStaleDescriptorCount();

	if (numDescriptorsToCommit > 0)
	{
		auto device = NFGE::Graphics::GetDevice();
		auto d3d12GraphicsCommandList = worker.GetGraphicsCommandList().Get();
		ASSERT(d3d12GraphicsCommandList, "Empty CommandList, PipleineWorker is not Begin work.");

		if (!mCurrentDescriptorHeap || mNumFreeHandles < numDescriptorsToCommit)
		{
			mCurrentDescriptorHeap = RequestDescriptorHeap();
			mCurrentCPUDescriptorHandle = mCurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			mCurrentGPUDescriptorHandle = mCurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			mNumFreeHandles = mNumDescriptorsPerHeap;

			worker.SetDescriptorHeap(mDescriptorHeapType, mCurrentDescriptorHeap.Get());

			// When updating the descriptor heap on the command list, all descriptor
			// tables must be (re)recopied to the new descriptor heap (not just
			// the stale descriptor tables).
			mStaleDescriptorTableBitMask = mDescriptorTableBitMask;
		}
		DWORD rootIndex;
		// Scan from LSB to MSB for a bit set in staleDescriptorsBitMask
		while (_BitScanForward(&rootIndex, mStaleDescriptorTableBitMask))
		{
			UINT numSrcDescriptors = mDescriptorTableCache[rootIndex].mNumDescriptors;
			D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHandles = mDescriptorTableCache[rootIndex].mBaseDescriptor;
			D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[] =
			{
				mCurrentCPUDescriptorHandle
			};
			UINT pDestDescriptorRangeSizes[] =
			{
				numSrcDescriptors
			};
			// Copy the staged CPU visible descriptors to the GPU visible descriptor heap.
			device->CopyDescriptors(1, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
				numSrcDescriptors, pSrcDescriptorHandles, nullptr, mDescriptorHeapType);
			// Set the descriptors on the command list using the passed-in setter function.
			setFunc(d3d12GraphicsCommandList, rootIndex, mCurrentGPUDescriptorHandle);
			// Offset current CPU and GPU descriptor handles.
			mCurrentCPUDescriptorHandle.Offset(numSrcDescriptors, mDescriptorHandleIncrementSize);
			mCurrentGPUDescriptorHandle.Offset(numSrcDescriptors, mDescriptorHandleIncrementSize);
			mNumFreeHandles -= numSrcDescriptors;
			// Flip the stale bit so the descriptor table is not recopied again unless it is updated with a new descriptor.
			mStaleDescriptorTableBitMask ^= (1 << rootIndex);
		}
	}
}

void NFGE::Graphics::DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(PipelineWorker& worker)
{
	CommitStagedDescriptors(worker, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

void NFGE::Graphics::DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(PipelineWorker& worker)
{
	CommitStagedDescriptors(worker, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

D3D12_GPU_DESCRIPTOR_HANDLE NFGE::Graphics::DynamicDescriptorHeap::CopyDescriptor(PipelineWorker& worker, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor)
{
	if (!mCurrentDescriptorHeap || mNumFreeHandles < 1)
	{
		mCurrentDescriptorHeap = RequestDescriptorHeap();
		mCurrentCPUDescriptorHandle = mCurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		mCurrentGPUDescriptorHandle = mCurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		mNumFreeHandles = mNumDescriptorsPerHeap;

		worker.SetDescriptorHeap(mDescriptorHeapType, mCurrentDescriptorHeap.Get());

		// When updating the descriptor heap on the command list, all descriptor
		// tables must be (re)recopied to the new descriptor heap (not just
		// the stale descriptor tables).
		mStaleDescriptorTableBitMask = mDescriptorTableBitMask;
	}

	auto device = NFGE::Graphics::GetDevice();

	D3D12_GPU_DESCRIPTOR_HANDLE hGPU = mCurrentGPUDescriptorHandle;
	device->CopyDescriptorsSimple(1, mCurrentCPUDescriptorHandle, cpuDescriptor, mDescriptorHeapType);

	mCurrentCPUDescriptorHandle.Offset(1, mDescriptorHandleIncrementSize);
	mCurrentGPUDescriptorHandle.Offset(1, mDescriptorHandleIncrementSize);
	mNumFreeHandles -= 1;

	return hGPU;
}

void NFGE::Graphics::DynamicDescriptorHeap::ParseRootSignature(const RootSignature& rootSignature)
{
	// If the root signature changes, all descriptors must be (re)bound to the
	// command list.
	mStaleDescriptorTableBitMask = 0;

	const auto& rootSignatureDesc = rootSignature.GetRootSignatureDesc();

	// Get a bit mask that represents the root parameter indices that match the 
	// descriptor heap type for this dynamic descriptor heap.
	mDescriptorTableBitMask = rootSignature.GetDescriptorTableBitMask(mDescriptorHeapType);
	uint32_t descriptorTableBitMask = mDescriptorTableBitMask;

	uint32_t currentOffset = 0;
	DWORD rootIndex;
	while (_BitScanForward(&rootIndex, descriptorTableBitMask) && rootIndex < rootSignatureDesc.NumParameters)
	{
		uint32_t numDescriptors = rootSignature.GetNumDescriptors(rootIndex);
	
		DescriptorTableCache& descriptorTableCache = mDescriptorTableCache[rootIndex];
		descriptorTableCache.mNumDescriptors = numDescriptors;
		descriptorTableCache.mBaseDescriptor = mDescriptorHandleCache.get() + currentOffset;
	
		currentOffset += numDescriptors;
	
		// Flip the descriptor table bit so it's not scanned again for the current index.
		descriptorTableBitMask ^= (1 << rootIndex);
	}

	// Make sure the maximum number of descriptors per descriptor heap has not been exceeded.
	ASSERT(currentOffset <= mNumDescriptorsPerHeap, "The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap.");
}

void NFGE::Graphics::DynamicDescriptorHeap::Reset()
{
	if (!mCurrentDescriptorHeap)
	{
		return;
	}
	mAvailableDescriptorHeaps = mDescriptorHeapPool;
	mCurrentDescriptorHeap.Reset();
	mCurrentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	mCurrentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	mNumFreeHandles = 0;
	mDescriptorTableBitMask = 0;
	mStaleDescriptorTableBitMask = 0;

	// Reset the table cache
	for (int i = 0; i < sMaxDescriptorTables; ++i)
	{
		mDescriptorTableCache[i].Reset();
	}
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> NFGE::Graphics::DynamicDescriptorHeap::RequestDescriptorHeap()
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	if (!mAvailableDescriptorHeaps.empty())
	{
		descriptorHeap = mAvailableDescriptorHeaps.front();
		mAvailableDescriptorHeaps.pop();
	}
	else
	{
		descriptorHeap = CreateDescriptorHeap();
		mDescriptorHeapPool.push(descriptorHeap);
	}

	return descriptorHeap;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> NFGE::Graphics::DynamicDescriptorHeap::CreateDescriptorHeap()
{
	auto device = NFGE::Graphics::GetDevice();

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = mDescriptorHeapType;
	descriptorHeapDesc.NumDescriptors = mNumDescriptorsPerHeap;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

uint32_t NFGE::Graphics::DynamicDescriptorHeap::ComputeStaleDescriptorCount() const
{
	uint32_t numStaleDescriptors = 0;
	DWORD i;
	DWORD staleDescriptorsBitMask = mStaleDescriptorTableBitMask;

	while (_BitScanForward(&i, staleDescriptorsBitMask))
	{
		numStaleDescriptors += mDescriptorTableCache[i].mNumDescriptors;
		staleDescriptorsBitMask ^= (1 << i);
	}

	return numStaleDescriptors;
}