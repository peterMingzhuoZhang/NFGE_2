#include "GameState.h"
#include "BallRenderComponent.h"

namespace
{
	std::string mainCameraID = "MainCamera";
	std::string gameObjectID = "GameObject1";
}

void GameState::Initialize()
{
	auto camera = mWorld.AddService<CameraService>()->AddCamera(mainCameraID.c_str());
	camera->SetDirection({ 0.0f,0.0f,1.0f });
	camera->SetPosition(0.0f);
	mWorld.Initialize(10);

	mGameObjectHandle =  mWorld.CreateEmpty(gameObjectID);
	mGameObjectHandle.Get()->GetComponent<NFGE::TransformComponent>()->position = { -15.0f,0.0f, 40.0f };
	mGameObjectHandle.Get()->AddComponent<BallRenderComponent>();
	mGameObjectHandle.Get()->Initialize();
}

void GameState::Terminate()
{
	mWorld.Terminate();
}

void GameState::Update(float deltaTime)
{
	mWorld.Update(deltaTime);
}

void GameState::Render()
{
	mWorld.Render();
}

void GameState::DebugUI()
{
	mWorld.DebugUI();
}
