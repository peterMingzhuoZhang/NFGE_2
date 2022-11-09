//====================================================================================================
// Filename:	SpriteRender.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/11
//====================================================================================================

#include "Precompiled.h"
#include "SpriteRenderer.h"

#include "D3DUtil.h"
#include "GraphicsSystem.h"
#include "Texture.h"
#include "PipelineWorker.h"
#include <DirectXTK12/Inc/SpriteBatch.h>
#include <DirectXTK12/Inc/CommonStates.h>
#include <DirectXTK12/Inc/ResourceUploadBatch.h>
#include <DirectXTK12/Inc/GraphicsMemory.h>
#include "DirectXTK12/Inc/WICTextureLoader.h"

using namespace NFGE;
using namespace NFGE::Graphics;

namespace
{
	std::unique_ptr<SpriteRenderer> sSpriteRenderer = nullptr;

	namespace
	{
		DirectX::XMFLOAT2 GetOrigin(uint32_t width, uint32_t height, Pivot pivot)
		{
			auto index = static_cast<std::underlying_type_t<Pivot>>(pivot);
			constexpr DirectX::XMFLOAT2 offsets[] =
			{
				{ 0.0f, 0.0f }, // TopLeft
			{ 0.5f, 0.0f }, // Top
			{ 1.0f, 0.0f }, // TopRight
			{ 0.0f, 0.5f }, // Left
			{ 0.5f, 0.5f }, // Center
			{ 1.0f, 0.5f }, // Right
			{ 0.0f, 1.0f }, // BottomLeft
			{ 0.5f, 1.0f }, // Bottom
			{ 1.0f, 1.0f }, // BottomRight
			};
			return { width * offsets[index].x, height * offsets[index].y };
		}

		DirectX::XMFLOAT2 GetOrigin(uint32_t width, uint32_t height, const NFGE::Math::Vector2& pivot)
		{
			return { width * pivot.x, height * pivot.y };
		}

		DirectX::XMFLOAT2 ToXMFLOAT2(const Math::Vector2& v)
		{
			return { v.x, v.y };
		}
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
	
}

void NFGE::Graphics::SpriteRenderer::StaticInitialize()
{
	ASSERT(sSpriteRenderer == nullptr, "[SpriteRenderer] System already initlizlized!");
	sSpriteRenderer = std::make_unique<SpriteRenderer>();
	sSpriteRenderer->Initialize();
}

void NFGE::Graphics::SpriteRenderer::StaticTerminate()
{
	if (sSpriteRenderer != nullptr)
	{
		sSpriteRenderer->Terminate();
		sSpriteRenderer.reset();
	}
}

SpriteRenderer* NFGE::Graphics::SpriteRenderer::Get()
{
	ASSERT(sSpriteRenderer != nullptr, "[SpriteRenderer] No instance registered.");
	return sSpriteRenderer.get();
}

NFGE::Graphics::SpriteRenderer::~SpriteRenderer()
{
	ASSERT(mSpriteBatch == nullptr, "[SpriteRenderer] Renderer not freed.");
}

void NFGE::Graphics::SpriteRenderer::Initialize()
{

	ASSERT(mSpriteBatch == nullptr, "[SpriteRenderer] already initlizlized!");
	Microsoft::WRL::ComPtr<ID3D12Device2> device = Graphics::GetDevice();
	mGraphicsMemory = std::make_unique<DirectX::GraphicsMemory>(device.Get());

	DirectX::ResourceUploadBatch uploadBatch(device.Get());
	uploadBatch.Begin(D3D12_COMMAND_LIST_TYPE_DIRECT);

	//const RenderTarget& renderTarge = GraphicsSystem::Get()->GetRenderTarget();
	auto rtFormat = GraphicsSystem::Get()->GetBackBufferFormat();
	auto dsFormat = GraphicsSystem::Get()->GetDepthBufferFormat();
	DirectX::SpriteBatchPipelineStateDescription desc(DirectX::RenderTargetState(rtFormat, dsFormat));
	mSpriteBatch = std::make_unique<DirectX::SpriteBatch>(device.Get(), uploadBatch, desc);
	mCommonStates = std::make_unique<DirectX::CommonStates>(device.Get());

	auto uploadBatchFinished = uploadBatch.End(Graphics::GetWorker(WorkerType::Direct)->GetCommandQueue()->GetD3D12CommandQueue().Get());
	uploadBatchFinished.wait();

}

void NFGE::Graphics::SpriteRenderer::Terminate()
{
	ASSERT(mSpriteBatch != nullptr, "[SpriteRenderer] already terminate!");
	mSpriteBatch.reset();
}

void NFGE::Graphics::SpriteRenderer::RegisterSpriteTexture(Texture& texture)
{
	mRegisterSprite.push_back(&texture);
}

void NFGE::Graphics::SpriteRenderer::PrepareRender()
{
	mSpriteTextureDescriptorHeap.push_back(Graphics::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mRegisterSprite.size()));

