//====================================================================================================
// Filename:	Light.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================
#pragma once

#include "Colors.h"

namespace NFGE::Graphics
{
	struct DirectionalLight
	{
		Math::Vector3 direction{ 0, 0, 1};
		float padding{ 0.0f };
		Color ambient{ 1.0f ,1.0f, 1.0f ,1.0f };
		Color diffuse{ 1.0f ,1.0f, 1.0f ,1.0f };;
		Color specular{ 1.0f ,1.0f, 1.0f ,1.0f };;
	};
} // namespace NFGE::Graphics