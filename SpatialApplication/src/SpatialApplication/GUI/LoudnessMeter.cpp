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

#include "LoudnessMeter.h"

#include <JPLSpatial/Containers/StaticArray.h>

#include "ImGui/ImGui.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <concepts>
#include <cstring>

namespace JPL
{
	namespace Detail
	{
		//==============================================================================
		/** Converts an absolute value to decibels.*/
		template<std::floating_point T>
		[[nodiscard]] static T ToDecibels(T absoluteValue, T minusInfinity)
		{
			return absoluteValue <= T(0.0)
				? minusInfinity
				: T(20.0) * std::log10(absoluteValue);
		}

		/** Converts a value in decibels to an absolute value.*/
		template<std::floating_point T>
		[[nodiscard]] static T DecibelsToAbsolute(T decibelsValue, T minusInfinity)
		{
			return decibelsValue <= minusInfinity
				? 0.0
				: std::pow(10, (decibelsValue * T(0.05)));
		}

#if 0
		/**	Calculate skewed value with clamped displayed value range (could use for frequency spectrum and filter cutoff values)
		*/
		static inline float Normalize(float enteredValue, float minEntry, float maxEntry, float normalizedMin, float normalizedMax)
		{
			float mx = (std::log((enteredValue - minEntry)) / (std::log(maxEntry - minEntry)));
			float preshiftNormalized = mx * (normalizedMax - normalizedMin);
			float shiftedNormalized = preshiftNormalized + normalizedMin;

			return shiftedNormalized;
		}
#endif

		// Apply cube root transformation to normalized value
		JPL_INLINE float SkewScale(float value) { return std::pow(value, 0.25f); }
		JPL_INLINE float UnSkewScale(float value) { return std::pow(value, 1.0f / 0.25f); }

		static const char* AbsoluteTodBString(float absoluteValue, float minusInfinity, std::span<char> outStringBuffer, bool addUnits = true)
		{
			JPL_ASSERT(outStringBuffer.size() >= 8);

			const float dbValue = Detail::ToDecibels(Detail::UnSkewScale(absoluteValue) * 2.0f, minusInfinity);

			static constexpr std::u8string_view MINUS_INF_CHAR = u8"-\u221E";

			if (dbValue <= minusInfinity)
			{
				std::strcpy(outStringBuffer.data(), reinterpret_cast<const char*>(MINUS_INF_CHAR.data()));
			}
			else
			{
				auto result = std::format_to_n(outStringBuffer.data(), outStringBuffer.size() - 1, "{:.1f}{}", dbValue, addUnits ? " dB" : "");
				*result.out = '\0';
			}

			return outStringBuffer.data();
		}

	}

	// Forward declare
	static void DrawBigLevelsLabel(const typename LoudnessMeter::Levels& levels, float minusInfinity, ImVec2 boundsStart, float channelWidth, ImVec2 textSize, float edgeOffset, float fontSize);
	static void DrawChannel(ImVec2 start, ImVec2 end, float width, float height, float level, const ImColor& colorTop, const ImColor& colorBot, float peakHoldLevel = 0.0f);
	static void DrawDBMark(ImVec2 start, ImVec2 end, float width, float height, float levelNormalized, float currentLevel, const char* dBText);

	void LoudnessMeter::SetNumberOfChannels(uint32_t numberOfChannels)
	{
		mNumberOfChannels = numberOfChannels;
		mPrevLevels.assign(numberOfChannels, { 0.0f, 0.0f });
		mLevels.assign(numberOfChannels, { 0.0f, 0.0f });
		mPeakHoldTicks.assign(numberOfChannels, mPeakHoldTime);
		m_PeakHoldLevels.assign(numberOfChannels, 0.0f);
	}

	void LoudnessMeter::SubmitLevels(std::span<const float> peak, std::span<const float> rms)
	{
		static constexpr uint32 cMaxNumChannels = 256;
		JPL_ASSERT(peak.size() <= cMaxNumChannels);
		JPL_ASSERT(peak.size() == rms.size());
		
		StaticArray<typename JPL::LoudnessMeter::Levels, cMaxNumChannels> levels;
		for (uint32_t i = 0; i < peak.size(); ++i)
			levels.push_back({ .Peak = peak[i], .RMS = rms[i] });

		SubmitLevels(levels);
	}

