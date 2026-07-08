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

namespace JPL::ImGuiEx
{
	namespace Constant
	{
		static constexpr float cPressedLabelOffset = 1.0f;
	}

	bool Checkbox(const char* label, bool& value)
	{
        ScopedItemOutline outline(label, OutlineFlags_NoOutlineInactive, ImColor(60, 60, 60));
		return ImGui::Checkbox(label, &value);
	}

    bool Checkbox(const char* label, Property<bool>& property)
    {
        bool bValue = property.Get();
        if (Checkbox(label, bValue))
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
            ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 10),
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

	//==========================================================================
	/// Window Buttons

	struct ButtonState
	{
		bool bClipped, bHovered, bHeld, bPressed;
	};

	namespace WindowButton
	{
		// Padding around icon shape within button rectangle
		static constexpr float cIconPadding = 13.0f;

		enum class EWindowType
		{
			Main,
			Regular
		};

		enum class EButtonType
		{
			Collapse,
			Minimize,
			Maximize,
			Close
		};

		static ButtonState Behavior(ImGuiID id, const ImRect& bb)
		{
			// Note (JP): this is mostly copy of the ImGui::CloseButton without rendering part

			const ImGuiWindow* window = GImGui->CurrentWindow;

			// Tweak 1: Shrink hit-testing area if button covers an abnormally large proportion of the visible region. That's in order to facilitate moving the window away. (#3825)
			// This may better be applied as a general hit-rect reduction mechanism for all widgets to ensure the area to move window is always accessible?
			ImRect bb_interact = bb;
			const float area_to_visible_ratio = window->OuterRectClipped.GetArea() / bb.GetArea();
			if (area_to_visible_ratio < 1.5f)
				bb_interact.Expand(ImTrunc(bb_interact.GetSize() * -0.25f));

			// Tweak 2: We intentionally allow interaction when clipped so that a mechanical Alt,Right,Activate sequence can always close a window.
			// (this isn't the common behavior of buttons, but it doesn't affect the user because navigation tends to keep items visible in scrolling layer).
			const bool is_clipped = !ImGui::ItemAdd(bb_interact, id);

			bool hovered, held;
			const bool pressed = ImGui::ButtonBehavior(bb_interact, id, &hovered, &held);

			return ButtonState{ .bClipped = is_clipped, .bHovered = hovered, .bHeld = held, .bPressed = pressed };
		}

		template<EWindowType WindowType, EButtonType ButtonType>
		ImU32 GetFillColour(const ButtonState& state)
		{
			if constexpr (ButtonType != EButtonType::Close)
			{
				return ImGui::IsItemActive()
					? IM_COL32(255, 255, 255, 10)
					: state.bHovered
					? IM_COL32(255, 255, 255, 20)
					: IM_COL32_BLACK_TRANS;
			}
			else
			{
				if constexpr (WindowType == EWindowType::Main)
				{
					// Close button is the only one that may use different fill colour (red)
					return ImGui::IsItemActive()
						? IM_COL32(255, 50, 50, 150)
						: state.bHovered
						? IM_COL32(235, 50, 50, 255)
						: IM_COL32_BLACK_TRANS;
				}
				else
				{
					return ImGui::IsItemActive()
						? IM_COL32(255, 255, 255, 10)
						: state.bHovered
						? IM_COL32(255, 255, 255, 20)
						: IM_COL32_BLACK_TRANS;
				}
			}
		}

		template<EWindowType WindowType, EButtonType ButtonType>
		ImU32 GetIconColour(const ButtonState& state)
		{
			if constexpr (ButtonType != EButtonType::Close)
			{
				return ImGui::IsItemActive()
					? JPL::GUI::Colours::Theme::TextDarker
					: (ImU32)JPL::Colour(JPL::GUI::Colours::Theme::Text).WithMultipliedValue(0.9f);
			}
			else
			{
				// Draw icon dark, to make sure it's visible on the red background fill

				if constexpr (WindowType == EWindowType::Main)
				{
					return (ImGui::IsItemActive() || state.bHovered)
						? JPL::GUI::Colours::Theme::Titlebar
						: (ImU32)JPL::Colour(JPL::GUI::Colours::Theme::Text).WithMultipliedValue(0.8f);
				}
				else // Window == EWindowType::Regular
				{
					return ImGui::IsItemActive()
						? JPL::GUI::Colours::Theme::TextDarker
						: (ImU32)JPL::Colour(JPL::GUI::Colours::Theme::Text).WithMultipliedValue(0.8f);
				}
			}
		}

