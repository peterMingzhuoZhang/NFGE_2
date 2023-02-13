#pragma once

namespace GlobalRootSignatureParams {
	enum Value {
		OutputViewSlot = 0,
		AccelerationStructureSlot,
        SceneConstantSlot,
        IndexBuffersSlot,
        VertexBuffersSlot,
		Count
	};
}

namespace LocalRootSignatureParams {
	enum Value {
		MaterialConstantSlot = 0,
		Count
	};
}

struct Viewport
{
    float left;
    float top;
    float right;
    float bottom;
};

struct RayGenConstantBuffer
{
    Viewport viewport;
    Viewport stencil;
};

#ifdef HLSL
typedef float3 XMFLOAT3;
typedef float4 XMFLOAT4;
typedef float4 XMVECTOR;
typedef float4x4 XMMATRIX;
typedef uint UINT;
#else
using namespace DirectX;
#endif


struct SceneConstantBuffer
{
    XMMATRIX projectionToWorld;
    XMVECTOR cameraPosition;
    XMVECTOR lightPosition;
    XMVECTOR lightAmbientColor;
    XMVECTOR lightDiffuseColor;
};

struct MaterialConstantBuffer
{
    XMFLOAT4 albedo;
};



struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
};