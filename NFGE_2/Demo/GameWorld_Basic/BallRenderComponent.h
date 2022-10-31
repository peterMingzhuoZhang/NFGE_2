#pragma once

#include "NFGE_2/Inc/NFGE_2.h"
using namespace NFGE;

class BallRenderComponent : public Component
{
public:
	RTTI_DELCEAR(BallRenderComponent)

	void Initialize() override;
	void Terminate() override;
	void Update(float deltaTime) override;
	void Render() override;
	void DebugUI() override;
private:
	Graphics::GeometryPX mGeometry;
};