//====================================================================================================
// Filename:	App.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#pragma once

#include "Timer.h"

namespace NFGE
{
	class AppState;
	class World;
	class CameraEntry;
	struct AppConfig
	{
		AppConfig() = default;
		AppConfig(std::string appName)
			: appName(std::move(appName))
		{}
		AppConfig(std::string appName, std::filesystem::path path)
			: appName(std::move(appName))
			, assetsDirectory(path)
		{}

		std::string appName = "Nicolas Four Game Engine";
		std::filesystem::path assetsDirectory = L"../../Assets";
		uint32_t windowWidth = 1920;//1280;
		uint32_t windowHeight = 1080;//720;
		bool maximize = false;
		bool imGuiDocking = false;
		bool isEditor = false;
	};

	class App
	{
	public:

		template<class StateType>
		void AddState(std::string name);
		void ChangeState(std::string name);

		void Run(AppConfig appConfig);

		// Time Functions
		float GetTime();
		float GetDeltaTime();

		

		void SetWorld(World& world) { mWorld = &world; };
	private:
		AppConfig mAppConfig;

		std::map<std::string, std::unique_ptr<AppState>> mAppStates;;
		AppState* mCurrentState = nullptr;
		AppState* mNextState = nullptr;

		NFGE::World* mWorld = nullptr;

		NFGE::Timer myTimer;
		bool initialized = false;

	};

	template<class StateType>
	void App::AddState(std::string name)
	{
		mAppStates.emplace(std::move(name), std::make_unique<StateType>());
	}
}