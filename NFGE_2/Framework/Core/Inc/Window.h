//====================================================================================================
// Filename:	Window.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#pragma once

namespace NFGE {
	namespace Core {

		class Window
		{
		public:
			Window() = default; //C++ 11

			// HINSTANCE: handle(OS create) -- pointer to process instance
			// LPCSTR:  
			// HWND: 
			void Initialize(HINSTANCE instance, LPCSTR appName, uint32_t width, uint32_t height, bool maximize = false);
			void Terminate();

			bool ProcessMessage();

			HWND GetWindowHandle() const { return mWindow; }

		private:
			//Default members initializtion
			HINSTANCE mInstance{ nullptr };
			HWND mWindow{ nullptr };
			std::string mAppName;

		};

	} // namepace Core
} // namespace NFGE