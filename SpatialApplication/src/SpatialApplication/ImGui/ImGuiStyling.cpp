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

#include "ImGuiStyling.h"

#include "ImGui/Colour.h"

#include <Walnut/ImGui/ImGuiTheme.h>
#include <Walnut/Application.h>

#include <unordered_map>
#include <string_view>

#include "fonts/FontIcons.h"

namespace JPL::GUI
{
	namespace Embedded
	{
		#include "fonts/FontIcons.embed"
		#include "fonts/IBMPLexSans-Light.embed"
		#include "fonts/IBMPLexSans-Medium.embed"
		#include "fonts/IBMPLexSans-MediumItalic.embed"
		#include "fonts/IBMPLexSans-Bold.embed"
		#include "fonts/CascadiaCode-SemiBold.embed"
	} // namespace Embedded

	std::unordered_map<std::string_view, ImFont*> gFonts;

	static void SetupFonts()
	{
		// Load Fonts
		ImGuiIO& io = ImGui::GetIO();
		auto* fontAtlas = io.Fonts;
		JPL_ASSERT(fontAtlas);

		// Load our fonts
		static constexpr float fontSizePx = 17.0f;

		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;

		ImFont* baseFont = fontAtlas->AddFontFromMemoryTTF((void*)Embedded::IBMPlexSansMedium_data, Embedded::IBMPlexSansMedium_size, fontSizePx, &fontConfig);
		GUI::gFonts["Default"] = baseFont;
		GUI::gFonts["Bold"] = fontAtlas->AddFontFromMemoryTTF((void*)Embedded::IBMPlexSansBold_data, Embedded::IBMPlexSansBold_size, fontSizePx, &fontConfig);
		GUI::gFonts["Italic"] = fontAtlas->AddFontFromMemoryTTF((void*)Embedded::IBMPlexSansMediumItalic_data, Embedded::IBMPlexSansMediumItalic_size, fontSizePx, &fontConfig);
		GUI::gFonts["Light"] = fontAtlas->AddFontFromMemoryTTF((void*)Embedded::IBMPlexSansLight_data, Embedded::IBMPlexSansLight_size, fontSizePx, &fontConfig);
		GUI::gFonts["Console"] = fontAtlas->AddFontFromMemoryTTF((void*)Embedded::CascadiaCodeSemiBold_data, Embedded::CascadiaCodeSemiBold_size, fontSizePx, &fontConfig);

		io.FontDefault = baseFont;

		// Merge icons to all existing fonts
		static constexpr float cIconsFontSize = 16.0f;

		for (ImFont* destinationFont : { GUI::GetRegularFont(), GUI::GetBoldFont(), GUI::GetItalicFont() })
		{
			if (JPL_ENSURE(destinationFont != nullptr))
			{
				static constexpr ImWchar iconsRanges[] = { ICON_MIN_jplsa, ICON_MAX_jplsa, 0 };
				ImFontConfig iconsConfig;
				iconsConfig.MergeMode = true;
				iconsConfig.PixelSnapH = true;
				iconsConfig.DstFont = destinationFont;
				fontAtlas->AddFontFromMemoryCompressedBase85TTF(Embedded::FONT_ICON_BUFFER_NAME_jplsa, cIconsFontSize, &iconsConfig, iconsRanges);
			}
		}
	}

