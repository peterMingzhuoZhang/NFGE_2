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

void NFGE::Graphics::Flush()
{
    NFGE::Graphics::GraphicsSystem::Get()->Flush();
}

void NFGE::Graphics::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}

void NFGE::Graphics::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void NFGE::Graphics::ClearDSV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void NFGE::Graphics::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	auto device = GetDevice();

	size_t bufferSize = numElements * elementSize;
	// Create a committed resource for the GPU resource in a default heap.
	{
		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(pDestinationResource)));
	}

	// Create an committed resource for the upload.
	if (bufferData)
	{
		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}
