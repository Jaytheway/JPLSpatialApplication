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

#include "RoomView.h"

#include <JPLSpatial/PathTracing/SpecularRayTracing.h>
#include <JPLSpatial/Math/MinimalVec2.h>
#include <JPLSpatial/AcousticMaterial.h>

#include "ImGui/ImGui.h"
#include "GUI/PropertyWidgets.h"

#include <Walnut/ImGui/ImGuiTheme.h> // colour utils (TODO: move to our ImGui utils)

#include <algorithm>
#include <cmath>
#include <string_view>

namespace JPL
{
    static ImRect GetCanvasBounds()
    {
        return ImGui::GetCurrentWindow()->Rect();
    }

    ImVec2 RoomView::GetAbsolutePosition(const ImRect& bounds, ImVec2 position)
    {
        return position * bounds.GetSize() + bounds.Min;
    }

    RoomView::RoomView(const std::shared_ptr<RoomModel>& model)
        : mModel(model)
    {
        JPL_ASSERT(mModel);
    }

    void RoomView::OnStart()
    {
    }

    void RoomView::DrawProperties()
    {
        using namespace ImGuiEx;

        const ImVec2 availableSize = ImGui::GetContentRegionAvail();
        const auto autoResizeXY = ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX;

        Child("PropertiesFrame", ChildConfig{.ChildFlags = autoResizeXY }, [&]
        {
            LayoutVertical("Room Props", ImVec2(0, 0), 0.0f, [&]
            {
                ScopedItemWidth width(200.0f);

                //? Not ideal, the spacing depends on the font, but works for now6
                ImGui::TextDisabled("       X                   Y                   Z");
                ImGui::Spring();

                const InputVec3Config roomSizePropconfig
                {
                    .Base = {
                        .Fmt = "%.1f",
                        .Min = MinimalVec3(2.0f, 2.0f, 2.0f)
                    },
                    .Flags = ImGuiInputTextFlags_EnterReturnsTrue
                };

                GUI::PropertyInputVec3("Room Size", Undoable(mModel, &RoomModel::RoomSize), roomSizePropconfig);

                Layout<Spacer, Spacer>();

                const MinimalVec3 roomSize = mModel->RoomSize.Get();

                const DragVec3Config positionsConfig
                {
                    .Base = {
                        .Fmt = "%.1f",
                        .Min = MinimalVec3(1.0f, 0.1, 1.0f),
                        .Max = roomSize - MinimalVec3(1.0f, 1.0f, 1.0f)
                    },
                    .VSpead = 0.33f
                };

                // We store positions as proportion to room size,
                // while display and edit in meters.
                GUI::PropertyTransform transform
                {
                    [&roomSize](const MinimalVec3& proportion) { return proportion * roomSize; },
                    [&roomSize](const MinimalVec3& position) { return position / roomSize; },
                };

                GUI::PropertyDragVec3("Source Pos.", Undoable(mModel, &RoomModel::SourcePosition), positionsConfig, transform);
                GUI::PropertyDragVec3("Listener Pos.", Undoable(mModel, &RoomModel::ListenerPosition), positionsConfig, transform);
            });
        });
    }

    ImDrawList* RoomView::DrawEnvironment()
    {
        using namespace ImGuiEx;

        const MinimalVec3 roomSize = mModel->RoomSize.Get();

        ImDrawList* canvasDrawList = nullptr;

        Child("Canvas Frame", [&]
        {
            ImRect bounds = ImGui::GetCurrentWindow()->Rect();
            ImVec2 canvasSize{ bounds.GetWidth(), bounds.GetHeight() };

            {
                const float aspectRatio = roomSize.Z / roomSize.X;

                canvasSize.y = canvasSize.x * aspectRatio;

                if (canvasSize.y > bounds.GetHeight())
                    canvasSize *= bounds.GetHeight() / canvasSize.y;

                if (canvasSize.x > bounds.GetWidth())
                    canvasSize *= bounds.GetWidth() / canvasSize.x;
            }

            mCanvasResolution = canvasSize.x / roomSize.X;

            {
                ScopedColour childBg(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_FrameBg));
                Child("Canvas", { .Size = canvasSize, .ChildFlags = ImGuiChildFlags_Borders }, [&]
                {
                    DrawSource();
                    DrawListener();

                    canvasDrawList = ImGui::GetWindowDrawList();
                });
            }
        });

