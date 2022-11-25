#include "Precompiled.h"
#include "SimpleDraw.h"

#include "Camera.h"
#include "GraphicsSystem.h"
#include "VertexType.h"
#include "PipelineComponent.h"
#include "RootSignature.h"
#include "D3DUtil.h"
#include "d3dx12.h"
#include "PipelineWorker.h"

using namespace NFGE;
using namespace NFGE::Graphics;

namespace
{
	const std::wstring ShaderFilename = L"SimpleDraw.hlsl";
	// Private implementation
	class SimpleDrawImpl
	{
	public:
		void Initialize(std::filesystem::path rootPath, uint32_t maxVertexCount);
		void Terminate();

		void AddLine(const Math::Vector3& v0, const Math::Vector3& v1, const Graphics::Color& color);
		void AddTriangle(const Math::Vector3& v0, const Math::Vector3& v1, const Math::Vector3& v2, const Graphics::Color& color);
		void AddScreenLine(const Math::Vector2& v0, const Math::Vector2& v1, const Graphics::Color& color);

		void RenderUpdate();
		void Render(const Camera& camera);

	private:
		//ConstantBuffer	mConstantBuffer;
		//VertexShader	mVertexShader;
		//PixelShader		mPixelShader;

		PipelineComponent_Basic<MeshPC>	mPipelineComp_Basic_Line;		//|
		std::vector<VertexPC> mVertices_line;	//|
		uint32_t		mMaxVertices_line;
		uint32_t		mVertices_line_count;

		PipelineComponent_Basic<MeshPC> mPipelineComp_Basic_Face;		//|
		std::vector<VertexPC> mVertices_face;	//|
		uint32_t		mVertices_face_count;
		uint32_t		mMaxVertices_face;

		bool mInitialized{ false };
	};
	void SimpleDrawImpl::Initialize(std::filesystem::path rootPath, uint32_t maxVertexCount)
	{
		ASSERT(!mInitialized, "[SimpleDrawImpl] Already initialized!");

		mPipelineComp_Basic_Line.mMesh.mVertices.reserve(maxVertexCount);
		mVertices_line.resize(maxVertexCount);
		mMaxVertices_line = maxVertexCount;
		mVertices_line_count = 0;
		mPipelineComp_Basic_Line.mShaderFilename = rootPath / ShaderFilename;
		mPipelineComp_Basic_Line.mD3d12Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		mPipelineComp_Basic_Line.mD3dTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

		mPipelineComp_Basic_Face.mMesh.mVertices.reserve(maxVertexCount);
		mVertices_face.resize(maxVertexCount);
		mMaxVertices_face = maxVertexCount;
		mVertices_face_count = 0;
		mPipelineComp_Basic_Face.mShaderFilename = rootPath / ShaderFilename;

		// Load custom root signature.
		auto device = NFGE::Graphics::GetDevice();
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		// A single 32-bit constant root parameter that is used by the vertex shader.
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsConstants(sizeof(NFGE::Math::Matrix4) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, NULL, rootSignatureFlags);

		mPipelineComp_Basic_Line.mRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);
		mPipelineComp_Basic_Face.mRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

		NFGE::Graphics::RegisterPipelineComponent_FirstLoad(WorkerType::Copy, &mPipelineComp_Basic_Line);
		NFGE::Graphics::RegisterPipelineComponent_FirstLoad(WorkerType::Copy, &mPipelineComp_Basic_Face);

