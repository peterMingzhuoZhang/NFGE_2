//====================================================================================================
// Filename:	TransformComponent.h
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#pragma once

#include "Component.h"

namespace NFGE
{
	class GameObject;
	class TransformComponent : public Component
	{
	public:
		RTTI_DELCEAR(TransformComponent)

		void Initialize() override;
		void Render() override;

		Math::Vector3 position;
		Math::EditorQuaternion rotation;
		Math::Vector3 scale{ 1.0f,1.0f,1.0f };

		Math::Matrix4 finalTransform;
		Math::Matrix4 finalRotationMatrix;

		void UpdateFinalTransform(GameObject* parent);


	};
}