		template<EWindowType WindowType>
		bool Minimize(const char* id, const ImRect& rect)
		{
			const ButtonState state = Behavior(ImGui::GetID(id), rect);

			if (state.bClipped)
				return state.bPressed;

			const ImU32 fillColour = GetFillColour<WindowType, EButtonType::Minimize>(state);
			const ImU32 iconColour = GetIconColour<WindowType, EButtonType::Minimize>(state);

			auto* drawList = ImGui::GetWindowDrawList();

			drawList->AddRectFilled(rect.Min, rect.Max, fillColour, 0.0f);

			const ImVec2 lineP1 = rect.Min + ImVec2(cIconPadding, rect.GetSize().y * 0.5f);
			const ImVec2 lineP2 = lineP1 + ImVec2(rect.GetSize().x - cIconPadding * 2.0f, 0.0f);
			drawList->AddLine(lineP1, lineP2, iconColour, 1.0f);

			return state.bPressed;
		}

		template<EWindowType WindowType>
		bool Maximize(const char* id, const ImRect& rect, bool bIsMaximized)
		{
			const ButtonState state = Behavior(ImGui::GetID(id), rect);

			if (state.bClipped)
				return state.bPressed;

			const ImU32 fillColour = GetFillColour<WindowType, EButtonType::Maximize>(state);
			const ImU32 iconColour = GetIconColour<WindowType, EButtonType::Maximize>(state);

			auto* drawList = ImGui::GetWindowDrawList();

			drawList->AddRectFilled(rect.Min, rect.Max, fillColour, 0.0f);

			const ImVec2 center = rect.GetCenter();
			const float radius = (rect.GetWidth() * 0.5f - cIconPadding) * 0.8f;
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

			return state.bPressed;
		}

		template<WindowButton::EWindowType WindowType>
		static bool Close(const char* id, const ImRect& rect)
		{
			const ButtonState state = Behavior(ImGui::GetID(id), rect);

			if (state.bClipped)
				return state.bPressed;

			const ImU32 fillColour = GetFillColour<WindowType, EButtonType::Close>(state);
			const ImU32 iconColour = GetIconColour<WindowType, EButtonType::Close>(state);

			auto* drawList = ImGui::GetWindowDrawList();

			drawList->AddRectFilled(rect.Min, rect.Max, fillColour, 0.0f);

			const ImVec2 center = rect.GetCenter();
			const float radius = (rect.GetWidth() * 0.5f - cIconPadding);
			const ImRect square{ center - ImVec2(radius, radius), center + ImVec2(radius, radius) };

			drawList->AddLine(square.GetTL(), square.GetBR() - ImVec2(0.5f, 0.5f), iconColour, 2.0f);
			drawList->AddLine(square.GetTR() - ImVec2(0.5f, 0.5f), square.GetBL() - ImVec2(0.0f, 1.0f), iconColour, 2.0f);

			return state.bPressed;
		}
	} // WindowButton

	void DrawMainWindowButtons(ImVec2 titlebarMin, ImVec2 titlebarMax)
	{
		JPL::ImGuiEx::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		const bool bIsMaximized = Walnut::Application::Get().IsMaximized();

		const float buttonWidth = 36.0f;
		const float buttonHeight = 26.0f;
		const float buttonsStartX = titlebarMax.x - buttonWidth * 3.0f;
		const float titlebarVerticalOffset = bIsMaximized ? 6.0f : 0.0f;
		const ImVec2 buttonSize(buttonWidth, buttonHeight);

		ImGui::SetCursorScreenPos(ImVec2(buttonsStartX, titlebarMin.y + titlebarVerticalOffset));

		// Minimize Button
		{
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImRect bb(pos, pos + buttonSize);

			if (WindowButton::Minimize<WindowButton::EWindowType::Main>("Minimize", bb))
				Walnut::Application::Get().Minimize();

			ImGui::ItemSize(buttonSize);
		}

		// Maximize Button
		{
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImRect bb(pos, pos + buttonSize);

			if (WindowButton::Maximize<WindowButton::EWindowType::Main>("Maximize", bb, bIsMaximized))
				Walnut::Application::Get().ToggleMaximize();
			
			ImGui::ItemSize(buttonSize);
		}

		// Close Button
		{
			const ImVec2 pos = ImGui::GetCursorScreenPos();
			const ImRect bb(pos, pos + buttonSize);

			if (WindowButton::Close<WindowButton::EWindowType::Main>("Close", bb))
				Walnut::Application::Get().Close();

			ImGui::ItemSize(buttonSize);
		}
	}

