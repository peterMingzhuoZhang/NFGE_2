//====================================================================================================
// Filename:	Material.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================
#pragma once

#include "Colors.h"

namespace NFGE::Graphics
{
	struct Material
	{
		Color ambient;
		Color diffuse;
		Color specular;
		float power;
		float padding[3];

		void SetAlpha(float alpha) { ambient.w = alpha; diffuse.w = alpha; specular.w = alpha; }
	};
} // namespace NFGE::Graphics