	Microsoft::WRL::ComPtr<ID3D12Device2> device = Graphics::GetDevice();
	DirectX::ResourceUploadBatch uploadBatch(device.Get());
	uploadBatch.Begin(D3D12_COMMAND_LIST_TYPE_DIRECT);
	
	for (auto texture: mRegisterSprite)
	{
		uint32_t index = mSpriteTextureIndex++;
		texture->SetQuickTexture(index, mSpriteTextureGeneration);
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = texture->GetD3D12Resource();
		ThrowIfFailed(DirectX::CreateWICTextureFromFile(device.Get(), uploadBatch, texture->GetName().c_str(), resource.ReleaseAndGetAddressOf()));

		texture->CreateSpriteTextureShaderResourceView(mSpriteTextureDescriptorHeap[HeapType::RESOURCE], index);
	}
}

void NFGE::Graphics::SpriteRenderer::BeginRender()// TODO rendertarget argument
{
	ASSERT(mSpriteBatch != nullptr, "[SpriteRenderer] Not initliazed.");
	ASSERT(mAssignedWorker == nullptr, "[SpriteRenderer] Not Clean.");
	mAssignedWorker = NFGE::Graphics::GetWorker(NFGE::Graphics::WorkerType::Direct);

	
	//mAssignedWorker->BeginWork();
	//mAssignedWorker->SetRenderTarget(GraphicsSystem::Get()->GetRenderTarget());
	//mDirectWorker->SetViewport(mMasterRenderTarget.GetViewport());TODO

	std::vector<ID3D12DescriptorHeap*> heaps;
	for (size_t i = 0; i < mSpriteTextureDescriptorHeap.size(); i++)
	{
		heaps.push_back(mSpriteTextureDescriptorHeap[i].Get());
	}
	mAssignedWorker->GetGraphicsCommandList().Get()->SetDescriptorHeaps(heaps.size(), heaps.data());
	

	auto commandList = mAssignedWorker->GetGraphicsCommandList().Get();
	mSpriteBatch->SetViewport(GraphicsSystem::Get()->GetRenderTarget().GetViewport());
	mSpriteBatch->Begin(commandList, DirectX::SpriteSortMode::SpriteSortMode_Deferred);
}

void NFGE::Graphics::SpriteRenderer::EndRender()
{
	ASSERT(mSpriteBatch != nullptr, "[SpriteRenderer] Not initliazed.");
	ASSERT(mAssignedWorker != nullptr, "[SpriteRenderer] No worker assigned.");
	mSpriteBatch->End();
	mAssignedWorker = nullptr;
}

void NFGE::Graphics::SpriteRenderer::ReleaseGraphicsMemory()
{
	mGraphicsMemory->Commit(Graphics::GetWorker(WorkerType::Direct)->GetCommandQueue()->GetD3D12CommandQueue().Get());
}

void NFGE::Graphics::SpriteRenderer::Reset()
{
	mRegisterSprite.clear();
	mSpriteTextureDescriptorHeap.clear();
	mSpriteTextureIndex = 0;
	mSpriteTextureGeneration++;
}

//----------------------------------------------------------------------------------------------------
void NFGE::Graphics::SpriteRenderer::Draw(const Texture& texture, const Math::Vector2& pos, float rotation, Pivot pivot)
{
	ASSERT(mSpriteBatch != nullptr, "[SpriteRenderer] Not initialized.");
	uint32_t width = texture.GetWidth();
	uint32_t height = texture.GetHeight();
	DirectX::XMFLOAT2 origin = GetOrigin(width, height, pivot);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = GetQuickTextureGPUHandle(texture);
	//worker->DrawCommit();
	mSpriteBatch->Draw(gpuHandle,DirectX::XMUINT2(width,height), ToXMFLOAT2(pos), nullptr, DirectX::Colors::White, rotation, origin);
}

