//====================================================================================================
// Filename:	Common.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// Win32 headers
#include <objbase.h>
#include <Windows.h>
#include <Commdlg.h>

// Standard headers
#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <list>
#include <optional>
#include <map>
#include <memory>
#include <new>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>

// External headers
#include <RapidJSON/Inc/document.h>
#include <RapidJSON/Inc/filereadstream.h>
#include <RapidJSON/Inc/filewritestream.h>

// Utility function to release D3D interfaces
template<class T>
inline void SafeRelease(T*& ptr)
{
	if (ptr)
	{
		ptr->Release();
		ptr = nullptr;
	}
}

template <typename T>
inline void SafeDelete(T*& ptr)
{
	delete ptr;
	ptr = nullptr;
}

//----------------------------------------------------------------------------------------------------

template <typename T>
inline void SafeDeleteArray(T*& ptr)
{
	delete[] ptr;
	ptr = nullptr;
}