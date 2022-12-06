//====================================================================================================
// Filename:	Debug.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#pragma once

#if defined(_DEBUG)

#define LOG(format, ...)\
		do {\
			char buffer[4096];\
			int ret = _snprintf_s(buffer, std::size(buffer), _TRUNCATE, "%s(%d) "##format, __FILE__, __LINE__, __VA_ARGS__);\
			OutputDebugStringA(buffer);\
			if (ret == -1)\
				OutputDebugStringA("** message truncated **\n");\
			OutputDebugStringA("\n");\
		}while (false)


#define ASSERT(condition, format, ...)\
		do {\
			if (!(condition))\
			{\
				LOG(format, __VA_ARGS__);\
				DebugBreak();\
			}\
		}while (false)
#else
#define LOG(format, ...)
#define ASSERT(condition, format, ...)
#endif

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

inline void ThrowIfFailed(HRESULT hr, const wchar_t* msg)
{
	if (FAILED(hr))
	{
		OutputDebugString(msg);
		throw std::exception();
	}
}

inline UINT Align(UINT size, UINT alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}