//----------------------------------------------------------------------------------------------------
void NFGE::Graphics::SpriteRenderer::Draw(const Texture& texture, const Math::Rect& sourceRect, const Math::Vector2& pos, float rotation, Pivot pivot)
{
	ASSERT(mSpriteBatch != nullptr, "[SpriteRenderer] Not initialized.");
	RECT rect;
	rect.left = static_cast<LONG>(sourceRect.left);
	rect.top = static_cast<LONG>(sourceRect.top);
	rect.right = static_cast<LONG>(sourceRect.right);
	rect.bottom = static_cast<LONG>(sourceRect.bottom);
	DirectX::XMFLOAT2 origin = GetOrigin(rect.right - rect.left, rect.bottom - rect.top, pivot);
	uint32_t width = texture.GetWidth();
	uint32_t height = texture.GetHeight();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	gpuHandle.ptr = texture.GetD3D12Resource()->GetGPUVirtualAddress();
	mSpriteBatch->Draw(gpuHandle, DirectX::XMUINT2(width, height), ToXMFLOAT2(pos), &rect, DirectX::Colors::White, rotation, origin);
}

void NFGE::Graphics::SpriteRenderer::Draw(const Texture& texture, const Math::Vector2& pos, float rotation, float alpha)
{
	ASSERT(mSpriteBatch != nullptr, "[SpriteRenderer] Not initialized.");
	uint32_t width = texture.GetWidth();
	uint32_t height = texture.GetHeight();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	gpuHandle.ptr = texture.GetD3D12Resource()->GetGPUVirtualAddress();
	DirectX::XMFLOAT2 origin{ width * 0.5f, height * 0.5f };
	mSpriteBatch->Draw(gpuHandle, DirectX::XMUINT2(width, height), *(DirectX::XMFLOAT2*)&pos, nullptr, DirectX::Colors::White * alpha, rotation, origin);
}

//---------------------- Mingzhuo zhang added draw with alpha & anchor point ---------------------
void NFGE::Graphics::SpriteRenderer::Draw(const Texture& texture, const Math::Vector2& pos, float rotation, float alpha, float anchorX, float anchorY, float xScale, float yScale)
{
	ASSERT(mSpriteBatch != nullptr, "[SpriteRenderer] Not initialized.");
	uint32_t width = texture.GetWidth();
	uint32_t height = texture.GetHeight();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	gpuHandle.ptr = texture.GetD3D12Resource()->GetGPUVirtualAddress();
	DirectX::XMFLOAT2 origin{ width * anchorX, height * anchorY };
	DirectX::XMFLOAT2 scale{ xScale, yScale };
	mSpriteBatch->Draw(gpuHandle, DirectX::XMUINT2(width, height), *(DirectX::XMFLOAT2*)&pos, nullptr, DirectX::Colors::White * alpha, rotation, origin, scale);
}

//---------------------- Mingzhuo zhang added draw with alpha & anchor point & Source rect ---------------------
void NFGE::Graphics::SpriteRenderer::Draw(const Texture& texture, const Math::Rect& sourceRect, const Math::Vector2& pos, float rotation, float alpha, float anchorX, float anchorY, float xScale, float yScale)
{
	ASSERT(mSpriteBatch != nullptr, "[SpriteRenderer] Not initialized.");
	//DirectX::XMFLOAT2 origin{ texture.GetWidth() * anchorX, texture.GetHeight() * anchorY };
	DirectX::XMFLOAT2 scale{ xScale, yScale };
	RECT rect;
	rect.left = static_cast<LONG>(sourceRect.left);
	rect.top = static_cast<LONG>(sourceRect.top);
	rect.right = static_cast<LONG>(sourceRect.right);
	rect.bottom = static_cast<LONG>(sourceRect.bottom);
	DirectX::XMFLOAT2 origin = GetOrigin(rect.right - rect.left, rect.bottom - rect.top, { anchorX ,anchorY });
	uint32_t width = texture.GetWidth();
	uint32_t height = texture.GetHeight();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	gpuHandle.ptr = texture.GetD3D12Resource()->GetGPUVirtualAddress();
	mSpriteBatch->Draw(gpuHandle, DirectX::XMUINT2(width, height), *(DirectX::XMFLOAT2*)&pos, &rect, DirectX::Colors::White * alpha, rotation, origin, scale);
}

D3D12_GPU_DESCRIPTOR_HANDLE NFGE::Graphics::SpriteRenderer::GetQuickTextureGPUHandle(const Texture& texture)
{
	uint32_t index = texture.GetQuickTextureID();
	uint32_t gen = texture.GetQuickTextureGen();
	ASSERT(gen == mSpriteTextureGeneration && index < mSpriteTextureDescriptorHeap.size(), "[SpriteRender] the texture is not register yet.");

	ComPtr<ID3D12DescriptorHeap> descriptorHeap = mSpriteTextureDescriptorHeap[HeapType::RESOURCE];
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto incrementSize = NFGE::Graphics::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	gpuHandle.ptr += index * incrementSize;

	return gpuHandle;
}
