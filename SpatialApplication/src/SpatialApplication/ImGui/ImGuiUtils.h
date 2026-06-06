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

// To avoid some circular dependencies, we shall inline
// only necessary headers in ImGuiUtils.h, instead of our ImGui.h
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <imgui_internal.h>

#include "ImGui/ImGuiStyling.h"

#include <concepts>
#include <string_view>
#include <functional>
#include <initializer_list>

namespace JPL::ImGuiEx
{
	// The delay won't work on texts, because the timer isn't tracked for them.
	inline bool IsItemHovered(float delayInSeconds = 0.1f, ImGuiHoveredFlags flags = 0)
	{
		return ImGui::IsItemHovered() && GImGui->HoveredIdTimer > delayInSeconds; /*HoveredIdNotActiveTimer*/
	}

	void SetTooltip(std::string_view text, float delayInSeconds = 0.1f, bool allowWhenDisabled = true, ImVec2 padding = ImVec2(5, 5));

	// Check if navigated to current item, e.g. with arrow keys
	inline bool NavigatedTo() { return GImGui->NavJustMovedToId == GImGui->LastItemData.ID; }

	inline void BeginDisabled(bool disabled = true) { if (disabled) ImGui::BeginDisabled(true); }
	inline bool IsItemDisabled() { return ImGui::GetItemFlags() & ImGuiItemFlags_Disabled; }
	inline void EndDisabled() { if (GImGui->DisabledStackSize > 0) ImGui::EndDisabled(); }

	/// Calculate combined height of number of items with padding and spacign between the items
	inline float GetItemsHeight(int numItems) { return ImGui::GetFrameHeightWithSpacing() * ImMax(0, numItems - 1) + ImGui::GetFrameHeight(); }

	//=========================================================================================
	/// Rectangle

	inline ImRect GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	inline ImRect RectExpanded(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x -= x;
		result.Min.y -= y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	inline ImRect RectOffset(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x += x;
		result.Min.y += y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}

	inline ImRect RectOffset(const ImRect& rect, ImVec2 xy)
	{
		return RectOffset(rect, xy.x, xy.y);
	}

	//=========================================================================================
	// Outline
	typedef int OutlineFlags;
	enum OutlineFlags_
	{
		OutlineFlags_None = 0,						// draw no activity outline
		OutlineFlags_WhenHovered = 1 << 1,			// draw an outline when item is hovered
		OutlineFlags_WhenActive = 1 << 2,			// draw an outline when item is active
		OutlineFlags_WhenInactive = 1 << 3,			// draw an outline when item is inactive
		OutlineFlags_HighlightActive = 1 << 4,		// when active, the outline is in highlight colour
		OutlineFlags_NoHighlightActive = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_WhenInactive,
		OutlineFlags_NoOutlineInactive = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_HighlightActive,
		OutlineFlags_All = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_WhenInactive | OutlineFlags_HighlightActive,
	};

	// Note: these work well for custom property draw functions where it can be called only after the input field item
	// otherwise the ouline is drawn around the text label, if used for default ImGui items
	void DrawItemActivityOutline(OutlineFlags flags = OutlineFlags_NoOutlineInactive, ImColor colourHighlight = GUI::Colours::Theme::Selected, float rounding = GImGui->Style.FrameRounding);

	//=========================================================================================
	/// Utility to draw item outline based on activity flags Active/Hovered
	/// It is to be used for the following item, id of which matches provided
	/// to the ScopedItemOutline constructor.
	/// E.e.:
	///		ScopedItemOutline outline("My Button");
	///		ImGui::Button("My Button");
	class ScopedItemOutline
	{
		bool bColourPushed;
	public:
		// This constructor handles cases ImGui items with multiple input components
		// where the actual active ID would be the index of the component hashed with
		// the parent item's ID.
		// E.g. for ImGui::InputFloat3("MyFloat3") it woul be "MyFlaot3" + index of the componetn [0, 1, 2]
		ScopedItemOutline(const char* baseItem, int subitemIndex, OutlineFlags flags = OutlineFlags_NoOutlineInactive, ImColor colourHighlight = GUI::Colours::Theme::Selected);

		// Same as the constructor for single subitemIndex, but takes a list of subutem indices and checks all of them
		ScopedItemOutline(const char* baseItem, std::initializer_list<int> subitemIndices, OutlineFlags flags = OutlineFlags_NoOutlineInactive, ImColor colourHighlight = GUI::Colours::Theme::Selected);

		template<class IDType>
		ScopedItemOutline(IDType itemId, OutlineFlags flags = OutlineFlags_NoOutlineInactive, ImColor colourHighlight = GUI::Colours::Theme::Selected)
			: bColourPushed(false)
		{
			ImGuiID id;
			if constexpr (std::same_as<IDType, ImGuiID>)
			{
				id = itemId;
			}
			else
			{
				id = ImGui::GetID(itemId);
			}
			if (!id)
			{
				return;
			}

			const bool wasActive = ImGui::GetActiveID() == id;
			const bool wasHovered = ImGui::GetHoveredID() == id;
			ParseColour(wasActive, wasHovered, flags, colourHighlight);
		}

		~ScopedItemOutline();

	private:
		void ParseColour(bool wasActive, bool wasHovered, int flags, ImColor colourHighlight);
	};

	//=========================================================================================
	inline void ShiftCursorX(float distance) { ImGui::SetCursorPosX(ImGui::GetCursorPosX() + distance); }
	inline void ShiftCursorY(float distance) { ImGui::SetCursorPosY(ImGui::GetCursorPosY() + distance); }
	inline void ShiftCursor(ImVec2 amount) { ImGui::SetCursorPos(ImGui::GetCursorPos() + amount); }
	inline void ShiftCursor(float x, float y) { ShiftCursor(ImVec2(x, y)); }

} // namespace JPL::ImGuiEx
