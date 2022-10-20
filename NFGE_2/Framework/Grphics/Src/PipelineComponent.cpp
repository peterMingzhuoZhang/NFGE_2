//====================================================================================================
// Filename:	PipelineComponent.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#include "Precompiled.h"
#include "PipelineComponent.h"
#include "PipelineWorker.h"
#include "D3DUtil.h"
#include "d3dx12.h"

using namespace NFGE::Graphics;
using namespace Microsoft::WRL;

void NFGE::Graphics::PipelineComponent_Basic::GetLoad(PipelineWorker& worker)
{
	ASSERT(!isLoaded, "Loading component second time is not allowed.");
	auto device = NFGE::Graphics::GetDevice();
	// Upload vertex buffer data.
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	auto& vertices = mMesh.GetVertices();
	worker.CopyVertexBuffer(mVertexBuffer, vertices);
	auto& indices = mMesh.GetIndices();
	worker.CopyIndexBuffer(mIndexBuffer, indices);

	// Load and compile the vertex shader.
	ComPtr<ID3DBlob> vertexShaderBlob = nullptr;
	ID3DBlob* shaderErrorBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(
		L"../../Assets/Shaders/TextureMesh.hlsl",
		nullptr, nullptr,
		"VS",
		"vs_5_1", 0, 0, // which compiler
		&vertexShaderBlob,	//
		&shaderErrorBlob);
	ASSERT(SUCCEEDED(hr), "Failed to compile vertex shader. Error: %s", (const char*)shaderErrorBlob->GetBufferPointer());
	SafeRelease(shaderErrorBlob);

	// Load and compile the pixel shader.
	ComPtr<ID3DBlob> pixelShaderBlob = nullptr;
	hr = D3DCompileFromFile(
		L"../../Assets/Shaders/TextureMesh.hlsl",
		nullptr, nullptr,
		"PS",
		"ps_5_1", 0, 0, // which compiler
		&pixelShaderBlob,	//
		&shaderErrorBlob);
	ASSERT(SUCCEEDED(hr), "Failed to compile pixel shader. Error: %s", (const char*)shaderErrorBlob->GetBufferPointer());
	SafeRelease(shaderErrorBlob);

	auto vertexLayout = NFGE::Graphics::GetVectexLayout(VertexPX::Format);

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = mRootSignature.GetRootSignature().Get();
	pipelineStateStream.InputLayout = { vertexLayout.data(), static_cast<UINT>(vertexLayout.size()) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&mPipelineState)));

	isLoaded = true;
}


void NFGE::Graphics::PipelineComponent_Basic::GetBind(PipelineWorker& worker)
{
	worker.SetPipelineState(mPipelineState);
	worker.SetGraphicsRootSignature(mRootSignature);
	worker.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	worker.SetVertexBuffer(0, mVertexBuffer);
	worker.SetIndexBuffer(mIndexBuffer);

}

void NFGE::Graphics::PipelineComponent_SingleTexture::GetLoad(PipelineWorker& worker)
{
	ASSERT(!isLoaded, "Loading component second time is not allowed.");

	mTexture = NFGE::Graphics::TextureManager::Get()->LoadTexture(filename, NFGE::Graphics::TextureUsage::Albedo, true);

	isLoaded = true;
}

void NFGE::Graphics::PipelineComponent_SingleTexture::GetBind(PipelineWorker& worker)
{
	Texture* texture = NFGE::Graphics::TextureManager::Get()->GetTexture(mTexture);
	worker.SetShaderResourceView(mRootParamterIndex, mDescriptorOffset, *texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
