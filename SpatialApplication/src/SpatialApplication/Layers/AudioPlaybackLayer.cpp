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

#include "AudioPlaybackLayer.h"

#include "Config.h"

#include "Controller/AudioPlayer.h"
#include "Utility/PerformanceMetering.h"
#include "ImGui/ImGui.h"

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>
#include <JPLSpatial/AirAbsorption.h>
#include <MiniaudioCpp/MiniaudioWrappers.h>
#include <JPLSpatial/Auralization/LateReverb.h>
#include <JPLSpatial/Math/DecibelsAndGain.h>
#include <JPLSpatial/Math/Position.h>
#include <JPLSpatial/Math/SIMD.h>
#include <JPLSpatial/PathTracing/Math.h>
#include <JPLSpatial/PathTracing/SpecularPath.h>

#include "choc/audio/choc_SampleBufferUtilities.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <ranges>

#undef max
#undef min

namespace JPL
{
	inline JPL::Engine& GetMAEngine()
	{
		static JPL::Engine sEngine;
		return sEngine;
	}

	AudioPlaybackLayer::AudioPlaybackLayer(std::shared_ptr<JPL::DirectSoundModel> directSoundModel)
		: mDirectSoundModel(directSoundModel)
	{
		JPL_ASSERT(directSoundModel);
		mDirectSoundModel->AddListener(this);
	}

	AudioPlaybackLayer::~AudioPlaybackLayer() = default;

	void AudioPlaybackLayer::OnAttach()
	{
		JPL::GetMiniaudioEngine = [](void*) -> JPL::Engine& { return GetMAEngine(); };

		// Note: we could simulate different endpoint channel count:
		// - if we set noDevice=TRUE,
		// - the number of channels and sample rate to what we want
		// - pull the sampeles manually using ma_engine_read_pcm_frames()

		if (not JPL_ENSURE(GetMAEngine().Init(/*numChannels (use native device channelcount)*/ 0, /* vfs */ nullptr)))
		{
			return;
		}

		const uint32_t numEngineChannels = GetMAEngine().GetEndpointBus().GetNumChannels();

		mPanner = std::make_unique<JPLPanner>();
		[[maybe_unused]] const bool bPannerInitialzied = mPanner->Initialize(numEngineChannels);
		JPL_ASSERT(bPannerInitialzied);

		mSampleRate = static_cast<float>(GetMAEngine().GetSampleRateDouble());

		const auto framesPerBlock = GetMAEngine().GetProcessingSizeInFrames();

		[[maybe_unused]] auto view = // we don't actually need the view, we just need to preallocate the buffer internally
			mDeinterleaveBuffer.getDeinterleavedBuffer(choc::buffer::Size::create(numEngineChannels, framesPerBlock * 4));

		static constexpr int cExprectedRequestRate = 60; // times/second
		mRMSMeter.Prepare(mSampleRate, numEngineChannels, GetMAEngine().GetProcessingSizeInFrames(), cExprectedRequestRate);
		mMeterProcessor = std::make_unique<Effect>(numEngineChannels, numEngineChannels, [&](JPL::ProcessCallbackData& callback)
		{
			const auto deinterleavedView = mDeinterleaveBuffer.deinterleave(callback.GetInputBuffer(0));
			mRMSMeter.ProcessBlock(deinterleavedView);

			callback.CopyInputsToOutputs();
		});
		mMeterProcessor->GetOutput().AttachTo(GetMAEngine().GetEndpointBus());

		mERProcessor = std::make_unique<JPL::ERBus>();
		mERProcessor->Prepare(mSampleRate, numEngineChannels);

		mLateReverb = std::make_unique<ReverbBus>();
		mLateReverb->SetRT60(mLateReverbModel->T60.Get());
		mLateReverb->Prepare(mSampleRate);
		mLateReverbModel->T60.AddChangeCallback(this, [](AudioPlaybackLayer* self, const simd& v)
		{
			if (self->mLateReverb)
				self->mLateReverb->SetRT60(v);
		});
		mLateReverbModel->DryLevel.AddChangeCallback(this, [](AudioPlaybackLayer* self, const float& v)
		{
			self->mERLevel.store(v, std::memory_order_release);
		});
		mLateReverbModel->WetLevel.AddChangeCallback(this, [](AudioPlaybackLayer* self, const float& v)
		{
			self->mLateReverbLevel.store(v, std::memory_order_release);
		});
		mERLevel = mLateReverbModel->DryLevel.Get();
		mLateReverbLevel = mLateReverbModel->WetLevel.Get();

		//SetupImpulseSource();

		mPlayer = std::make_unique<AudioPlayer>();
		mPlayer->SetLooping(true);

		mAudioPlayerGUI = std::make_unique<AudioPlayerGUI>(*mPlayer);

		// Note: before new source is initialied, we need to clean up the old processing chain,
		// because the endpoint is going to call it back for new samples, even if it doesn't have a source.
		// For that reason we subscibe to Audio Player OnAudioFileChanged clallback, which is called
		// while the old source is still active and we cleanup the processing chain first.
		mPlayer->AddListener(this);

		mPlayer->onSoundCreated = [this](MA::Sound& sound)
		{
			Broadcast<&ChangeListenerType::OnSoundChanged>(sound);
			UpdateProcessingChain(sound);
		};

		mDirectory.onSelectionChanged = [this](const std::filesystem::path& filepath)
		{
			if (mPlayer)
				mPlayer->SetFile(filepath);
		};
	}

