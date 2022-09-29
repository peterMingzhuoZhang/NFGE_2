//====================================================================================================
// Filename:	PipelineWork.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#pragma once

#include "Geometry.h"
#include "VertexType.h"

namespace NFGE::Graphics 
{
	class PipelineWork 
	{
	public:
		void Load(GeometryPNX& renderObject);
	};

	//comptr d3d12 rootSignature
	//comptr d3d12 pipelineState
		// |-VertexShader
		// |-PixelShader

	//Constant buffer
	//Resource for Texture

	
}
