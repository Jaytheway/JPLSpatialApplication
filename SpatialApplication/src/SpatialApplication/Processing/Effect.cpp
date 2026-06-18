//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright Jaroslav Pevno, JPL Spatial Application is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "Effect.h"

#include <utility>

namespace JPL
{
	//==========================================================================
	Effect::Effect(uint32_t numChannels, ProcessCbFunc processCallback)
		: Effect(numChannels, numChannels, processCallback)
	{

	}

	Effect::Effect(uint32_t numInChannels, uint32_t numOutChannels, ProcessCbFunc processCallback)
	{
		const bool bInitialized = m_Node.Init(JPL::NodeLayout()
											  .WithInputs(numInChannels)
											  .WithOutputs(numOutChannels),
											  false); // init started

		if (JPL_ENSURE(bInitialized, "Failed to initialize Effect."))
		{
			m_Node->ProcessCb = processCallback;
			JPL_ENSURE(m_Node.StartNode());
		}
	}

	Effect::Effect(Effect&& other) noexcept
		: m_Node(std::move(other.m_Node))
	{
	}

	Effect& Effect::operator=(Effect&& other) noexcept
	{
		m_Node = std::move(other.m_Node);
		return *this;
	}

#if 0
	//==========================================================================
	EffectBus::EffectBus()
	{
		const bool bSuccess = mBus.InitGroup(
			JPL::EngineNode::GroupNodeSettings{
				.NumInChannels = 2,
				.NumOutChannels = 2,
				.PitchDisabled = true
			});

		massert(bSuccess);
		if (bSuccess)
		{
			// Attach to the endpoint initially
			mDestination = JPL::GetMiniaudioEngine(nullptr).GetEndpointBus();
			mBus.OutputBus(0).AttachTo(mDestination);
		}
	}

	EffectBus::~EffectBus()
	{
	}

	void EffectBus::Attach(JPL::OutputBus effectBusInput)
	{
		if (!effectBusInput)
			return;

		const bool bSuccess = effectBusInput.AttachTo(mBus.InputBus(0));
		massert(bSuccess);
	}

	static inline JPL::NodeIO GetNodeIO(Effect& node)
	{
		return node.GetIO();
	}

	static inline JPL::NodeIO GetNodeIO(auto& node)
	{
		return JPL::NodeIO{ .Input = JPL::InputBusIndex(0).Of(&node), .Output = JPL::OutputBusIndex(0).Of(&node) };
	}


	void EffectBus::Insert(ProcessCbFunc&& processor, uint32_t index, bool replace)
	{
		JPL::NodeIO head;	// effect input
		JPL::NodeIO effect;	// effect processor
		JPL::NodeIO tail;	// effect output

		if (index == LastIndex || index >= m_EffectChain.size())
		{
			if (!m_EffectChain.empty())
			{
				head = GetNodeIO(m_EffectChain.back());
			}
			else
			{
				head = GetNodeIO(mBus);
			}

			m_EffectChain.emplace_back(2, std::forward<ProcessCbFunc>(processor));
			effect = GetNodeIO(m_EffectChain.back());

			tail = JPL::NodeIO{ .Input = mDestination };
		}
		else
		{
			if (index > 0)
			{
				head = GetNodeIO(m_EffectChain[index - 1]);
			}
			else /*if (index == 0)*/
			{
				head = GetNodeIO(mBus);
			}

			if (replace)
			{
				m_EffectChain[index] = std::move(Effect(2, std::forward<ProcessCbFunc>(processor)));
			}
			else
			{
				auto headIt = m_EffectChain.begin() + index;
				auto inserted = m_EffectChain.insert(headIt, Effect(2, std::forward<ProcessCbFunc>(processor)));
			}

			effect = GetNodeIO(m_EffectChain[index]);
			tail = GetNodeIO(m_EffectChain[index + 1]);
		}

		if (not JPL_ENSURE(head) || not JPL_ENSURE(effect))
		{
			return;
		}

		bool bSuccess = true;

		bSuccess = head.AttachTo(effect);
		if (not JPL_ENSURE(bSuccess))
		{
			return;
		}

		if (tail.Input.IsValid())
		{
			JPL_ENSURE(effect.AttachTo(tail));
		}
	}
#endif
} // namespace JPL
