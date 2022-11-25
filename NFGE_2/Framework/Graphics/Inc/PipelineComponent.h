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
}
