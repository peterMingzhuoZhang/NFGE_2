#pragma once

#include <NFGE_2/Inc/NFGE_2.h>

using namespace NFGE;

class GameState : public AppState
{
public:
	void Initialize() override;
	void Terminate() override;
	void Update(float deltaTime) override;
	void Render() override;
	void DebugUI() override;
private:
	NFGE::World mWorld;

	GameObjectHandle mGameObjectHandle;
	Graphics::TextureId mLogo;
};