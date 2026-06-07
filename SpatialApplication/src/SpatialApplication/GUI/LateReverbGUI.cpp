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

#include "GUI/LateReverbGUI.h"
#include "ImGui/ImGui.h"
#include "Systems/EventsLoop.h"

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>
#include <JPLSpatial/Algo/Algorithm.h>
#include <JPLSpatial/Containers/StaticArray.h>
#include <JPLSpatial/Utilities/Variance.h>

#define JPL_PROFILE_IR_GEN 0

#if JPL_PROFILE_IR_GEN 
#include "Utility/Timer.h"
#endif

namespace JPL
{
	//==========================================================================
	LateReverbGUI::LateReverbGUI(std::shared_ptr<LateReverbModel> model)
		: mModel(model)
		, mIRReverb(std::make_shared<ReverbBus>())
		, mIRAudioPreview(mIRWaveformSource)
		, bUpdatingWaveform(false)
	{
		JPL_ASSERT(model);

		// Display spectrogram by default
		mIRAudioPreview.SetMode(GUI::EAudioPreviewMode::Spectrogram);

		mIRWaveformSource.AddListener(this);

		// Sample rate doesn't matter, this is only for display
		mIRReverb->Prepare(cSampleRate);
		mIRWaveformSource.SetReverb(mIRReverb);

#if 0 // JPL_DBG_CONTROLS
		mModel->DryLevel.AddChangeCallback(this, [](LateReverbGUI* self, const float& v)
		{
			if (not self->bUpdatingWaveform) self->mIRReverb->SetDryLevel(v);
		});
		mModel->WetLevel.AddChangeCallback(this, [](LateReverbGUI* self, const float& v)
		{
			if (not self->bUpdatingWaveform) self->mIRReverb->SetWetLevel(v);
		});
#endif
		mModel->T60.AddChangeCallback(this, [](LateReverbGUI* self, const simd& v)
		{
			if (not self->bUpdatingWaveform)
				self->mIRReverb->SetRT60(v);

			// Safe to call here, it just sets flag, which is checked
			// when waveform is not updating.
			self->mIRWaveformSource.GenerateIRWaveformData();
		});

		RefreshReverbProperties();
	}

	LateReverbGUI::~LateReverbGUI()
	{
	}

#if 0 // JPL_PROFILE_IR_GEN
						LayoutVertical("IR Info", [&]
						{
							/*if (ImGui::Button("Update Waveform"))
							{
								mIRWaveformSource.GenerateIRWaveformData();
							}*/
							ImGui::Spring();
							ImGui::TextDisabled("IR Updated in %.3f ms", IRGenTime);
						});
#endif // JPL_PROFILE_IR_GEN

	void LateReverbGUI::Draw()
	{
		if (not mModel)
			return;
		
		using namespace JPL::ImGuiEx;

		static bool bShowIRAudioPreview = false;

		//Child("Late Reverb Properties", propsPanelConfig, [&]
		//{
		LayoutHorizontal("Props Layout", [&]
		{
			ImGuiEx::DrawGEQ("RT60",
							 mModel->T60,
							 cBandCenters,
							 LateReverbModel::cMinReverbTime,
							 LateReverbModel::cMaxReverbTime,
							 ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 3.0f));

			LayoutVertical("vert", [&]
			{
				// ImGui doesn't include label size into item size calculation,
				// which causes lable to be cropped out, so we have to manually
				// set the width and factor in label size.
				const float maxWidth = 140.0f;
				const float labelSize = 100.0f; // arbitrary number to fit our label
				ScopedItemWidth width(ImMin(maxWidth, ImGui::GetContentRegionAvail().x - labelSize));

				Slider("ER Level", mModel->DryLevel, 0.0f, 1.0f, SliderConfig{ .Fmt = "%.2f" });
				Slider("Reverb Level", mModel->WetLevel, 0.0f, 1.0f, SliderConfig{ .Fmt = "%.2f" });

				ImGui::Checkbox("Show IR", &bShowIRAudioPreview);
			});
		});