        return canvasDrawList;
    }

    void RoomView::SetSourceSize(float newSize)
    {
        mSourceSize = newSize;
    }

    void RoomView::DrawListener()
    {
        ImVec2 listenerPosition{ mModel->ListenerPosition.Get().X, mModel->ListenerPosition.Get().Z };

        const auto baseColour = Colour(mListener.Colour).WithValue(0.65f).WithSaturation(0.75f);
        const auto heldColour = baseColour.WithMultipliedValue(1.5f).WithMultipliedSaturation(1.1f);
        // we don't need to draw outer radius for the listener for now,
        // since we're not drawing speakers or VBAP poitns in this view

        const bool bListenerPositionChanged =
            DrawObjectCircle("Listener",
                             listenerPosition,
                             mListener.Radius,
                             baseColour,
                             heldColour);

        auto& history = JPLSpatialApplication::GetCommandHistory();

        if (ImGui::IsItemActivated())
            history.BeginPropertyEdit(Undoable(mModel, &RoomModel::ListenerPosition), "Listener Position");

        if (bListenerPositionChanged)
        {
            const float Y = mModel->ListenerPosition.Get().Y;
            mModel->ListenerPosition.Set({ listenerPosition.x, Y, listenerPosition.y });
        }

        if (ImGui::IsItemDeactivated())
            history.EndPropertyEdit(Undoable(mModel, &RoomModel::ListenerPosition));
    }

    void RoomView::DrawSource()
    {
        ImVec2 sourcePosition{ mModel->SourcePosition.Get().X, mModel->SourcePosition.Get().Z };

        const auto baseColour = Colour(mSource.Colour).WithValue(0.8f).WithSaturation(0.7f);
        const auto heldColour = baseColour.WithMultipliedValue(1.5f).WithMultipliedSaturation(1.1f);
        const float sourceRadius = mSourceSize * mCanvasResolution * 0.5f;

        const bool bSourcePositionChanged =
            DrawObjectCircle("Source",
                             sourcePosition,
                             mSource.Radius,
                             baseColour,
                             heldColour,
                             sourceRadius);

        auto& history = JPLSpatialApplication::GetCommandHistory();

        if (ImGui::IsItemActivated())
            history.BeginPropertyEdit(Undoable(mModel, &RoomModel::SourcePosition), "Source Position");

        if (bSourcePositionChanged)
        {
            const float Y = mModel->SourcePosition.Get().Y;
            mModel->SourcePosition.Set({ sourcePosition.x, Y, sourcePosition.y });
        }

        if (ImGui::IsItemDeactivated())
            history.EndPropertyEdit(Undoable(mModel, &RoomModel::SourcePosition));
    }

    bool RoomView::DrawObjectCircle(const char* stringId,
                                    ImVec2& objectPosition,
                                    float radiusPx,
                                    Colour baseColour,
                                    Colour heldColour,
                                    std::optional<float> outerRadius)
    {
        bool bPositionChanged = false;

        auto* drawList = ImGui::GetWindowDrawList();
        const ImRect bounds = GetCanvasBounds();
        const ImVec2 center = bounds.GetCenter();

        if (objectPosition == ImVec2(-1.0f, -1.0f))
        {
            objectPosition = { 0.5f, 0.4f };
            bPositionChanged = true;
        }

        const ImVec2 objectPosAbs = GetAbsolutePosition(bounds, objectPosition);

        const ImVec2 objectItemSize = ImVec2(radiusPx, radiusPx);
        const ImVec2 objectMin = objectPosAbs - objectItemSize;
        const ImVec2 objectMax = objectPosAbs + objectItemSize;
        const ImRect objectBounds = { objectMin, objectMax };

        const ImGuiID objectID = ImGui::GetID(stringId);
        if (not ImGui::ItemAdd(objectBounds, objectID))
        {
            return bPositionChanged;
        }

        const ImVec2 mousePos = ImGui::GetMousePos();
        static ImVec2 mouseClickOffset;
        bool hovered = false, held = false, pressed = false;
        if (ImGui::ButtonBehavior(objectBounds, objectID, &hovered, &held, ImGuiButtonFlags_PressedOnClick))
        {
            pressed = true;
            mouseClickOffset = mousePos - objectPosAbs;
        }

        const ImU32 colour = pressed || held ? heldColour : baseColour;
        const ImU32 colourHovered = IM_COL32_WHITE;

        // Hover outline
        if (hovered || held)
        {
            drawList->AddCircle(objectPosAbs, radiusPx + 1.0f, colourHovered, 0, 2.0f);
        }

        // Fill
        drawList->AddCircleFilled(objectPosAbs, radiusPx, colour);

        if (outerRadius.has_value())
        {
            // Actual object radius as per VBAP object size property
            drawList->AddCircle(objectPosAbs, *outerRadius, IM_COL32(255, 255, 255, 125), 0, 1.0f);
        }

        if (held)
        {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0f))
            {
                ImVec2 positionPixels = ImClamp(mousePos - mouseClickOffset, bounds.GetTL() + objectItemSize, bounds.GetBR() - objectItemSize) - bounds.Min;
                objectPosition = positionPixels / bounds.GetSize();
                bPositionChanged = true;
            }
        }

        return bPositionChanged;
    }
} // namespace JPL
