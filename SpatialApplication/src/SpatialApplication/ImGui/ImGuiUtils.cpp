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

#include "ImGuiUtils.h"

#include "ImGui/Colour.h"
#include "ImGui/ImGuiScopedUtils.h"

namespace JPL::ImGuiEx
{
	void SetTooltip(std::string_view text, float delayInSeconds, bool allowWhenDisabled, ImVec2 padding)
	{
		if (IsItemHovered(delayInSeconds, allowWhenDisabled ? ImGuiHoveredFlags_AllowWhenDisabled : 0))
		{
			ScopedStyle tooltipPadding(ImGuiStyleVar_WindowPadding, padding);
			ScopedColour textCol(ImGuiCol_Text, GUI::Colours::Theme::TextBrighter);
			ImGui::SetTooltip(text.data());
		}
	}

	void DrawItemActivityOutline(OutlineFlags flags, ImColor colourHighlight, float rounding)
	{
		if (IsItemDisabled())
			return;

		auto* drawList = ImGui::GetWindowDrawList();
		const ImRect rect = RectExpanded(GetItemRect(), 1.0f, 1.0f);
		if ((flags & OutlineFlags_WhenActive) && ImGui::IsItemActive())
		{
			if (flags & OutlineFlags_HighlightActive)
			{
				drawList->AddRect(rect.Min, rect.Max, colourHighlight, rounding, 0, 1.5f);
			}
			else
			{
				const auto outlineColor = Colour::WhiteWithAlpha(42);
				drawList->AddRect(rect.Min, rect.Max, outlineColor, rounding, 0, 1.5f);
			}
		}
		else if ((flags & OutlineFlags_WhenHovered) && ImGui::IsItemHovered() && !ImGui::IsItemActive())
		{
			const auto outlineColor = Colour::WhiteWithAlpha(42);
			drawList->AddRect(rect.Min, rect.Max, outlineColor, rounding, 0, 1.5f);
		}
		else if ((flags & OutlineFlags_WhenInactive) && !ImGui::IsItemHovered() && !ImGui::IsItemActive())
		{
			const auto outlineColor = Colour::WhiteWithAlpha(32);
			drawList->AddRect(rect.Min, rect.Max, outlineColor, rounding, 0, 1.0f);
		}
	}

	ScopedItemOutline::ScopedItemOutline(const char* baseItem, int subitemIndex, OutlineFlags flags, ImColor colourHighlight)
		: ScopedItemOutline(ImGui::GetIDWithSeed(ImGui::GetID(baseItem), subitemIndex), flags, colourHighlight)
	{
	}

	ScopedItemOutline::ScopedItemOutline(const char* baseItem, std::initializer_list<int> subitemIndices, OutlineFlags flags, ImColor colourHighlight)
		: bColourPushed(false)
	{
		if (!baseItem || subitemIndices.size() == 0)
			return;

		const ImGuiID activeId = ImGui::GetActiveID();
		const ImGuiID hoveredId = ImGui::GetHoveredID();
		const ImGuiID baseId = ImGui::GetID(baseItem);

		auto contains = [baseId, &subitemIndices](ImGuiID id)
		{
			for (int subitemIndex : subitemIndices)
			{
				if (ImGui::GetIDWithSeed(baseId, subitemIndex) == id)
					return true;
			}
			return false;
		};

		const bool wasActive = contains(activeId);
		const bool wasHovered = contains(hoveredId);
		ParseColour(wasActive, wasHovered, flags, colourHighlight);
	}

	ScopedItemOutline::~ScopedItemOutline()
	{
		if (bColourPushed)
		{
			ImGui::PopStyleColor();
		}
	}

	void ScopedItemOutline::ParseColour(bool wasActive, bool wasHovered, int flags, ImColor colourHighlight)
	{
		ImU32 outlineColor = 0u;

		if ((flags & OutlineFlags_WhenActive) && wasActive)
		{
			if (flags & OutlineFlags_HighlightActive)
			{
				outlineColor = colourHighlight;
			}
			else
			{
				outlineColor = Colour::WhiteWithAlpha(42);
			}
		}
		else if ((flags & OutlineFlags_WhenHovered) && wasHovered && not wasActive)
		{
			outlineColor = Colour::WhiteWithAlpha(42);
		}
		else if ((flags & OutlineFlags_WhenInactive) && not wasHovered && not wasActive)
		{

			outlineColor = Colour::WhiteWithAlpha(32);
		}

		if (outlineColor != ImU32(0u))
		{
			ImGui::PushStyleColor(ImGuiCol_Border, outlineColor);
			bColourPushed = true;
		}
	}

} // namespace JPL::ImGuiEx
