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
#include "CompiledShaders\Raytracing.hlsl.h"
#include "Graphics/Inc/GraphicsSystem.h"


#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

using namespace NFGE::Graphics;
using namespace Microsoft::WRL;

// PipelineComponent_Basic -------------------------------------------------------------------------
template<typename T>
void NFGE::Graphics::PipelineComponent_Basic<T>::GetLoad(PipelineWorker& worker)
{
	ASSERT(!isLoaded, "Loading component second time is not allowed.");
	auto device = NFGE::Graphics::GetDevice();
	// Upload vertex buffer data.
	auto& vertices = mMesh.GetVertices();
	worker.CopyVertexBuffer(mVertexBuffer, vertices);
	auto& indices = mMesh.GetIndices();
	worker.CopyIndexBuffer(mIndexBuffer, indices);

	// Load and compile the vertex shader.
	ComPtr<ID3DBlob> vertexShaderBlob = nullptr;
	ID3DBlob* shaderErrorBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(
		mShaderFilename.c_str(),
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
		mShaderFilename.c_str(),
		nullptr, nullptr,
		"PS",
		"ps_5_1", 0, 0, // which compiler
		&pixelShaderBlob,	//
		&shaderErrorBlob);
	ASSERT(SUCCEEDED(hr), "Failed to compile pixel shader. Error: %s", (const char*)shaderErrorBlob->GetBufferPointer());
	SafeRelease(shaderErrorBlob);

	auto vertexLayout = NFGE::Graphics::GetVectexLayout(mMesh.GetVertexFormat());

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
	pipelineStateStream.PrimitiveTopologyType = mD3d12Topology;
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

template<typename T>
void NFGE::Graphics::PipelineComponent_Basic<T>::GetUpdate(PipelineWorker& worker)
{
	// Upload vertex buffer data.
	auto& vertices = mMesh.GetVertices();
	worker.CopyVertexBuffer(mVertexBuffer, vertices);
	auto& indices = mMesh.GetIndices();
	worker.CopyIndexBuffer(mIndexBuffer, indices);
}

template<typename T>
void NFGE::Graphics::PipelineComponent_Basic<T>::GetBind(PipelineWorker& worker)
{
	worker.SetPipelineState(mPipelineState);
	worker.SetGraphicsRootSignature(mRootSignature);
	worker.SetPrimitiveTopology(mD3dTopology);
	worker.SetVertexBuffer(0, mVertexBuffer);
	worker.SetIndexBuffer(mIndexBuffer);

}

template<typename T>
void NFGE::Graphics::PipelineComponent_Basic<T>::UpdateVertices(const std::vector<typename T::VertexType>& vertexData, uint32_t numVertices)
{
	auto it = vertexData.begin() + numVertices;
	mMesh.mVertices.assign(vertexData.begin(), it);
}

// Explicit instantiations
template struct PipelineComponent_Basic<MeshPC>;
template struct PipelineComponent_Basic<MeshPX>;

// PipelineComponent_SingleTexture -----------------------------------------------------------------
void NFGE::Graphics::PipelineComponent_SingleTexture::GetLoad(PipelineWorker& worker)
{
	ASSERT(!isLoaded, "Loading component second time is not allowed.");

	mTextureId = NFGE::Graphics::TextureManager::Get()->LoadTexture(filename, NFGE::Graphics::TextureUsage::Albedo, true);

	isLoaded = true;
}

void NFGE::Graphics::PipelineComponent_SingleTexture::GetBind(PipelineWorker& worker)
{
	Texture* texture = NFGE::Graphics::TextureManager::Get()->GetTexture(mTextureId);
	worker.SetShaderResourceView(mRootParamterIndex, mDescriptorOffset, *texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

// PipelineComponent_Raytracing -----------------------------------------------------------------
const wchar_t* PipelineComponent_RayTracing::sHitGroupName = L"MyHitGroup";
const wchar_t* PipelineComponent_RayTracing::sRaygenShaderName = L"MyRaygenShader";
const wchar_t* PipelineComponent_RayTracing::sClosestHitShaderName = L"MyClosestHitShader";
const wchar_t* PipelineComponent_RayTracing::sMissShaderName = L"MyMissShader";

void NFGE::Graphics::PipelineComponent_RayTracing::GetLoad(PipelineWorker& worker)
{
	auto device = Graphics::GetDevice();
	auto commandList = worker.GetGraphicsCommandList();

	ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&mDxrDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");
	ThrowIfFailed(commandList->QueryInterface(IID_PPV_ARGS(&mDxrCommandList)), L"Couldn't get DirectX Raytracing interface for the command list.\n");

	// Global Root Signature
	{
		CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
		UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count];
		rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &UAVDescriptor);
		rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
		CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &mRaytracingGlobalRootSignature);
	}
	// Local Root Signature
	{
		CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count];
		rootParameters[LocalRootSignatureParams::ViewportConstantSlot].InitAsConstants(SizeOfInUint32(mRayGenCB), 0, 0);
		CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &mRaytracingLocalRootSignature);
	}

	CreateRaytracingPSO();

	CreateDescriptorHeap();

	auto graphicsSystem = Graphics::GraphicsSystem::Get();
	// Upload vertex data.
	auto& vertices = mMesh.GetVertices();
	//worker.CopyVertexBuffer(mVertexBuffer, vertices);
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
	graphicsSystem->AllocateUploadBuffer(device.Get(), vertices.data(), vertices.size(), &d3d12Resource);
	mVertexBuffer.SetD3D12Resource(d3d12Resource);
	auto& indices = mMesh.GetIndices();
	//worker.CopyIndexBuffer(mIndexBuffer, indices);
	graphicsSystem->AllocateUploadBuffer(device.Get(), indices.data(), indices.size(), &d3d12Resource);
	mIndexBuffer.SetD3D12Resource(d3d12Resource);
	
	BuildAccelerationStructures(worker);

	BuildShaderTables();

	CreateRaytracingOutputResource();

	graphicsSystem->RegisterResizeComponent(this);
}

