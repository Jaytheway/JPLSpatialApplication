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

#include "ConsoleLogGUI.h"

#include "ImGui/ImGui.h"
#include "fonts/FontIcons.h"
#include "GUI/PropertyWidgets.h"

#include "Utility/Command.h"

#include <array>

namespace JPL::GUI
{
	ConsoleLogGUI::ConsoleLogGUI()
	{
		mProps = std::make_shared<Properties>();

		// Enable all flags initially
		mProps->LogLevelFlags.set();
	}

	void ConsoleLogGUI::Draw()
	{
		using namespace JPL::ImGuiEx;

		Window("Output Log", [&]
		{
			GetLatestLog();

			DrawControlsPanel();

			ImGui::Spacing();

			// Set log text field background to a darker one
			ScopedColour bg(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_FrameBg));

			// Fill the entire width of the window with scrollable log text field
			ShiftCursorX(-ImGui::GetStyle().WindowPadding.x);

			const ChildConfig config
			{
				.Size = ImVec2(ImGui::GetWindowSize().x, 0.0f),
				.ChildFlags = ImGuiChildFlags_NavFlattened,
				.WindowFlags = ImGuiWindowFlags_HorizontalScrollbar
			};
			
			Child("LogScrollingRegion", config, [&]
			{
				DrawLog();
			});
		});
	}

	void ConsoleLogGUI::ClearLog()
	{
		mCurrentLog.clear();
		mBuffer.clear();
	}

	void ConsoleLogGUI::GetLatestLog()
	{
		Log::ExtractLastMessages(mBuffer);

		if (not mBuffer.empty())
		{
			// Move-append new history elements
			mCurrentLog.insert(mCurrentLog.end(),
							   std::make_move_iterator(mBuffer.begin()),
							   std::make_move_iterator(mBuffer.end()));

			mBuffer.clear();
		}
	}

	void ConsoleLogGUI::DrawControlsPanel()
	{
		using namespace JPL::ImGuiEx;

		LayoutHorizontal("Controls", ImVec2(ImGui::GetContentRegionAvail().x, 0), -1.0f, [&]
		{
			// Settings menu
			DrawSettingsPopup();

			// Filters menu
			DrawFiltersPopup();

#if 0		// Debug prints

			/*if (ImGuiEx::Button("Test Err"))
				Log::Error("Example Error!");

			if (ImGuiEx::Button("Test Warn"))
				Log::Warn("Example Warn! '{}'", "extra format");

			if (ImGuiEx::Button("Test Info"))
				Log::Info("Something useful, confirmation maybe");*/

			if (ImGuiEx::Button("1k Rnd"))
			{
				using Func = void(*)(std::string_view);
				std::array<Func, 4> logFunks = { &Log::Error, &Log::Warn, &Log::Info, &Log::Trace };

				auto logText = std::to_array({ "Short text",
											 "Maybe a bit longer text",
											 "Word",
											 "Multi-line, first line ends here.\nThis is second line",
											 "Super long text in next log message:",
											 "Lorem ipsum dolor sit amet, consectetur adipiscing elit. In accumsan neque at libero condimentum, eu lacinia arcu blandit. Donec vel purus nec quam rutrum auctor. Phasellus neque tortor, semper id enim in, fermentum sagittis dolor. Vestibulum ullamcorper scelerisque nisl. Mauris commodo quam nec ante pulvinar porttitor. Suspendisse arcu nulla, ornare nec mattis vel, venenatis non lectus. Vivamus tincidunt massa eget libero mollis elementum."
											 });

				for (uint32 i = 0; i < 1000; ++i)
					std::invoke(logFunks[i % logFunks.size()], logText[i % logText.size()]);
			}
#endif

			// Text filter widget
			DrawTextFilterWidget();

			const Colour colourN = Colour(GUI::Colours::Theme::Text).WithMultipliedValue(0.9f);
			const Colour colourH = colourN;
			const Colour colourP = colourH.WithMultipliedValue(0.8f);
			const ImGuiEx::IconButtonStyle iconButtonStyle
			{
				.ColourNormal = colourN,
				.ColourHovered = colourH,
				.ColourPressed = colourP,
				.BgColourHovered = IM_COL32(255, 255, 255, 10),
				.BgColourPressed = IM_COL32(255, 255, 255, 10)
			};

			// Filters button
			{
				if (ImGuiEx::IconButton(ICON_jplsa_SLIDERS" Filters...", iconButtonStyle))
					ImGui::OpenPopup("Filters");
			}

			ImGui::Spring();

			// Clear Log button
			{
				// Slightly modified style for "Clear" button that doesn't open a popup
				auto buttonStyle = iconButtonStyle;
				buttonStyle.BgColourNormal = IM_COL32(255, 255, 255, 10);
				buttonStyle.BgColourHovered = IM_COL32(255, 255, 255, 20);
				buttonStyle.BgColourPressed = buttonStyle.BgColourHovered;

				if (ImGuiEx::IconButton("Clear Log", buttonStyle))
					ClearLog();
			}

			// Settings button
			{
				if (ImGuiEx::IconButton(ICON_jplsa_GEAR, iconButtonStyle))
					ImGui::OpenPopup("Settings");
			}
		});
	}

	void ConsoleLogGUI::DrawSettingsPopup()
	{
		ImGuiEx::ScopedItemWidth width(140.0f);

		if (ImGui::BeginPopup("Settings"))
		{
			GUI::PropertyCheckbox("Enable Auto-Scroll", Undoable(mProps, &Properties::bAutoScroll));
			GUI::PropertyCheckbox("Enable Word Wrapping", Undoable(mProps, &Properties::bWordWrap));

			ImGui::EndPopup();
		}
	}

	void ConsoleLogGUI::DrawFiltersPopup()
	{
		ImGuiEx::ScopedItemWidth width(140.0f);

		if (ImGui::BeginPopup("Filters"))
		{
			ImGui::SeparatorText("Verbocity");

			DrawLevelToggle(ELogLevel::Critical);
			DrawLevelToggle(ELogLevel::Error);
			DrawLevelToggle(ELogLevel::Warn);
			DrawLevelToggle(ELogLevel::Info);
			DrawLevelToggle(ELogLevel::Trace);
			DrawLevelToggle(ELogLevel::Debug);

			ImGui::EndPopup();
		}
	}

	void ConsoleLogGUI::DrawTextFilterWidget()
	{
		// TODO: maybe move this to Widgets header as "Search" widget

		using namespace ImGuiEx;

		// Push input text closer to magnifying glass icon
		ScopedStyle spacing(ImGuiStyleVar_ItemSpacing,
							ImVec2(6.0f, ImGui::GetStyle().ItemSpacing.y));

		// Draw magnifying glass icon
		{
			ScopedColour colour(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));

			ShiftCursorY(3.0f); // center icon with the following text input widget
			ImGui::TextUnformatted((const char*)ICON_jplsa_MAGNIFYING_GLASS);
			ShiftCursorY(-3.0f);
		}

		// Draw actual filter text input
		const float filterWidth = std::min(ImGui::GetContentRegionAvail().x * 0.4f, 150.0f);
		ImGui::SetNextItemWidth(filterWidth);

		Conditional<ScopedFont> italicFont(not mTextFilter.IsActive(), GetItalicFont());
		ScopedStyle rounderCorners(ImGuiStyleVar_FrameRounding, 8.0f);
		ScopedStyle largerPadding(ImGuiStyleVar_FramePadding, ImVec2(10.0f, ImGui::GetStyle().FramePadding.y));

		const bool filterChagned =
			ImGui::InputTextWithHint("##Search Log",
									 "Search log...",
									 mTextFilter.InputBuf, IM_ARRAYSIZE(mTextFilter.InputBuf));
		if (filterChagned)
			mTextFilter.Build();
	}

	void ConsoleLogGUI::DrawLog()
	{
		using namespace JPL::ImGuiEx;

		auto& style = ImGui::GetStyle();

		{
			ScopedStyle spacing(ImGuiStyleVar_WindowPadding, style.FramePadding);

			if (ImGui::BeginPopupContextWindow())
			{
				static constexpr float itemWidth = 140.0f;
				
				// For now setting selectables individually
				// TODO: Popup wrapper
				if (ImGui::Selectable("Clear Log", false, 0, ImVec2(itemWidth, ImGui::GetTextLineHeight())))
					ClearLog();
				
				ImGui::EndPopup();
			}
		}

		// Remove Y spacing
		ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 1.0f));
		ScopedFont font(GUI::GetConsoleFont(), 14.0f);

		const ImGuiID logItemGoupID = ImGui::GetID("LogItemGroup");

		// TODO: 
		//		1. Select text for copy-paste
		//		2. Multi-line select text for copy-paste

		{
			ScopedGroup itemGroup(logItemGoupID);

			// We can't use clipper if we want word wrap around,
			// so we're just going to leave clipping up to ImGui
			Conditional<ScopedTextWrapPos> wrap(mProps->bWordWrap, ImGui::GetContentRegionAvail().x);

			for (uint32 i = 0; i < mCurrentLog.size(); ++i)
			{
				if (not PassesFilter(mCurrentLog[i]))
					continue;

				ScopedColour logLevelColour(ImGuiCol_Text, GetLogColour(mCurrentLog[i].Level));

				// Add a bit of padding that's missing from ImGui Child window
				ShiftCursorX(style.FramePadding.x);

				ImGui::TextUnformatted(mCurrentLog[i].Message.c_str());
			}
		}

		// Auto-scroll to the bottom on new logs
		if (bScrollToBottom || (mProps->bAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			ImGui::SetScrollHereY(1.0f);
		
		bScrollToBottom = false;
	}

	uint32 ConsoleLogGUI::GetLogColour(ELogLevel level)
	{
		switch (level)
		{
		default:
		case ELogLevel::Trace:		return Colours::Theme::TextDarker;
		case ELogLevel::Debug:		return Colours::Theme::TextDarker;
		case ELogLevel::Info:		return IM_COL32(50, 205, 50, 255);	// Lime Gren
		case ELogLevel::Warn:		return IM_COL32(218, 165, 32, 255); // Goldenrod
		case ELogLevel::Error:		return IM_COL32(255, 49, 49, 255);	// Neon Red
		case ELogLevel::Critical:	return IM_COL32(255, 0, 0, 255);	// Red
		}
	}

	void ConsoleLogGUI::DrawLevelToggle(ELogLevel level)
	{
		using namespace EnumFlags;

		ImGuiEx::ScopedColour textColour(ImGuiCol_Text, GetLogColour(level));

		bool bEnabled = mProps->LogLevelFlags.test(level);

		if (ImGuiEx::Checkbox(LogLevelToString(level).data(), bEnabled))
		{
			auto oldValue = mProps->LogLevelFlags;

			mProps->LogLevelFlags.set(level, bEnabled);

			JPLSpatialApplication::GetCommandHistory()
				.PropertyEdited(Undoable(mProps, &Properties::LogLevelFlags), OldValue(oldValue), "Log Verbocity");
		}
	}

	bool ConsoleLogGUI::PassesFilter(const LogMessage& message) const
	{
		return mProps->LogLevelFlags.test(message.Level)
			and mTextFilter.PassFilter(message.Message.c_str());
	}

} // namespace JPL::GUI
