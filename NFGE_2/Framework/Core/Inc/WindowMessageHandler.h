//====================================================================================================
// Filename:	WindowMessageHandler.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#pragma once

namespace NFGE::Core {

	class WindowMessageHandler
	{
	public:
		using Callback = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);

		void Hook(HWND window, Callback cb);
		void Unhook();

		LRESULT ForwardMessage(HWND window, UINT messgae, WPARAM wparam, LPARAM lParam);

	private:
		HWND mWindow{ nullptr };
		Callback mPreviousCallback{ nullptr };
	};

}
