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
		void Load(Geometry<VertexPNX>& renderObject);
	};
}
