#include "RaytracingShapeComponent.h"

RTTI_DEFINITION(RayTracingShapeComponent)
void RayTracingShapeComponent::Initialize()
{
    std::vector<UINT16> indices{ 0,1,2 };
    float depthValue = 1.0;
    float offset = 0.7f;
    std::vector<Graphics::PipelineComponent_RayTracing::Vertex> vertices{
        { 0, 0.5, depthValue },
        { 0.5, -0.5, depthValue },
        { -0.5, -0.5, depthValue }
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