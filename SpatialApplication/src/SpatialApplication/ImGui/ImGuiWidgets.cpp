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

#include "ImGuiWidgets.h"

#include "Walnut/Application.h"

#include <JPLSpatial/Core.h>

namespace JPL::ImGuiEx
{
    bool PropertyCheckbox(const char* label, Property<bool>& property)
    {
        bool bValue = property.Get();
        ScopedItemOutline outline(label, OutlineFlags_NoOutlineInactive, ImColor(60, 60, 60));
        if (ImGui::Checkbox(label, &bValue))
        {
            property.Set(bValue);
            return true;
        }
        else
        {
            return false;
        }
    }

    void DrawButtonImage(const Walnut::Image& imageNormal, const Walnut::Image& imageHovered, const Walnut::Image& imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax, ImVec2 uv0, ImVec2 uv1)
    {
        auto* drawList = ImGui::GetWindowDrawList();
        if (ImGui::IsItemActive())
        {
            drawList->AddImage(imagePressed.GetDescriptorSet(), rectMin, rectMax, uv0, uv1, tintPressed);
        }
        else if (ImGui::IsItemHovered())
        {
            drawList->AddImage(imageHovered.GetDescriptorSet(), rectMin, rectMax, uv0, uv1, tintHovered);
        }
        else
        {
            drawList->AddImage(imageNormal.GetDescriptorSet(), rectMin, rectMax, uv0, uv1, tintNormal);
        }
    }

    bool PathButton(const char* label, ImVec2 size)
    {
#if defined(JPL_PLATFORM_WINDOWS)
        static constexpr std::string_view separator = "\\";
#elif defined(JPL_PLATFORM_LINUX) or defined(JPL_PLATFORM_FREEBSD) or defined(JPL_PLATFORM_MACOS) or defined(JPL_PLATFORM_WASM)
        static constexpr std::string_view separator = "/";
#endif
        ScopedColourStack buttonColours(
            ImGuiCol_Button, IM_COL32_BLACK_TRANS,
            ImGuiCol_Border, IM_COL32_BLACK_TRANS
        );
        Conditional<ScopedColour> separatorColor(
            std::string_view(label) == separator,
            ImGuiCol_Text, GUI::Colours::Theme::TextDarker
        );
        ScopedStyle innerPadding(
            ImGuiStyleVar_FramePadding, ImVec2(2.0f, ImGui::GetStyle().FramePadding.y)
        );
        return ImGui::Button(label, size);
    }

