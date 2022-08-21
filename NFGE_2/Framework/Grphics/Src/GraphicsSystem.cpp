//====================================================================================================
// Filename:	GraphicsSystem.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/8
//====================================================================================================

#include "GraphicsSystem.h"

using namespace NFGE;
using namespace NFGE::Graphics;

// Internal linkage
namespace 
{
	std::unique_ptr<GraphicsSystem> sGraphicsSystem;
	Core::WindowMessageHandler sWindowMessageHandler;
}

LRESULT CALLBACK Graphics::GraphicsSystemMessageHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (sGraphicsSystem)
	{
		switch (message)
		{
		case WM_SIZE:
		{
			const uint32_t width = static_cast<uint32_t>(LOWORD(lParam));
			const uint32_t height = static_cast<uint32_t>(HIWORD(lParam));
			sGraphicsSystem->Resize(width, height);
			break;
		}
		}
	}
	return sWindowMessageHandler.ForwardMessage(window, message, wParam, lParam);
}

void GraphicsSystem::StaticInitialize(HWND window, bool fullscreen)
{
	ASSERT(sGraphicsSystem == nullptr, "[Graphics::GraphicsSystem] System already initialized!");
	sGraphicsSystem = std::make_unique<GraphicsSystem>();
	sGraphicsSystem->Initialize(window, fullscreen);
}

void GraphicsSystem::StaticTerminate()
{
	if (sGraphicsSystem != nullptr)
	{
		sGraphicsSystem->Terminate();
		sGraphicsSystem.reset();
	}
}

GraphicsSystem* GraphicsSystem::Get()
{
	ASSERT(sGraphicsSystem != nullptr, "[Graphics::graphicsSystem] No system registered.");
	return sGraphicsSystem.get();
}

GraphicsSystem::~GraphicsSystem()
{
	//ASSERT(mD3ddDevice == nullptr, "[Graphics::GraphicsSystem] Terminate() must be called to clean up!");
}

void GraphicsSystem::Initialize(HWND window, bool fullscreen)
{
	//TODO
}

void GraphicsSystem::Terminate()
{
	// Restore original windows procedure
	sWindowMessageHandler.Unhook();

	//TODO
}

void GraphicsSystem::BeginRender()
{
	//TODO
}

void GraphicsSystem::EndRender()
{
	//TODO
}

void GraphicsSystem::ToggleFullscreen()
{
	//TODO
}

void NFGE::Graphics::GraphicsSystem::Resize(uint32_t width, uint32_t height)
{
	//TODO
}

void NFGE::Graphics::GraphicsSystem::ResetRenderTarget()
{
	//TODO
}

void NFGE::Graphics::GraphicsSystem::ResetViewport()
{
	//TODO
}

uint32_t NFGE::Graphics::GraphicsSystem::GetBackBufferWidth() const
{
	//TODO
}

uint32_t NFGE::Graphics::GraphicsSystem::GetBackBufferHeight() const
{
	//TODO
}