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
		Math::Vector3 direction;
		float padding;
		Color ambient;
		Color diffuse;
		Color specular;
	};
} // namespace NFGE::Graphics