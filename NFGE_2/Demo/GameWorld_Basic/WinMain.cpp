//====================================================================================================
// Filename:	WinMain.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#include "GameState.h"
#include "resource.h"

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	NFGE::AppConfig config;
	config.appName = "GameWorld Basic";
	config.appIcon = IDI_ICON1;
	config.maximize = true;
	config.imGuiDocking = true;

	NFGEApp::AddState<GameState>("State_0");
	NFGEApp::Run(config);
	return 0;
}