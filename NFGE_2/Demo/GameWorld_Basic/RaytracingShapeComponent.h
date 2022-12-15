#pragma once
#include "NFGE_2/Inc/NFGE_2.h"

using namespace NFGE;
class RayTracingShapeComponent : public Component
{
public:
	RTTI_DELCEAR(RayTracingShapeComponent)

	void Initialize() override;
	void Terminate() override;
	void Update(float deltaTime) override;
	void Render() override;
	void DebugUI() override;
private:
	Graphics::GeometryRaytracing mGeometry;
};