void NFGE::Graphics::PipelineComponent_RayTracing::GetBind(PipelineWorker& worker)
{
}

void NFGE::Graphics::PipelineComponent_RayTracing::DoRaytracing(PipelineWorker& worker)
{
	auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
	auto commandList = worker.GetGraphicsCommandList();

	auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
	{
		// Since each shader table has only one shader record, the stride is same as the size.
		dispatchDesc->HitGroupTable.StartAddress = mHitGroupShaderTable->GetGPUVirtualAddress();
		dispatchDesc->HitGroupTable.SizeInBytes = mHitGroupShaderTable->GetDesc().Width;
		dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
		dispatchDesc->MissShaderTable.StartAddress = mMissShaderTable->GetGPUVirtualAddress();
		dispatchDesc->MissShaderTable.SizeInBytes = mMissShaderTable->GetDesc().Width;
		dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
		dispatchDesc->RayGenerationShaderRecord.StartAddress = mRayGenShaderTable->GetGPUVirtualAddress();
		dispatchDesc->RayGenerationShaderRecord.SizeInBytes = mRayGenShaderTable->GetDesc().Width;
		dispatchDesc->Width = graphicSystem->GetBackBufferWidth();
		dispatchDesc->Height = graphicSystem->GetBackBufferHeight();
		dispatchDesc->Depth = 1;
		commandList->SetPipelineState1(stateObject);
		commandList->DispatchRays(dispatchDesc);
	};

	commandList->SetComputeRootSignature(mRaytracingGlobalRootSignature.Get());

	// Bind the heaps, acceleration structure and dispatch rays.    
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	commandList->SetDescriptorHeaps(1, mDescriptorHeap.GetAddressOf());
	commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, mRaytracingOutputResourceUAVGpuDescriptor);
	commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, mTopLevelAccelerationStructure->GetGPUVirtualAddress());
	DispatchRays(mDxrCommandList.Get(), mDxrStateObject.Get(), &dispatchDesc);
}

