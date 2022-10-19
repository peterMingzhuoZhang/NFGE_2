//====================================================================================================
// Filename:	PipelineContext.h
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#pragma once

#include "RootSignature.h"

namespace NFGE::Graphics
{
	// Component base style
	struct PipelineComponent_Basic
	{
		// Vertex buffer
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		// Index buffer
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;

		RootSignature rootSignature;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	};

	struct PipelineComponent_SingleTexture
	{
		size_t mTexture;
	};
}
