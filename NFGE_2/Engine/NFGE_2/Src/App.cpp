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
	
}

float NFGE::App::GetTime()
{
	return mTimer.GetTotalTime();
}

float NFGE::App::GetDeltaTime()
{
	return mTimer.GetElapsedTime();
}