		mInitialized = true;
	}
	void SimpleDrawImpl::Terminate()
	{
		ASSERT(mInitialized, "[SimpleDrawImpl] Already terminated!");

		mInitialized = false;
	}
	void SimpleDrawImpl::AddLine(const Math::Vector3& v0, const Math::Vector3& v1, const Graphics::Color& color)
	{
		ASSERT(mInitialized, "[SimpleDrawImpl] Not initialized.");
		if (mVertices_line_count + 2 <= mMaxVertices_line)
		{
			mVertices_line[mVertices_line_count++] = { v0, color };
			mVertices_line[mVertices_line_count++] = { v1, color };
		}

	}

	void SimpleDrawImpl::AddTriangle(const Math::Vector3& v0, const Math::Vector3& v1, const Math::Vector3& v2, const Graphics::Color& color)
	{
		ASSERT(mInitialized, "[SimpleDrawImpl] Not initialized.");
		if (mVertices_face_count + 3 <= mMaxVertices_face)
		{
			mVertices_face[mVertices_face_count++] = { v0, color };
			mVertices_face[mVertices_face_count++] = { v1, color };
			mVertices_face[mVertices_face_count++] = { v2, color };
		}
	}

	void SimpleDrawImpl::AddScreenLine(const Math::Vector2& v0, const Math::Vector2& v1, const Graphics::Color& color)
	{
		ASSERT(mInitialized, "[SimpleDrawImpl] Not initialized.");
		//if (mVertices_line_2D_count + 2 <= mMaxVertices_line_2D)
		//{
		//	mLine2DDepth -= 0.0000001f;
		//	mVertices_line_2D[mVertices_line_2D_count++] = { Math::Vector3(v0.x, v0.y, mLine2DDepth), color };
		//	mVertices_line_2D[mVertices_line_2D_count++] = { Math::Vector3(v1.x, v1.y, mLine2DDepth), color };
		//}
	}

	void SimpleDrawImpl::RenderUpdate()
	{
		std::vector<VertexPC>::iterator it = mVertices_line.begin() + 1;
		mPipelineComp_Basic_Line.UpdateVertices(mVertices_line, static_cast<uint32_t>(mVertices_line_count));
		mPipelineComp_Basic_Face.UpdateVertices(mVertices_face, static_cast<uint32_t>(mVertices_face_count));
		NFGE::Graphics::RegisterPipelineComponent_Update(WorkerType::Copy, &mPipelineComp_Basic_Line);
		NFGE::Graphics::RegisterPipelineComponent_Update(WorkerType::Copy, &mPipelineComp_Basic_Face);
	}

	void SimpleDrawImpl::Render(const Camera& camera)
	{
		ASSERT(mInitialized, "[SimpleDrawImpl] Not initialized.");

		GraphicsSystem* graphicSystem = NFGE::Graphics::GraphicsSystem::Get();
		PipelineWorker* worker = NFGE::Graphics::GetWorker(WorkerType::Direct);

		auto matView = camera.GetViewMatrix();
		auto matProj = camera.GetPerspectiveMatrix();
		auto transform = Math::Transpose(matView * matProj);

		// Draw line
		mPipelineComp_Basic_Line.GetBind(*worker);
		worker->SetGraphics32BitConstants(0, sizeof(NFGE::Math::Matrix4) / 4, &transform);
		worker->Draw(static_cast<uint32_t>(mPipelineComp_Basic_Line.mMesh.GetVertices().size()), 1, 0, 0);
		mVertices_line_count = 0;
		mPipelineComp_Basic_Line.mMesh.Destroy();

		// Draw face
		mPipelineComp_Basic_Line.GetBind(*worker);
		worker->SetGraphics32BitConstants(0, sizeof(NFGE::Math::Matrix4) / 4, &transform);
		worker->Draw(static_cast<uint32_t>(mPipelineComp_Basic_Face.mMesh.GetVertices().size()), 1, 0, 0);
		mVertices_face_count = 0;
		mPipelineComp_Basic_Face.mMesh.Destroy();

		// Draw 2D
		//const uint32_t w = system->GetBackBufferWidth();
		//const uint32_t h = system->GetBackBufferHeight();
		//Math::Matrix4 matInvScreen
		//(
		//	2.0f / w, 0.0f, 0.0f, 0.0f,
		//	0.0f, -2.0f / h, 0.0f, 0.0f,
		//	0.0f, 0.0f, 1.0f, 0.0f,
		//	-1.0f, 1.0f, 0.0f, 1.0f
		//);
		//matInvScreen.Transpose();
	}

	std::unique_ptr<SimpleDrawImpl> sInstance;
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::StaticInitialize(std::filesystem::path rootPath, uint32_t maxVertexCount)
{
	sInstance = std::make_unique<SimpleDrawImpl>();
	sInstance->Initialize(rootPath, maxVertexCount);
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::StaticTerminate()
{
	sInstance->Terminate();
	sInstance.reset();
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::AddLine(const Math::Vector3& v0, const Math::Vector3& v1, const Graphics::Color& color)
{
	sInstance->AddLine(v0, v1, color);
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::AddLine(float x0, float y0, float z0, float x1, float y1, float z1, const Math::Vector4& color)
{
	AddLine(Math::Vector3(x0, y0, z0), Math::Vector3(x1, y1, z1), color);
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::AddAABB(const Math::AABB& aabb, const Math::Vector4& color, bool isFace)
{
	float minX = aabb.center.x - aabb.extend.x;
	float minY = aabb.center.y - aabb.extend.y;
	float minZ = aabb.center.z - aabb.extend.z;
	float maxX = aabb.center.x + aabb.extend.x;
	float maxY = aabb.center.y + aabb.extend.y;
	float maxZ = aabb.center.z + aabb.extend.z;

	//		  4	 ______________________ 5  
	//			|\				       \
	//			| \				        \
	//		  6	|  \			         \---->7
	//			\   \_0___________________\ 1  
	//			 \  |					  |
	//			  \ |					  |
	//			  2\|_____________________| 3

	Math::Vector3 p0(minX, maxY, minZ);
	Math::Vector3 p1(maxX, maxY, minZ);
	Math::Vector3 p2(minX, minY, minZ);
	Math::Vector3 p3(maxX, minY, minZ);
	Math::Vector3 p4(minX, maxY, maxZ);
	Math::Vector3 p5(maxX, maxY, maxZ);
	Math::Vector3 p6(minX, minY, maxZ);
	Math::Vector3 p7(maxX, minY, maxZ);

	if (isFace)
	{
		sInstance->AddTriangle(p0, p1, p3, color);
		sInstance->AddTriangle(p0, p3, p2, color);
		sInstance->AddTriangle(p1, p5, p3, color);
		sInstance->AddTriangle(p5, p7, p3, color);
		sInstance->AddTriangle(p5, p4, p6, color);
		sInstance->AddTriangle(p5, p6, p7, color);
		sInstance->AddTriangle(p4, p0, p6, color);
		sInstance->AddTriangle(p0, p2, p6, color);
		sInstance->AddTriangle(p4, p5, p0, color);
		sInstance->AddTriangle(p5, p1, p0, color);
		sInstance->AddTriangle(p2, p3, p6, color);
		sInstance->AddTriangle(p3, p7, p6, color);
	}
	else
	{
		sInstance->AddLine(p2, p6, color);
		sInstance->AddLine(p6, p7, color);
		sInstance->AddLine(p7, p3, color);
		sInstance->AddLine(p3, p2, color);
		sInstance->AddLine(p2, p0, color);
		sInstance->AddLine(p6, p4, color);
		sInstance->AddLine(p7, p5, color);
		sInstance->AddLine(p3, p1, color);
		sInstance->AddLine(p0, p4, color);
		sInstance->AddLine(p4, p5, color);
		sInstance->AddLine(p5, p1, color);
		sInstance->AddLine(p1, p0, color);
	}
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::AddAABB(const Math::Vector3& min, const Math::Vector3& max, const Math::Vector4& color, bool isFace)
{
	AddAABB(Math::AABB((min + max) * 0.5f, (max - min) * 0.5f), color, isFace);
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::AddAABB(const Math::Vector3& center, float radius, const Math::Vector4& color, bool isFace)
{
	AddAABB(Math::AABB(center, Math::Vector3(radius, radius, radius)), color, isFace);
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::AddAABB(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, const Math::Vector4& color, bool isFace)
{
	AddAABB(Math::Vector3(minX, minY, minZ), Math::Vector3(maxX, maxY, maxZ), color, isFace);
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::AddOBB(const Math::OBB& obb, const Math::Vector4& color)
{
	Math::Matrix4 matTrans = Math::Matrix4::sTranslation(obb.center);
	Math::Matrix4 matRot = Math::MatrixRotationQuaternion(obb.orientation);
	Math::Matrix4 matScale = Math::Matrix4::sScaling(obb.extend);
	Math::Matrix4 toWorld = matScale * matRot * matTrans;

	Math::Vector3 points[] =
	{
		Math::Vector3(-1.0f, -1.0f, -1.0f),
		Math::Vector3(-1.0f,  1.0f, -1.0f),
		Math::Vector3(1.0f,  1.0f, -1.0f),
		Math::Vector3(1.0f, -1.0f, -1.0f),
		Math::Vector3(-1.0f, -1.0f,  1.0f),
		Math::Vector3(-1.0f,  1.0f,  1.0f),
		Math::Vector3(1.0f,  1.0f,  1.0f),
		Math::Vector3(1.0f, -1.0f,  1.0f)
	};

	for (uint32_t i = 0; i < 8; ++i)
	{
		points[i] = Math::TransformCoord(points[i], toWorld);
	}

	AddLine(points[0], points[1], color);
	AddLine(points[1], points[2], color);
	AddLine(points[2], points[3], color);
	AddLine(points[3], points[0], color);

	AddLine(points[0], points[4], color);
	AddLine(points[1], points[5], color);
	AddLine(points[2], points[6], color);
	AddLine(points[3], points[7], color);

	AddLine(points[4], points[5], color);
	AddLine(points[5], points[6], color);
	AddLine(points[6], points[7], color);
	AddLine(points[7], points[4], color);
}

void NFGE::Graphics::SimpleDraw::AddCircle_FaceY(float x, float y, float z, float r, const Math::Vector4& color, uint32_t slices)
{
	const uint32_t kSlices = Math::Max(16u, slices);
	const float kAngle = Math::Constants::TwoPi / (float)kSlices;
	for (uint32_t i = 0; i < kSlices; ++i)
	{
		const float alpha = i * kAngle;
		const float beta = alpha + kAngle;
		const float x0 = x + (r * sin(alpha));
		const float z0 = z + (r * cos(alpha));
		const float x1 = x + (r * sin(beta));
		const float z1 = z + (r * cos(beta));
		sInstance->AddLine(Math::Vector3(x0, y, z0), Math::Vector3(x1, y, z1), color);
	}
}

void NFGE::Graphics::SimpleDraw::AddSphere(const Math::Sphere& sphere, const Math::Vector4& color, uint32_t slices, uint32_t rings)
{
	const float x = sphere.center.x;
	const float y = sphere.center.y;
	const float z = sphere.center.z;
	const float radius = sphere.radius;

	const uint32_t kSlices = Math::Max(3u, slices);
	const uint32_t kRings = Math::Max(2u, rings);
	const float kUnitVertiAngle = Math::Constants::Pi / (float)kRings;
	const float kUnitHorizAngle = Math::Constants::TwoPi / (float)kSlices;

	for (uint32_t i = 0; i < kRings; ++i)
	{
		for (uint32_t j = 0; j < kSlices; ++j)
		{
			const float vAngle = i * kUnitVertiAngle;
			const float p0y = radius * cos(vAngle);
			const float p1y = radius * cos(vAngle + kUnitVertiAngle);
			const float p0r = sqrt(radius * radius - p0y * p0y);
			const float p1r = sqrt(radius * radius - p1y * p1y);

			const float hAngle = j * kUnitHorizAngle;

			const float x0 = x + p0r * sin(hAngle);
			const float y0 = y + p0y;
			const float z0 = z + p0r * cos(hAngle);

			const float x1 = x + p1r * sin(hAngle);
			const float y1 = y + p1y;
			const float z1 = z + p1r * cos(hAngle);

			sInstance->AddLine(Math::Vector3(x0, y0, z0), Math::Vector3(x1, y1, z1), color);
			if (i < kRings - 1)
			{
				const float x2 = x + p1r * sin(hAngle + kUnitHorizAngle);
				const float y2 = y + p1y;
				const float z2 = z + p1r * cos(hAngle + kUnitHorizAngle);
				sInstance->AddLine(Math::Vector3(x1, y1, z1), Math::Vector3(x2, y2, z2), color);
			}
		}
	}
}

void NFGE::Graphics::SimpleDraw::AddSphere(const Math::Vector3& center, float radius, const Math::Vector4& color, uint32_t slices, uint32_t rings)
{
	AddSphere(Math::Sphere(center, radius), color, slices, rings);
}

void NFGE::Graphics::SimpleDraw::AddSphere(float x, float y, float z, float radius, const Math::Vector4& color, uint32_t slices, uint32_t rings)
{
	AddSphere(Math::Sphere(x, y, z, radius), color, slices, rings);
}

void NFGE::Graphics::SimpleDraw::AddTransform(const Math::Matrix4& transform)
{
	Math::Vector3 position = Math::GetTranslation(transform);
	Math::Vector3 right = Math::GetRight(transform);
	Math::Vector3 up = Math::GetUp(transform);
	Math::Vector3 forward = Math::GetForward(transform);
	sInstance->AddLine(position, position + right, Colors::Red);
	sInstance->AddLine(position, position + up, Colors::Green);
	sInstance->AddLine(position, position + forward, Colors::Blue);
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::AddTriangle(const Math::Vector3& v0, const Math::Vector3& v1, const Math::Vector3& v2, const Graphics::Color& color)
{
	sInstance->AddTriangle(v0, v1, v2, color);
}

void NFGE::Graphics::SimpleDraw::AddScreenLine(const Math::Vector2& v0, const Math::Vector2& v1, const Math::Vector4& color)
{
	sInstance->AddScreenLine(v0, v1, color);
}

void NFGE::Graphics::SimpleDraw::AddScreenLine(float x0, float y0, float x1, float y1, const Math::Vector4& color)
{
	sInstance->AddScreenLine(Math::Vector2(x0, y0), Math::Vector2(x1, y1), color);
}

void NFGE::Graphics::SimpleDraw::AddScreenRect(const Math::Rect& rect, const Math::Vector4& color)
{
	float l = rect.left;
	float t = rect.top;
	float r = rect.right;
	float b = rect.bottom;

	// Add lines
	sInstance->AddScreenLine(Math::Vector2(l, t), Math::Vector2(r, t), color);
	sInstance->AddScreenLine(Math::Vector2(r, t), Math::Vector2(r, b), color);
	sInstance->AddScreenLine(Math::Vector2(r, b), Math::Vector2(l, b), color);
	sInstance->AddScreenLine(Math::Vector2(l, b), Math::Vector2(l, t), color);
}

void NFGE::Graphics::SimpleDraw::AddScreenRect(const Math::Vector2& min, const Math::Vector2& max, const Math::Vector4& color)
{
	AddScreenRect(Math::Rect(min.x, min.y, max.x, max.y), color);
}

void NFGE::Graphics::SimpleDraw::AddScreenRect(float left, float top, float right, float bottom, const Math::Vector4& color)
{
	AddScreenRect(Math::Rect(left, top, right, bottom), color);
}

void NFGE::Graphics::SimpleDraw::AddScreenCircle(const Math::Circle& circle, const Math::Vector4& color, uint32_t slices)
{
	float x = circle.center.x;
	float y = circle.center.y;
	float r = circle.radius;

	const uint32_t kSlices = Math::Max(16u, slices);
	const float kAngle = Math::Constants::TwoPi / (float)kSlices;
	for (uint32_t i = 0; i < kSlices; ++i)
	{
		const float alpha = i * kAngle;
		const float beta = alpha + kAngle;
		const float x0 = x + (r * sin(alpha));
		const float y0 = y + (r * cos(alpha));
		const float x1 = x + (r * sin(beta));
		const float y1 = y + (r * cos(beta));
		sInstance->AddScreenLine(Math::Vector2(x0, y0), Math::Vector2(x1, y1), color);
	}
}

void NFGE::Graphics::SimpleDraw::AddScreenCircle(const Math::Vector2& center, float r, const Math::Vector4& color, uint32_t slices)
{
	AddScreenCircle(Math::Circle(center, r), color, slices);
}

void NFGE::Graphics::SimpleDraw::AddScreenCircle(float x, float y, float r, const Math::Vector4& color, uint32_t slices)
{
	AddScreenCircle(Math::Circle(x, y, r), color, slices);
}

void NFGE::Graphics::SimpleDraw::AddScreenDiamond(const Math::Vector2& center, float size, const Math::Vector4& color)
{
	sInstance->AddScreenLine(Math::Vector2(center.x, center.y - size), Math::Vector2(center.x + size, center.y), color);
	sInstance->AddScreenLine(Math::Vector2(center.x + size, center.y), Math::Vector2(center.x, center.y + size), color);
	sInstance->AddScreenLine(Math::Vector2(center.x, center.y + size), Math::Vector2(center.x - size, center.y), color);
	sInstance->AddScreenLine(Math::Vector2(center.x - size, center.y), Math::Vector2(center.x, center.y - size), color);
}

void NFGE::Graphics::SimpleDraw::AddScreenDiamond(float x, float y, float size, const Math::Vector4& color)
{
	AddScreenDiamond(Math::Vector2(x, y), size, color);
}

// -----------------------------------------------------------------------------------------------------------

void NFGE::Graphics::SimpleDraw::RenderUpdate()
{
	sInstance->RenderUpdate();
}

void NFGE::Graphics::SimpleDraw::Render(const Camera& camera)
{
	sInstance->Render(camera);
}

// -----------------------------------------------------------------------------------------------------------
