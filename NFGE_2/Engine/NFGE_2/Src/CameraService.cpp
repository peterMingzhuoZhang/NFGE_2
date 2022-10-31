//====================================================================================================
// Filename:	CameraService.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/10
//====================================================================================================

#include "Precompiled.h"
#include "CameraService.h"
#include "NFGE_2.h"

using namespace NFGE;

namespace
{
	std::unique_ptr<Service> sCameraService;
}

const Service* NFGE::CameraService::StaticGetClass()
{
	sCameraService = std::make_unique<CameraService>();
	return sCameraService.get();
}

void NFGE::CameraService::WorldViewUI()
{
	if (ImGui::TreeNode("Cameras"))
	{
		for (size_t i = 0; i < mCameraList.size(); ++i)
		{
			auto& camera = mCameraList[i];

			if (ImGui::Selectable(camera.name.c_str(), mSelectedCamera == i))
			{
				mSelectedCamera = i;
			}
		}
		ImGui::TreePop();
	}

}

Graphics::Camera* CameraService::AddCamera(const char* name)
{
	// Hack
	if (mCameraList.empty())
	{
		mCameraList.reserve(10);
	}

	auto camera = FindCamera(name);
	if (camera == nullptr)
	{
		auto& entry = mCameraList.emplace_back(name);
		entry.name = name;
		camera = &entry.camera;
	}
	return camera;
}

Graphics::Camera* CameraService::FindCamera(const char* name)
{
	auto iter = std::find_if(mCameraList.begin(), mCameraList.end(), [name](auto& entry)
		{
			return entry.name.c_str();
		});
	return (iter == mCameraList.end()) ? nullptr : &iter->camera;
}

size_t NFGE::CameraService::GetCameraIndex(const char* name)
{
	auto iter = std::find_if(mCameraList.begin(), mCameraList.end(), [name](auto& entry) { return entry.name.c_str(); });
	return (iter == mCameraList.end()) ? UINT_MAX : iter - mCameraList.begin();
}

void CameraService::SetActiveCamera(const char* name)
{
	for (size_t i = 0; i < mCameraList.size(); ++i)
	{
		auto& entry = mCameraList[i];
		if (entry.name == name)
		{
			mActiveCameraIndex = static_cast<int>(i);
			break;
		}
	}
}

Graphics::Camera& CameraService::GetActiveCamera()
{
	ASSERT(mActiveCameraIndex < mCameraList.size(), "[CameraService] No active camera!");
	return mCameraList[mActiveCameraIndex].camera;
}

CameraEntry& NFGE::CameraService::GetActiveCameraEntry()
{
	ASSERT(mActiveCameraIndex < mCameraList.size(), "[CameraService] No active camera!");
	return mCameraList[mActiveCameraIndex];
}

const Graphics::Camera& CameraService::GetActiveCamera() const
{
	ASSERT(mActiveCameraIndex < mCameraList.size(), "[CameraService] No active camera!");
	return mCameraList[mActiveCameraIndex].camera;
}

void NFGE::CameraService::SetCameraPosition(const char* name, const Math::Vector3& pos)
{
	size_t camIndex = GetCameraIndex(name);
	ASSERT(camIndex != UINT_MAX, "[CameraService] access none existing camera.");
	mCameraList[camIndex].mPosition = pos;
	RefreshWithCameraEntry(camIndex);
}

void NFGE::CameraService::SetCameraDirection(const char* name, const Math::Vector3& dir)
{
	size_t camIndex = GetCameraIndex(name);
	ASSERT(camIndex != UINT_MAX, "[CameraService] access none existing camera.");
	mCameraList[camIndex].mDirection = dir;
	RefreshWithCameraEntry(camIndex);
}

void NFGE::CameraService::SetCameraFOV(const char* name, float fov)
{
	size_t camIndex = GetCameraIndex(name);
	ASSERT(camIndex != UINT_MAX, "[CameraService] access none existing camera.");
	mCameraList[camIndex].Fov = fov;
	RefreshWithCameraEntry(camIndex);
}

void NFGE::CameraService::RefreshWithCameraEntry(size_t camIndex)
{
	auto& cameraEntry = mCameraList[camIndex];
	cameraEntry.camera.SetPosition(cameraEntry.mPosition);

	if (cameraEntry.mDirection.x == 0.0f && cameraEntry.mDirection.y == 0.0f && cameraEntry.mDirection.z == 0.0f) // TODO:: Override == operator in future
	{
		cameraEntry.mDirection = NFGE::Math::Vector3::One();
	}
	cameraEntry.mDirection = NFGE::Math::Normalize(cameraEntry.mDirection);
	cameraEntry.camera.SetDirection(cameraEntry.mDirection);
	cameraEntry.camera.SetFOV(cameraEntry.Fov * Math::Constants::DegToRad);
}
