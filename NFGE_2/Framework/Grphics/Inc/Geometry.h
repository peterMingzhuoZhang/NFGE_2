//====================================================================================================
// Filename:	Geometry.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#pragma once

#include "Mesh.h"
#include "Light.h"
#include "Camera.h"
#include "PipelineComponent.h"
#include "TextureManager.h"

namespace NFGE::Graphics
{
	class GeometryPX
	{
	public:
		
		PipelineComponent_Basic mPipelineComp_Basic;
		PipelineComponent_SingleTexture mPipelineComp_Texture;

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
		void Prepare(MeshPX mesh, DirectionalLight* directionLight);
		void UnLoad();
		void Update(float deltaTime);
		void Render(const NFGE::Graphics::Camera& camera);
		void DebugUI();
	};
	

}