	bool CloseButton(const char* id, const ImVec2& size)
	{
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const ImRect bb(pos, pos + size);
		const bool bPressed = WindowButton::Close<WindowButton::EWindowType::Regular>(id, bb);
		ImGui::ItemSize(size);
		return bPressed;
	}

	//==========================================================================
	/// Icon Button

	namespace IconStyle
	{
		const IconButtonStyle cIconLabelBgHovered = []()
		{
			const Colour colourN = Colour(GUI::Colours::Theme::Text).WithMultipliedValue(0.9f);
			const Colour colourH = colourN;
			const Colour colourP = colourH.WithMultipliedValue(0.8f);
			return IconButtonStyle{
				.ColourNormal = colourN,
				.ColourHovered = colourH,
				.ColourPressed = colourP,
				.BgColourHovered = IM_COL32(255, 255, 255, 10),
				.BgColourPressed = IM_COL32(255, 255, 255, 10)
			};
		}();

		const IconButtonStyle cIconLabelBgAlways = []()
		{
			IconButtonStyle style = cIconLabelBgHovered;

			style.BgColourNormal = IM_COL32(255, 255, 255, 10);
			style.BgColourHovered = IM_COL32(255, 255, 255, 20);
			style.BgColourPressed = style.BgColourHovered;
			return style;
		}();

	} // namespace IconStyle

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

		const bool bIsActive = ImGui::IsItemActive();
		const bool bIsHovered = ImGui::IsItemHovered();

		const ImU32 backgroundColour =
			bIsActive ? style.BgColourPressed :
			bIsHovered ? style.BgColourHovered :
			style.BgColourNormal;

		const ImU32 iconColour =
			bIsActive ? style.ColourPressed :
			bIsHovered ? style.ColourHovered :
			style.ColourNormal;

		ImVec2 bbMin = ImGui::GetItemRectMin();
		ImVec2 bbMax = ImGui::GetItemRectMax();

		// Draw background and border
		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(bbMin, bbMax, backgroundColour, ImGui::GetStyle().FrameRounding);
		if (style.bDrawBorder)
		{
			drawList->AddRect(bbMin, bbMax, style.BorderColour, ImGui::GetStyle().FrameRounding);
		}

		if (bIsActive)
		{
			bbMin.y += Constant::cPressedLabelOffset;
			bbMax.y += Constant::cPressedLabelOffset;
		}

		// Draw icon text label
		DrawTextCentered(*drawList, bbMin, bbMax, label, iconColour);

