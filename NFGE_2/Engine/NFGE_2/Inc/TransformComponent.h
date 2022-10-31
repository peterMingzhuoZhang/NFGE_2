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
		static const Component* StaticGetClass();
		virtual const Component* GetClass() const { return StaticGetClass(); }

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