#pragma once

namespace NFGE::Graphics
{
	class Texture;
	class CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice();
	CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type);
	uint8_t GetFrameCount();
	void Flush();
}