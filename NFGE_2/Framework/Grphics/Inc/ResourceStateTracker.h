//====================================================================================================
// Filename:	ResourceStateTracker.h
// Created by:	Mingzhuo Zhang
// Date:		2022/9
// Instruction: https://www.3dgep.com/learning-directx-12-3/
//====================================================================================================

#pragma once

namespace NFGE::Graphics 
{
	class CommandList;

	class ResourceStateTracker
	{
	public:
		static void Lock();
		static void Unlock();
	
		static void AddGlobalResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);
		static void RemoveGlobalResourceState(ID3D12Resource* resource);

	public:
		ResourceStateTracker();
		virtual ~ResourceStateTracker();

		// Record a resource barrier to the resource state tracker.
		void ResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier);

		void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

		void UAVBarrier(ID3D12Resource* resource = nullptr);
		void AliasBarrier(ID3D12Resource* resourceBefore = nullptr, ID3D12Resource* resourceAfter = nullptr);

		uint32_t FlushPendingResourceBarriers(CommandList& commandList);
		void FlushResourceBarriers(CommandList& commandList);

		void CommitFinalResourceStates();
		void Reset();
	private:
		// An array (vector) of resource barriers.
		using ResourceBarriers = std::vector<D3D12_RESOURCE_BARRIER>;
		
		ResourceBarriers mPendingResourceBarriers;
		ResourceBarriers mResourceBarriers;
		
		// Tracks the state of a particular resource and all of its subresources.
		struct ResourceState
		{
			// Initialize all of the subresources within a resource to the given state.
			explicit ResourceState(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
				: mState(state)
			{}

			// Set a subresource to a particular state.
			void SetSubresourceState(UINT subresource, D3D12_RESOURCE_STATES state)
			{
				if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
				{
					mState = state;
					mSubresourceState.clear();
				}
				else
				{
					mSubresourceState[subresource] = state;
				}
			}

			// Get the state of a (sub)resource within the resource.
			// If the specified subresource is not found in the SubresourceState array (map)
			// then the state of the resource (D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) is
			// returned.
			D3D12_RESOURCE_STATES GetSubresourceState(UINT subresource) const
			{
				D3D12_RESOURCE_STATES state = mState;
				const auto iter = mSubresourceState.find(subresource);
				if (iter != mSubresourceState.end())
				{
					state = iter->second;
				}
				return state;
			}
			// If the SubresourceState array (map) is empty, then the State variable defines 
			// the state of all of the subresources.
			D3D12_RESOURCE_STATES mState;
			std::map<UINT, D3D12_RESOURCE_STATES> mSubresourceState;
		};

		using ResourceStateMap = std::unordered_map<ID3D12Resource*, ResourceState>;
		ResourceStateMap mFinalResourceState;

		static ResourceStateMap sGlobalResourceState;
		// The mutex protects shared access to the GlobalResourceState map.
		static std::mutex sGlobalMutex;
		static bool sIsLocked;
	};
}