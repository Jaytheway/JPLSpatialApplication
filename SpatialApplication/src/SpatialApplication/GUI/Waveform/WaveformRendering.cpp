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

#include "WaveformRendering.h"

#include "Systems/EventsLoop.h"

#include <JPLSpatial/Math/SIMD.h>
#include <JPLSpatial/Math/SIMDMath.h>

#undef min
#undef max

#include <algorithm>
#include <vector>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <implot.h>

namespace JPL::GUI
{
	// Forward declaration
	static [[nodiscard]] std::vector<typename Waveform::ChannelData>
		GenerateMinMaxValues(const std::vector<float>& sampleData, uint64_t numFrames, int frameWidth);


	Waveform::Waveform(WaveformDataSource& waveformDataSorce)
		: mDataSource(waveformDataSorce)
	{
		mDataSource.AddListener(this);

		mWaveformUpdateRoutine = WaveformUpdateRoutine();
		mWaveformUpdateRoutine.Resume(); // start the routine
	}

	Waveform::~Waveform() noexcept
	{
		mDataSource.RemoveListener(this);
	}

	void Waveform::OnSourceChanged()
	{
		// Set the update state to display message
		// to the user immediately when source changes.
		// 
		// OnNewDataAwailable will trigger the actual update,
		// when new sample data is loaded.
		bUpdatingWaveform = true;
	}

	void Waveform::OnNewDataAwailable(const SampleData& newSampleData)
	{
		mSampleDataCache = newSampleData;
		bUpdatingWaveform = true;
		mWaveformNeedsUpdate.Set();
	}

	bool Waveform::Draw(const char* itemId)
	{
		ImGuiEx::ScopedGroup group;

		const uint32_t numChannels = mChannelData.size();

		const float borderSize = ImGui::GetStyle().ChildBorderSize;

		// When we have multiple subplots, ImGui adds a scrollbard, even uf item spacing set to 0
		const ImVec2 plotSpace =
			Vec2(ImGui::GetContentRegionAvail()) -			// content region
			Vec2(borderSize * 2.0f, borderSize * 2.0f) - 	// subtract border
			Vec2(0.0f, numChannels > 0 ? float(numChannels - 1) : 0.0); // subtract border between channels

		// If we have valid data source, but the waveform widget was never draw,
		// the width in pixels would be 0, and no waveform generated for the valid source.
		// Therefore we need to manually trigger update.
		if (mWaveformWidthPx == 0)
		{
			mWaveformWidthPx = static_cast<int>(plotSpace.x);

			if (not mSampleDataCache.Samples.empty())
			{
				mWaveformNeedsUpdate.Set();
			}
		}

		// TODO: we may want to also update the waveform if the display width changes
		mWaveformWidthPx = static_cast<int>(plotSpace.x);

		// Use unique item ID
		const ImGuiID itemID = ImGui::GetCurrentWindow()->GetID(itemId);

		const ImVec2 bbMin = ImGui::GetCursorScreenPos();
		const ImVec2 bbMax = bbMin + ImGui::GetContentRegionAvail();
		const ImRect itemBB{ bbMin, bbMax };

		if (not ImGui::ItemAdd(itemBB, itemID))
			return false;

		// Draw border outline
		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(bbMin, bbMax, ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().FrameRounding);


		if (mChannelData.empty() and not bUpdatingWaveform)
		{
			// The waveform item exists, just empty
			return true;
		}

#if 0	// Enable to play around with waveform colour
		JPL::ImGuiEx::Window("Waveform Col", [&]
		{
			ImGui::ColorPicker4("Waveform Fill", &mWaveformFillColour.Value.x);
			ImGui::ColorPicker4("Waveform Outline", &mWaveformLineColour.Value.x);
		});
#endif

		JPL::ImGuiEx::ScopedColour frameBg(ImGuiCol_FrameBg, IM_COL32(14, 14, 14, 255));
		JPL::ImGuiEx::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		if (bUpdatingWaveform)
		{
			// TODO: draw a nice animation isntead of just boring text

			ImGuiEx::LayoutHorizontal("text_align", ImGui::GetContentRegionAvail(), 0.5f, [&]
			{
				ImGuiEx::ScopedDisable greyColour;
				ImGui::TextAligned(0.5f, mWaveformWidthPx, "Generating waveform...");
			});
		}
		else
		{
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0.0f, 0.0f));
			const ImPlotSpec spec(
				ImPlotProp_LineColor, mWaveformLineColour,
				ImPlotProp_FillColor, mWaveformFillColour,
				ImPlotProp_LineWeight, mLineThickness
			);

			std::vector<float> indices; // TODO: this technically should be time position and based on time position it asks for a min-max values?
			indices.resize(mChannelData[0].Min.size());
			for (size_t i = 0; i < indices.size(); ++i)
				indices[i] = (float)i;

			const auto drawChannel = [&](const char* plotId, const std::vector<float>& min, const std::vector<float>& max, bool drawTimeline)
			{
				if (ImPlot::BeginPlot(plotId, ImVec2(0, 0), ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame/* | ImPlotFlags_NoChild*/))
				{
					ImPlot::SetupAxis(ImAxis_X1, nullptr,
										(drawTimeline ? 0 : ImPlotAxisFlags_NoTickLabels)
										| ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks
										| ImPlotAxisFlags_NoHighlight
										| ImPlotAxisFlags_AutoFit);

					ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight);
					ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImPlotCond_Always);

					ImPlot::PlotShaded("fill", indices.data(), min.data(), max.data(), indices.size(), spec);

					ImPlot::PlotLine("min", min.data(), min.size(), 1.0f, 0.0, spec);
					ImPlot::PlotLine("max", max.data(), max.size(), 1.0f, 0.0, spec);

