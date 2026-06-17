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

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>

namespace JPL::GUI
{
	namespace Colours
	{
		namespace Theme
		{
			inline constexpr ImU32 WindowBackround = IM_COL32(21, 21, 21, 255);
			inline constexpr ImU32 ChildBackround = IM_COL32(36, 36, 36, 30);
			inline constexpr ImU32 ChildBackroundDark = IM_COL32(36, 36, 36, 255);
			inline constexpr ImU32 BackgroundPopup = IM_COL32(35, 35, 35, 255);
			
			inline constexpr ImU32 BackgroundDark = IM_COL32(26, 26, 26, 255);

			inline constexpr ImU32 Titlebar = IM_COL32(21, 25, 27, 255);

			inline constexpr ImU32 Text = IM_COL32(220, 220, 220, 255);
			inline constexpr ImU32 TextBrighter = IM_COL32(255, 255, 255, 255);
			inline constexpr ImU32 TextDarker = IM_COL32(128, 128, 128, 255);
			inline constexpr ImU32 TextSlightlyDarker = IM_COL32(174, 174, 174, 255);

			inline constexpr ImU32 Highlight = IM_COL32(91, 195, 239, 255);
			inline constexpr ImU32 Selected = IM_COL32(235, 186, 97, 255);

			inline constexpr ImU32 GroupHeader = IM_COL32(47, 47, 47, 255);
			inline constexpr ImU32 PropertyField = IM_COL32(15, 15, 15, 255);

			inline constexpr ImU32 Border = IM_COL32(255, 255, 255, 20);
		}
	}

	void SetImGuiStyle();

	ImFont* GetRegularFont();
	ImFont* GetBoldFont();
	ImFont* GetItalicFont();
	ImFont* GetLightFont();
	ImFont* GetConsoleFont();
}
