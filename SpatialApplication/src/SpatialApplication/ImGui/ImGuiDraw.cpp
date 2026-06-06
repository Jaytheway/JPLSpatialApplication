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

#include "ImGuiDraw.h"

#include <JPLSpatial/Math/Math.h>
#include <JPLSpatial/Math/MinimalVec2.h>

namespace JPL::ImGuiEx
{
	void DrawArrow(ImDrawList& drawList, const ImVec2& lineStart, const ImVec2& lineEnd, ImU32 colour, float arrowSize)
	{
		Vec2 dir = (lineEnd - lineStart);
		const float vectorLength = dir.Length();
		if (vectorLength < JPL_FLOAT_EPS)
			return;

		dir /= vectorLength;
		arrowSize = std::min(vectorLength, arrowSize);

		const Vec2 arrowStart = lineEnd - dir * arrowSize;

		drawList.AddLine(lineStart, lineEnd, colour, 3.0f);
		drawList.AddLine(lineEnd, arrowStart + Vec2(dir.Y, -dir.X) * arrowSize * 0.5f, colour, 3.0f);
		drawList.AddLine(lineEnd, arrowStart + Vec2(-dir.Y, dir.X) * arrowSize * 0.5f, colour, 3.0f);
	}
} // namespace JPL::ImGuiEx
