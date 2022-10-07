//====================================================================================================
// Filename:	PipelineWork.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================
#include "Precompiled.h"
#include "PipelineWorker.h"

#include "D3DUtil.h"

#include "GraphicsSystem.h"

using namespace NFGE::Graphics;

void NFGE::Graphics::PipelineWork::Load(GeometryPX& renderObject)
{
    auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
    ASSERT(graphicSystem, "GraphicsSystem is not initialized.");
    auto device = NFGE::Graphics::GetDevice();
    auto commandQueue = NFGE::Graphics::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();


}