void NFGE::Graphics::PipelineComponent_RayTracing::CopyToBackBuffer(PipelineWorker& worker)
{
	auto commandList = worker.GetGraphicsCommandList();
	auto renderTarget = NFGE::Graphics::GraphicsSystem::Get()->GetRenderTarget().GetTexture(AttachmentPoint::Color0).GetD3D12Resource();

	//worker.TransitionBarrier(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,true);
	//worker.TransitionBarrier(mRaytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	D3D12_RESOURCE_BARRIER preCopyBarriers[2];
	preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(mRaytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

	commandList->CopyResource(renderTarget.Get(), mRaytracingOutput.Get());

	//worker.TransitionBarrier(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	//worker.TransitionBarrier(mRaytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	D3D12_RESOURCE_BARRIER postCopyBarriers[2];
	postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(mRaytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);
}

void NFGE::Graphics::PipelineComponent_RayTracing::CreateDescriptorHeap()
{
	auto device = Graphics::GetDevice();

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	// Allocate a heap for a single descriptor:
	// 1 - raytracing output texture UAV
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&mDescriptorHeap));
	mDescriptorHeap->SetName(L"mDescriptorHeap");

	mDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void NFGE::Graphics::PipelineComponent_RayTracing::CreateRaytracingPSO()
{
	// Create PSO
	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };
	// PSO::DXIL library
	// This contains the shaders and their entrypoints for the state object.
	// Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
	CD3DX12_DXIL_LIBRARY_SUBOBJECT* lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));
	lib->SetDXILLibrary(&libdxil);
	// Define which shader exports to surface from the library.
	// If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
	// In this sample, this could be omitted for convenience since the sample uses all shaders in the library. 
	{
		lib->DefineExport(sRaygenShaderName);
		lib->DefineExport(sClosestHitShaderName);
		lib->DefineExport(sMissShaderName);
	}

	// PSO::Triangle hit group
	// A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
	CD3DX12_HIT_GROUP_SUBOBJECT* hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	hitGroup->SetClosestHitShaderImport(sClosestHitShaderName);
	hitGroup->SetHitGroupExport(sHitGroupName);
	hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// PSO::Ray payload and attribute structure
	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT payloadSize = 4 * sizeof(float);   // float4 color
	UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
	shaderConfig->Config(payloadSize, attributeSize);

	// PSO:: Set Local root signature and shader association
	{
		CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT* localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		localRootSignature->SetRootSignature(mRaytracingLocalRootSignature.Get());
		// Shader association
		auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
		rootSignatureAssociation->AddExport(sRaygenShaderName);
	}

	// PSO:: Set Global root signature
	// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSignature->SetRootSignature(mRaytracingGlobalRootSignature.Get());

	// PSO::Pipeline config
	// Defines the maximum TraceRay() recursion depth.
	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	// PERFOMANCE TIP: Set max recursion depth as low as needed 
	// as drivers may apply optimization strategies for low recursion depths. 
	UINT maxRecursionDepth = 1; // ~ primary rays only. 
	pipelineConfig->Config(maxRecursionDepth);

	// Create the state object.
	ThrowIfFailed(mDxrDevice->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&mDxrStateObject)), L"Couldn't create DirectX Raytracing state object.\n");
}

