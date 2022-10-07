//====================================================================================================
// Filename:	Geometry.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#include "Precompiled.h"
#include "Geometry.h"

#include "D3DUtil.h"
#include "d3dx12.h"
#include "GraphicsSystem.h"

using namespace NFGE::Graphics;
using namespace NFGE::Math;
using namespace Microsoft::WRL;

void NFGE::Graphics::GeometryPX::Load(MeshPX mesh, DirectionalLight* directionLight)
{
	mMesh = std::move(mesh);
	mMeshContext.mLight = directionLight;

	auto device = NFGE::Graphics::GetDevice();
	auto commandQueue = NFGE::Graphics::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	// Upload vertex buffer data.
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	auto& vertices = mMesh.GetVertices();
	NFGE::Graphics::GraphicsSystem::UpdateBufferResource(commandList.Get(),
		&mMeshRenderStrcuture.vertexBuffer, &intermediateVertexBuffer,
		vertices.size(), sizeof(VertexPX), vertices.data());
	// Create the vertex buffer view.
	mMeshRenderStrcuture.vertexBufferView.BufferLocation = mMeshRenderStrcuture.vertexBuffer->GetGPUVirtualAddress();
	mMeshRenderStrcuture.vertexBufferView.SizeInBytes = vertices.size();
	mMeshRenderStrcuture.vertexBufferView.StrideInBytes = sizeof(VertexPX);
	// Upload index buffer data.
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	auto& indices = mMesh.GetIndices();
	NFGE::Graphics::GraphicsSystem::UpdateBufferResource(commandList.Get(),
		&mMeshRenderStrcuture.indexBuffer, &intermediateIndexBuffer,
		indices.size(), sizeof(WORD), indices.data());
	// Create index buffer view.
	mMeshRenderStrcuture.indexBufferView.BufferLocation = mMeshRenderStrcuture.indexBuffer->GetGPUVirtualAddress();
	mMeshRenderStrcuture.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	mMeshRenderStrcuture.indexBufferView.SizeInBytes = indices.size();

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

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

	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsConstants(sizeof(NFGE::Math::Matrix4) * 3 / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameters[1].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
	// Create the root signature.
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&mMeshRenderStrcuture.rootSignature)));

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

	pipelineStateStream.pRootSignature = mMeshRenderStrcuture.rootSignature.Get();
	pipelineStateStream.InputLayout = { vertexLayout.data(), static_cast<UINT>(vertexLayout.size()) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&mMeshRenderStrcuture.pipelineState)));

}

void NFGE::Graphics::GeometryPX::UnLoad()
{
}

void NFGE::Graphics::GeometryPX::Update(float deltaTime)
{
}

void NFGE::Graphics::GeometryPX::Render(const NFGE::Graphics::Camera& camera)
{
	auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
	auto commandList = graphicSystem->GetCurrentCommandList();

	commandList->SetPipelineState(mMeshRenderStrcuture.pipelineState.Get());
	commandList->SetGraphicsRootSignature(mMeshRenderStrcuture.rootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, & mMeshRenderStrcuture.vertexBufferView);
	commandList->IASetIndexBuffer(&mMeshRenderStrcuture.indexBufferView);

	Matrix4 matrices[3];
	matrices[0] = Matrix4::sScaling(mMeshContext.scale) * MatrixRotationQuaternion(mMeshContext.rotation) * Matrix4::sTranslation(mMeshContext.position);
	matrices[1] = camera.GetViewMatrix();
	matrices[2] = camera.GetPerspectiveMatrix();
	matrices[0] = Transpose(matrices[0]);
	matrices[1] = Transpose(matrices[1]);
	matrices[2] = Transpose(matrices[2]);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix4) * 3 / 4, &matrices, 0);
	// Bind texture
	Texture* texture = NFGE::Graphics::TextureManager::Get()->GetTexture(mMeshRenderStrcuture.mTexture_0);
	graphicSystem->SetShaderResourceView(1, 0, *texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	graphicSystem->DrawIndexed(mMesh.GetIndices().size(), 1, 0, 0, 0);

}
