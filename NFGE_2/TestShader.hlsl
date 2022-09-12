//====================================================================================================
// Filename:	TestShader.fx
// Created by:	Mingzhuo Zhang
// Description: Shader that applies transformation matrices.
// Date:		2022/9
//====================================================================================================

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB: register(b0);

struct VertexPC
{
	float3 Position: POSITION;
	float3 Color : COLOR;
};

struct VSOutput
{
	float4 Color    : COLOR;
	float4 Position : SV_Position;
};

struct ModelViewProjection
{
	matrix MVP;
};


VertexShaderOutput main(VertexPC IN)
{
	VSOutput OUT;

	OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
	OUT.Color = float4(IN.Color, 1.0f);

	return OUT;
}

struct PixelShaderInput
{
	float4 Color : COLOR;
};

float4 main(PixelShaderInput IN) : SV_Target
{
	return IN.Color;
}