	void LoudnessMeter::SubmitLevels(std::span<const Levels> levelsNormalized)
	{
		JPL_ASSERT(!levelsNormalized.empty());

		if (mLevels.empty())
		{
			// Initialize vectors of datas for the number of channels
			SetNumberOfChannels(levelsNormalized.size());

			// Initialize previous levels
			mPrevLevels.resize(levelsNormalized.size());
			std::ranges::copy(levelsNormalized, mPrevLevels.begin());

			// Pre-calculate skew for a more usable display of levels
			for (auto& l : mPrevLevels)
			{
				JPL_ASSERT(l.Peak >= 0.0f && l.RMS >= 0.0f);

				l.Peak = Detail::SkewScale(0.5f * l.Peak);
				l.RMS = Detail::SkewScale(0.5f * l.RMS);
			}
		}
		else
		{
			mPrevLevels = mLevels;
		}

		// Update current levels
		mLevels.resize(levelsNormalized.size());
		std::ranges::copy(levelsNormalized, mLevels.begin());

		// Pre-calculate skew for a more usable display of levels
		for (auto& l : mLevels)
		{
			JPL_ASSERT(l.Peak >= 0.0f && l.RMS >= 0.0f);

			l.Peak = Detail::SkewScale(0.5f * l.Peak);
			l.RMS = Detail::SkewScale(0.5f * l.RMS);
		}
	}

