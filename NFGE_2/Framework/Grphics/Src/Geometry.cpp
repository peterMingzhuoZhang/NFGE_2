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
#include "PipelineWorker.h"

using namespace NFGE::Graphics;
using namespace NFGE::Math;
using namespace Microsoft::WRL;

const std::wstring NFGE::Graphics::GeometryPC::sShaderFilename = L"../../Assets/Shaders/ColorMesh.hlsl";
const std::wstring NFGE::Graphics::GeometryPX::sShaderFilename = L"../../Assets/Shaders/TextureMesh.hlsl";

// ---------------------------- Geometry PC -----------------------------------------
void NFGE::Graphics::GeometryPC::Prepare(MeshPC mesh, DirectionalLight* directionLight) 
{
	mMeshContext.mLight = directionLight;
	//Load custom mesh
	mPipelineComp_Basic.mMesh = std::move(mesh);
	mPipelineComp_Basic.mShaderFilename = GeometryPC::sShaderFilename;

	// Load custom root signature.
	auto device = NFGE::Graphics::GetDevice();
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
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(NFGE::Math::Matrix4) * 3 / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, NULL, rootSignatureFlags);

	mPipelineComp_Basic.mRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

	NFGE::Graphics::RegisterPipelineComponent(WorkerType::Copy, &mPipelineComp_Basic);
}

void NFGE::Graphics::GeometryPC::UnLoad()
{}

void NFGE::Graphics::GeometryPC::Update(float deltaTime)
{}

void NFGE::Graphics::GeometryPC::Render(const NFGE::Graphics::Camera& camera) 
{
	auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
	auto worker = NFGE::Graphics::GetWorker(WorkerType::Direct);

	mPipelineComp_Basic.GetBind(*worker);

	Matrix4 matrices[3]{};
	matrices[0] = Matrix4::sScaling(mMeshContext.scale) * MatrixRotationQuaternion(mMeshContext.rotation) * Matrix4::sTranslation(mMeshContext.position);
	matrices[1] = camera.GetViewMatrix();
	matrices[2] = camera.GetPerspectiveMatrix();
	matrices[0] = Transpose(matrices[0]);
	matrices[1] = Transpose(matrices[1]);
	matrices[2] = Transpose(matrices[2]);
	worker->SetGraphics32BitConstants(0, sizeof(Matrix4) * 3 / 4, &matrices);

	worker->DrawIndexed(static_cast<uint32_t>(mPipelineComp_Basic.mMesh.GetIndices().size()), 1, 0, 0, 0);
}

// ---------------------------- Geometry PX -----------------------------------------
void NFGE::Graphics::GeometryPX::Prepare(MeshPX mesh, DirectionalLight* directionLight, const std::filesystem::path& texturePath)
{
	mMeshContext.mLight = directionLight;

	//Load custom mesh
	mPipelineComp_Basic.mMesh = std::move(mesh);
	mPipelineComp_Basic.mShaderFilename = GeometryPX::sShaderFilename;

	// Load custom root signature.
	auto device = NFGE::Graphics::GetDevice();
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
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsConstants(sizeof(NFGE::Math::Matrix4) * 3 / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	rootParameters[1].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

	mPipelineComp_Basic.mRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

	NFGE::Graphics::RegisterPipelineComponent(WorkerType::Copy, &mPipelineComp_Basic);

	mPipelineComp_Texture.mRootParamterIndex = 1;
	mPipelineComp_Texture.mDescriptorOffset = 0;
	mPipelineComp_Texture.filename = texturePath;
	NFGE::Graphics::RegisterPipelineComponent(WorkerType::Compute, &mPipelineComp_Texture);
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
	auto worker = NFGE::Graphics::GetWorker(WorkerType::Direct);

	mPipelineComp_Basic.GetBind(*worker);

	Matrix4 matrices[3]{};
	matrices[0] = Matrix4::sScaling(mMeshContext.scale) * MatrixRotationQuaternion(mMeshContext.rotation) * Matrix4::sTranslation(mMeshContext.position);
	matrices[1] = camera.GetViewMatrix();
	matrices[2] = camera.GetPerspectiveMatrix();
	matrices[0] = Transpose(matrices[0]);
	matrices[1] = Transpose(matrices[1]);
	matrices[2] = Transpose(matrices[2]);
	worker->SetGraphics32BitConstants(0, sizeof(Matrix4) * 3 / 4, &matrices);
	// Bind texture
	mPipelineComp_Texture.GetBind(*worker);
	

	worker->DrawIndexed(static_cast<uint32_t>(mPipelineComp_Basic.mMesh.GetIndices().size()), 1, 0, 0, 0);
}
