//====================================================================================================
// Filename:	Component.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================
#include "Precompiled.h"
#include "Component.h"
using namespace NFGE;

namespace
{
	std::unique_ptr<Component> sComponent;
}

const Component* NFGE::Component::StaticGetClass()
{
	sComponent = std::make_unique<Component>();
    return sComponent.get();
}
