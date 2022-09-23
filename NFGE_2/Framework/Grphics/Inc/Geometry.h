//====================================================================================================
// Filename:	Geometry.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#pragma once

#include "Mesh.h"
#include "Light.h"
namespace NFGE::Graphics
{
	//template <typename T>
	class Geometry
	{
		MeshPC mMesh;
		struct GeometryMeshRenderConstants {
			// Constant buffer for Transform matrixs
			// Constant buffer for Light
			// Constant buffer for material

			// VertexShader --> pipeline state
			// PixelShader --> pipeline state

			// Textures
		} mModelMeshRenderConstants;

		struct GeometryMeshContext {
			NFGE::Math::Matrix4 custumAdjustMatrix = NFGE::Math::Matrix4::sIdentity();

			NFGE::Math::Matrix4 custumToWorldMatrix = NFGE::Math::Matrix4::sIdentity();
			NFGE::Math::Vector3 position = NFGE::Math::Vector3::Zero();
			NFGE::Math::Quaternion rotation = NFGE::Math::Quaternion::Identity();
			NFGE::Math::Vector3 scale = NFGE::Math::Vector3::One();

			// TransformData mTransformData;
			// DirectionalLight* light;
			// std::vector<Material> materials;
		} mModelMeshContext;

	public:
		void Load(DirectionalLight* directionLight = nullptr);
		void UnLoad();
		void Update(float deltaTime);
		void Render(const NFGE::Graphics::Camera& camera);
		void DebugUI();
	};

	
}