//====================================================================================================
// Filename:	TransformComponent.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#include "Precompiled.h"
#include "TransformComponent.h"
#include "GameObject.h"

using namespace NFGE;

void NFGE::TransformComponent::Initialize()
{
	rotation.mEularAngle = NFGE::Math::GetEular(rotation.mQuaternion);
}

void NFGE::TransformComponent::Render()
{
}

void NFGE::TransformComponent::UpdateFinalTransform(GameObject* gameObject)
{
	auto parent = gameObject->GetParent();
	if (parent == nullptr)
	{
		finalTransform = NFGE::Math::Matrix4::sIdentity();
		auto rotationMat = NFGE::Math::MatrixRotationQuaternion(rotation.mQuaternion);

		finalTransform = rotationMat * NFGE::Math::Matrix4::sScaling(scale) * NFGE::Math::Matrix4::sTranslation(position) * finalTransform;
		finalRotationMatrix = rotationMat * NFGE::Math::Matrix4::sTranslation(position);
	}
	else
	{
		auto rotationMat = NFGE::Math::MatrixRotationQuaternion(rotation.mQuaternion);
		finalTransform = rotationMat * NFGE::Math::Matrix4::sScaling(scale) * NFGE::Math::Matrix4::sTranslation(position) * parent->GetComponent<TransformComponent>()->finalTransform;
		finalRotationMatrix = NFGE::Math::MatrixRotationQuaternion(rotation.mQuaternion) * NFGE::Math::Matrix4::sTranslation(position) * parent->GetComponent<TransformComponent>()->finalRotationMatrix;
	}

	for (auto child : gameObject->GetChildren())
	{
		child->GetComponent<TransformComponent>()->UpdateFinalTransform(child);
	}
}