	void AudioPlaybackLayer::OnDetach()
	{
		mAudioPlayerGUI = nullptr;

		mRMSMeter.Reset();
		mMeterProcessor = nullptr;

		ResetProcessingChain();

		mPlayer = nullptr;
		mPanner = nullptr;
	}

	void AudioPlaybackLayer::OnUpdate(float ts)
	{
	}

	void AudioPlaybackLayer::OnUIRender()
	{
		using namespace JPL::ImGuiEx;

		Window("Sound Sources", { .Flags = ImGuiWindowFlags_NoCollapse }, [&]
		{
			LayoutVertical("##vertical", [&]
			{
				LayoutHorizontal("##directory_hor", [&]
				{
					Child("Directory List", [&]
					{
						DirectoryDisplay::Draw(mDirectory);
					});

					ImGui::Spring();
				});
			});
		}); // Audio Player

		Window("Late Reverb", { .Flags = ImGuiWindowFlags_NoCollapse }, [&]
		{
			/*if (ImGui::Button("Impulse"))
			{
				if (mImpulseSource)
				{
					mSendImpulse.store(true, std::memory_order_release);
				}
			}*/

			if (mLateReverbGUI)
			{
				mLateReverbGUI->Draw();
			}
		});

		const WindowConfig config
		{
			.Flags = ImGuiWindowFlags_NoCollapse,
			.DockFlags = ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther
		};

		Window("Audio Player", config, [&]
		{
			// TODO: maybe sprinkle this over everywhere to not have to include imgui.ini to the release
			//		to have some kind of usable initial layout
			//ImGui::SetWindowSize(ImVec2(400.0f, 100.0f), ImGuiCond_FirstUseEver);

			if (mAudioPlayerGUI)
			{
				mAudioPlayerGUI->Draw();
			}
		});// , ImVec2(0.0f, std::min(waveformHeith, ImGui::GetContentRegionAvail().y)));

		{
			const WindowConfig meterConfig
			{
				.MinSize = ImVec2(32.0f, 100.0f),
				.Flags = ImGuiWindowFlags_NoCollapse,
				.DockFlags = ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther
			};
			ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

			Window("Metering", meterConfig, [&]
			{
				const auto& peak = mRMSMeter.GetPeakForIndividualChannels();
				const auto& rms = mRMSMeter.GetRMSForIndividualChannels();
				mLoudnessMeter.SubmitLevels(peak, rms);

				mLoudnessMeter.OnRender(ImGui::GetIO().DeltaTime);
			});
		}
	}

	void AudioPlaybackLayer::SetTaps(std::span<const typename JPL::ERBus::ERUpdateData> newTaps)
	{
		mTaps.resize(newTaps.size());
		std::ranges::copy(newTaps, mTaps.begin());

		// This might called from sound changed callback,
		// in which case we may not have processing chain yet,
		// the chain update will be called later and will SetTaps
		if (mERProcessor)
		{
			const auto timer = PerfMeterUpdateTaps::MakeScopedTimer();
			mERProcessor->SetTaps(mTaps);
		}
	}

