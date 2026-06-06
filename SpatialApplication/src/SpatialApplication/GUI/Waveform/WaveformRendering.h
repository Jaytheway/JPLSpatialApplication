//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatial
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

#include <JPLSpatial/Core.h>

#include "ImGui/ImGui.h"
#include "Coroutine/Coroutine.h"
#include "GUI/Waveform/WaveformDataSource.h"

#include <coroutine>
#include <filesystem>
#include <vector>

namespace JPL::GUI
{
	//==========================================================================
	/// Provides waveform drawing routine
	class Waveform : ChangeListener<SampleData>
	{
	public:
		/// Per unit length min/max sample walues
		/// that represent a waveform
		struct ChannelData
		{
			std::vector<float> Min;
			std::vector<float> Max;
		};

	public:
		explicit Waveform(WaveformDataSource& waveformDataSorce);
		
		~Waveform() noexcept;
		
		bool Draw(const char* itemId);

	private:
		Coro::Task<> WaveformUpdateRoutine();

		// ~ Begin ChangeListener<SampleData> interface
		void OnSourceChanged() override;
		void OnNewDataAwailable(const SampleData& newSampleData) override;
		// ~ End ChangeListener<SampleData> interface

	private:
		WaveformDataSource& mDataSource;
		SampleData mSampleDataCache;

		Coro::Task<> mWaveformUpdateRoutine;
		Coro::Flag mWaveformNeedsUpdate;
		bool bUpdatingWaveform = false;
		
		std::vector<ChannelData> mChannelData;
		int mWaveformWidthPx = 0;

		ImColor mWaveformFillColour = JPL::GUI::Colours::Theme::Selected;
		ImColor mWaveformLineColour = JPL::GUI::Colours::Theme::Selected;
		float mLineThickness = 1.0f;
	};
} // namespace JPL::GUI
