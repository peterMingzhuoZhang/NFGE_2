//====================================================================================================
// Filename:	TextureMeshEffect.hlsl
// Created by:	Mingzhuo Zhang
// Description:	Simple shader for applying diffuse texture to TextureMesh.
//====================================================================================================

cbuffer WVP : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}

struct VS_INPUT
{
	float3 position	: POSITION;
	float4 color	: COLOR;
};

struct VS_OUTPUT
{
	float4 position	: SV_POSITION;
	float4 color	: COLOR;
};

VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.position = mul(float4(input.position.xyz, 1.0f), World);
	output.position = mul(output.position, View);
	output.position = mul(output.position, Projection);
	output.color = input.color;
	return output;
}

float4 PS(VS_OUTPUT input) : SV_Target
{
	return input.color;
}