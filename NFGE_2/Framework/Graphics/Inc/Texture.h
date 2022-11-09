//====================================================================================================
// Filename:	Texture.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#pragma once

#include "Resource.h"
#include "DescriptorAllocation.h"

namespace NFGE::Graphics {
    enum class TextureUsage
    {
        Albedo,
        Diffuse = Albedo,       // Treat Diffuse and Albedo textures the same.
        Heightmap,
        Depth = Heightmap,      // Treat height and depth textures the same.
        Normalmap,
        RenderTarget,           // Texture is used as a render target.
        Sprite
    };

    class Texture : public Resource
    {
    public:
        explicit Texture(TextureUsage textureUsage = TextureUsage::Albedo,
            const std::wstring& name = L"");
        explicit Texture(const D3D12_RESOURCE_DESC& resourceDesc,
            const D3D12_CLEAR_VALUE* clearValue = nullptr,
            TextureUsage textureUsage = TextureUsage::Albedo,
            const std::wstring& name = L"");
        explicit Texture(Microsoft::WRL::ComPtr<ID3D12Resource> resource,
            TextureUsage textureUsage = TextureUsage::Albedo,
            const std::wstring& name = L"");

        Texture(const Texture& copy);
        Texture(Texture&& copy) noexcept;

        Texture& operator=(const Texture& other);
        Texture& operator=(Texture&& other) noexcept;

        virtual ~Texture();

        virtual void Reset();

        TextureUsage GetTextureUsage() const { return mTextureUsage; }

        void SetTextureUsage(TextureUsage textureUsage) { mTextureUsage = textureUsage; }

        uint32_t GetWidth() const { return mWidth; }
        uint32_t GetHeight() const { return mHeight; }
        std::wstring GetName() const { return mResourceName; }

        /**
         * Resize the texture.
         */
        void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);

        /**
         * Create SRV and UAVs for the resource.
         */
        virtual void CreateViews();

        /**
        * Get the SRV for a resource.
        *
        * @param dxgiFormat The required format of the resource. When accessing a
        * depth-stencil buffer as a shader resource view, the format will be different.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

        /**
        * Get the UAV for a (sub)resource.
        */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

        /**
         * Get the RTV for the texture.
         */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;

        /**
         * Get the DSV for the texture.
         */
        virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

        void CreateSpriteTextureShaderResourceView(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t index);

        bool CheckSRVSupport()
        {
            return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);
        }

        bool CheckRTVSupport()
        {
            return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
        }

        bool CheckUAVSupport()
        {
            return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
                CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
                CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
        }

        bool CheckDSVSupport()
        {
            return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
        }

        static bool IsUAVCompatibleFormat(DXGI_FORMAT format);
        static bool IsSRGBFormat(DXGI_FORMAT format);
        static bool IsBGRFormat(DXGI_FORMAT format);
        static bool IsDepthFormat(DXGI_FORMAT format);

        // Return a typeless format from the given format.
        static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);
        static DXGI_FORMAT GetUAVCompatableFormat(DXGI_FORMAT format);

        void SetQuickTexture(uint32_t index, uint32_t generation) { mQuickTextureIndex = index; mQuickTextureGeneration = generation; };
        uint32_t GetQuickTextureID() const { return mQuickTextureIndex; };
        uint32_t GetQuickTextureGen() const { return mQuickTextureGeneration; };
    protected:

    private:
        DescriptorAllocation CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const;
        DescriptorAllocation CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const;

        mutable std::unordered_map<size_t, DescriptorAllocation> mShaderResourceViews;
        mutable std::unordered_map<size_t, DescriptorAllocation> mUnorderedAccessViews;

        mutable std::mutex mShaderResourceViewsMutex;
        mutable std::mutex mUnorderedAccessViewsMutex;

        DescriptorAllocation mRenderTargetView;
        DescriptorAllocation mDepthStencilView;

        TextureUsage mTextureUsage;

        uint32_t mWidth{ 0 };
        uint32_t mHeight{ 0 };

        uint32_t mQuickTextureIndex{ 0 };
        uint32_t mQuickTextureGeneration{ 0 };
        
    };
}

/*
 *  Copyright(c) 2018 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

 /**
  *  @file Texture.h
  *  @date October 24, 2018
  *  @author Jeremiah van Oosten
  *
  *  @brief A wrapper for a DX12 Texture object.
  */