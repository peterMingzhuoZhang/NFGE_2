//====================================================================================================
// Filename:	GameObjectFactory.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#include "Precompiled.h"
#include "GameObjectFactory.h"

#include "Component.h"

using namespace NFGE;

NFGE::GameObjectFactory::GameObjectFactory(GameObjectAllocator& allocator)
	: mGameObjectAllocator(allocator)
{
}

GameObject* NFGE::GameObjectFactory::CreateEmpty()
{
	return mGameObjectAllocator.New();
}

void NFGE::GameObjectFactory::Destory(GameObject* gameObject)
{
	mGameObjectAllocator.Delete(gameObject);
}
