#include "Precompiled.h"
#include "D3DUtil.h"

#include "GraphicsSystem.h"
#include "d3dx12.h"
#include "DescriptorAllocation.h"
#include "DescriptorAllocator.h"
#include "VertexType.h"
#include "PipelineComponent.h"
#include "PipelineWorker.h"

//#include "Texture.h"

Microsoft::WRL::ComPtr<ID3D12Device2> NFGE::Graphics::GetDevice()
{
	return NFGE::Graphics::GraphicsSystem::Get()->mDevice;
}

void NFGE::Graphics::RegisterPipelineComponent(NFGE::Graphics::WorkerType type, PipelineComponent* component)
{
    auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
    switch (type)
    {
    case NFGE::Graphics::WorkerType::Direct:
        graphicSystem->mDirectWorker->RegisterComponent(component);
        break;
    case NFGE::Graphics::WorkerType::Compute:
        graphicSystem->mComputeWorker->RegisterComponent(component);
        break;
    case NFGE::Graphics::WorkerType::Copy:
        graphicSystem->mCopyWorker->RegisterComponent(component);
        break;
    default:
        ASSERT(false, "Invalid command worker type.");
        break;
    }
}

NFGE::Graphics::PipelineWorker* NFGE::Graphics::GetWorker(NFGE::Graphics::WorkerType type)
{
    PipelineWorker* worker = nullptr;
    auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
    switch (type)
    {
    case NFGE::Graphics::WorkerType::Direct:
        worker = graphicSystem->mDirectWorker.get();
        break;
    case NFGE::Graphics::WorkerType::Compute:
        worker = graphicSystem->mComputeWorker.get();
        break;
    case NFGE::Graphics::WorkerType::Copy:
        worker = graphicSystem->mCopyWorker.get();
        break;
    default:
        ASSERT(false, "Invalid command worker type.");
        break;
    }

    ASSERT(worker, "Worker should not be nullptr.");
    graphicSystem->mLastWorker = worker;
    return worker;
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

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> NFGE::Graphics::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.Type = type;

    ThrowIfFailed(GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
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
