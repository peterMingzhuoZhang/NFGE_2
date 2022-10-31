//====================================================================================================
// Filename:	NFGE_2.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#pragma once

#include "Common.h"

// App headers
#include "App.h"
#include "AppState.h"

#include "GameObject.h"
#include "GameObjectFactory.h"
#include "World.h"

#include "Component.h"
#include "TransformComponent.h"

#include "Service.h"
#include "CameraService.h"

namespace NFGE { extern App sApp; }

namespace NFGEApp
{

	template<class T>
	void AddState(std::string name)
	{
		NFGE::sApp.AddState<T>(name);
	}

	void ChangeState(std::string name);
	void Run(NFGE::AppConfig appConfig);
	void ShutDown();

}