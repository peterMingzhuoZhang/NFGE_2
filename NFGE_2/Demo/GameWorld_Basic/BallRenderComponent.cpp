#include "BallRenderComponent.h"

RTTI_DEFINITION(BallRenderComponent)

void BallRenderComponent::Initialize()
{
    mGeometry.Prepare(NFGE::Graphics::MeshBuilder::CreateSpherePX(100, 100, 10), &sApp.GetMainLight(), "texcoord.png");
}

void BallRenderComponent::Terminate()
{
}

void BallRenderComponent::Update(float deltaTime)
{
    mGeometry.mMeshContext.position = GetOwner().GetComponent<NFGE::TransformComponent>()->position;
}

void BallRenderComponent::Render()
{
    mGeometry.Render(sApp.GetMainCamera());
}

void BallRenderComponent::DebugUI()
{

}
