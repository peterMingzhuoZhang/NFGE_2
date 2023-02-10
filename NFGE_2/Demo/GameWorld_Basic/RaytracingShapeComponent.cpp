#include "RaytracingShapeComponent.h"

RTTI_DEFINITION(RayTracingShapeComponent)
void RayTracingShapeComponent::Initialize()
{
    Graphics::MeshPN mesh = Graphics::MeshBuilder::CreateTestCubePN();
    mGeometry.Prepare(mesh.GetVertices(), mesh.GetIndices(), Graphics::Colors::White);
}

void RayTracingShapeComponent::Terminate()
{
}

void RayTracingShapeComponent::Update(float deltaTime)
{
    mGeometry.mMeshContext.position = GetOwner().GetComponent<NFGE::TransformComponent>()->position;
}

void RayTracingShapeComponent::Render()
{
    mGeometry.Render(sApp.GetMainCamera(), sApp.GetMainLight());
}

void RayTracingShapeComponent::DebugUI()
{
}