		return bPressed;
	}

	bool Button(const char* label, const ImVec2& size_arg)
	{
		// Pretty much copy of ImGui::ButtonEx,
		// - removed ImGuiButton flags handling, since default ImGui::Button just passes _None
		// - added label offset when button is pressed

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		ImVec2 pos = window->DC.CursorPos;
		ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, pos + size);
		ImGui::ItemSize(size, style.FramePadding.y);
		if (!ImGui::ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

		// Render
		const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		ImGui::RenderNavCursor(bb, id);
		ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

		if (g.LogEnabled)
			ImGui::LogSetNextTextDecoration("[", "]");

		//! This is where we modify bb to render text "pressed"
		// (note using button pressed result doesn't work, we have to check held and hovered)
		const ImVec2 labelOffset(0.0f, (held && hovered) ? Constant::cPressedLabelOffset : 0.0f);

		ImGui::RenderTextClipped(bb.Min + style.FramePadding + labelOffset,
								 bb.Max - style.FramePadding + labelOffset,
								 label, NULL, &label_size,
								 style.ButtonTextAlign,
								 &bb);

		return pressed;
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

	uint32 DrawGEQ(const char* itemIdStr, std::span<float> values, std::span<const float> frequencies, float minValue, float maxValue, const ImVec2& size)
	{
		if (not JPL_ENSURE(frequencies.size() == values.size() - 1 or frequencies.size() == values.size()))
		{
			return 0;
		}

		using namespace ImGuiEx;

		ScopedID geqID(ImGui::GetID(itemIdStr));

		// Note: Layouts push their own ImGui::ItemAdd, overriding the last item ID with 0,
		// so we need to wrap all our internal interactions in a group.
		ScopedGroup group;

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
			
			// Make sure to extend layout to prevent out labels from being clipped
			const ImVec2 lableRowSize(sliderWidth * values.size(), ImGui::GetTextLineHeightWithSpacing());

			LayoutHorizontal("Frequency Labels", ImVec2(0.0f, lableRowSize.y), -1.0f, [&]
			{
				auto* drawList = ImGui::GetWindowDrawList();
				
				ImVec2 cursor = ImGui::GetCursorScreenPos();
				char buffer[32]{};

				if (frequencies.size() == values.size())
				{
					// Draw band center frequencies
					const float halfSliderWidth = sliderWidth * 0.5f;
					
					cursor.x += halfSliderWidth;
					
					for (float freq : frequencies)
					{
						getFrequencyStr(buffer, 64, freq);

						const float stringWidth = ImGui::CalcTextSize(buffer).x;
						const ImVec2 textPos(cursor.x - stringWidth * 0.5f, cursor.y);

						drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), buffer);
						
						cursor.x += sliderWidth;
					}
				}
				else
				{
					// Draw split frequencies
					for (float freq : frequencies)
					{
						cursor.x += sliderWidth;

						getFrequencyStr(buffer, 64, freq);

						const float stringWidth = ImGui::CalcTextSize(buffer).x;
						const ImVec2 textPos(cursor.x - stringWidth * 0.5f, cursor.y);

						drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), buffer);
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

	bool InputVec3(const char* label, MinimalVec3& value, const ImGuiEx::InputVec3Config& config)
	{
		// ImGui multi-input widgets push id of the field index,
		// so we need to detect component id for our outline logic
		ImGuiEx::ScopedItemOutline outline(label, { 0, 1, 2 });

		int flags = config.Flags;

		// ImGui doesn't support this flag for input scalar natively
		bool bTrueOnEnter = false;
		if ((flags & ImGuiInputTextFlags_EnterReturnsTrue))
		{
			flags ^= ImGuiInputTextFlags_EnterReturnsTrue;
			bTrueOnEnter = true;
		}
		
		MinimalVec3 originalValue = value;

		const bool bModified = ImGui::InputFloat3(label, &value.X, config.Base.Fmt, flags);

		if (bModified)
		{
			if (bTrueOnEnter and not ImGui::IsItemDeactivatedAfterEdit())
			{
				value = originalValue;
				return false;
			}

			if (config.Base.Min)
			{
				value.X = std::max(config.Base.Min->X, value.X);
				value.Y = std::max(config.Base.Min->Y, value.Y);
				value.Z = std::max(config.Base.Min->Z, value.Z);
			}

			if (config.Base.Max)
			{
				value.X = std::min(config.Base.Max->X, value.X);
				value.Y = std::min(config.Base.Max->Y, value.Y);
				value.Z = std::min(config.Base.Max->Z, value.Z);
			}
		}

		return bModified;
	}

	bool InputVec3(const char* label, Property<MinimalVec3>& value, const ImGuiEx::InputVec3Config& config)
	{
		MinimalVec3 v = value.Get();
		if (InputVec3(label, v, config))
		{
			value.Set(v);
			return true;
		}
		return false;
	}

	bool DragVec3(const char* label, MinimalVec3& value, const ImGuiEx::DragVec3Config& config)
	{
		// ImGui multi-input widgets push id of the field index,
		// so we need to detect component id for our outline logic
		ImGuiEx::ScopedItemOutline outline(label, { 0, 1, 2 });

		const bool bModified = ImGui::DragFloat3(label, &value.X, config.VSpead, 0.0f, 0.0f, config.Base.Fmt, config.Flags);

		if (bModified)
		{
			if (config.Base.Min)
			{
				value.X = std::max(config.Base.Min->X, value.X);
				value.Y = std::max(config.Base.Min->Y, value.Y);
				value.Z = std::max(config.Base.Min->Z, value.Z);
			}

			if (config.Base.Max)
			{
				value.X = std::min(config.Base.Max->X, value.X);
				value.Y = std::min(config.Base.Max->Y, value.Y);
				value.Z = std::min(config.Base.Max->Z, value.Z);
			}
		}

		return bModified;
	}

	bool DragVec3(const char* label, Property<MinimalVec3>& value, const ImGuiEx::DragVec3Config& config)
	{
		MinimalVec3 v = value.Get();
		if (DragVec3(label, v, config))
		{
			value.Set(v);
			return true;
		}
		return false;
	}
} // namespace JPL::ImGuiEx
