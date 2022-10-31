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
	struct GeometryMeshContext {
		NFGE::Math::Matrix4 custumAdjustMatrix = NFGE::Math::Matrix4::sIdentity();

		NFGE::Math::Matrix4 custumToWorldMatrix = NFGE::Math::Matrix4::sIdentity();
		NFGE::Math::Vector3 position = NFGE::Math::Vector3::Zero();
		NFGE::Math::Quaternion rotation = NFGE::Math::Quaternion::Identity();
		NFGE::Math::Vector3 scale = NFGE::Math::Vector3::One();

		// TransformData mTransformData;
		DirectionalLight* mLight{ nullptr };
		// std::vector<Material> materials;
	};

	class GeometryPC
	{
	public:
		static const std::wstring sShaderFilename;

		PipelineComponent_Basic<MeshPC> mPipelineComp_Basic;

		GeometryMeshContext mMeshContext;

	public:
		void Prepare(MeshPC mesh, DirectionalLight* directionLight);
		void UnLoad();
		void Update(float deltaTime);
		void Render(const NFGE::Graphics::Camera& camera);
		void DebugUI();
	};

	class GeometryPX
	{
	public:
		static const std::wstring sShaderFilename;

		PipelineComponent_Basic<MeshPX> mPipelineComp_Basic;
		PipelineComponent_SingleTexture mPipelineComp_Texture;

		GeometryMeshContext mMeshContext;

	public:
		void Prepare(MeshPX mesh, DirectionalLight* directionLight, const std::filesystem::path& texturePath);
		void UnLoad();
		void Update(float deltaTime);
		void Render(const NFGE::Graphics::Camera& camera);
		void DebugUI();
	};
}