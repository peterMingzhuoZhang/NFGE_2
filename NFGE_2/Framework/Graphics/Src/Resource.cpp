//====================================================================================================
// Filename:	Resource.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#include "Precompiled.h"
#include "Resource.h"

#include "D3DUtil.h"
#include "d3dx12.h"
#include "ResourceStateTracker.h"

using namespace NFGE::Graphics;

Resource::Resource(const std::wstring& name)
    : mResourceName(name)
{}

Resource::Resource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue, const std::wstring& name)
{
    auto device = NFGE::Graphics::GetDevice();

    if (clearValue)
    {
        mD3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }

    auto propertiesHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &propertiesHeap,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        mD3d12ClearValue.get(),
        IID_PPV_ARGS(&mD3d12Resource)
    ));

    ResourceStateTracker::AddGlobalResourceState(mD3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

    CheckFeatureSupport();
    SetName(name);
}

Resource::Resource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, const std::wstring& name)
    : mD3d12Resource(resource)
{
    CheckFeatureSupport();
    SetName(name);
}

Resource::Resource(const Resource& copy)
    : mD3d12Resource(copy.mD3d12Resource)
    , mResourceName(copy.mResourceName)
    , mD3d12ClearValue(std::make_unique<D3D12_CLEAR_VALUE>(*copy.mD3d12ClearValue))
{
    int i = 3;
}

Resource::Resource(Resource&& copy)
    : mD3d12Resource(std::move(copy.mD3d12Resource))
    , mResourceName(std::move(copy.mResourceName))
    , mD3d12ClearValue(std::move(copy.mD3d12ClearValue))
{
}

Resource& Resource::operator=(const Resource& other)
{
    if (this != &other)
    {
        mD3d12Resource = other.mD3d12Resource;
        mResourceName = other.mResourceName;
        if (other.mD3d12ClearValue)
        {
            mD3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*other.mD3d12ClearValue);
        }
    }

    return *this;
}

Resource& Resource::operator=(Resource&& other)
{
    if (this != &other)
    {
        mD3d12Resource = other.mD3d12Resource;
        mResourceName = other.mResourceName;
        mD3d12ClearValue = std::move(other.mD3d12ClearValue);

        other.mD3d12Resource.Reset();
        other.mResourceName.clear();
    }

    return *this;
}


Resource::~Resource()
{
}

void Resource::SetD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource, const D3D12_CLEAR_VALUE* clearValue)
{
    mD3d12Resource = d3d12Resource;
    if (mD3d12ClearValue)
    {
        mD3d12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }
    else
    {
        mD3d12ClearValue.reset();
    }
    CheckFeatureSupport();
    SetName(mResourceName);
}

void Resource::SetName(const std::wstring& name)
{
    mResourceName = name;
    if (mD3d12Resource && !mResourceName.empty())
    {
        mD3d12Resource->SetName(mResourceName.c_str());
    }
}

void Resource::Reset()
{
    mD3d12Resource.Reset();
    mD3d12ClearValue.reset();
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const
{
    return (mFormatSupport.Support1 & formatSupport) != 0;
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const
{
    return (mFormatSupport.Support2 & formatSupport) != 0;
}

void Resource::CheckFeatureSupport()
{
    if (mD3d12Resource)
    {
        auto desc = mD3d12Resource->GetDesc();
        auto device = NFGE::Graphics::GetDevice();

        mFormatSupport.Format = desc.Format;
        ThrowIfFailed(device->CheckFeatureSupport(
            D3D12_FEATURE_FORMAT_SUPPORT,
            &mFormatSupport,
            sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
    }
    else
    {
        mFormatSupport = {};
    }
}
