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

#pragma once

#include "CoreInclude.h"
#include <MiniaudioCpp/MiniaudioWrappers.h>

#include <functional>

namespace JPL
{
	using ProcessCbFunc = std::function<void(JPL::ProcessCallbackData&)>;

	namespace Internal
	{
		struct process_effect
		{
			ma_node_base base;
			ProcessCbFunc ProcessCb;
			void Process(JPL::ProcessCallbackData& processBlock) { ProcessCb(processBlock); }

			static constexpr int FLAGS = MA_NODE_FLAG_CONTINUOUS_PROCESSING;
		};
	}

	//==========================================================================
	/// Effect is wrapper around a function processing input audio frames
	class Effect
	{
	public:
		// Non-copyable
		Effect(const Effect&) = delete;
		Effect& operator=(const Effect&) = delete;

		Effect() = default;
		Effect(Effect&&) noexcept;
		Effect& operator=(Effect&&) noexcept;

		Effect(uint32_t numChannels, ProcessCbFunc processCallback);
		Effect(uint32_t numInChannels, uint32_t numOutChannels, ProcessCbFunc processCallback);

		virtual ~Effect() = default;

		JPL::InputBus GetInput() { return m_Node.InputBus(0); }
		JPL::OutputBus GetOutput() { return m_Node.OutputBus(0); }
		JPL::NodeIO GetIO() { return { GetInput(), GetOutput() }; }

	private:
		JPL::TBaseNode<Internal::process_effect> m_Node;
	};

#if 0
	struct EffectBus
	{
		EffectBus();
		virtual ~EffectBus();

		void Attach(JPL::OutputBus effectBusInput);

		static constexpr auto LastIndex = uint32_t(-1);

		void Insert(ProcessCbFunc&& processor, uint32_t index = LastIndex, bool replace = false);

	private:
		JPL::EngineNode mBus;
		//ma_engine_node m_Bus;
		JPL::InputBus mDestination;

		std::vector<Effect> m_EffectChain;
	};
#endif
} // namespace JPL