					ImPlot::EndPlot();
				}
			};

			const ImPlotSubplotFlags flags
				= ImPlotSubplotFlags_NoResize
				| ImPlotSubplotFlags_LinkRows
				| ImPlotSubplotFlags_LinkAllX
				| ImPlotSubplotFlags_LinkAllY
				| ImPlotSubplotFlags_NoLegend
				| ImPlotSubplotFlags_ShareItems;

			if (indices.size() && ImPlot::BeginSubplots("##Waveform Plot", numChannels, 1, plotSpace, flags))
			{
				char id = 'a';
				for (const ChannelData& channelData : mChannelData)
				{
					drawChannel(&id, channelData.Min, channelData.Max, false);
					id++;
				}

				ImPlot::EndSubplots();
			}

			ImPlot::PopStyleVar(); // ImPlotStyleVar_PlotPadding
		}

		return true;
	}

	Coro::Task<> Waveform::WaveformUpdateRoutine()
	{
		// The memory won't be deallocated until the Task is destroyed
		std::vector<float> sampleData;
		std::vector<ChannelData> outChannelData;
		SampleData newSampleData;

		// Hold on to the background thread handle to not let it dangle away
		std::jthread backgroundThread;

		for (;;)
		{
			// Wait for the waveform data source to be updated
			co_await mWaveformNeedsUpdate;

			newSampleData = mSampleDataCache;

			// Get the latest required waveform width in pixels
			const uint64 waveformPx = mWaveformWidthPx;

			if (newSampleData.NumFrames > 0)
			{
				co_await Coro::SwitchToAsync(backgroundThread); // --- Process file on background thread
				{
					outChannelData = GenerateMinMaxValues(newSampleData.Samples, newSampleData.NumFrames, waveformPx);
				}
				co_await Coro::SwitchToMainThread(); // --- Update waveform data on main thread
			}

			mChannelData = outChannelData;
			outChannelData.clear();

			// Let the GUI know the update is no longer in progress
			bUpdatingWaveform = false;
		}
	}

	std::vector<typename Waveform::ChannelData> GenerateMinMaxValues(const std::vector<float>& sampleData, uint64_t numFrames, int frameWidth)
	{
		using ChannelData = typename Waveform::ChannelData;

		std::vector<ChannelData> outChannelData;

		const uint32_t numChannels = sampleData.size() / numFrames;

		if (numChannels == 0 || frameWidth < 1 || numFrames < 1 || sampleData.empty())
		{
			return outChannelData;
		}

		outChannelData.resize(numChannels);

		for (ChannelData& channelData : outChannelData)
		{
			channelData.Min.clear();
			channelData.Max.clear();
		}

		const uint64 resolutionWidth = std::min(static_cast<uint64>(frameWidth), numFrames);
		const float timeStep = 1.0f / static_cast<float>(resolutionWidth);
		const int64 itemCount = numFrames - 1;

		for (uint32_t channelIndex = 0; channelIndex < outChannelData.size(); ++channelIndex)
		{
			ChannelData& channelData = outChannelData[channelIndex];

			// Get pointer to channel sample data in our contiguous buffer
			const float* data = &sampleData[channelIndex * numFrames];

			//! Note: we can pass in value offset to display a shorter window of the sample data

			//float v0 = data[0 /* valueOffset */];
			float t0 = 0.0f;

			uint64 v0_idx = 0; /* valueOffset */

#if 1		// Slightly vectorized version of the algo below

			channelData.Min.reserve(resolutionWidth);
			channelData.Max.reserve(resolutionWidth);

			for (uint64 s = 0; s < resolutionWidth; ++s)
			{
				const float t1 = std::min(1.0f, t0 + timeStep);

				const auto v1_idx = static_cast<uint64>(t1 * itemCount + 0.5f);
				JPL_ASSERT(v1_idx < numFrames);

				const float v1 = data[/* valueOffset +*/ (v1_idx + 1) % numFrames];

				simd minV = v1, maxV = v1;

				const uint64 numValuesInItem = v1_idx - v0_idx;
				const uint64 toSIMD = JPL::FloorToSIMDSize(numValuesInItem);

				uint32_t si = 0;
				for (; si < toSIMD; si += simd::size())
				{
					JPL_ASSERT((v0_idx + si + simd::size()) < numFrames);

					const simd v(&data[v0_idx + si]);
					minV = min(v, minV);
					maxV = max(v, maxV);
				}

				float minS = minV.reduce_min(), maxS = maxV.reduce_max();

				for (; si < numValuesInItem; ++si)
				{
					const float v = data[(v0_idx + si) % numFrames];

					if (v < minS)
						minS = v;
					if (v > maxS)
						maxS = v;
				}

				channelData.Min.push_back(minS);
				channelData.Max.push_back(maxS);

				t0 = t1;
				v0_idx = v1_idx;
			}
#else
			channelData.Min.reserve(resolutionWidth);
			channelData.Max.reserve(resolutionWidth);

			for (uint64 s = 0; s < resolutionWidth; ++s)
			{
				const float t1 = std::min(1.0f, t0 + timeStep);

				const auto v1_idx = static_cast<uint64>(t1 * itemCount + 0.5f);
				JPL_ASSERT(v1_idx < numFrames);

				const float v1 = data[/* valueOffset +*/ (v1_idx + 1) % numFrames];

				float min = v1, max = v1;
				for (uint32_t s = 0; s < v1_idx - v0_idx; ++s)
				{
					const float v = data[/* valueOffset +*/ (v0_idx + s) % numFrames];

					if (v < min)
						min = v;
					if (v > max)
						max = v;
				}

				channelData.Min.push_back(min);
				channelData.Max.push_back(max);

				t0 = t1;
				v0_idx = v1_idx;
			}
#endif
		}

		return outChannelData;
	}

} // namespace JPL::GUI
