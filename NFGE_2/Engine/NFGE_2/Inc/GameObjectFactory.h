//====================================================================================================
// Filename:	GameObjectFactory.h
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#pragma once

#include "GameObject.h"

namespace NFGE
{
	class GameObjectFactory
	{
	public:
		GameObjectFactory(GameObjectAllocator& allocator);

		GameObject* CreateEmpty();
		void Destory(GameObject* gameObject);

	private:
		GameObjectAllocator& mGameObjectAllocator;
	};
}
