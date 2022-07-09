//====================================================================================================
// Filename:	NFGE_2.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#include "Precompiled.h"
#include "NFGE_2.h"
#include "App.h"

NFGE::App NFGE::sApp;

void NFGEApp::ChangeState(std::string name)
{
	NFGE::sApp.ChangeState(std::move(name));
}

void NFGEApp::Run(NFGE::AppConfig appConfig)
{
	NFGE::sApp.Run(std::move(appConfig));
}

void NFGEApp::ShutDown()
{
	PostQuitMessage(0);
}