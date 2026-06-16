//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatial
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

#include "Model/RoomModel.h"
#include "ImGui/ImGui.h"

#include <optional>

struct ImRect;

namespace JPL
{
	class RoomView
	{
	public:
		struct Listener
		{
			float Radius = 16.0f; // pixels
			ImU32 Colour = JPL::GUI::Colours::Theme::Titlebar;
		};

		struct Source
		{
			float Radius = 13.0f; // pixels
			ImU32 Colour = JPL::GUI::Colours::Theme::Selected;
		};

	public:
		explicit RoomView(const std::shared_ptr<RoomModel>& model);
		void OnStart();
		void DrawProperties();

		// @returns draw list of the canvas, in case something needs to be drawn on top
		ImDrawList* DrawEnvironment();

		// Set source size in world units (not pixels)
		void SetSourceSize(float newSize);

	private:
		void DrawListener();
		void DrawSource();

		static bool DrawObjectCircle(const char* stringId,
									 ImVec2& objectPosition,
									 float radiusPx,
									 JPL::Colour baseColour,
									 JPL::Colour heldColour,
									 std::optional<float> outerRadius = {});

		static ImVec2 GetAbsolutePosition(const ImRect& bounds, ImVec2 position);

	private:
		std::shared_ptr<RoomModel> mModel;

		Listener mListener;
		Source mSource;

		// Source size in world units
		float mSourceSize = 1.0f;
		float mCanvasResolution = 1.0f;
	};
} // namespace JPL

