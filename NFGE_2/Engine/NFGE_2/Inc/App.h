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
	struct CameraEntry;
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
		int appIcon{ 0 };
		std::filesystem::path assetsDirectory = L"../../Assets";
		uint32_t windowWidth = 1920;//1280;
		uint32_t windowHeight = 1080;//720;
		bool maximize = false;
		bool imGuiDocking = false;
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

		NFGE::Graphics::Camera& GetMainCamera();
		NFGE::Graphics::DirectionalLight& GetMainLight();
		void SoSoCameraControl(float turnSpeed, float moveSpeed, NFGE::Graphics::Camera& camera, float deltaTime);
		void SoSoCameraControl(float turnSpeed, float moveSpeed, CameraEntry& camera, float deltaTime);

		void SetWorld(World& world) { mWorld = &world; };

		// Sprites Functions
		void DrawSprite(Graphics::TextureId textureId, const Math::Vector2& position);
		void DrawSprite(Graphics::TextureId textureId, const Math::Vector2& position, float rotation, Graphics::Pivot pivot = Graphics::Pivot::Center);
		void DrawSprite(Graphics::TextureId textureId, const Math::Rect& sourceRect, const Math::Vector2& position);
		void DrawSprite(Graphics::TextureId textureId, const Math::Vector2& position, float rotation, float alpha);
		void DrawSprite(Graphics::TextureId textureId, const Math::Vector2& position, float rotation, float alpha, float anchorX, float anchorY, float xScale, float yScale, Math::Rect = Math::Rect{});
		uint32_t GetSpriteWidth(Graphics::TextureId textureId);
		uint32_t GetSpriteHeight(Graphics::TextureId textureId);

	private:
		AppConfig mAppConfig;
		Core::Window mWindow;
		std::map<std::string, std::unique_ptr<AppState>> mAppStates;
		AppState* mCurrentState = nullptr;
		AppState* mNextState = nullptr;

		NFGE::World* mWorld = nullptr;
		NFGE::Graphics::Camera mVoidCamera;
		NFGE::Graphics::DirectionalLight mVoidLight;

		NFGE::Timer mTimer;
		bool mInitialized = false;
		std::vector<NFGE::Graphics::SpriteCommand> mySpriteCommands;

		void ExecuteSpriteCommand();
	};

	template<class StateType>
	void App::AddState(std::string name)
	{
		mAppStates.emplace(std::move(name), std::make_unique<StateType>());
	}
}