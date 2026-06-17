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

	void DrawTextCentered(ImDrawList& drawList, const char* text, ImU32 colour)
	{
		const ImVec2 bbMin = ImGui::GetCursorScreenPos();
		const ImVec2 bbMax = bbMin + ImGui::GetContentRegionAvail();
		DrawTextCentered(drawList, bbMin, bbMax, text, colour);
	}

	void DrawTextCentered(ImDrawList& drawList, const ImVec2& boundsMin, const ImVec2& boundsMax, const char* text, ImU32 colour)
	{
		const ImVec2 textSize = ImGui::CalcTextSize(text);
		const ImVec2 textPos = ImRect(boundsMin, boundsMax).GetCenter() - textSize * 0.5f;
		drawList.AddText(textPos, colour, text);
	}

	void DrawShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, EShadowEdge edge, const ShadowEdgeStyle& style)
    {
        const float rectW = pMax.x - pMin.x;
        const float rectH = pMax.y - pMin.y;

        if (rectW <= 0.0f || rectH <= 0.0f)
            return;

        const bool horizontal = edge == EShadowEdge::Left || edge == EShadowEdge::Right;

        const float shadowDepth = ImMin(style.ShadowSize, horizontal ? rectW : rectH);

        if (shadowDepth <= 0.0f)
            return;

        drawList.PushClipRect(pMin, pMax, true);

        const ImVec2 uv = drawList._Data->TexUvWhitePixel;

        const int vertsAlong = style.AlongSegments + 1;
        const int vertsAcross = style.AcrossSegments + 1;

        drawList.PrimReserve(style.AlongSegments * style.AcrossSegments * 6, vertsAlong * vertsAcross);

        const ImDrawIdx baseIdx = drawList._VtxCurrentIdx;

        auto smoothStep = [](float t)
        {
            t = ImClamp(t, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        };

        const int baseAlpha = (style.Colour >> IM_COL32_A_SHIFT) & 0xFF;
        const ImU32 colorNoAlpha = style.Colour & ~IM_COL32_A_MASK;

        const float alongLength = horizontal ? rectH : rectW;
        const float feather = ImClamp(style.EdgeFeather, 0.0f, alongLength * 0.5f);

        for (int a = 0; a < vertsAlong; ++a)
        {
            const float ta = static_cast<float>(a) / static_cast<float>(style.AlongSegments);

            for (int d = 0; d < vertsAcross; ++d)
            {
                const float td = static_cast<float>(d) / static_cast<float>(style.AcrossSegments);

                ImVec2 p{};

                float distanceFromEdge01 = td;
                float distanceAlong01 = ta;

                switch (edge)
                {
                case EShadowEdge::Left:
                {
                    p.x = pMin.x + td * shadowDepth;
                    p.y = pMin.y + ta * rectH;
                    break;
                }

                case EShadowEdge::Right:
                {
                    p.x = pMax.x - td * shadowDepth;
                    p.y = pMin.y + ta * rectH;
                    break;
                }

                case EShadowEdge::Top:
                {
                    p.x = pMin.x + ta * rectW;
                    p.y = pMin.y + td * shadowDepth;
                    break;
                }

                case EShadowEdge::Bottom:
                {
                    p.x = pMin.x + ta * rectW;
                    p.y = pMax.y - td * shadowDepth;
                    break;
                }
                }

                // Strong at the edge, fades inward
                const float depthT = smoothStep(distanceFromEdge01);
                const float alphaDepth = powf(1.0f - depthT, ImMax(0.1f, style.FalloffPower));

                // Soft ends along the edge
                float alphaAlong = 1.0f;

                if (feather > 0.0f)
                {
                    const float alongPx = distanceAlong01 * alongLength;

                    const float fromStart = alongPx;
                    const float fromEnd = alongLength - alongPx;

                    const float startFade = smoothStep(fromStart / feather);
                    const float endFade = smoothStep(fromEnd / feather);

                    alphaAlong = startFade * endFade;
                }

                const int alpha = static_cast<int>(baseAlpha * alphaDepth * alphaAlong + 0.5f);
                const ImU32 vtxColor = colorNoAlpha | (static_cast<ImU32>(alpha) << IM_COL32_A_SHIFT);

                drawList.PrimWriteVtx(p, uv, vtxColor);
            }
        }

        for (int a = 0; a < style.AlongSegments; ++a)
        {
            for (int d = 0; d < style.AcrossSegments; ++d)
            {
                const ImDrawIdx i0 = baseIdx + a * vertsAcross + d;
                const ImDrawIdx i1 = i0 + 1;
                const ImDrawIdx i2 = i0 + vertsAcross;
                const ImDrawIdx i3 = i2 + 1;

                drawList.PrimWriteIdx(i0);
                drawList.PrimWriteIdx(i1);
                drawList.PrimWriteIdx(i3);

                drawList.PrimWriteIdx(i0);
                drawList.PrimWriteIdx(i3);
                drawList.PrimWriteIdx(i2);
            }
        }

        drawList.PopClipRect();
    }

    void DrawLeftShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, const ShadowEdgeStyle& style)
    {
        DrawShadowEdge(drawList, pMin, pMax, EShadowEdge::Left, style);
    }

    void DrawRightShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, const ShadowEdgeStyle& style)
    {
        DrawShadowEdge(drawList, pMin, pMax, EShadowEdge::Right, style);
    }

    void DrawTopShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, const ShadowEdgeStyle& style)
    {
        DrawShadowEdge(drawList, pMin, pMax, EShadowEdge::Top, style);
    }

    void DrawBottomShadowEdge(ImDrawList& drawList, ImVec2 pMin, ImVec2 pMax, const ShadowEdgeStyle& style)
    {
        DrawShadowEdge(drawList, pMin, pMax, EShadowEdge::Bottom, style);
    }

} // namespace JPL::ImGuiEx
