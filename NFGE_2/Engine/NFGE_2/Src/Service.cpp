//====================================================================================================
// Filename:	Service.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#include "Precompiled.h"
#include "Service.h"

using namespace NFGE;

namespace
{
	std::unique_ptr<Service> sService;
}

const Service* NFGE::Service::StaticGetClass()
{
	sService = std::make_unique<Service>();
	return sService.get();
}
