#include "Precompiled.h"
#include "D3DUtil.h"

#include "GraphicsSystem.h"
#include "d3dx12.h"
#include "DescriptorAllocation.h"
#include "DescriptorAllocator.h"

//#include "Texture.h"

Microsoft::WRL::ComPtr<ID3D12Device2> NFGE::Graphics::GetDevice()
{
	return NFGE::Graphics::GraphicsSystem::Get()->mDevice;
}

NFGE::Graphics::CommandQueue* NFGE::Graphics::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
    CommandQueue* commandQueue = nullptr;
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        commandQueue = NFGE::Graphics::GraphicsSystem::Get()->mDirectCommandQueue.get();
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        commandQueue = NFGE::Graphics::GraphicsSystem::Get()->mComputeCommandQueue.get();
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        commandQueue = NFGE::Graphics::GraphicsSystem::Get()->mCopyCommandQueue.get();
        break;
    default:
        ASSERT(false, "Invalid command queue type.");
    }

    ASSERT(commandQueue, "CommandQueue should not be nullptr.");
    return commandQueue;
}

uint8_t  NFGE::Graphics::GetFrameCount()
{
    return NFGE::Graphics::GraphicsSystem::Get()->sNumFrames;
}

void NFGE::Graphics::Flush()
{
    NFGE::Graphics::GraphicsSystem::Get()->Flush();
}

NFGE::Graphics::DescriptorAllocation NFGE::Graphics::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    return NFGE::Graphics::GraphicsSystem::Get()->mDescriptorAllocators[type]->Allocate(numDescriptors);
}

void NFGE::Graphics::ReleaseStaleDescriptors(uint64_t finishedFrame)
{
    auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        graphicSystem->mDescriptorAllocators[i]->ReleaseStaleDescriptors(finishedFrame);
    }
}

UINT NFGE::Graphics::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return NFGE::Graphics::GraphicsSystem::Get()->mDevice->GetDescriptorHandleIncrementSize(type);
}
