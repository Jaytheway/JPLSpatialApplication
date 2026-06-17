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

	//======================================================================
	/// Shadow Edge

	enum class EShadowEdge
	{
		Left, Right, Top, Bottom,
	};

	struct ShadowEdgeStyle
	{
		float ShadowSize = 24.0f;
		float EdgeFeather = 6.0f;
		float FalloffPower = 2.0f; // 1.0f - soft; 2.0f - tighter near edge; 0.5f - longer, mistier shadow
		ImU32 Colour = IM_COL32(0, 0, 0, 80);
		int AlongSegments = 8;
		int AcrossSegments = 8;
	};

	void DrawShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, EShadowEdge edge, const ShadowEdgeStyle& style = {});
	
	void DrawLeftShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, const ShadowEdgeStyle& style = {});
	void DrawRightShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, const ShadowEdgeStyle& style = {});
	void DrawTopShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, const ShadowEdgeStyle& style = {});
	void DrawBottomShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, const ShadowEdgeStyle& style = {});

} // namespace JPL::ImGuiEx
