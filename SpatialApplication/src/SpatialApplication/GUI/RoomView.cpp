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

    RoomView::RoomView(RoomModel& model)
        : mModel(model)
    {
    }

    void RoomView::OnStart()
    {
    }

    void RoomView::DrawProperties()
    {
        using namespace ImGuiEx;

        bool bRoomSizeChanged = false;
        MinimalVec3 roomSize = mModel.RoomSize.Get().Size;

        const ImVec2 availableSize = ImGui::GetContentRegionAvail();

        const ChildConfig propsPanelConfig
        {
            .ChildFlags = ImGuiChildFlags_AutoResizeY
        };

        // Settings
        Child("PropertiesFrame", propsPanelConfig, [&]
        {
            LayoutHorizontal("Properties", [&]
            {
                LayoutVertical("Room Props", ImVec2(0, 0), 0.0f, [&]
                {
                    ScopedItemWidth width(200.0f);

                    //? Not ideal, the spacing depends on the font, but works for now6
                    ImGui::TextDisabled("       X                 Y                Z");
                    ImGui::Spring();

                    {
                        // ImGui multi-input widgets push id of the field index,
                        // so we need to detect component id for our outline logic
                        ScopedItemOutline outline("Room Size", { 0, 1, 2 });

                        auto roomSizeEdit = roomSize;
                        ImGui::InputFloat3("Room Size", &roomSizeEdit.X, "%.1f");
                        if (ImGui::IsItemDeactivatedAfterEdit())
                        {
                            roomSize.X = std::max(2.0f, roomSizeEdit.X);
                            roomSize.Y = std::max(2.0f, roomSizeEdit.Y);
                            roomSize.Z = std::max(2.0f, roomSizeEdit.Z);
                            bRoomSizeChanged = true;
                        }
                    }

                    ImGui::Spacing();
                    ImGui::Spacing();

                    auto clampToRoomBounds = [&roomSize](const MinimalVec3& position)
                    {
                        return MinimalVec3{
                            std::clamp(position.X, 1.0f, roomSize.X - 1),
                            std::clamp(position.Y, 0.1f, roomSize.Y - 1),
                            std::clamp(position.Z, 1.0f, roomSize.Z - 1)
                        };
                    };

                    {
                        ScopedItemOutline outline("Source Pos.", { 0, 1, 2 });

                        auto sourcePos = mModel.GetSourceAbsPosition();
                        if (ImGui::DragFloat3("Source Pos.", sourcePos.mVF.data(), 0.33f, 0.0f, 0.0f, "%.1f"))
                        {
                            sourcePos = clampToRoomBounds(sourcePos);

                            const MinimalVec3 newSourcePos = sourcePos / roomSize;
                            mModel.Source.Set({ newSourcePos });
                        }
                    }

                    {
                        ScopedItemOutline outline("Listener Pos.", { 0, 1, 2 });

                        auto listenerPos = mModel.GetListenerAbsPosition();
                        if (ImGui::DragFloat3("Listener Pos.", listenerPos.mVF.data(), 0.33f, 0.0f, 0.0f, "%.1f"))
                        {
                            listenerPos = clampToRoomBounds(listenerPos);

                            const MinimalVec3 newListenerPos = listenerPos / roomSize;
                            mModel.Listener.Set({ newListenerPos });
                        }
                    }
                });

                ImGui::Spacing();

                ScopedItemWidth width(140.0f);
                LayoutHorizontal("Options", [&]
                {
                    LayoutVertical("Specular Options", ImVec2(0, 0), 0.0f, [&]
                    {
                        PropertyCheckbox("Specular Reflections", mModel.EnableSpecular);

                        ImGui::Spacing();

                        ScopedDisable disable(not mModel.EnableSpecular.Get());

                        {
                            ScopedItemOutline outline("Num Prim. Rays");

                            int numPrimaryRays = static_cast<int>(mModel.NumPrimaryRays.Get());

                            if (ImGui::InputInt("Num Prim. Rays", &numPrimaryRays))
                            {
                                numPrimaryRays = std::max(0, numPrimaryRays);
                                mModel.NumPrimaryRays.Set(static_cast<uint32_t>(numPrimaryRays));
                            }
                        }

                        {
                            ScopedItemOutline outline("Max Spec. Order");

                            int maxSpecularOrder = static_cast<int>(mModel.MaxOrder.Get());
                            if (ImGui::InputInt("Max Spec. Order", &maxSpecularOrder))
                            {
                                maxSpecularOrder = std::clamp(maxSpecularOrder, 0, static_cast<int>(SpecularRayTracing::cMaxOrder));
                                mModel.MaxOrder.Set(static_cast<uint32_t>(maxSpecularOrder));
                            }
                        }

                        {
                            ScopedItemOutline outline("Surface Material");

                            static uint32_t selectedMaterialId = mModel.SurfaceMaterial.Get()->ID;
                            std::string_view currentMaterial = mModel.SurfaceMaterial.Get()->Name;

                            if (ImGui::BeginCombo("Surface Material", currentMaterial.data()))
                            {
                                const auto& acousticMaterials = AcousticMaterial::GetListOfMaterials();

                                for (const auto& [id, material] : acousticMaterials)
                                {
                                    bool bSelected = id == selectedMaterialId;
                                    if (ImGui::Selectable(material.Name.data(), &bSelected))
                                    {
                                        selectedMaterialId = id;
                                        mModel.SurfaceMaterial.Set(&material);
                                    }
                                }
                                ImGui::EndCombo();
                            }
                            ImGuiEx::SetTooltip(currentMaterial);
                        }

                    });

                    ImGui::Spacing();

                    LayoutVertical("Direct Options", [&]
                    {
                        PropertyCheckbox("Direct Sound", mModel.EnableDirect);

                        ImGui::Spacing();

                        ScopedDisable disable(not mModel.EnableDirect.Get());

                        PropertyCheckbox("Air Absorption", mModel.DirectSound->EnableAirAbsorption);
                        PropertyCheckbox("Distance Attenuation", mModel.DirectSound->EnableDistanceAttenuation);
                        PropertyCheckbox("PropagaionDelay", mModel.DirectSound->EnablePropagationDelay);
                    });
                });
            });
        });

        if (bRoomSizeChanged)
        {
            mModel.RoomSize.Set({ roomSize });
        }
    }

    ImDrawList* RoomView::DrawEnvironment()
    {
        using namespace ImGuiEx;

        MinimalVec3 roomSize = mModel.RoomSize.Get().Size;

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
        ImVec2 listenerPosition{ mModel.Listener.Get().Position.X, mModel.Listener.Get().Position.Z };

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

        if (bListenerPositionChanged)
        {
            const float Y = mModel.Listener.Get().Position.Y;
            mModel.Listener.Set({ { listenerPosition.x, Y, listenerPosition.y } });
        }
    }

    void RoomView::DrawSource()
    {
        ImVec2 sourcePosition{ mModel.Source.Get().Position.X, mModel.Source.Get().Position.Z };

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

        if (bSourcePositionChanged)
        {
            const float Y = mModel.Source.Get().Position.Y;
            mModel.Source.Set({ { sourcePosition.x, Y, sourcePosition.y } });
        }
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
