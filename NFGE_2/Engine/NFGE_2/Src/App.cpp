//====================================================================================================
// Filename:	App.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#include "Precompiled.h"
#include "App.h"

#include "AppState.h"

#include "World.h"
#include "CameraService.h"

using namespace NFGE;

void NFGE::App::ChangeState(std::string name)
{
	if (auto iter = mAppStates.find(name); iter != mAppStates.end())
	{
		mNextState = iter->second.get();
	}
}

void NFGE::App::Run(AppConfig appConfig)
{
	LOG("App starting ...");
	mAppConfig = std::move(appConfig);

	LOG("Creating window ...");
	mWindow.Initialize(GetModuleHandle(NULL), mAppConfig.appName.c_str(), mAppConfig.windowWidth, mAppConfig.windowHeight, mAppConfig.maximize, appConfig.appIcon);
	
	// Initialize input system TODO

	// Initialize grapics system
	Graphics::GraphicsSystem::StaticInitialize(mWindow, true, false, 0);
	Graphics::TextureManager::StaticInitialize(mAppConfig.assetsDirectory / "Images");
	mTimer.Initialize();

	// App system initialize finished
	mInitialized = true;

	// Initialize the starting state
	mCurrentState = mAppStates.begin()->second.get();
	mCurrentState->Initialize();
	
	auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
	graphicSystem->PrepareRender();

	bool done = false;
	while (!done)
	{
		done = mWindow.ProcessMessage();
		if (done)
			break;

		if (mNextState)
		{
			mCurrentState->Terminate();
			mCurrentState = std::exchange(mNextState, nullptr);
			mCurrentState->Initialize();
			graphicSystem->PrepareRender();
		}

		// ----------------------- Update ------------------------------
		// InputSystem update TODO
		mTimer.Update();
		mCurrentState->Update(GetDeltaTime());

		// ----------------------- Render ------------------------------
		graphicSystem->IncrementFrameCount();
		graphicSystem->BeginRender(NFGE::Graphics::RenderType::Direct);
		mCurrentState->Render();
		graphicSystem->EndRender(NFGE::Graphics::RenderType::Direct);
	}

	// Clean Up
	mCurrentState->Terminate();
	Graphics::TextureManager::StaticTerminate();
	Graphics::GraphicsSystem::StaticTerminate();
	// InputSystem terminate TODO
	mWindow.Terminate();
}

float NFGE::App::GetTime()
{
	return mTimer.GetTotalTime();
}

float NFGE::App::GetDeltaTime()
{
	return mTimer.GetElapsedTime();
}

Graphics::Camera& NFGE::App::GetMainCamera()
{
	if (mWorld != nullptr)
		return mWorld->GetService<NFGE::CameraService>()->GetActiveCamera();
	else
		return mVoidCamera;
}

Graphics::DirectionalLight& NFGE::App::GetMainLight()
{
	if (mWorld != nullptr)
		return mWorld->GetMainLight();
	else
		return mVoidLight;
}

void NFGE::App::SoSoCameraControl(float turnSpeed, float moveSpeed, NFGE::Graphics::Camera& camera, float deltaTime)
{
	//TODO
	//auto inputSystem = InputSystem::Get();
	//if (inputSystem->IsKeyDown(KeyCode::W))
	//	camera.Walk(moveSpeed * deltaTime);
	//if (inputSystem->IsKeyDown(KeyCode::S))
	//	camera.Walk(-moveSpeed * deltaTime);
	//if (inputSystem->IsKeyDown(KeyCode::D))
	//	camera.Strafe(moveSpeed * deltaTime);
	//if (inputSystem->IsKeyDown(KeyCode::A))
	//	camera.Strafe(-moveSpeed * deltaTime);
	//if (inputSystem->IsMouseDown(MouseButton::RBUTTON))
	//{
	//	camera.Yaw(inputSystem->GetMouseMoveX() * turnSpeed * deltaTime);
	//	camera.Pitch(-inputSystem->GetMouseMoveY() * turnSpeed * deltaTime);
	//}
}
void NFGE::App::SoSoCameraControl(float turnSpeed, float moveSpeed, CameraEntry& cameraEntry, float deltaTime)
{
	SoSoCameraControl(turnSpeed, moveSpeed, cameraEntry.camera, deltaTime);

	cameraEntry.mPosition = cameraEntry.camera.GetPosition();
	cameraEntry.mDirection = cameraEntry.camera.GetDirection();
}
