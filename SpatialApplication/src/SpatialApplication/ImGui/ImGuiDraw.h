//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPLSpatialApplication **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright 2026 Jaroslav Pevno, JPLSpatialApplication is offered under the terms of the ISC license:
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

#include "ImGui/ImGui.h"

namespace JPL::ImGuiEx
{
	static void DrawArrow(ImDrawList& drawList, const ImVec2& lineStart, const ImVec2& lineEnd, ImU32 colour, float arrowSize = 12.0f);
	
	// Draw text cetnered within rectangle [cursor position, content region available]
	void DrawTextCentered(ImDrawList& drawList, const char* text, ImU32 colour = ImGui::GetColorU32(ImGuiCol_Text));
	
	// Draw text cetnered within rectangle [boundsMin, boundsMax]
	void DrawTextCentered(ImDrawList& drawList, const ImVec2& boundsMin, const ImVec2& boundsMax, const char* text, ImU32 colour = ImGui::GetColorU32(ImGuiCol_Text));

} // namespace JPL::ImGuiEx
