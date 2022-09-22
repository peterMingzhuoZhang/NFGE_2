//====================================================================================================
// Filename:	Buffer.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#include "Precompiled.h"
#include "Buffer.h"

using namespace NFGE::Graphics;

Buffer::Buffer(const std::wstring& name)
    : Resource(name)
{}

Buffer::Buffer(const D3D12_RESOURCE_DESC& resDesc,
    size_t numElements, size_t elementSize,
    const std::wstring& name)
    : Resource(resDesc, nullptr, name)
{}