void NFGE::Graphics::PipelineComponent_RayTracing::BuildAccelerationStructures(PipelineWorker& worker)
{
	auto device = Graphics::GetDevice();
	worker.EndWork();
	worker.BeginWork();
	auto commandList = worker.GetGraphicsCommandList();

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.IndexBuffer = mIndexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(mIndexBuffer.GetD3D12ResourceDesc().Width) / sizeof(uint16_t);
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(mVertexBuffer.GetD3D12ResourceDesc().Width) / sizeof(Vertex);
	geometryDesc.Triangles.VertexBuffer.StartAddress = mVertexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // Opaque can enable important ray processing optimization

	// Get required sizes for an acceleration structure.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	mDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	ASSERT(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, "TopLevelPrebuildInfo can not be empty.");

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = &geometryDesc;
	mDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
	ASSERT(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, "BottomLevelPrebuildInfo can not be empty.");

	//auto graphicSystem = Graphics::GraphicsSystem::Get();
	Microsoft::WRL::ComPtr<ID3D12Resource> scratchResource;
	GraphicsSystem::AllocateUAVBuffer(device.Get(), NFGE::Math::Max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

	{
		D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		GraphicsSystem::AllocateUAVBuffer(device.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &mBottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
		GraphicsSystem::AllocateUAVBuffer(device.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &mTopLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
	}

	// Create an instance desc for the bottom-level acceleration structure.
	ComPtr<ID3D12Resource> instanceDescs;
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.InstanceMask = 1;
	instanceDesc.AccelerationStructure = mBottomLevelAccelerationStructure->GetGPUVirtualAddress();
	GraphicsSystem::AllocateUploadBuffer(device.Get(), &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

	// Bottom Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	{
		bottomLevelBuildDesc.Inputs = bottomLevelInputs;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = mBottomLevelAccelerationStructure->GetGPUVirtualAddress();
	}

	// Top Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	{
		topLevelInputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs = topLevelInputs;
		topLevelBuildDesc.DestAccelerationStructureData = mTopLevelAccelerationStructure->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
	}

	//auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
	//{
		mDxrCommandList.Get()->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
		worker.UAVBarrier(mBottomLevelAccelerationStructure,true);
		mDxrCommandList.Get()->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
	//};

	// Build acceleration structure.
	//BuildAccelerationStructure(mDxrCommandList.Get());

	worker.EndWork();
	worker.BeginWork();
}

void NFGE::Graphics::PipelineComponent_RayTracing::BuildShaderTables()
{
	auto device = Graphics::GetDevice();

	void* rayGenShaderIdentifier;
	void* missShaderIdentifier;
	void* hitGroupShaderIdentifier;

	auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
	{
		rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sRaygenShaderName);
		missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sMissShaderName);
		hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(sHitGroupName);
	};

	// Get shader identifiers.
	UINT shaderIdentifierSize;
	{
		Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		ThrowIfFailed(mDxrStateObject.As(&stateObjectProperties));
		GetShaderIdentifiers(stateObjectProperties.Get());
		shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}

	// Ray gen shader table
	{
		struct RootArguments {
			RayGenConstantBuffer cb;
		} rootArguments;
		rootArguments.cb = mRayGenCB;

		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
		NFGE::Graphics::ShaderTable rayGenShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
		rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
		mRayGenShaderTable = rayGenShaderTable.GetResource();
	}

	// Miss shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
		mMissShaderTable = missShaderTable.GetResource();
	}

	// Hit group shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable hitGroupShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
		hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize));
		mHitGroupShaderTable = hitGroupShaderTable.GetResource();
	}
}

void NFGE::Graphics::PipelineComponent_RayTracing::CreateRaytracingOutputResource()
{
	auto device = Graphics::GetDevice();
	auto graphicsSystem = Graphics::GraphicsSystem::Get();
	auto backbufferFormat = graphicsSystem->GetBackBufferFormat();

	// Create the output resource. The dimensions and format should match the swap-chain.
	auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, graphicsSystem->GetBackBufferWidth(), graphicsSystem->GetBackBufferHeight(), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&mRaytracingOutput)));
	mRaytracingOutput->SetName(L"mRayTracingOutput");

	D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;
	mRaytracingOutputResourceUAVDescriptorHeapIndex = AllocateDescriptor(&uavDescriptorHandle, mRaytracingOutputResourceUAVDescriptorHeapIndex);
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(mRaytracingOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
	mRaytracingOutputResourceUAVGpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), mRaytracingOutputResourceUAVDescriptorHeapIndex, mDescriptorSize);
}

void NFGE::Graphics::PipelineComponent_RayTracing::OnSizeChanged(UINT width, UINT height)
{
	mRayGenShaderTable.Reset();
	mMissShaderTable.Reset();
	mHitGroupShaderTable.Reset();
	mRaytracingOutput.Reset();

	CreateRaytracingOutputResource();
	BuildShaderTables();
}

void NFGE::Graphics::PipelineComponent_RayTracing::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>* rootSig)
{
	auto device = Graphics::GetDevice();
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> error;

	ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
	ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))));
}

UINT NFGE::Graphics::PipelineComponent_RayTracing::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse)
{
	auto descriptorHeapCpuBase = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	if (descriptorIndexToUse >= mDescriptorHeap->GetDesc().NumDescriptors)
	{
		descriptorIndexToUse = mDescriptorsAllocated++;
	}
	*cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, mDescriptorSize);
	return descriptorIndexToUse;
}
