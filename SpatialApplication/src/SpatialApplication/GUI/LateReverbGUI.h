//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright 2026 Jaroslav Pevno, JPL Spatial Application is offered under the terms of the ISC license:
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

#include "Model/LateReverbModel.h"
#include "GUI/AudioPreview.h"
#include "GUI/Waveform/WaveformDataSource.h"
#include "Coroutine/Coroutine.h"

#include <JPLSpatial/Auralization/LateReverb.h>

#include <memory>
#include <utility>

namespace JPL
{
	//==========================================================================
	/// GUI displaying editable properties of a Late Reverb
	class LateReverbGUI : public ChangeListener<GUI::SampleData>
	{
	public:
		LateReverbGUI(std::shared_ptr<LateReverbModel> model);
		~LateReverbGUI();

		void Draw();
		void DrawPreview();

	private:
		void RefreshReverbProperties();

		// Begin ChangeListener<GUI::SampleData> interface
		void OnSourceChanged() final;
		void OnNewDataAwailable(const GUI::SampleData& sampleData) final;
		// ~End ChangeListener<GUI::SampleData> interface

	private:
		// Sample rate to compute IR for
		// (doesn't matter since the IR here is for display only)
		static constexpr float cSampleRate = 48'000.0f;

		//======================================================================
		/// Waveform data source generating sample data from Late Reverb
		/// impulse response
		class IRWaveformSource : public GUI::WaveformDataSource
		{
		public:
			IRWaveformSource();
			void GenerateIRWaveformData();

			void SetReverb(std::shared_ptr<ReverbBus> reverb);

		private:
			Coro::Task<> DataUpdateRoutine();
			static float GenerateIRSampleData(uint64 minLengthInSamples,
											  uint64 maxLengthInSamples,
											  ReverbBus& reverb,
											  GUI::SampleData& outSampleData);

		public:
			//? Profile
			//float IRGenTime = 0.0f;

		private:
			std::weak_ptr<ReverbBus> mReverb;
			Coro::Task<> mDataUpdateRoutine;
			Coro::Flag mUpdateIR;
		};
	private:
		std::shared_ptr<LateReverbModel> mModel;

		std::shared_ptr<ReverbBus> mIRReverb;
		IRWaveformSource mIRWaveformSource;
		GUI::AudioPreview mIRAudioPreview;

		// we don't want to touch reverb if it's used
		// by background thread to update IR waveform
		bool bUpdatingWaveform;

		// shared_ptr to make it undoable
		std::shared_ptr<bool> bShowIRAudioPreview;
		
		//? Profile
		//float IRGenTime = 0.0f;
	};
} // namespace JPL