		if (bShowIRAudioPreview)
		{
			const ImVec2 windowPaddingBckp = ImGui::GetStyle().WindowPadding;

			// Remove padding for the window displaying IR
			ScopedStyle removePadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

			const WindowConfig config
			{
				.Size = ImVec2(300.0f, 150.0f),
				.SizeCond = ImGuiCond_FirstUseEver,
				.MinSize = ImVec2(200.0f, 100.0f),
				.Flags = ImGuiWindowFlags_NoCollapse
			};

			Window("Reverb IR Preview", config, [&]
			{
				// Restore padding for any pupups created by the Audio Preview
				ScopedStyle restorePadding(ImGuiStyleVar_WindowPadding, windowPaddingBckp);

				mIRAudioPreview.Draw("IR Audio Preview");
			}, &bShowIRAudioPreview);
		}
	}


	void LateReverbGUI::RefreshReverbProperties()
	{
		if (bUpdatingWaveform)
			return;
#if 0 //JPL_DBG_CONTROLS
		mIRReverb->SetDryLevel(mModel->DryLevel.Get());
		mIRReverb->SetWetLevel(mModel->WetLevel.Get());
#endif
		mIRReverb->SetRT60(mModel->T60.Get());
	}

	void LateReverbGUI::OnSourceChanged()
	{
		bUpdatingWaveform = true;
	}

	void LateReverbGUI::OnNewDataAwailable(const GUI::SampleData& sampleData)
	{
		bUpdatingWaveform = false;
#if JPL_PROFILE_IR_GEN
		IRGenTime = mIRWaveformSource.IRGenTime;
#endif
		RefreshReverbProperties();
	}

	//==========================================================================
	LateReverbGUI::IRWaveformSource::IRWaveformSource()
	{
		mDataUpdateRoutine = DataUpdateRoutine();
		mDataUpdateRoutine.Resume();
	}

	void LateReverbGUI::IRWaveformSource::GenerateIRWaveformData()
	{
		mUpdateIR.Set();
	}

	void LateReverbGUI::IRWaveformSource::SetReverb(std::shared_ptr<ReverbBus> reverb)
	{
		mReverb = reverb;
	}

	Coro::Task<> LateReverbGUI::IRWaveformSource::DataUpdateRoutine()
	{
		static constexpr float minIRLength = 1.0f;
		static constexpr float maxIRLength = 10.0f;
		static constexpr uint64 minLengthInSamples = minIRLength * cSampleRate;
		static constexpr uint64 maxLengthInSamples = maxIRLength * cSampleRate;

		GUI::SampleData sampleData;
		std::jthread backgroundThread;

		for (;;)
		{
			co_await mUpdateIR;

			sampleData.NumFrames = 0;
			sampleData.Samples.clear();

			Broadcast<&ChangeListener<GUI::SampleData>::OnSourceChanged>();

			float newIRGenTime = 0.0f;
			
			if (auto reverb = mReverb.lock())
			{
				co_await Coro::SwitchToAsync(backgroundThread);
				{
					newIRGenTime = GenerateIRSampleData(minLengthInSamples, maxLengthInSamples, *reverb, sampleData);
				}
				co_await Coro::SwitchToMainThread();
			}
			else
			{
				// Reverb wasn't set or was destroyed
				JPL_ASSERT(false);
			}

#if JPL_PROFILE_IR_GEN
			IRGenTime = newIRGenTime;
#endif

			Broadcast<&ChangeListener<GUI::SampleData>::OnNewDataAwailable>(sampleData);
		}
	}

	float LateReverbGUI::IRWaveformSource::GenerateIRSampleData(uint64 minLengthInSamples,
															   uint64 maxLengthInSamples,
															   ReverbBus& reverb,
															   GUI::SampleData& outSampleData)
	{
#if JPL_PROFILE_IR_GEN
		OnlineVariance variance;
#endif

		outSampleData.NumFrames = 0;
		outSampleData.Samples.clear();
		outSampleData.Samples.resize(maxLengthInSamples, 0.0f);

		static constexpr std::size_t cBufferSize = 480 * 2;

		StaticArray<float, cBufferSize> inBuffer;

		static const float cSilenceThreshold = dBToGain(-96.0f);

		float maxValue = 0.0f;

		int numSilentBlocks = 0;

		uint64 framesProcessed = 0;
		while (framesProcessed < maxLengthInSamples)
		{
			const uint64 numToProcess = std::min(cBufferSize, maxLengthInSamples - framesProcessed);

			inBuffer.resize(numToProcess, 0.0f);
			inBuffer[0] = framesProcessed == 0; // Add impulse if this is the first iteration

			std::span<float> outBuffer(&outSampleData.Samples[framesProcessed], numToProcess);

#if JPL_PROFILE_IR_GEN
			Timer timer;
#endif
		
			reverb.ProcessInterleaved(inBuffer, outBuffer, numToProcess);

#if JPL_PROFILE_IR_GEN
			variance.Add(timer.ElapsedMillis());
#endif

			framesProcessed += numToProcess;

			const float maxElement = *std::ranges::max_element(outBuffer, {}, [](float v) { return Math::Abs(v); });

			maxValue = std::max(maxElement, maxValue);

			if (framesProcessed > minLengthInSamples)
			{
				if (maxElement <= cSilenceThreshold)
				{
					if ((++numSilentBlocks) >= 10)
						break;
				}
				else
				{
					numSilentBlocks = 0;
				}
			}
		}

		// Subtract at least half silent blocks to make the cropping faster
		framesProcessed -= (numSilentBlocks >> 1) * cBufferSize;

		// Crop to frames processed
		outSampleData.Samples.resize(framesProcessed);

		// Crop to the first non-silent sample from the end
		auto reverseSamples = outSampleData.Samples | std::views::reverse;
		auto rit = std::ranges::find_if(reverseSamples, [](float v) { return Math::Abs(v) > cSilenceThreshold; });
		if (rit != reverseSamples.end())
		{
			framesProcessed = std::ranges::distance(outSampleData.Samples.begin(), rit.base());
			outSampleData.Samples.resize(framesProcessed);
		}

		outSampleData.NumFrames = framesProcessed;

		// Normalize, stretch to the entire vertical range
		if (framesProcessed > 0 and not Math::IsNearlyZero(maxValue, 1e-12f))
		{
			std::ranges::for_each(outSampleData.Samples, Algo::Multiply{ 1.0f / maxValue });
		}

#if JPL_PROFILE_IR_GEN
		return variance.Mean;
#else
		return -1.0f;
#endif
	}
} // namespace JPL
