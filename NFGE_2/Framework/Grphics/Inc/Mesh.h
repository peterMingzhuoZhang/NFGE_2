//====================================================================================================
// Filename:	Mesh.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
//====================================================================================================

#pragma once

#include "VertexType.h"

namespace NFGE::Graphics
{
	template <typename T>
	struct MeshBase
	{
		using VertexType = T;

	public:
		void Allocate(uint32_t numVertices, uint32_t numIndices)
		{
			mVertices.resize(numVertices);
			mIndices.resize(numIndices);
			mNumVertices = numVertices;
			mNumIndices = numIndices;
		}

		void Destroy()
		{
			mVertices.clear();
			mIndices.clear();
			mNumVertices = 0;
			mNumIndices = 0;
		}

		const VertexType& GetVertex(uint32_t index) const
		{
			ASSERT(index < mNumVertices, "[Mesh] Index out of range.");
			return mVertices[index];
		}

		VertexType& GetVertex(uint32_t index)
		{
			return const_cast<VertexType&>(static_cast<const MeshBase*>(this)->GetVertex(index));
		}

		const uint32_t& GetIndex(uint32_t index) const
		{
			ASSERT(index < mNumIndices, "[Mesh] Index out of range.");
			return mIndices[index];
		}

		uint32_t& GetIndex(uint32_t index)
		{
			return const_cast<uint32_t&>(static_cast<const MeshBase*>(this)->GetIndex(index));
		}

		const std::vector<VertexType>& GetVertices() const { return mVertices; }
		const std::vector<uint16_t>& GetIndices() const { return mIndices; }
		uint32_t GetVertexCount() const { return mNumVertices; }
		uint32_t GetIndexCount() const { return mNumIndices; }

		uint32_t GetVertexFormat() const { return VertexType::Format; }

	public:
		std::vector<VertexType> mVertices;
		std::vector<uint16_t> mIndices;

		uint32_t mNumVertices{ 0 };
		uint32_t mNumIndices{ 0 };
	};

	using MeshPC = MeshBase<VertexPC>;
	using MeshPX = MeshBase<VertexPX>;
	using MeshPN = MeshBase<VertexPN>;
	using MeshPNX = MeshBase<VertexPNX>;
	using Mesh = MeshBase<Vertex>;
	using BoneMesh = MeshBase<BoneVertex>;

	class MeshBuilder
	{
	public:
		static MeshPC CreateCubePC();
		static MeshPC CreateFlipCubePC(float size);
		static MeshPC CreatePlanePC(int row, int col, float unit);
		static MeshPC CreateCylinderPC(int HeightFactor, int roundFactor, float radius, float height);
		static MeshPC CreateConePC(int HeightFactor, int roundFactor, float radius, float height);
		static MeshPC CreateSpherePC(int HeightFactor, int roundFactor, float radius);
		static MeshPC CreateTorusPC(int LengthFactor, int roundFactor, float innerRadius, float outerRadius);

		static MeshPX CreateCubePX();
		static MeshPX CreatePlanePX();
		static MeshPX CreatePlanePX(int row, int col, float unit);
		static MeshPX CreateCylinderPX(int HeightFactor, int roundFactor, float radius, float height);
		static MeshPX CreateConePX(int HeightFactor, int roundFactor, float radius, float height);
		static MeshPX CreateSpherePX(int HeightFactor, int roundFactor, float radius);
		static MeshPX CreateTorusPX(int LengthFactor, int roundFactor, float innerRadius, float outerRadius);
		static MeshPX CreateNDCQuad();

		static MeshPN CreateSpherePN(int HeightFactor, int roundFactor, float radius);
		static MeshPN CreateConePN(int HeightFactor, int roundFactor, float radius, float height);

		static Mesh CreatePlane(int row, int col, float unit);
		static Mesh CreateCube();
		static Mesh CreateCone(int HeightFactor, int roundFactor, float radius, float height);
		static Mesh CreateCylinder(int HeightFactor, int roundFactor, float radius, float height);
		static Mesh CreateSphere(int HeightFactor, int roundFactor, float radius);
		static Mesh CreateTorus(int LengthFactor, int roundFactor, float innerRadius, float outerRadius);

		static BoneMesh CreateTentacle(int numOfSeg, int sliceFactor, int roundFactor, float radius, float height, float(*radiusShaper)(float));

		static NFGE::Math::AABB ComputeBound(const Mesh& mesh);
		static void ComputeNormals(Mesh& mesh);
	};



} // namespace NFGE::Graphics
