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
		
		void GetLoad(PipelineWorker& worker) override;
		void GetBind(PipelineWorker& worker) override;
	};

	struct PipelineComponent_SingleTexture : public PipelineComponent
	{
		std::filesystem::path filename;
		size_t mTextureId;
		uint32_t mRootParamterIndex;
		uint32_t mDescriptorOffset;

		void GetLoad(PipelineWorker& worker) override;
		void GetBind(PipelineWorker& worker) override;
	};
}