	void DrawMainWindowButtons(ImVec2 titlebarMin, ImVec2 titlebarMax)
	{
		JPL::ImGuiEx::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		const bool bIsMaximized = Walnut::Application::Get().IsMaximized();

		const float buttonWidth = 36.0f;
		const float buttonHeight = 26.0f;
		const float buttonsStartX = titlebarMax.x - buttonWidth * 3.0f;
		const float titlebarVerticalOffset = bIsMaximized ? 6.0f : 0.0f;

		ImGui::SetCursorScreenPos(ImVec2(buttonsStartX, titlebarMin.y + titlebarVerticalOffset));

		const float iconPadding = 13.0f;

		auto getFillColour = []
		{
			return ImGui::IsItemActive()
				? IM_COL32(255, 255, 255, 10)
				: ImGui::IsItemHovered()
					? IM_COL32(255, 255, 255, 20)
					: IM_COL32_BLACK_TRANS;
		};

		auto getIconColour = []
		{
			return ImGui::IsItemActive()
				? JPL::GUI::Colours::Theme::TextDarker
				: (ImU32)JPL::Colour(JPL::GUI::Colours::Theme::Text).WithMultipliedValue(0.9f);
		};

		
		auto* drawList = ImGui::GetForegroundDrawList();

		// Minimize Button
		{
			if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight)))
			{
				Walnut::Application::Get().Minimize();
			}
			const ImU32 fillColour = getFillColour();
			const ImU32 iconColour = getIconColour();

			auto rect = JPL::ImGuiEx::GetItemRect();

			drawList->AddRectFilled(rect.Min, rect.Max, fillColour, 0.0f);

			const ImVec2 lineP1 = rect.Min + ImVec2(iconPadding, rect.GetSize().y * 0.5f);
			const ImVec2 lineP2 = lineP1 + ImVec2(rect.GetSize().x - iconPadding * 2.0f, 0.0f);
			drawList->AddLine(lineP1, lineP2, iconColour, 1.0f);
		}

		// Maximize Button
		{
			if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
			{
				Walnut::Application::Get().ToggleMaximize();
			}
			const ImU32 fillColour = getFillColour();
			const ImU32 iconColour = getIconColour();

			auto rect = JPL::ImGuiEx::GetItemRect();

			drawList->AddRectFilled(rect.Min, rect.Max, fillColour, 0.0f);

			const ImVec2 center = rect.GetCenter();
			const float radius = (rect.GetWidth() * 0.5f - iconPadding) * 0.8f;
			ImRect square{ center - ImVec2(radius, radius), center + ImVec2(radius, radius) };

			if (bIsMaximized)
			{
				// translate for the first square
				square.Translate(ImVec2(1.0f, -1.0f));
				drawList->AddRect(square.Min, square.Max, iconColour, 0.0f, 0, 1.0f);

				// translate for the second square that overlaps the first one
				square.Translate(ImVec2(-2.0f, 2.0f));

				// fill the suqare on top with opaque titlebar colour
				const ImU32 iconFill = JPL::GUI::Colours::Theme::Titlebar;
				drawList->AddRectFilled(square.Min, square.Max, iconFill, 0.0f, 0);
				// add the hover effect to our opaque fill
				drawList->AddRectFilled(square.Min, square.Max, fillColour, 0.0f, 0);
				// add bright outline
				drawList->AddRect(square.Min, square.Max, iconColour, 0.0f, 0, 1.0f);
			}
			else
			{
				square.Expand(1.0f);
				drawList->AddRect(square.Min, square.Max, iconColour, 0.0f, 0, 1.0f);
			}
		}

		// Close Button
		{
			if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
			{
				Walnut::Application::Get().Close();
			}

			auto rect = JPL::ImGuiEx::GetItemRect();

			// Close button is the only one uses different fill colour (red)
			const ImU32 fillColour = ImGui::IsItemActive()
									? IM_COL32(255, 50, 50, 150)
									: ImGui::IsItemHovered()
										? IM_COL32(235, 50, 50, 255)
										: IM_COL32_BLACK_TRANS;

			// Draw icon dark, to make sure it's visible on the red background fill
			const ImU32 iconColour = ImGui::IsItemActive() || ImGui::IsItemHovered()
						? JPL::GUI::Colours::Theme::Titlebar
						: getIconColour();

			drawList->AddRectFilled(rect.Min, rect.Max, fillColour, 0.0f);

			const ImVec2 center = rect.GetCenter();
			const float radius = (rect.GetWidth() * 0.5f - iconPadding);
			const ImRect square{ center - ImVec2(radius, radius), center + ImVec2(radius, radius) };

			drawList->AddLine(square.GetTL(), square.GetBR() - ImVec2(0.5f, 0.5f), iconColour, 1.5f);
			drawList->AddLine(square.GetTR() - ImVec2(0.5f, 0.5f), square.GetBL() - ImVec2(0.0f, 1.0f), iconColour, 1.5f);
		}
	}

	bool IconButton(const char* label, const IconButtonStyle& style)
	{
		const ImVec2 position = ImGui::GetCursorScreenPos();

		ImVec2 buttonSize = ImGui::CalcTextSize(label) + ImGui::GetStyle().FramePadding * 2.0f;

		if (style.ButtonSize.x != 0.0f)
		{
			buttonSize.x = ImMax(buttonSize.x, style.ButtonSize.x);
		}

		if (style.ButtonSize.y != 0.0f)
		{
			buttonSize.y = ImMax(buttonSize.y, style.ButtonSize.y);
		}

		// Mouse interaction handled by invisible button
		const bool bPressed = ImGui::InvisibleButton(label, buttonSize);
		
		// Visuals we draw outselves...

		const ImU32 backgroundColour =
			ImGui::IsItemActive() ? style.BgColourPressed :
			ImGui::IsItemHovered() ? style.BgColourHovered :
			style.BgColourNormal;

		const ImU32 iconColour =
			ImGui::IsItemActive() ? style.ColourPressed :
			ImGui::IsItemHovered() ? style.ColourHovered :
			style.ColourNormal;

		const ImVec2 bbMin = ImGui::GetItemRectMin();
		const ImVec2 bbMax = ImGui::GetItemRectMax();

		// Draw background and border
		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(bbMin, bbMax, backgroundColour, ImGui::GetStyle().FrameRounding);
		if (style.bDrawBorder)
		{
			drawList->AddRect(bbMin, bbMax, style.BorderColour, ImGui::GetStyle().FrameRounding);
		}
		
		// Draw icon text label
		DrawTextCentered(*drawList, bbMin, bbMax, label, iconColour);

		return bPressed;
	}

	/*IconButtonStyle IconButtonStyle::Make(ImU32 colourNormal, ImU32 colourHovered, ImU32 colourPressed)
	{
		return IconButtonStyle{
			.ColourNormal = colourNormal,
			.ColourHovered = colourHovered,
			.ColourPressed = colourPressed,
			.BgColourNormal = IM_COL32_BLACK_TRANS,
			.BgColourHovered = IM_COL32_BLACK_TRANS,
			.BgColourPressed = IM_COL32_BLACK_TRANS
		};
	}

	IconButtonStyle IconButtonStyle::Default()
	{
		return IconButtonStyle();
	}*/

	uint32 DrawGEQ(const char* itemId, std::span<float> values, std::span<const float> frequencies, float minValue, float maxValue, const ImVec2& size)
	{
		if (not JPL_ENSURE(frequencies.size() == values.size() - 1 or frequencies.size() == values.size()))
		{
			return 0;
		}

		using namespace ImGuiEx;

		ScopedID geqID(itemId);

		uint32 bModifiedBand = 0;

		static auto getFrequencyStr = [](char* outStr, uint32 bufferSize, float freq)
		{
			std::format_to_n(outStr, bufferSize, "{:.0f}", freq);
		};

		static auto getFreqStrWidth = [](float freq)
		{
			char str[16]{};
			getFrequencyStr(str, 15, freq);
			return ImGui::CalcTextSize(str).x;
		};

		const float minSliderWidth = ImGui::CalcTextSize("10.0").x + GImGui->Style.FramePadding.x * 2.0f;
		const float requestedSliderWidth = size.x / values.size();

		const float sliderWidth = [&]()
		{
			float maxSize = std::max(minSliderWidth, requestedSliderWidth);
			for (float freq : frequencies)
			{
				maxSize = std::max(maxSize, getFreqStrWidth(freq));
			}
			return maxSize;
		}();

		auto freqSlider = [=](float* v)
		{
			ScopedID id(v);
			bool bModified = false;
			LayoutVertical("Slider", [&]
			{
				ScopedItemOutline outline("##dummy_id");

				const ImVec2 sliderSize(sliderWidth, size.y);
				bModified = ImGui::VSliderFloat(
					"##dummy_id", sliderSize, v,
					minValue, maxValue,
					"%.2f", ImGuiSliderFlags_AlwaysClamp
				);
			});

			return bModified;
		};

		LayoutVertical("GEQ Layout", [&]
		{
			LayoutHorizontal("Sliders", [&]
			{
				ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, GImGui->Style.ItemSpacing.y));

				for (uint32 b = 0; b < values.size(); ++b)
				{
					if (freqSlider(&values[b]))
						bModifiedBand = b + 1;
				}
			});

			LayoutHorizontal("Frequency Labels", [&]
			{
				ImVec2 cursor = ImGui::GetCursorScreenPos();

				if (frequencies.size() == values.size())
				{
					// Draw band center frequencies
					const float halfSliderWidth = sliderWidth * 0.5f;
					ImGui::SetCursorScreenPos((cursor += ImVec2(halfSliderWidth, 0.0f)));
					
					for (float freq : frequencies)
					{
						ShiftCursorX(-getFreqStrWidth(freq) * 0.5f);
						ImGui::Text("%.0f", freq);
						ImGui::SetCursorScreenPos((cursor += ImVec2(sliderWidth, 0.0f)));
					}
				}
				else
				{
					// Draw split frequencies
					for (float freq : frequencies)
					{
						ImGui::SetCursorScreenPos((cursor += ImVec2(sliderWidth, 0.0f)));
						ShiftCursorX(-getFreqStrWidth(freq) * 0.5f);
						ImGui::Text("%.0f", freq);
					}
				}
			});
		});

		return bModifiedBand;
	}

	uint32 DrawGEQ(const char* itemId, simd& values, const simd& frequencies, float minValue, float maxValue, const ImVec2& size)
	{
		float v[4]{}; values.store(v);
		float freq[4]{}; frequencies.store(freq);

		const uint32 modifiedBand = DrawGEQ(itemId, v, freq, minValue, maxValue, size);
		if (modifiedBand)
			values.load(v);

		return modifiedBand;
	}

	uint32 DrawGEQ(const char* itemId, Property<simd>& values, const simd& frequencies, float minValue, float maxValue, const ImVec2& size)
	{
		simd v = values.Get();

		const uint32 modifiedBand = DrawGEQ(itemId, v, frequencies, minValue, maxValue, size);
		if (modifiedBand)
			values.Set(v);

		return modifiedBand;
	}
} // namespace JPL::ImGuiEx
