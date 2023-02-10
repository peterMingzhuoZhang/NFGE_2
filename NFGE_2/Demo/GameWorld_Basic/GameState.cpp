#include "GameState.h"
#include "BallRenderComponent.h"
#include "RaytracingShapeComponent.h"

namespace
{
	std::string mainCameraID = "MainCamera";
	std::string gameObjectID = "GameObject1";
	std::string LogoID = "basketball.jpg";
}

void GameState::Initialize()
{
	auto camera = mWorld.AddService<CameraService>()->AddCamera(mainCameraID.c_str());
	camera->SetDirection({ 0.0f,0.0f,1.0f });
	camera->SetPosition(0.0f);
	mWorld.Initialize(10);
	
	Graphics::DirectionalLight& light = mWorld.GetMainLight();
	light.direction = { 0.0f, 1.8f, -3.0f};
	light.ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
	light.diffuse = { 0.5f, 0.0f, 0.0f, 1.0f };

	mGameObjectHandle =  mWorld.CreateEmpty(gameObjectID);
	mGameObjectHandle.Get()->GetComponent<NFGE::TransformComponent>()->position = { -15.0f,0.0f, 40.0f };
	mGameObjectHandle.Get()->AddComponent<RayTracingShapeComponent>();
	mGameObjectHandle.Get()->AddComponent<BallRenderComponent>();
	mGameObjectHandle.Get()->Initialize();

	mLogo = Graphics::TextureManager::Get()->LoadTexture(LogoID, Graphics::TextureUsage::Sprite);
}

void GameState::Terminate()
{
	mWorld.Terminate();
}

void GameState::Update(float deltaTime)
{
	Graphics::DirectionalLight& light = mWorld.GetMainLight();
	light.direction = Math::TransfromCoord(light.direction, Math::Matrix4::sRotationY(10.0f * deltaTime));

	mWorld.Update(deltaTime);
	
	sApp.SoSoCameraControl(10, 10, mWorld.GetService<CameraService>()->GetActiveCamera(), deltaTime);
}

void GameState::Render()
{
	mWorld.Render();
	sApp.DrawSprite(mLogo, { 500.0f,500.0f });
}

void GameState::DebugUI()
{
	mWorld.DebugUI();
	for (float i = -50.0f; i <= 50.0f; i += 1.0f)
	{
		NFGE::Graphics::SimpleDraw::AddLine({ i,0.0f,-50.0f }, { i,0.0f,50.0f }, NFGE::Graphics::Colors::DarkGray);
		NFGE::Graphics::SimpleDraw::AddLine({ -50.0f,0.0f,i }, { 50.0f,0.0f,i }, NFGE::Graphics::Colors::DarkGray);
	}

}