	void AudioPlaybackLayer::SetVBAPModel(std::shared_ptr<JPL::VBAPModel> newModel)
	{
		if (mVBAPModel)
		{
			mVBAPModel->RemoveListener(this);
		}

		mVBAPModel = newModel;

		mVBAPModel->AddListener(this);

		UpdateDirectSoundParameters();
		ApplyDirectEffectParams();
	}

	void AudioPlaybackLayer::OnChange(JPL::GenericChangeBroadcaster* broadcaster)
	{
		if (broadcaster == mVBAPModel.get() || broadcaster == mDirectSoundModel.get())
		{
			UpdateDirectSoundParameters();
			ApplyDirectEffectParams();
		}
	}

	void AudioPlaybackLayer::OnAudioFileChanged(const std::filesystem::path& newAudioFile)
	{
		ResetProcessingChain();
	}

	void AudioPlaybackLayer::SetEnableDirectSound(bool bShouldBeEnabled)
	{
		// TODO: we need to first fade out, otherwise it can click when un-bypassing
		// or keep processing and just silence the output
		mBypassDirectSound.store(not bShouldBeEnabled, std::memory_order_release);

	}

	void AudioPlaybackLayer::UpdateProcessingChain(JPL::Sound& sound)
	{
		if (!sound)
		{
			ResetProcessingChain();
			return;
		}

		const uint32_t numSourceChannels = sound.GetNumOutputChannels(0);
		const uint32_t numOutChannels = GetMAEngine().GetEndpointBus().GetNumChannels();

		mLoudnessMeter.SetNumberOfChannels(numOutChannels);

		//? for now we only work with stereo (for no good reason, except time)
		JPL_ASSERT(numOutChannels == 2);
		//JPL_ASSERT(numSourceChannels == numOutChannels);

#if 1 // We actually don't need to clear mix map, if we only allow stereo to stereo source->output
		mChannelMixMap.clear();
		mChannelMixMap.resize(numSourceChannels * numOutChannels, 0.0f);
#else
		if (mChannelMixMap.empty())
		{
			mChannelMixMap =
			{
				1.0f, 0.0f, // left channel to left channel
				0.0f, 1.0f	// right channel to right channel
			};
		}
#endif

		mSourceLayoutHandle = mPanner->InitializeSourceLayout(numSourceChannels);
		JPL_ASSERT(mSourceLayoutHandle);

		// Make sure we have updated data before we start rendering audio
		UpdateDirectSoundParameters();
		//..now mFilterGains, mDelayTime and mChannelMixMap are updated

		// Create and initialize Direct Sound Effect
		mDirectEffectProcessor = std::make_unique<JPL::DirectSoundEffect>(
			static_cast<float>(GetMAEngine().GetSampleRateDouble()),
			numSourceChannels,
			numOutChannels,
			mFilterGains,
			mDelayTime,
			mChannelMixMap
		);
	
		// Create Direct Sound Effect bus
		// TODO: this does not allow panning to a higher channel count, we need to decouple direct effect from target bus panning
		mDirectEffectBus = std::make_unique<Effect>(numSourceChannels, numOutChannels, [this](JPL::ProcessCallbackData& callback)
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

			mDirectEffectProcessor->ProcessInterleaved(inData, outData, numFrames);

			if (mBypassDirectSound.load(std::memory_order_acquire))
			{
				// Note: we keep processing even in bypass mode,
				// because this way it's much simpler to handle un-bypassing
				// without jumps in wolume due to parameters interpolation
				// from previously "stuck" "current" values.
				callback.FillOutputWithSilence();
			}
		});


#if 1

		// The delay buffer is not valid for new soumd,
		// so we need to reset ER Bus
		mERProcessor = std::make_unique<JPL::ERBus>();
		mERProcessor->Prepare(mSampleRate, numOutChannels);
		{
			const auto timer = PerfMeterUpdateTaps::MakeScopedTimer();
			mERProcessor->SetTaps(mTaps);
		}