	static void SetupColours()
	{
		auto& colors = ImGui::GetStyle().Colors;

		// Headers (also Selectable is using these colours
		colors[ImGuiCol_Header] = ImColor(Colours::Theme::GroupHeader);
		colors[ImGuiCol_HeaderHovered] = ImColor(Colours::Theme::GroupHeader);
		colors[ImGuiCol_HeaderActive] = ImColor(Colours::Theme::GroupHeader);

		// Buttons
		colors[ImGuiCol_Button] = ImColor(56, 56, 56, 200);
		colors[ImGuiCol_ButtonHovered] = ImColor(70, 70, 70, 255);
		colors[ImGuiCol_ButtonActive] = ImColor(56, 56, 56, 150);

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImColor(Colours::Theme::PropertyField);
		colors[ImGuiCol_FrameBgHovered] = ImColor(Colours::Theme::PropertyField);
		colors[ImGuiCol_FrameBgActive] = ImColor(Colours::Theme::PropertyField);


		// Tabs
		colors[ImGuiCol_Tab] = ImColor(0, 0, 0, 0);
		colors[ImGuiCol_TabSelected] = ImColor(255, 255, 255, 15);
		colors[ImGuiCol_TabHovered] = colors[ImGuiCol_TabSelected];
		colors[ImGuiCol_TabDimmed] = ImColor(0, 0, 0, 0);
		colors[ImGuiCol_TabDimmedSelected] = colors[ImGuiCol_TabHovered];
		colors[ImGuiCol_TabSelectedOverline] = ImColor(Colours::Theme::Selected);
		colors[ImGuiCol_TabDimmedSelectedOverline] = ImColor(ImGui::GetColorU32(ImGuiCol_TabSelectedOverline, 0.5f));
		
		// Title
		colors[ImGuiCol_TitleBg] = ImColor(Colours::Theme::Titlebar);
		colors[ImGuiCol_TitleBgActive] = JPL::Colour(Colours::Theme::Titlebar).WithMultipliedValue(1.2f).WithMultipliedSaturation(1.4f);
		colors[ImGuiCol_TitleBgCollapsed] = colors[ImGuiCol_TitleBg];


		// Scrollbar
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.0f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

		// Checkbox
		colors[ImGuiCol_CheckMark] = ImColor(Colours::Theme::Text);

		// Slider
		colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

		// Text
		colors[ImGuiCol_Text] = ImColor(Colours::Theme::Text);

		// Separator
		colors[ImGuiCol_Separator] = ImColor(255, 255, 255, 20);
		colors[ImGuiCol_SeparatorActive] = ImColor(Colours::Theme::Highlight);
		colors[ImGuiCol_SeparatorHovered] = ImColor(ImGui::GetColorU32(ImGuiCol_SeparatorActive, 0.58f));

		// Docking
		Walnut::UI::Colors::dockspaceSplitterColor = ImColor(15, 15, 15, 255);
		colors[ImGuiCol_DockingPreview] = ImColor(ImGui::GetColorU32(ImGuiCol_SeparatorActive, 0.7f));

		// Window Background
		colors[ImGuiCol_WindowBg] = ImColor(Colours::Theme::WindowBackround);
		colors[ImGuiCol_ChildBg] = ImColor(Colours::Theme::ChildBackround);
		colors[ImGuiCol_PopupBg] = ImColor(Colours::Theme::BackgroundPopup);

		colors[ImGuiCol_Border] = ImColor(Colours::Theme::Border);
		colors[ImGuiCol_BorderShadow] = ImColor(0, 0, 0, 00);

		// Tables
		colors[ImGuiCol_TableHeaderBg] = ImColor(Colours::Theme::GroupHeader);
		colors[ImGuiCol_TableBorderLight] = ImColor(Colours::Theme::BackgroundDark);

		// Menubar
		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };
	}

	void SetImGuiStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(14.0f, 12.0f);
		style.FramePadding = ImVec2(6.0f, 4.0f);
		style.ItemSpacing = ImVec2(12.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

		style.ScrollbarSize = 12.0f;
		style.GrabMinSize = 9.0f;
		style.SeparatorTextBorderSize = 2.0f; // thickness of the separator

		style.WindowRounding = 4.0f;
		style.TabRounding = 1.0f;
		style.FrameRounding = 2.0f;
		style.PopupRounding = style.FrameRounding;
		style.GrabRounding = 1.0f;
		style.ScrollbarRounding = style.GrabRounding;

		SetupFonts();
		SetupColours();
	}

	ImFont* GetRegularFont()
	{
		return gFonts["Default"];
	}

	ImFont* GetBoldFont()
	{
		return gFonts["Bold"];
	}

	ImFont* GetItalicFont()
	{
		return gFonts["Italic"];
	}

	ImFont* GetLightFont()
	{
		return gFonts["Light"];
	}

	ImFont* GetConsoleFont()
	{
		return gFonts["Console"];
	}
} // namespace JPL::GUI