	void LoudnessMeter::OnRender(float deltaTime)
	{
		auto* drawList = ImGui::GetWindowDrawList();
		ImVec2 start = ImGui::GetCursorScreenPos();
		ImVec2 bounds = ImGui::GetContentRegionAvail();
		ImVec2 end{ start.x + bounds.x, start.y + bounds.y };
		const float edgeOffset = 4.0f;

		// Fill background with (almost) black colour
		drawList->AddRectFilled(start, end, IM_COL32(7, 7, 7, 255));

		// Calculate channel meter sizes
		const uint32_t numChannels = mNumberOfChannels;
		const float channelWidth = (bounds.x - 1.0f) / numChannels;
		float channelHeight = bounds.y;
		const float gap = 1.0f;

		static constexpr float cValueLabelFontSizeBig = 23.0f;
		static constexpr float cValueLabelFontSizeSmall = 18.0f;

		// TODO: move this big label size calculation into a function

		// Calculate text size and how much space we have for text labels
		ImVec2 textSize;
		{
			ImGuiEx::ScopedFont boldFont(GUI::GetBoldFont(), cValueLabelFontSizeBig);
			textSize = ImGui::CalcTextSize("-48.0");
		}

		bool peakDBLableCanFit = true;
		const bool peakDBLabelFullSize = channelWidth >= edgeOffset * 3.0f + textSize.x;
		if (!peakDBLabelFullSize)
		{
			ImGuiEx::ScopedFont boldFont(GUI::GetBoldFont(), cValueLabelFontSizeSmall);
			textSize = ImGui::CalcTextSize("-48.0");

			peakDBLableCanFit = channelWidth >= edgeOffset * 2.0f + textSize.x;
		}

		const float bigLabelFontSize = peakDBLabelFullSize ? cValueLabelFontSizeBig : cValueLabelFontSizeSmall;

		// Calculate channel meter offset to give space for the current level label
		const float peakDBValueHeight = textSize.y + edgeOffset * 5.0f;
		channelHeight -= peakDBValueHeight;

		// Tooltip about what kind of dB values displayed on the big label
		if (ImGui::ItemHoverable({ start, start + ImVec2(end.x, peakDBValueHeight) }, ImGui::GetID("BigLabel"), 0))
		{
			ImGuiEx::ScopedStyle tooltipPadding(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
			ImGuiEx::ScopedColour textCol(ImGuiCol_Text, GUI::Colours::Theme::TextBrighter);
			// TODO: we may want to let user switch between Peak vs RMS display for the big label
			ImGui::SetTooltip("RMS");
		}

		ImGuiEx::ScopedID meterID("LoudnessMeter");

		for (uint32_t i = 0; i < mNumberOfChannels; ++i)
		{
			Levels& levels = mLevels[i];
			const Levels& prevLevels = mPrevLevels[i];

			// Calculate channel meter bounds
			ImVec2 channelBoundsStart{ gap + start.x + channelWidth * i, start.y };
			const ImVec2 channelBoundsEnd{ start.x + channelWidth * (i + 1), end.y };

			// Curren dB level text label
			if (peakDBLableCanFit)
			{
				DrawBigLevelsLabel(levels, mMinusInfinity, channelBoundsStart, channelWidth, textSize, edgeOffset, bigLabelFontSize);
			}

			// Offset channel meter to give space for current level label
			channelBoundsStart.y += peakDBValueHeight;

			// Smoothly decay level meters
			const float fallOff = deltaTime / mFallOffTime;
			if (levels.Peak < prevLevels.Peak)
			{
				levels.Peak = std::max(levels.Peak, prevLevels.Peak - fallOff);
			}

			if (levels.RMS < prevLevels.RMS)
			{
				levels.RMS = std::max(levels.RMS, prevLevels.RMS - fallOff);
			}

			// Peak hold
			float& pickHoldTick = mPeakHoldTicks[i];
			float& pickHoldLevel = m_PeakHoldLevels[i];
			if (levels.Peak < pickHoldLevel)
			{
				// Peak hold tick
				pickHoldTick -= deltaTime;
				if (pickHoldTick <= 0.0f)
					pickHoldLevel -= fallOff;
			}
			else
			{
				// Update held Peak level
				pickHoldTick = mPeakHoldTime;
				pickHoldLevel = levels.Peak;
			}

			// dB Values Tooltip
			if (ImGui::ItemHoverable({ channelBoundsStart, channelBoundsEnd }, i, 0))
			{
				ImGuiEx::ScopedStyle tooltipPadding(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
				ImGuiEx::ScopedColour textCol(ImGuiCol_Text, GUI::Colours::Theme::TextBrighter);

				char bufferPeak[16]{}; char bufferRMS[16]{};

				ImGui::SetTooltip("Peak: %s RMS: %s",
								  Detail::AbsoluteTodBString(levels.Peak, mMinusInfinity, bufferPeak),
								  Detail::AbsoluteTodBString(levels.RMS, mMinusInfinity, bufferRMS));
			}

			auto lerpColour = [](const ImColor& a, const ImColor& b, float t)
			{
				auto& colA = a.Value;
				auto& colB = b.Value;

				return ImGui::ColorConvertFloat4ToU32(ImVec4{ colA.x + t * (colB.x - colA.x),
															colA.y + t * (colB.y - colA.y),
															colA.z + t * (colB.z - colA.z),
															colA.w + t * (colB.w - colA.w) });
			};

			// Peak meter
			{
				const ImColor colBot(39, 179, 74, 255);
				const ImColor colTop = lerpColour(colBot, ImColor(52, 235, 98, 255), std::min(levels.Peak, 1.0f));

				DrawChannel(channelBoundsStart, channelBoundsEnd,
							channelWidth, channelHeight,
							levels.Peak,
							colTop, colBot,
							m_PeakHoldLevels[i]);
			}

			// RMS meter
			{
				const ImColor colBot(22, 102, 73, 255);
				const ImColor colTop = lerpColour(colBot, ImColor(34, 153, 109, 255), std::min(levels.RMS, 1.0f));

				DrawChannel(channelBoundsStart, channelBoundsEnd,
							channelWidth, channelHeight,
							levels.RMS,
							colTop, colBot);
			}


			// TODO: move this ruler definition and drawing into a function

			struct SkewedMark
			{
				SkewedMark(float exp, const char* label)
					: Height(Detail::SkewScale(std::pow(0.5f, exp)))
					, Label(label)
				{}
				float Height;
				const char* Label;
			};

			static const auto marks = std::to_array(
			{
				SkewedMark(0.0f, "6"),
				SkewedMark(1.0f, "0"),
				SkewedMark(2.0f, "6"),
				SkewedMark(3.0f, "12"),
				SkewedMark(4.0f, "18"),
				SkewedMark(5.0f, "24"),
				SkewedMark(6.0f, "30"),
				SkewedMark(7.0f, "36"),
				SkewedMark(9.0f, "48"),
				SkewedMark(12.0f, "66")
			});

			// dB ruler
			// TODO: dynamically distribute available height between dB markers
			{
				ImGuiEx::ScopedFont font(GUI::GetBoldFont(), 15.0f);

				for (const SkewedMark mark : marks)
				{
					DrawDBMark(channelBoundsStart, channelBoundsEnd, channelWidth, channelHeight, mark.Height, levels.Peak, mark.Label);
				}
			}
		}
	}

	static void DrawBigLevelsLabel(const typename LoudnessMeter::Levels& levels, float minusInfinity, ImVec2 boundsStart, float channelWidth, ImVec2 textSize, float edgeOffset, float fontSize)
	{
		// TODO: display peak hold value and reset when stopped or clicked?

		ImGui::SetCursorScreenPos(ImVec2(boundsStart.x, boundsStart.y + edgeOffset));
		{
			ImGuiEx::ScopedColour textColour(ImGuiCol_Text, IM_COL32(255, 255, 255, 70));
			ImGuiEx::ScopedFont font(GUI::GetBoldFont(), fontSize);

			char dbLabelString[16]{};
			Detail::AbsoluteTodBString(levels.RMS, minusInfinity, dbLabelString, /*addUnits*/ false);
			ImGui::TextAligned(0.5f, channelWidth, dbLabelString);
		}
	}

	static void DrawChannel(ImVec2 start, ImVec2 end, float width, float height, float level, const ImColor& colorTop, const ImColor& colorBot, float peakHoldLevel)
	{
		const bool clipping = level >= Detail::SkewScale(0.5f);
		const ImColor colClipping(255, 50, 75, 255);

		level = std::min(level, 1.0f);

		// TODO
		////	1. clip indicator
		////	2. darker line at the top edge of meters
		////	3. dB markings
		////	4. dynamic layout
		////	5. abstract peak from rms into separat DrawChannel calls
		////	6. move text level lables outside of this function?
		////	7. add +6 dB meter area
		////	8. peak hold
		//	9. vertical dynamic layout

		const float levelY = end.y - level * height;
		const ImVec2 rectTop{ start.x, levelY };

		const ImU32 colTop = clipping ? colClipping : colorTop;
		const ImU32 colBot = clipping ? colClipping : colorBot;

		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilledMultiColor(rectTop, end, colTop, colTop, colBot, colBot);

		// Draw a darker cap
		drawList->AddRectFilled(rectTop, { end.x, rectTop.y + 2.0f }, IM_COL32(0, 0, 0, 90));

		// Draw held peak
		if (peakHoldLevel > 0.0f)
		{
			peakHoldLevel = std::min(peakHoldLevel, 1.0f);
			const float peakHoldLevelY = std::floorf(end.y - peakHoldLevel * height);
			const ImU32 colPeakHold = IM_COL32(69, 230, 171, 255);

			drawList->AddRectFilled({ start.x, peakHoldLevelY }, { end.x, peakHoldLevelY + 3.0f }, colPeakHold);
		}
	}

	static void DrawDBMark(ImVec2 start, ImVec2 end, float width, float height, float levelNormalized, float currentLevel, const char* dBText)
	{
		const float currentLevelY = end.y - currentLevel * height;
		const float levelY = std::floorf(end.y - levelNormalized * height);

		const float lineLength = std::min(8.0f, width);
		const ImVec2 linePos{ start.x, levelY };
		const ImVec2 lineEnd{ start.x + lineLength, levelY };

		const float textEdgeOffset = 3.0f;
		const ImVec2 textSize = ImGui::CalcTextSize(dBText);
		const bool textCanFit = width >= textSize.x + textEdgeOffset * 2.0f;
		const float textAndLineFullWidth = textSize.x + textEdgeOffset * 2 + lineLength;

		//? This would be needed if we don't offset for the current level value label
		const bool textTouchingTopEdge = false;// start.y == levelY;

		const float textTop = width >= textAndLineFullWidth && !textTouchingTopEdge
			? levelY - textSize.y * 0.5f// + 1.0f
			: levelY + 6.0f;

		const bool textCoveredByLevelBar = currentLevelY <= textTop;
		const bool lineCoveredByLevelBar = currentLevel > levelNormalized;

		const ImU32 colBright = IM_COL32(255, 255, 255, 70);
		const ImU32 colDark = IM_COL32(15, 15, 15, 255);
		const ImU32 colLine = lineCoveredByLevelBar ? colDark : colBright;

		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddLine(linePos, lineEnd, colLine, 1.0f);

		// Draw text if we have enough space
		if (textCanFit)
		{
			const float textXFullWidth = textTouchingTopEdge ? linePos.x + textEdgeOffset
				: linePos.x + lineLength + textEdgeOffset;

			const ImVec2 textPos = width >= textAndLineFullWidth
				? ImVec2{ textXFullWidth, textTop }
				: ImVec2{ linePos.x + textEdgeOffset, textTop };

			const bool textCoverecPartially = currentLevelY >= textPos.y && currentLevelY <= textPos.y + ImGui::GetTextLineHeight();

			ImGui::SetCursorScreenPos(textPos);
			{
				ImGuiEx::ScopedColour text(ImGuiCol_Text,
										   textCoverecPartially ? IM_COL32_WHITE : textCoveredByLevelBar ? colDark : colBright);
				ImGui::Text(dBText);
			}
		}
	}
} // namespace JPL