		mLateReverb = std::make_unique<ReverbBus>();
		mLateReverb->SetRT60(mLateReverbModel->T60.Get());
		mLateReverb->Prepare(mSampleRate);

		mERBus = std::make_unique<Effect>(numSourceChannels, numOutChannels, [this](JPL::ProcessCallbackData& callback)
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

			// Downmix input to mono
			StaticArray<float, 4048> bufferIn(numFrames, 0.0f);
			StaticArray<float, 4048> bufferOut(outData.size(), 0.0f);

			DownmixToMono(bufferIn.data(), inData.data(), numInChannels, inData.size());
			// Normalize downmixed frames
			ApplyGain(bufferIn.data(), bufferIn.size(), 1.0f / numInChannels);

			const float ERLevel = mERLevel.load(std::memory_order_acquire);
			const float reverbLevel = mLateReverbLevel.load(std::memory_order_acquire);

#if JPL_DEV_DIRECT_TO_LR
			{
				const auto timer = PerfMeterAudioCallback::MakeScopedTimer();
				mLateReverb->ProcessInterleaved(std::span(bufferIn), outData, numFrames);
				ApplyGain(outData.data(), outData.size(), reverbLevel);
			}
#else
			{
				// Process ERs
				const auto timer = PerfMeterAudioCallback::MakeScopedTimer();
				mERProcessor->ProcessInterleaved(std::span(bufferIn), outData, 1, numFrames);

				// TODO: proper mixing ER -> Reverb

				// Make a copy of ER output at full volume
				std::ranges::fill(bufferIn, 0.0f);
				bufferIn.resize(outData.size(), 0.0f);
				std::ranges::copy(outData, bufferIn.begin());
				
				// Apply ER level
				ApplyGain(outData.data(), outData.size(), ERLevel);

				// Process ER copy with Late Reverb
				mLateReverb->ProcessInterleaved(bufferIn, bufferOut, numFrames);

				// Apply reverb level
				ApplyGain(bufferOut.data(), bufferOut.size(), reverbLevel);

				// Mix in Reverb to the output
				for (uint32 c = 0; c < numOutChannels; ++c)
				{
					Add(outData.data(), bufferOut.data(), c, c, numOutChannels, numOutChannels, numFrames);
				}
			}
#endif
		});

		// --- Routing

		// Initialize a 2 bus splitter
		const uint32_t numSplitBusses = 2;
		if (not JPL_ENSURE(mSplitter.Init(numSourceChannels, numSplitBusses)))
			return;

		// Attach Audio Player's Sound object to splitter's input
		if (not JPL_ENSURE(sound.OutputBus(0).AttachTo(mSplitter.InputBus(0))))
			return;

		// Attach "dry" signal to Direct Sound Effect bus
		if (not JPL_ENSURE(mSplitter.OutputBus(0).AttachTo(mDirectEffectBus->GetInput())))
			return;

		// Attach "wet" signal to bus that will render ERs
		if (not JPL_ENSURE(mSplitter.OutputBus(1).AttachTo(mERBus->GetInput())))
			return;
