//====================================================================================================
// Filename:	Component.h
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#pragma once

namespace NFGE
{
	class GameObject;
	class Component
	{
	public:
		static const Component* StaticGetClass();
		virtual const Component* GetClass() const { return StaticGetClass(); }

		virtual ~Component() = default;

		virtual void Initialize() {}
		virtual void Terminate() {}

		virtual void Update(float deltaTime) {}
		virtual void Render() {}
		virtual void DebugUI() {}

		GameObject& GetOwner() { return *mOwner; }
		const GameObject& GetOwner() const { return *mOwner; }

	private:
		friend class GameObject;
		GameObject* mOwner = nullptr;
	};
}
