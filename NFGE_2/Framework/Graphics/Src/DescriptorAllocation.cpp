//====================================================================================================
// Filename:	DescriptorAllocator.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#include "Precompiled.h"
#include "DescriptorAllocation.h"
#include "DescriptorAllocatorPage.h"
#include "D3DUtil.h"

using namespace NFGE::Graphics;

DescriptorAllocation::DescriptorAllocation()
	: mDescriptor{ 0 }
	, mNumHandles{ 0 }
	, mDescriptorSize{ 0 }
	, mPage{ nullptr }
{}

NFGE::Graphics::DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numHandles, uint32_t descriptorSize, DescriptorAllocatorPage* page)
	: mDescriptor{ descriptor }
	, mNumHandles( numHandles )
	, mDescriptorSize{ descriptorSize }
	, mPage{ page }
{}

NFGE::Graphics::DescriptorAllocation::~DescriptorAllocation()
{
	Free();
}

NFGE::Graphics::DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& other) noexcept
	: mDescriptor{ other.mDescriptor }
	, mNumHandles( other.mNumHandles)
	, mDescriptorSize{ other.mDescriptorSize }
	, mPage{ other.mPage }
{
	ASSERT(&other != this, "Can not move DescriptroAllocation to itself.");
	other.mDescriptor.ptr = 0;
	other.mNumHandles = 0;
	other.mDescriptorSize = 0;
}

DescriptorAllocation& NFGE::Graphics::DescriptorAllocation::operator=(DescriptorAllocation&& other) noexcept
{
	ASSERT(&other != this, "Can not move DescriptroAllocation to itself.");

	// Free this descriptor if it points to anything.
	Free();

	mDescriptor = other.mDescriptor;
	mNumHandles = other.mNumHandles;
	mDescriptorSize = other.mDescriptorSize;
	mPage = std::move(other.mPage);

	other.mDescriptor.ptr = 0;
	other.mNumHandles = 0;
	other.mDescriptorSize = 0;

	return *this;
}

bool NFGE::Graphics::DescriptorAllocation::IsNull() const
{
	return mDescriptor.ptr == 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE NFGE::Graphics::DescriptorAllocation::GetDescriptorHandle(uint32_t offset) const
{
	ASSERT(offset < mNumHandles, "DescriptorAllocation access out of range. Offset too big.");;
	return { mDescriptor.ptr + (mDescriptorSize * offset) };
}

void NFGE::Graphics::DescriptorAllocation::Free()
{
	if (!IsNull() && mPage)
	{
		mPage->Free(std::move(*this), NFGE::Graphics::GetFrameCount());

		mDescriptor.ptr = 0;
		mNumHandles = 0;
		mDescriptorSize = 0;
		mPage = nullptr;
	}
}


