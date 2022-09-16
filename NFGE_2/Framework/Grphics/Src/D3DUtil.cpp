#include "Precompiled.h"
#include "D3DUtil.h"

#include "GraphicsSystem.h"
#include "d3dx12.h"

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
