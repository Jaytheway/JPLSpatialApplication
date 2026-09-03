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

#include "ImGuiScopedUtils.h"

#include "ImGui/ImGui.h"

namespace JPL::ImGuiEx
{
	ScopedDisable::ScopedDisable(bool disabled)
	{
		ImGuiEx::BeginDisabled(disabled);
	}

	ScopedDisable::~ScopedDisable()
	{
		ImGuiEx::EndDisabled();
	}

	ScopedGroup::~ScopedGroup()
	{
		ImGui::EndGroup();

		// Assing last item ID, if use requested the group to have a specifice ID to be able to do hover/click checks more reliably.
		// 
		// ImGui::EndGroup assigns LastItemData.ID from the ActiveID (active widget inside of it), if any is active,
		// when that's the case, we don't override it with our use ID.

		ImGuiContext& g = *GImGui;

		if (ID != 0 and g.LastItemData.ID == 0)
		{
			ImGui::SetLastItemData(ID, g.LastItemData.ItemFlags, g.LastItemData.StatusFlags, g.LastItemData.Rect);
		}
	}
} // namespace JPL::ImGuiEx
