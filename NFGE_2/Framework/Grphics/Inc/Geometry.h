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
	template <typename MeshType>
	class Geometry
	{
		MeshType mMesh;
		struct GeometryMeshRenderStrcuture {
			// Constant buffer for Transform matrixs
			// Constant buffer for Light
			// Constant buffer for material

			// VertexShader --> pipeline state
			// PixelShader --> pipeline state

			// Textures
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
		void Load(MeshType mesh, DirectionalLight* directionLight);
		void UnLoad();
		void Update(float deltaTime);
		void Render(const NFGE::Graphics::Camera& camera);
		void DebugUI();
	};

	
	template<typename MeshType>
	inline void Geometry<MeshType>::Load(MeshType mesh, DirectionalLight* directionLight)
	{
		mMesh = std::move(mesh);
		mModelMeshContext.mLight = directionLight;
	}

	template<typename MeshType>
	inline void Geometry<MeshType>::UnLoad()
	{
	}

}