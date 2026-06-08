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

#include "ReverbPreview.h"

#include "Processing/Effect.h"

#include <JPLSpatial/Auralization/LateReverb.h>
#include <JPLSpatial/Containers/StaticArray.h>

namespace JPL
{
	ReverbPreview::ReverbPreview(MA::InputBus endpoint,
								 const std::shared_ptr<LateReverbModel>& reverbModel,
								 float sampleRate)
		: mReverbModel(reverbModel)
		, mSampleRate(sampleRate)
	{
		JPL_ASSERT(reverbModel);
		JPL_ASSERT(sampleRate > 0);

		mReverbModel->T60.AddChangeCallback(this, [](ReverbPreview* self, const simd& v)
		{
			if (self->mReverb)
				self->mReverb->SetRT60(v);
		});
		mReverbModel->DryLevel.AddChangeCallback(this, [](ReverbPreview* self, const float& v)
		{
			self->mDryLevel.store(v, std::memory_order_release);
		});
		mReverbModel->WetLevel.AddChangeCallback(this, [](ReverbPreview* self, const float& v)
		{
			self->mReverbLevel.store(v, std::memory_order_release);
		});
		mDryLevel = mReverbModel->DryLevel.Get();
		mReverbLevel = mReverbModel->WetLevel.Get();

		SetupImpulseSource(endpoint);
	}

	ReverbPreview::~ReverbPreview()
	{
		if (mReverbModel)
		{
			mReverbModel->T60.RemoveChangeCallback(this);
			mReverbModel->DryLevel.RemoveChangeCallback(this);
			mReverbModel->WetLevel.RemoveChangeCallback(this);
		}
	}

	void ReverbPreview::TriggerImpulse()
	{
		mSendImpulse.store(true, std::memory_order_relaxed);
	}

	void ReverbPreview::SetupImpulseSource(MA::InputBus endpoint)
	{
		mReverb = std::make_unique<ReverbBus>();
		mReverb->SetRT60(mReverbModel->T60.Get());
		mReverb->Prepare(mSampleRate);

		const uint32_t numOutChannels = endpoint.GetNumChannels();

		mImpulseSource = std::make_unique<Effect>(1, 1, [this](JPL::ProcessCallbackData& callback)
		{
			// Clear the output first
			callback.FillOutputWithSilence();

			if (mSendImpulse.exchange(false, std::memory_order_relaxed))
			{
				auto output = callback.GetOutputBuffer(0);
				
				JPL_ASSERT(output.getNumChannels() == 1);
				
				output.getChannel(0).data.data[0] = 1.0f;
			}
		});

		mReverbEffectBus = std::make_unique<Effect>(1, numOutChannels, [this](JPL::ProcessCallbackData& callback)
		{
			// Clear the output first
			callback.FillOutputWithSilence();

			auto input = callback.GetInputBuffer(0);
			auto output = callback.GetOutputBuffer(0);
			const uint32_t numInChannels = input.getNumChannels();
			const uint32_t numOutChannels = output.getNumChannels();
			const uint32_t numFrames = callback.GetOutputFrameCount();
			std::span<const float> inData(input.data.data, numFrames * numInChannels);
			std::span<float> outData(output.data.data, numFrames * numOutChannels);

			const float dryLevel = mDryLevel.load(std::memory_order_acquire);
			const float reverbLevel = mReverbLevel.load(std::memory_order_acquire);

			StaticArray<float, 2048> bufferDry(numFrames, 0.0f);

			// Make a copy of dry output at full volume
			std::ranges::fill(bufferDry, 0.0f);
			bufferDry.resize(inData.size(), 0.0f);
			std::ranges::copy(inData, bufferDry.begin());

			// Apply Dry level
			ApplyGain(bufferDry.data(), bufferDry.size(), dryLevel * (1.0f / numOutChannels));

			// Process impulse with reverb
			mReverb->ProcessInterleaved(inData, outData, numFrames);

			// Apply Wet gain
			ApplyGain(outData.data(), outData.size(), reverbLevel);
			
			// Add dry impulse to the output
			for (uint32 outChannelIdx = 0; outChannelIdx < numOutChannels; ++outChannelIdx)
				Add(outData.data(), bufferDry.data(), outChannelIdx, numOutChannels, numFrames);

		});

		if (JPL_ENSURE(mImpulseSource->GetOutput().AttachTo(mReverbEffectBus->GetInput())))
		{
			JPL_ENSURE(mReverbEffectBus->GetOutput().AttachTo(endpoint));
		}
	}

} // namespace JPL
