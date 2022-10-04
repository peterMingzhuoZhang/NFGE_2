//====================================================================================================
// Filename:	ResourceStateTracker.cpp
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#include "Precompiled.h"
#include "ResourceStateTracker.h"

#include "d3dx12.h"
#include "CommandList.h"

using namespace NFGE::Graphics;

// Static definitions.
std::mutex ResourceStateTracker::sGlobalMutex;
bool ResourceStateTracker::sIsLocked = false;
ResourceStateTracker::ResourceStateMap ResourceStateTracker::sGlobalResourceState;

void NFGE::Graphics::ResourceStateTracker::Lock()
{
    sGlobalMutex.lock();
    sIsLocked = true;
}

void NFGE::Graphics::ResourceStateTracker::Unlock()
{
    sGlobalMutex.unlock();
    sIsLocked = false;
}

void NFGE::Graphics::ResourceStateTracker::AddGlobalResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
{
    if (resource != nullptr)
    {
        std::lock_guard<std::mutex> lock(sGlobalMutex);
        sGlobalResourceState[resource].SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, state);
    }
}

void NFGE::Graphics::ResourceStateTracker::RemoveGlobalResourceState(ID3D12Resource* resource)
{
    if (resource != nullptr)
    {
        std::lock_guard<std::mutex> lock(sGlobalMutex);
        sGlobalResourceState.erase(resource);
    }
}

NFGE::Graphics::ResourceStateTracker::ResourceStateTracker()
{}

NFGE::Graphics::ResourceStateTracker::~ResourceStateTracker()
{}

void NFGE::Graphics::ResourceStateTracker::ResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier)
{
    if (barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
    {
        const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier = barrier.Transition;

        // First check if there is already a known "final" state for the given resource.
        // If there is, the resource has been used on the command list before and
        // already has a known state within the command list execution.
        const auto iter = mFinalResourceState.find(transitionBarrier.pResource);
        if (iter != mFinalResourceState.end())
        {
            auto& resourceState = iter->second;
            // If the known final state of the resource is different...
            if (transitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES &&
                !resourceState.mSubresourceState.empty())
            {
                // First transition all of the subresources if they are different than the StateAfter.
                for (auto subresourceState : resourceState.mSubresourceState)
                {
                    if (transitionBarrier.StateAfter != subresourceState.second)
                    {
                        D3D12_RESOURCE_BARRIER newBarrier = barrier;
                        newBarrier.Transition.Subresource = subresourceState.first;
                        newBarrier.Transition.StateBefore = subresourceState.second;
                        mResourceBarriers.push_back(newBarrier);
                    }
                }
            }
            else
            {
                auto finalState = resourceState.GetSubresourceState(transitionBarrier.Subresource);
                if (transitionBarrier.StateAfter != finalState)
                {
                    // Push a new transition barrier with the correct before state.
                    D3D12_RESOURCE_BARRIER newBarrier = barrier;
                    newBarrier.Transition.StateBefore = finalState;
                    mResourceBarriers.push_back(newBarrier);
                }
            }
        }
        else // In this case, the resource is being used on the command list for the first time. 
        {
            // Add a pending barrier. The pending barriers will be resolved
            // before the command list is executed on the command queue.
            mPendingResourceBarriers.push_back(barrier);
        }

        // Push the final known state (possibly replacing the previously known state for the subresource).
        mFinalResourceState[transitionBarrier.pResource].SetSubresourceState(transitionBarrier.Subresource, transitionBarrier.StateAfter);
    }
    else
    {
        // Just push non-transition barriers to the resource barriers array.
        mResourceBarriers.push_back(barrier);
    }
}

void NFGE::Graphics::ResourceStateTracker::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource)
{
    if (resource)
    {
        ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource));
    }
}

void NFGE::Graphics::ResourceStateTracker::UAVBarrier(ID3D12Resource* resource)
{
    ResourceBarrier(CD3DX12_RESOURCE_BARRIER::UAV(resource));
}

void NFGE::Graphics::ResourceStateTracker::AliasBarrier(ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter)
{
    ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Aliasing(resourceBefore, resourceAfter));
}

uint32_t NFGE::Graphics::ResourceStateTracker::FlushPendingResourceBarriers(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList)
{
    ASSERT(sIsLocked, "ResoureStateTracker should be locked.");

    // Resolve the pending resource barriers by checking the global state of the 
    // (sub)resources. Add barriers if the pending state and the global state do
    //  not match.
    ResourceBarriers resourceBarriers;
    // Reserve enough space (worst-case, all pending barriers).
    resourceBarriers.reserve(mPendingResourceBarriers.size());

    for (auto pendingBarrier : mPendingResourceBarriers)
    {
        if (pendingBarrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)  // Only transition barriers should be pending...
        {
            auto pendingTransition = pendingBarrier.Transition;
            const auto& iter = sGlobalResourceState.find(pendingTransition.pResource);
            if (iter != sGlobalResourceState.end())
            {
                // If all subresources are being transitioned, and there are multiple
                // subresources of the resource that are in a different state...
                auto& resourceState = iter->second;
                if (pendingTransition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES &&
                    !resourceState.mSubresourceState.empty())
                {
                    // Transition all subresources
                    for (auto subresourceState : resourceState.mSubresourceState)
                    {
                        if (pendingTransition.StateAfter != subresourceState.second)
                        {
                            D3D12_RESOURCE_BARRIER newBarrier = pendingBarrier;
                            newBarrier.Transition.Subresource = subresourceState.first;
                            newBarrier.Transition.StateBefore = subresourceState.second;
                            resourceBarriers.push_back(newBarrier);
                        }
                    }
                }
                else
                {
                    // No (sub)resources need to be transitioned. Just add a single transition barrier (if needed).
                    auto globalState = (iter->second).GetSubresourceState(pendingTransition.Subresource);
                    if (pendingTransition.StateAfter != globalState)
                    {
                        // Fix-up the before state based on current global state of the resource.
                        pendingBarrier.Transition.StateBefore = globalState;
                        resourceBarriers.push_back(pendingBarrier);
                    }
                }
            }
        }
    }

    UINT numBarriers = static_cast<UINT>(resourceBarriers.size());
    if (numBarriers > 0)
    {
        commandList->ResourceBarrier(numBarriers, resourceBarriers.data());
    }

    mPendingResourceBarriers.clear();

    return numBarriers;
}

void NFGE::Graphics::ResourceStateTracker::FlushResourceBarriers(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList)
{
    UINT numBarriers = static_cast<UINT>(mResourceBarriers.size());
    if (numBarriers > 0)
    {
        commandList->ResourceBarrier(numBarriers, mResourceBarriers.data());
        mResourceBarriers.clear();
    }
}

void NFGE::Graphics::ResourceStateTracker::CommitFinalResourceStates()
{
    ASSERT(sIsLocked, "Can not commit on unLock resource state. ");

    // Commit final resource states to the global resource state array (map).
    for (const auto& resourceState : mFinalResourceState)
    {
        sGlobalResourceState[resourceState.first] = resourceState.second;
    }

    mFinalResourceState.clear();
}

void NFGE::Graphics::ResourceStateTracker::Reset()
{
    // Reset the pending, current, and final resource states.
    mPendingResourceBarriers.clear();
    mResourceBarriers.clear();
    mFinalResourceState.clear();
}
