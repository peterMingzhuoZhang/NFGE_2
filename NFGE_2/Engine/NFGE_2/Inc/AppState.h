//====================================================================================================
// Filename:	appState.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#pragma once

namespace NFGE
{
	class AppState
	{
	public:
		virtual void Initialize() = 0;
		virtual void Terminate() = 0;

		virtual void Update(float deltaTime) = 0;
		virtual void Render() = 0;
		virtual void DebugUI() = 0;

	private:

	};
}