#endif

		// Attach the buses directly to the endpoint
		//! Note: it's important to do at the very end,
		//! to not start processing before the chain is fully set up
		JPL_ENSURE(mDirectEffectBus->GetOutput().AttachTo(mMeterProcessor->GetInput()));
		JPL_ENSURE(mERBus->GetOutput().AttachTo(mMeterProcessor->GetInput()));
	}

	void AudioPlaybackLayer::ResetProcessingChain()
	{
		mERBus = nullptr;
		mERProcessor = nullptr;
		mDirectEffectBus = nullptr;
		mDirectEffectProcessor = nullptr;
		mReverbEffectBus = nullptr;
		mLateReverb = nullptr;
		mImpulseSource = nullptr;
	}

	void AudioPlaybackLayer::UpdateDirectSoundParameters()
	{
		if (mPlayer == nullptr || not mSourceLayoutHandle)
		{
			return;
		}

		JPLPanUpdateData panData;
		const float distance = mVBAPModel->ComputePanUpdateData(mPanner->GetChannelMap().HasTopChannels(), panData);

		const uint32_t numSourceChannels = mPlayer->GetSound().GetNumOutputChannels(0);
		const uint32_t numOutChannels = GetMAEngine().GetEndpointBus().GetNumChannels();

		if (mVBAPModel->UseSourceOrientation.Get())
		{
			// TODO: we might want to be able to rotate the soruce as well in the GUI
			const auto orientation = JPL::OrientationData<JPL::MinimalVec3>::IdentityForward();
			mPanner->ProcessVBAP(mSourceLayoutHandle, panData, orientation, mChannelMixMap);
		}
		else
		{
			mPanner->ProcessVBAP(mSourceLayoutHandle, panData, mChannelMixMap);
		}

		if (mDirectSoundModel->EnableDistanceAttenuation.Get())
		{
			// TODO: do we want source size to be where distance attenuation begins,
			//		or leave it to the user-defined curves in the future?
			//
			// ...for now we just clamp to min distance 1.0
			// to avoid boosting volume if source is closer

			const float invDistance = 1.0f / std::max(distance, 1.0f);
			for (float& g : mChannelMixMap) // apply distance attenuation
			{
				g *= invDistance;
			}
		}

		if (mDirectSoundModel->EnableAirAbsorption.Get())
		{
			const JPL::simd airAbsorptionLoss = JPL::AirAbsorption::ComputeForDistance(distance, cDefaultAirAbsCache);
			// this overrides `airAbsorptionLoss` param, don't put it at the end
			mFilterGains = JPL::dBToGain(-airAbsorptionLoss);
		}
		else
		{
			mFilterGains = 1.0f;
		}

		if (mDirectSoundModel->EnablePropagationDelay.Get())
		{
			mDelayTime = distance * JPL::JPL_INV_SPEAD_OF_SOUND;
		}
		else
		{
			mDelayTime = 0.0f;

		}
	}

	void AudioPlaybackLayer::ApplyDirectEffectParams()
	{
		if (mDirectEffectProcessor)
		{
			mDirectEffectProcessor->UpdateParameters(mFilterGains, mDelayTime, mChannelMixMap);
		}
	}

	void AudioPlaybackLayer::SetupImpulseSource()
	{
		// TODO: temporarily setting up reverb only for impulse sygnal
		mLateReverb = std::make_unique<ReverbBus>();
		mLateReverb->SetRT60(mLateReverbModel->T60.Get());
		mLateReverb->Prepare(mSampleRate);

#if 0 //JPL_DBG_CONTROLS
		mLateReverbModel->DryLevel.AddChangeCallback(this, [](AudioPlaybackLayer* self, const float& v)
		{
			self->mLateReverb->SetDryLevel(v);
		});
		mLateReverbModel->WetLevel.AddChangeCallback(this, [](AudioPlaybackLayer* self, const float& v)
		{
			self->mLateReverb->SetWetLevel(v);
		});
#endif
	
		const uint32_t numOutChannels = GetMAEngine().GetEndpointBus().GetNumChannels();

		mImpulseSource = std::make_unique<Effect>(1, numOutChannels, [this](JPL::ProcessCallbackData& callback)
		{
			// Clear the output first
			callback.FillOutputWithSilence();

			if (mSendImpulse.exchange(false, std::memory_order_acq_rel))
			{
				auto output = callback.GetOutputBuffer(0);
				const uint32_t numOutChannels = output.getNumChannels();

				for (uint32 ch = 0; ch < numOutChannels; ++ch)
					output.getChannel(ch).data.data[0] = 1.0f;
			}
		});

		mReverbEffectBus = std::make_unique<Effect>(numOutChannels, [this](JPL::ProcessCallbackData& callback)
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

			{
				//const auto timer = PerfMeterAudioCallback::MakeScopedTimer();
				mLateReverb->ProcessInterleaved(inData, outData, numFrames);
			}
		});

		if (JPL_ENSURE(mImpulseSource->GetOutput().AttachTo(mReverbEffectBus->GetInput())))
		{
			JPL_ENSURE(mReverbEffectBus->GetOutput().AttachTo(mMeterProcessor->GetInput()));
		}
	}
} // namespace JPL
