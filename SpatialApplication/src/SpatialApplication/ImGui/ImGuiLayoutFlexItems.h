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

#pragma once

#include "ImGui/ImGuiLayoutFlex.h"


//==============================================================================
/// A collection of Flex-ible items
namespace JPL::ImGuiEx::Flex
{
	namespace Impl
	{
		[[nodiscard]] inline auto Label(const char* text)
		{
			return [text] { ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(text); };
		}
	}

	/// An item based on Flex::Params and a fixed-size lable on the right.
	// Note: if font changes at runtime, Flex::Layout holding this has to be rebuilt,
	// otherwise the lable fixed size will not updated accordingly.
	template<CLayoutOrDrawCbParam Content>
	[[nodiscard]] auto Labeled(Content&& content, const Flex::Params& params, const char* label, float labelSize = 0.0f)
	{
		if (labelSize <= 0.0f)
			labelSize = ImGui::CalcTextSize(label).x;

		return Row(Item(params, std::forward<Content>(content)), Fixed(labelSize, Impl::Label(label)));
	}

	/// An item based on Flex::Params and a fixed-size lable on the left.
	// Note: if font changes at runtime, Flex::Layout holding this has to be rebuilt,
	// otherwise the lable fixed size will not updated accordingly.
	template<CLayoutOrDrawCbParam Content>
	[[nodiscard]] auto Labeled(const char* label, Content&& content, const Flex::Params& params, float labelSize = 0.0f)
	{
		if (labelSize <= 0.0f)
			labelSize = ImGui::CalcTextSize(label).x;

		return Row(Fixed(labelSize, Impl::Label(label)), Item(params, std::forward<Content>(content)));
	}

	/// A growing item and a fixed-size lable on the right.
	template<CLayoutOrDrawCbParam Content>
	[[nodiscard]] auto Labeled(Content&& content, const char* label, float labelSize = 0.0f)
	{
		return Labeled(std::forward<Content>(content), Params{ .Weight = 1 }, label, labelSize);
	}

	/// A growing item and a fixed-size lable on the left.
	template<CLayoutOrDrawCbParam Content>
	[[nodiscard]] auto Labeled(const char* label, Content&& content, float labelSize = 0.0f)
	{
		return Labeled(label, std::forward<Content>(content), Params{ .Weight = 1 }, labelSize);
	}

	/// Align element horizontally to middle third of the available space.
	template<CElement Element>
	auto AlignHor(Element&& element)
	{
		return RowGrow(0, Spring(), std::forward<Element>(element), Spring());
	}

	/// Align item horizontally to middle third of the available space.
	template<CLayoutOrDrawCbParam Content>
	auto AlignHor(Content&& content, Params params = { .Weight = 1 })
	{
		return AlignHor(Item(params, std::forward<Content>(content)));
	}

} // namespace JPL::ImGuiEx::Flex

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

namespace JPL::ImGuiEx::Flex
{
   
} // namespace JPL::ImGuiEx::Flex
