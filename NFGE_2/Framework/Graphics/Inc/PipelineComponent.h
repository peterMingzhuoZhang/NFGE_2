//====================================================================================================
// Filename:	PipelineContext.h
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#pragma once

#include "RootSignature.h"
#include "Mesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "../RaytracingHlslCompat.h"

namespace NFGE::Graphics
{
	class PipelineWorker;
	// Component base style
	struct PipelineComponent
	{
		bool isLoaded = false;
		virtual void GetLoad(PipelineWorker& worker) = 0;
		virtual void GetUpdate(PipelineWorker& worker) {};
		virtual void GetBind(PipelineWorker& worker) = 0;
	};

	template <typename T>
	struct PipelineComponent_Basic : public PipelineComponent
	{
		using MeshType = T;

		MeshType mMesh;
		// Vertex buffer
		VertexBuffer mVertexBuffer;
		// Index buffer
		IndexBuffer mIndexBuffer;

		RootSignature mRootSignature;

		std::wstring mShaderFilename;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE mD3d12Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		D3D_PRIMITIVE_TOPOLOGY mD3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		
		void GetLoad(PipelineWorker& worker) override;
		void GetUpdate(PipelineWorker& worker) override;
		void GetBind(PipelineWorker& worker) override;

		void UpdateVertices(const std::vector<typename T::VertexType>& vertexData, uint32_t numVertices);
	};

	struct PipelineComponent_SingleTexture : public PipelineComponent
	{
		std::filesystem::path filename;
		size_t mTextureId{ 0 };
		uint32_t mRootParamterIndex{ 0 };
		uint32_t mDescriptorOffset{ 0 };

		void GetLoad(PipelineWorker& worker) override;
		void GetBind(PipelineWorker& worker) override;
	};

	struct PipelineComponent_RayTracing: public PipelineComponent
	{
		// D3D 12 control
		Microsoft::WRL::ComPtr<ID3D12Device5> mDxrDevice;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> mDxrCommandList;
		Microsoft::WRL::ComPtr<ID3D12StateObject> mDxrStateObject;
		// Root signatures
		Microsoft::WRL::ComPtr<ID3D12RootSignature> mRaytracingGlobalRootSignature;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> mRaytracingLocalRootSignature;
		// Descriptors
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
		UINT mDescriptorsAllocated;
		UINT mDescriptorSize;
		// Raytracing scene
		RayGenConstantBuffer mRayGenCB;
		// Acceleration structure
		Microsoft::WRL::ComPtr<ID3D12Resource> mAccelerationStructure;
		Microsoft::WRL::ComPtr<ID3D12Resource> mBottomLevelAccelerationStructure;
		Microsoft::WRL::ComPtr<ID3D12Resource> mTopLevelAccelerationStructure;

		// Shader tables
		static const wchar_t* sHitGroupName;
		static const wchar_t* sRaygenShaderName;
		static const wchar_t* sClosestHitShaderName;
		static const wchar_t* sMissShaderName;
		Microsoft::WRL::ComPtr<ID3D12Resource> mMissShaderTable;
		Microsoft::WRL::ComPtr<ID3D12Resource> mHitGroupShaderTable;
		Microsoft::WRL::ComPtr<ID3D12Resource> mRayGenShaderTable;

		// Gemotry
		struct Vertex { float v1, v2, v3; };
		MeshBase<Vertex> mMesh;
		VertexBuffer mVertexBuffer;
		IndexBuffer mIndexBuffer;

		// Raytracing output
		Microsoft::WRL::ComPtr<ID3D12Resource> mRaytracingOutput;
		D3D12_GPU_DESCRIPTOR_HANDLE mRaytracingOutputResourceUAVGpuDescriptor;
		UINT mRaytracingOutputResourceUAVDescriptorHeapIndex;

		void GetLoad(PipelineWorker& worker) override;
		void GetBind(PipelineWorker& worker) override;
		void DoRaytracing(PipelineWorker& worker);
		void CopyToBackBuffer(PipelineWorker& worker);

		void OnSizeChanged(UINT width, UINT height);
	private:
		void CreateDescriptorHeap();
		void CreateRaytracingPSO();
		void BuildAccelerationStructures(PipelineWorker& worker);
		void BuildShaderTables();
		void CreateRaytracingOutputResource();
		void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>* rootSig);
		UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse);

		void UpdateForSizeChange(UINT width, UINT height);
	};
}
