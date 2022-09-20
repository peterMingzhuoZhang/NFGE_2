//====================================================================================================
// Filename:	RenderTaget.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#include "Precompiled.h"
#include "RenderTarget.h"

using namespace NFGE::Graphics;

RenderTarget::RenderTarget()
    : mTextures(AttachmentPoint::NumAttachmentPoints)
{}

// Attach a texture to the render target.
// The texture will be copied into the texture array.
void RenderTarget::AttachTexture(AttachmentPoint attachmentPoint, const Texture& texture)
{
    mTextures[attachmentPoint] = texture;
}

const Texture& RenderTarget::GetTexture(AttachmentPoint attachmentPoint) const
{
    return mTextures[attachmentPoint];
}

// Resize all of the textures associated with the render target.
void RenderTarget::Resize(uint32_t width, uint32_t height)
{
    for (auto& texture : mTextures)
    {
        texture.Resize(width, height);
    }
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
