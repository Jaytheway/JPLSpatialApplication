//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright Jaroslav Pevno 2026, JPL Spatial Application is offered under the terms of the ISC license:
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

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>
#include <MiniaudioCpp/MiniaudioWrappers.h>

#include "Model/LateReverbModel.h"

#include <atomic>
#include <memory>

namespace JPL
{
	class ReverbBus;
	class Effect;

	class ReverbPreview
	{
	public:
		ReverbPreview(MA::InputBus endpoint,
					  const std::shared_ptr<LateReverbModel>& reverbModel,
					  float sampleRate);

		~ReverbPreview();

		void TriggerImpulse();

	private:
		void SetupImpulseSource(MA::InputBus endpoint);

	private:
		float mSampleRate;
		std::shared_ptr<LateReverbModel> mReverbModel;
		std::unique_ptr<ReverbBus> mReverb;
		std::unique_ptr<Effect> mReverbEffectBus;
		std::unique_ptr<Effect> mImpulseSource;

		std::atomic<bool> mSendImpulse{ false };
		std::atomic<float> mDryLevel{ 0.0f };
		std::atomic<float> mReverbLevel{ 1.0f };
	};
} // namespac JPL
