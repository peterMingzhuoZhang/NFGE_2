//====================================================================================================
// Filename:	Model.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#pragma once

#include "Mesh.h"
//#include "MeshBuffer.h"

namespace NFGE::Graphics
{
	class Model
	{
	public:
		//Peter chan suggest
		struct MeshData
		{
			BoneMesh mesh;
			uint32_t materialIndex;
			//MeshBuffer meshBuffer;
		};
	public:
		bool isRenderWithEffect = false;;

		std::vector<MeshData> mMeshData;

		struct ModelMeshRenderConstants {
			// Constant buffer for Transform matrixs
			// Constant buffer for Light
			// Constant buffer for material
			// Constant buffer for option

			// VertexShader --> pipeline state
			// PixelShader --> pipeline state

			// Textures
		} mModelMeshRenderConstants;

		struct ModelMeshContext {
			NFGE::Math::Matrix4 custumAdjustMatrix = NFGE::Math::Matrix4::sIdentity();
			NFGE::Math::Vector3 AdjustPosition = NFGE::Math::Vector3::Zero();
			NFGE::Math::Vector3 AdjustRotation = NFGE::Math::Vector3::Zero();
			NFGE::Math::Vector3 AdjustScale = NFGE::Math::Vector3::One();

			NFGE::Math::Matrix4 custumToWorldMatrix = NFGE::Math::Matrix4::sIdentity();
			NFGE::Math::Vector3 position = NFGE::Math::Vector3::Zero();
			NFGE::Math::Quaternion rotation = NFGE::Math::Quaternion::Identity();
			NFGE::Math::Vector3 scale = NFGE::Math::Vector3::One();

			NFGE::Math::Vector3 currentFoward = NFGE::Math::Vector3{ 0.0f,0.0f,1.0f };

			// TransformData mTransformData;
			// DirectionalLight* light;
			// std::vector<Material> materials;
			// float bumpWeight = 0.0f;
		} mModelMeshContext;
	public:
		void Load(const char* modelFileName, DirectionalLight* directionLight = nullptr);
		void Load(const std::filesystem::path& fileName, DirectionalLight* directionLight = nullptr);
		void UnLoad();
		void Update(float deltaTime);
		void Render(const NFGE::Graphics::Camera& camera);
		void DebugUI();

		bool SaveToFile(FILE* file, NFGE::Graphics::IOOption option = NFGE::Graphics::IOOption::Binary) const;
	private:
		bool LoadFromFile(FILE* file, NFGE::Graphics::IOOption option = NFGE::Graphics::IOOption::Binary);

	};
} // namespace NFGE::Graphics
