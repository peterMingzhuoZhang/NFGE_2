//====================================================================================================
// Filename:	RenderTaget.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#include "Precompiled.h"
#include "RenderTarget.h"

using namespace NFGE::Graphics;

const size_t RenderTarget::sMaxRendertarget = 8;

RenderTarget::RenderTarget()
    : mTextures(AttachmentPoint::NumAttachmentPoints)
    , mSize(0,0)
{}

// Attach a texture to the render target.
// The texture will be copied into the texture array.
void RenderTarget::AttachTexture(AttachmentPoint attachmentPoint, const Texture& texture)
{
    mTextures[attachmentPoint] = texture;
    
    if (texture.GetD3D12Resource())
    {
        auto desc = texture.GetD3D12Resource()->GetDesc();
        mSize.x = static_cast<uint32_t>(desc.Width);
        mSize.y = static_cast<uint32_t>(desc.Height);
    }
}

const Texture& RenderTarget::GetTexture(AttachmentPoint attachmentPoint) const
{
    return mTextures[attachmentPoint];
}

DirectX::XMUINT2 RenderTarget::GetSize() const
{
    return mSize;
}

uint32_t RenderTarget::GetWidth() const
{
    return mSize.x;
}

uint32_t RenderTarget::GetHeight() const
{
    return mSize.y;
}

D3D12_VIEWPORT RenderTarget::GetViewport(DirectX::XMFLOAT2 scale, DirectX::XMFLOAT2 bias, float minDepth, float maxDepth) const
{
    UINT64 width = 0;
    UINT height = 0;

    for (int i = AttachmentPoint::Color0; i <= AttachmentPoint::Color7; ++i)
    {
        const Texture& texture = mTextures[i];
        if (texture.IsValid())
        {
            auto desc = texture.GetD3D12ResourceDesc();
            width = std::max(width, desc.Width);
            height = std::max(height, desc.Height);
        }
    }

    D3D12_VIEWPORT viewport = {
        (width * bias.x),       // TopLeftX
        (height * bias.y),      // TopLeftY
        (width * scale.x),      // Width
        (height * scale.y),     // Height
        minDepth,               // MinDepth
        maxDepth                // MaxDepth
    };

    return viewport;
}

// Resize all of the textures associated with the render target.
void NFGE::Graphics::RenderTarget::Resize(DirectX::XMUINT2 size)
{
    mSize = size;
    for (auto& texture : mTextures)
    {
        texture.Resize(mSize.x, mSize.y);
    }
}

void RenderTarget::Resize(uint32_t width, uint32_t height)
{
    Resize(DirectX::XMUINT2(width, height));
}

// Get a list of the textures attached to the render target.
// This method is primarily used by the CommandList when binding the
// render target to the output merger stage of the rendering pipeline.
const std::vector<Texture>& RenderTarget::GetTextures() const
{
    return mTextures;
}

D3D12_RT_FORMAT_ARRAY RenderTarget::GetRenderTargetFormats() const
{
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};


    for (int i = AttachmentPoint::Color0; i <= AttachmentPoint::Color7; ++i)
    {
        const Texture& texture = mTextures[i];
        if (texture.IsValid())
        {
            rtvFormats.RTFormats[rtvFormats.NumRenderTargets++] = texture.GetD3D12ResourceDesc().Format;
        }
    }

    return rtvFormats;
}

DXGI_FORMAT RenderTarget::GetDepthStencilFormat() const
{
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
    const Texture& depthStencilTexture = mTextures[AttachmentPoint::DepthStencil];
    if (depthStencilTexture.IsValid())
    {
        dsvFormat = depthStencilTexture.GetD3D12ResourceDesc().Format;
    }

    return dsvFormat;
}
