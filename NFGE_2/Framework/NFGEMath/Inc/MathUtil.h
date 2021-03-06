//====================================================================================================
// Filename:	MathUtil.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#pragma once

namespace NFGE {
	namespace Math
	{
		constexpr float Epsilon = 1.0e-6f;

#define PI         3.1415926535897932384626433832795f
#define HalfPI     1.5707963267948966192313216916398f
#define TwoPI      2.0f*PI

#define DEG2RAD(x) ((x)*PI/180.0f)
#define RAD2DEG(x) (180.0f*(x)/PI)

		inline bool IsZero(float a) { return (fabsf(a) < Epsilon); }
	} // namespace Math
} // namespce NFGE