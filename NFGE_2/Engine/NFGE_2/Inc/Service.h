//====================================================================================================
// Filename:	Service.h
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#pragma once

namespace NFGE
{
	class World;

	class Service
	{
	public:
		static const Service* StaticGetClass();
		virtual const Service* GetClass() const { return StaticGetClass(); }

		Service() = default;
		virtual ~Service() = default;

		virtual void Initialize() {}
		virtual void Terminate() {}

		virtual void Update(float deltaTime) {}
		virtual void Render() {}
		virtual void WorldViewUI() {}
		virtual void DebugUI() {}

		World& GetWorld() { return *mWorld; }
		const World& GetWorld() const { return *mWorld; }

	private:
		friend class World;
		World* mWorld = nullptr;
	};
}