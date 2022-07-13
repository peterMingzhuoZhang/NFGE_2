//====================================================================================================
// Filename:	PerlinNoise.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
// Resources:	https://solarianprogrammer.com/2012/07/18/perlin-noise-cpp-11/
//				https://flafla2.github.io/2014/08/09/perlinnoise.html
//				https://www.redblobgames.com/maps/terrain-from-noise/
//====================================================================================================

#pragma once

namespace NFGE::Math {
	class PerlinNoise
	{
	public:
		PerlinNoise();
		PerlinNoise(uint32_t seed);

		float Get(float x, float y, float z);

	private:
		float Fade(float t);
		float Grad(int hash, float x, float y, float z);

		std::vector<int> p;
	};
}
