//====================================================================================================
// Filename:	Common.h
// Created by:	Mingzhuo Zhang
// Date:		2022/7
//====================================================================================================

#pragma once

// Engine headers
#include <Core/Inc/Core.h>
#include <NFGEMath/Inc/NFGEMath.h>

// DirectX headers
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <../External/DirectXTex-main/DirectXTex/DirectXTex.h>

// External headers
#include <ImGui/Inc/imgui.h>

// DirectX libraries
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib") // for DirectX reflect

// Hashers for D3d12 view descriptions.
namespace std
{
    // Source: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
    template <typename T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<>
    struct hash<D3D12_SHADER_RESOURCE_VIEW_DESC>
    {
        std::size_t operator()(const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc) const noexcept
        {
            std::size_t seed = 0;

            hash_combine(seed, srvDesc.Format);
            hash_combine(seed, srvDesc.ViewDimension);
            hash_combine(seed, srvDesc.Shader4ComponentMapping);

            switch (srvDesc.ViewDimension)
            {
            case D3D12_SRV_DIMENSION_BUFFER:
                hash_combine(seed, srvDesc.Buffer.FirstElement);
                hash_combine(seed, srvDesc.Buffer.NumElements);
                hash_combine(seed, srvDesc.Buffer.StructureByteStride);
                hash_combine(seed, srvDesc.Buffer.Flags);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE1D:
                hash_combine(seed, srvDesc.Texture1D.MostDetailedMip);
                hash_combine(seed, srvDesc.Texture1D.MipLevels);
                hash_combine(seed, srvDesc.Texture1D.ResourceMinLODClamp);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
                hash_combine(seed, srvDesc.Texture1DArray.MostDetailedMip);
                hash_combine(seed, srvDesc.Texture1DArray.MipLevels);
                hash_combine(seed, srvDesc.Texture1DArray.FirstArraySlice);
                hash_combine(seed, srvDesc.Texture1DArray.ArraySize);
                hash_combine(seed, srvDesc.Texture1DArray.ResourceMinLODClamp);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2D:
                hash_combine(seed, srvDesc.Texture2D.MostDetailedMip);
                hash_combine(seed, srvDesc.Texture2D.MipLevels);
                hash_combine(seed, srvDesc.Texture2D.PlaneSlice);
                hash_combine(seed, srvDesc.Texture2D.ResourceMinLODClamp);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
                hash_combine(seed, srvDesc.Texture2DArray.MostDetailedMip);
                hash_combine(seed, srvDesc.Texture2DArray.MipLevels);
                hash_combine(seed, srvDesc.Texture2DArray.FirstArraySlice);
                hash_combine(seed, srvDesc.Texture2DArray.ArraySize);
                hash_combine(seed, srvDesc.Texture2DArray.PlaneSlice);
                hash_combine(seed, srvDesc.Texture2DArray.ResourceMinLODClamp);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DMS:
                //                hash_combine(seed, srvDesc.Texture2DMS.UnusedField_NothingToDefine);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
                hash_combine(seed, srvDesc.Texture2DMSArray.FirstArraySlice);
                hash_combine(seed, srvDesc.Texture2DMSArray.ArraySize);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE3D:
                hash_combine(seed, srvDesc.Texture3D.MostDetailedMip);
                hash_combine(seed, srvDesc.Texture3D.MipLevels);
                hash_combine(seed, srvDesc.Texture3D.ResourceMinLODClamp);
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBE:
                hash_combine(seed, srvDesc.TextureCube.MostDetailedMip);
                hash_combine(seed, srvDesc.TextureCube.MipLevels);
                hash_combine(seed, srvDesc.TextureCube.ResourceMinLODClamp);
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
                hash_combine(seed, srvDesc.TextureCubeArray.MostDetailedMip);
                hash_combine(seed, srvDesc.TextureCubeArray.MipLevels);
                hash_combine(seed, srvDesc.TextureCubeArray.First2DArrayFace);
                hash_combine(seed, srvDesc.TextureCubeArray.NumCubes);
                hash_combine(seed, srvDesc.TextureCubeArray.ResourceMinLODClamp);
                break;
             case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
                hash_combine(seed, srvDesc.RaytracingAccelerationStructure.Location);
                break;
            }

            return seed;
        }
    };

    template<>
    struct hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>
    {
        std::size_t operator()(const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc) const noexcept
        {
            std::size_t seed = 0;

            hash_combine(seed, uavDesc.Format);
            hash_combine(seed, uavDesc.ViewDimension);

            switch (uavDesc.ViewDimension)
            {
            case D3D12_UAV_DIMENSION_BUFFER:
                hash_combine(seed, uavDesc.Buffer.FirstElement);
                hash_combine(seed, uavDesc.Buffer.NumElements);
                hash_combine(seed, uavDesc.Buffer.StructureByteStride);
                hash_combine(seed, uavDesc.Buffer.CounterOffsetInBytes);
                hash_combine(seed, uavDesc.Buffer.Flags);
                break;
            case D3D12_UAV_DIMENSION_TEXTURE1D:
                hash_combine(seed, uavDesc.Texture1D.MipSlice);
                break;
            case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
                hash_combine(seed, uavDesc.Texture1DArray.MipSlice);
                hash_combine(seed, uavDesc.Texture1DArray.FirstArraySlice);
                hash_combine(seed, uavDesc.Texture1DArray.ArraySize);
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2D:
                hash_combine(seed, uavDesc.Texture2D.MipSlice);
                hash_combine(seed, uavDesc.Texture2D.PlaneSlice);
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
                hash_combine(seed, uavDesc.Texture2DArray.MipSlice);
                hash_combine(seed, uavDesc.Texture2DArray.FirstArraySlice);
                hash_combine(seed, uavDesc.Texture2DArray.ArraySize);
                hash_combine(seed, uavDesc.Texture2DArray.PlaneSlice);
                break;
            case D3D12_UAV_DIMENSION_TEXTURE3D:
                hash_combine(seed, uavDesc.Texture3D.MipSlice);
                hash_combine(seed, uavDesc.Texture3D.FirstWSlice);
                hash_combine(seed, uavDesc.Texture3D.WSize);
                break;
            }

            return seed;
        }
    };
}

namespace NFGE::Graphics
{
    enum class IOOption
    {
        Binary = 0,
        RV			//------------- Readable Value
    };
}