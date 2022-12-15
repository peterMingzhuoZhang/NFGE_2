#include "RaytracingShapeComponent.h"

RTTI_DEFINITION(RayTracingShapeComponent)
void RayTracingShapeComponent::Initialize()
{
    std::vector<UINT16> indices{ 1,2,3 };
    float depthValue = 1.0;
    float offset = 0.7f;
    std::vector<Graphics::PipelineComponent_RayTracing::Vertex> vertices{
        { 0, -offset, depthValue },
        { -offset, offset, depthValue },
        { offset, offset, depthValue }
    };
    mGeometry.Prepare(vertices, indices, &sApp.GetMainLight());
}

void RayTracingShapeComponent::Terminate()
{
}

void RayTracingShapeComponent::Update(float deltaTime)
{
}

void RayTracingShapeComponent::Render()
{
    mGeometry.Render(sApp.GetMainCamera());
}

void RayTracingShapeComponent::DebugUI()
{
}