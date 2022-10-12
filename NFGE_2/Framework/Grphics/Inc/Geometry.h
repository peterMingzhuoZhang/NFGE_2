//====================================================================================================
// Filename:	Geometry.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#pragma once

#include "Mesh.h"
#include "Light.h"
#include "Camera.h"
#include "RootSignature.h"
#include "TextureManager.h"

namespace NFGE::Graphics
{
	class GeometryPX
	{
	public:
		MeshPX mMesh;
		struct GeometryMeshRenderStrcuture {
			// Vertex buffer
			Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
			D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
			// Index buffer
			Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
			D3D12_INDEX_BUFFER_VIEW indexBufferView;

			RootSignature rootSignature; // PipelineWorker

			Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState; // PipelineWorker

			//D3D12_INPUT_ELEMENT_DESC inputLayout[];	// PipelineWorker

			// Constant buffer for Transform matrixs
			// Constant buffer for Light
			// Constant buffer for material

			// VertexShader --> pipeline state
			// PixelShader --> pipeline state

			size_t mTexture_0;
		} mMeshRenderStrcuture;

		struct GeometryMeshContext {
			NFGE::Math::Matrix4 custumAdjustMatrix = NFGE::Math::Matrix4::sIdentity();

			NFGE::Math::Matrix4 custumToWorldMatrix = NFGE::Math::Matrix4::sIdentity();
			NFGE::Math::Vector3 position = NFGE::Math::Vector3::Zero();
			NFGE::Math::Quaternion rotation = NFGE::Math::Quaternion::Identity();
			NFGE::Math::Vector3 scale = NFGE::Math::Vector3::One();

			// TransformData mTransformData;
			DirectionalLight* mLight;
			// std::vector<Material> materials;
		} mMeshContext;

	public:
		void Load(MeshPX mesh, DirectionalLight* directionLight);
		void UnLoad();
		void Update(float deltaTime);
		void Render(const NFGE::Graphics::Camera& camera);
		void DebugUI();
	};
	

}