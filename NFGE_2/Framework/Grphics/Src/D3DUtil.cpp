#include "Precompiled.h"
#include "D3DUtil.h"

#include "GraphicsSystem.h"
#include "d3dx12.h"
#include "DescriptorAllocation.h"
#include "DescriptorAllocator.h"
#include "VertexType.h"

//#include "Texture.h"

Microsoft::WRL::ComPtr<ID3D12Device2> NFGE::Graphics::GetDevice()
{
	return NFGE::Graphics::GraphicsSystem::Get()->mDevice;
}

NFGE::Graphics::CommandQueue* NFGE::Graphics::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
    CommandQueue* commandQueue = nullptr;
    auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        commandQueue = graphicSystem->mDirectCommandQueue.get();
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
    graphicSystem->mCurrentCommandList = commandQueue->GetCommandList();
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

void NFGE::Graphics::TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object)
{
    NFGE::Graphics::GraphicsSystem::Get()->TrackObject(object);
}

std::vector<D3D12_INPUT_ELEMENT_DESC> NFGE::Graphics::GetVectexLayout(uint32_t vertexFormat)
{
    std::vector<D3D12_INPUT_ELEMENT_DESC> desc;
    if (vertexFormat & VE_Position)
        desc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    if (vertexFormat & VE_Normal)
        desc.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    if (vertexFormat & VE_Tangent)
        desc.push_back({ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    if (vertexFormat & VE_Color)
        desc.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    if (vertexFormat & VE_Texcoord)
        desc.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    if (vertexFormat & VE_BlendIndex)
        desc.push_back({ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    if (vertexFormat & VE_BlendWeight)
        desc.push_back({ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    return desc;
}
