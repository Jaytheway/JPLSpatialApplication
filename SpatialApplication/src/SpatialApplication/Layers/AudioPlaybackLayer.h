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

#include <Walnut/Layer.h>

#include "GUI/DirectoryDisplay.h"
#include "GUI/AudioPlayerGUI.h"
#include "GUI/LoudnessMeter.h"
#include "Model/VBAPModel.h"
#include "Model/DirectSoundModel.h"
#include "Processing/Effect.h"
#include "Processing/Panner.h"
#include "Processing/RMSMeter.h"
#include "Utility/MVCUtils.h"

#include <JPLSpatial/Auralization/EarlyReflectionsBus.h>
#include <JPLSpatial/Auralization/DirectSound.h>
#include <JPLSpatial/Math/SIMD.h>

#include "choc/audio/choc_SampleBufferUtilities.h"

#include <memory>
#include <span>
#include <vector>
#include <filesystem>

class AudioPlayer;

namespace JPL
{
	struct Sound;
	class AudioPlaybackLayer;

	template<>
	class ChangeListener<AudioPlaybackLayer>
	{
	public:
		virtual ~ChangeListener() = default;
		virtual void OnSoundChanged(const Sound& newSound) {}
	};

	class AudioPlaybackLayer : public Walnut::Layer
							, public ChangeBroadcaster<AudioPlaybackLayer>
							, public ChangeListener<AudioPlayer>
							, public GenericChangeListener
	{
	public:
		explicit AudioPlaybackLayer(std::shared_ptr<DirectSoundModel> directSoundModel);
		~AudioPlaybackLayer();

		// ~ Begin Walnut::Layer interface
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnUIRender() override;
		// ~ End Walnut::Layer interface

		void SetTaps(std::span<const typename JPL::ERBus::ERUpdateData> newTaps);

		std::shared_ptr<VBAPModel> GetVBAPModel() { return mVBAPModel; }
		void SetVBAPModel(std::shared_ptr<VBAPModel> newModel);

		virtual void OnChange(GenericChangeBroadcaster* broadcaster) override;

		virtual void OnAudioFileChanged(const std::filesystem::path& newAudioFile);

		void SetEnableDirectSound(bool bShouldBeEnabled);

	private:
		void UpdateProcessingChain(Sound& sound);
		void ResetProcessingChain();

		void UpdateDirectSoundParameters();
		void ApplyDirectEffectParams();

	private:
		Directory mDirectory{ "assets\\sound sources" };
		std::unique_ptr<AudioPlayer> mPlayer{ nullptr };
		std::unique_ptr<Effect> mERBus{ nullptr };

		// Splitting into Direct Sound Effect and Early Reflections bus
		SplitterNode mSplitter;

		// Direct Sound Effect stuff
		std::atomic<bool> mBypassDirectSound{ false };
		std::unique_ptr<JPLPanner> mPanner{ nullptr };
		JPLSourceLayoutHandle mSourceLayoutHandle;
		std::vector<float> mChannelMixMap;
		simd mFilterGains{ 1.0f };
		float mDelayTime{ 0.0f };
		std::unique_ptr<Effect> mDirectEffectBus;
		std::unique_ptr<DirectSoundEffect> mDirectEffectProcessor;

		// Early Reflection stuff
		std::unique_ptr<ERBus> mERProcessor{ nullptr };
		std::vector<typename ERBus::ERUpdateData> mTaps;

		float mSampleRate = 48'000.0f;

		std::unique_ptr<AudioPlayerGUI> mAudioPlayerGUI{ nullptr };
		std::shared_ptr<VBAPModel> mVBAPModel{ std::make_shared<VBAPModel>() };
		std::shared_ptr<DirectSoundModel> mDirectSoundModel;

		LoudnessMeter mLoudnessMeter;
		RMSMeter mRMSMeter;
		// TODO: add interleaved callback to RMSMeter
		choc::buffer::DeinterleavingScratchBuffer<float> mDeinterleaveBuffer;
		std::unique_ptr<Effect> mMeterProcessor;
	};
} // namespace JPL
