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
	
	HWND handle = mWindow.GetWindowHandle();
	Input::InputSystem::StaticInitialize(handle);
	// Initialize grapics system
	Graphics::GraphicsSystem::StaticInitialize(mWindow, true, false, 0);
	Graphics::TextureManager::StaticInitialize(mAppConfig.assetsDirectory / "Images");
	Graphics::SpriteRenderer::StaticInitialize();
	Graphics::SimpleDraw::StaticInitialize(mAppConfig.assetsDirectory / "Shaders");
	mTimer.Initialize();

	// App system initialize finished
	mInitialized = true;

	// Initialize the starting state
	mCurrentState = mAppStates.begin()->second.get();
	
	auto graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
	graphicSystem->BeginUpload();
	mCurrentState->Initialize();
	graphicSystem->EndUpload();
	Graphics::SpriteRenderer::Get()->PrepareRender();

	bool done = false;
	while (!done)
	{
		done = mWindow.ProcessMessage();
		if (done)
			break;

		if (mNextState)
		{
			graphicSystem->Reset();
			Graphics::SpriteRenderer::Get()->Reset();
			mCurrentState->Terminate();
			mCurrentState = std::exchange(mNextState, nullptr);
			graphicSystem->BeginUpload();
			mCurrentState->Initialize();
			graphicSystem->EndUpload();
			Graphics::SpriteRenderer::Get()->PrepareRender();
		}

		// ----------------------- Update ------------------------------
		Input::InputSystem::Get()->Update();
		mTimer.Update();
		mCurrentState->Update(GetDeltaTime());

		// ----------------------- Render ------------------------------
		graphicSystem->IncrementFrameCount();

		graphicSystem->BeginMasterRender();
		{
			mCurrentState->Render();

			Graphics::SpriteRenderer::Get()->BeginRender();
			ExecuteSpriteCommand();
			Graphics::SpriteRenderer::Get()->EndRender();

			graphicSystem->BeginUpload();
			mCurrentState->DebugUI();
			Graphics::SimpleDraw::RenderUpdate();
			graphicSystem->EndUpload();

			Graphics::SimpleDraw::Render(GetMainCamera());
		}
		graphicSystem->EndMasterRender();

		graphicSystem->Present();
	}

	// Clean Up
	mCurrentState->Terminate();
	Graphics::SimpleDraw::StaticTerminate();
	Graphics::SpriteRenderer::StaticTerminate();
	Graphics::TextureManager::StaticTerminate();
	Graphics::GraphicsSystem::StaticTerminate();
	Input::InputSystem::StaticTerminate();
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
	auto inputSystem = Input::InputSystem::Get();
	if (inputSystem->IsKeyDown(Input::KeyCode::W))
		camera.Walk(moveSpeed * deltaTime);
	if (inputSystem->IsKeyDown(Input::KeyCode::S))
		camera.Walk(-moveSpeed * deltaTime);
	if (inputSystem->IsKeyDown(Input::KeyCode::D))
		camera.Strafe(moveSpeed * deltaTime);
	if (inputSystem->IsKeyDown(Input::KeyCode::A))
		camera.Strafe(-moveSpeed * deltaTime);
	if (inputSystem->IsMouseDown(Input::MouseButton::RBUTTON))
	{
		camera.Yaw(inputSystem->GetMouseMoveX() * turnSpeed * deltaTime);
		camera.Pitch(-inputSystem->GetMouseMoveY() * turnSpeed * deltaTime);
	}
}
void NFGE::App::SoSoCameraControl(float turnSpeed, float moveSpeed, CameraEntry& cameraEntry, float deltaTime)
{
	SoSoCameraControl(turnSpeed, moveSpeed, cameraEntry.camera, deltaTime);

	cameraEntry.mPosition = cameraEntry.camera.GetPosition();
	cameraEntry.mDirection = cameraEntry.camera.GetDirection();
}

void NFGE::App::DrawSprite(Graphics::TextureId textureId, const Math::Vector2& position)
{
	ASSERT(mInitialized, "[XEngine] Engine not started.");
	mySpriteCommands.emplace_back(textureId, position, 0.0f);
}

void NFGE::App::DrawSprite(Graphics::TextureId textureId, const Math::Vector2& position, float rotation, Graphics::Pivot pivot)
{
	ASSERT(mInitialized, "[XEngine] Engine not started.");
	mySpriteCommands.emplace_back(textureId, position, rotation, pivot);
}

void NFGE::App::DrawSprite(Graphics::TextureId textureId, const Math::Rect& sourceRect, const Math::Vector2& position)
{
	ASSERT(mInitialized, "[XEngine] Engine not started.");
	mySpriteCommands.emplace_back(textureId, sourceRect, position, 0.0f);
}

void NFGE::App::DrawSprite(Graphics::TextureId textureId, const Math::Vector2& position, float rotation, float alpha)
{
	ASSERT(mInitialized, "[XEngine] Engine not started.");
	mySpriteCommands.emplace_back(textureId, position, rotation, alpha);
}

void NFGE::App::DrawSprite(Graphics::TextureId textureId, const Math::Vector2& position, float rotation, float alpha, float anchorX, float anchorY, float xScale, float yScale, Math::Rect rect)
{
	ASSERT(mInitialized, "[XEngine] Engine not started.");
	mySpriteCommands.emplace_back(textureId, position, rotation, alpha, anchorX, anchorY, xScale, yScale, rect);
}

uint32_t NFGE::App::GetSpriteWidth(Graphics::TextureId textureId)
{
	Graphics::Texture* texture = Graphics::TextureManager::Get()->GetTexture(textureId);
	return texture ? texture->GetWidth() : 0u;
}

uint32_t NFGE::App::GetSpriteHeight(Graphics::TextureId textureId)
{
	Graphics::Texture* texture = Graphics::TextureManager::Get()->GetTexture(textureId);
	return texture ? texture->GetHeight() : 0u;
}

void NFGE::App::ExecuteSpriteCommand()
{
	for (const auto& command : mySpriteCommands)
	{
		Graphics::Texture* texture = Graphics::TextureManager::Get()->GetTexture(command.textureId);
		if (texture)
		{
			if (command.alpha == 1.0f)
			{
				if (Math::IsEmpty(command.sourceRect))
				{
					if (command.anchorX != FLT_MIN || command.anchorY != FLT_MIN)
					{
						Graphics::SpriteRenderer::Get()->Draw(*texture, command.position, command.rotation, command.alpha, command.anchorX, command.anchorY, command.xScale, command.yScale);
					}
					else
					{
						Graphics::SpriteRenderer::Get()->Draw(*texture, command.position, command.rotation, command.pivot);
					}
				}
				else
				{
					if (command.anchorX != FLT_MIN || command.anchorY != FLT_MIN)
					{
						Graphics::SpriteRenderer::Get()->Draw(*texture, command.sourceRect, command.position, command.rotation, command.alpha, command.anchorX, command.anchorY, command.xScale, command.yScale);
					}
					else
					{
						Graphics::SpriteRenderer::Get()->Draw(*texture, command.sourceRect, command.position, command.rotation, command.pivot);
					}
				}
			}
			else
			{
				if (command.anchorX != FLT_MIN || command.anchorY != FLT_MIN)
				{
					Graphics::SpriteRenderer::Get()->Draw(*texture, command.position, command.rotation, command.alpha, command.anchorX, command.anchorY, command.xScale, command.yScale);
				}
				else
				{
					Graphics::SpriteRenderer::Get()->Draw(*texture, command.position, command.rotation, command.alpha);
				}
			}
		}
	}
	mySpriteCommands.clear();
}
