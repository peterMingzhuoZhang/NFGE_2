#pragma once

namespace NFGE::Graphics
{
	class Texture;
	class CommandQueue;
	class DescriptorAllocation;
	struct PipelineComponent;
	enum class WorkerType;
	class PipelineWorker;

	Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice();
	void RegisterPipelineComponent(NFGE::Graphics::WorkerType type, PipelineComponent* component);
	PipelineWorker* GetWorker(NFGE::Graphics::WorkerType type);
	uint8_t GetFrameCount();
	void Flush();

	DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);
	// Release stale descriptors. This should only be called with a completed frame counter.
	void ReleaseStaleDescriptors(uint64_t finishedFrame);
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type);

	std::vector<D3D12_INPUT_ELEMENT_DESC> GetVectexLayout(uint32_t vertexFormat);
}