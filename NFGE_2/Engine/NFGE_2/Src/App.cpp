//====================================================================================================
// Filename:	App.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#include "Precompiled.h"
#include "App.h"
#include "../Resource/resource.h" // for window icon

#include "AppState.h"

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
	mWindow.Initialize(GetModuleHandle(NULL), mAppConfig.appName.c_str(), mAppConfig.windowWidth, mAppConfig.windowHeight, mAppConfig.maximize);
	
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
		}

		// ----------------------- Update ------------------------------
		// InputSystem update TODO
		mTimer.Update();
		mCurrentState->Update(GetDeltaTime());

		// ----------------------- Render ------------------------------
		auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
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

