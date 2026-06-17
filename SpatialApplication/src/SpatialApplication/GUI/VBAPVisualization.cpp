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

#include "VBAPVisualization.h"

#include <JPLSpatial/ErrorReporting.h>
#include <JPLSpatial/Panning/VBAPanning2D.h>
#include <JPLSpatial/Math/MinimalQuat.h>

#include "ImGui/ImGui.h"
#include "GUI/PropertyWidgets.h"

#include <algorithm>
#include <span>

namespace JPL
{

    using PannerType = JPL::VBAPanner2D<>;
    using SourceLayoutType = typename PannerType::SourceLayoutType;
    using UpdateData = PannerType::PanUpdateData;

    struct Panner2D : PannerType {};
    struct SourceLayout2D : SourceLayoutType {};

    namespace stdr = std::ranges;
    namespace stdv = std::views;

    namespace detail
    {
        static const std::vector<JPL::NamedChannelMask> ValidSourceMasks
        {
            { JPL::ChannelMask::Mono },
            { JPL::ChannelMask::Stereo },
            { JPL::ChannelMask::LCR },
            { JPL::ChannelMask::Quad },
            { JPL::ChannelMask::Surround_5_0 },
            { JPL::ChannelMask::Surround_6_0 },
            { JPL::ChannelMask::Surround_7_0 },
        };

        static const std::vector<JPL::NamedChannelMask> ValidTrargetMasks
        {
            { JPL::ChannelMask::Stereo },
            { JPL::ChannelMask::Quad },
            { JPL::ChannelMask::Surround_4_1 },
            { JPL::ChannelMask::Surround_5_1 },
            { JPL::ChannelMask::Surround_6_1 },
            { JPL::ChannelMask::Surround_7_1 },
            { JPL::ChannelMask::Surround_7_0 },

            //========================================
            { JPL::ChannelMask::Surround_5_0_2 },
            { JPL::ChannelMask::Surround_5_1_2 },
            { JPL::ChannelMask::Surround_5_0_4 },
            { JPL::ChannelMask::Surround_5_1_4 },

            //========================================
            // Dolby Atmos surround setups
            { JPL::ChannelMask::Surround_7_0_2 },
            { JPL::ChannelMask::Surround_7_1_2 },
            { JPL::ChannelMask::Surround_7_0_4 },
            { JPL::ChannelMask::Surround_7_1_4 },

            { JPL::ChannelMask::Surround_7_0_6 },
            { JPL::ChannelMask::Surround_7_1_6 },

            //========================================
            // Atmos surround setups
            { JPL::ChannelMask::Surround_9_0_4 },
            { JPL::ChannelMask::Surround_9_1_4 },
            { JPL::ChannelMask::Surround_9_0_6 },
            { JPL::ChannelMask::Surround_9_1_6 }
        };
    }

    static inline Vec3 DirectionFrom(float azimuth, float elevation)
    {
        const auto [sa, ca] = JPL::Math::SinCos(azimuth);
        const auto [se, ce] = JPL::Math::SinCos(elevation);
        return Vec3{
            ce * sa,
            se,
            -ce * ca
        };
    }

    VBAPVisualization::VBAPVisualization(const std::shared_ptr<VBAPVisualizationModel>& model)
        : mModel(model)
        , mSourceChannelSet(JPL::ChannelMask::Invalid)
        , mTargetChannelSet(JPL::ChannelMask::Invalid)
    {
        JPL_ASSERT(mModel);

        mModel->TargetChannelMap.AddChangeCallback<&VBAPVisualization::OnTargetChannelsChanged>(this);
        mModel->SourceChannelMap.AddChangeCallback<&VBAPVisualization::OnSourceChannelsChanged>(this);

        JPL_ASSERT(mModel->VBAPModel);
        mModel->VBAPModel->AddListener(this);

        OnTargetChannelsChanged(mModel->TargetChannelMap.Get());
        OnSourceChannelsChanged(mModel->SourceChannelMap.Get());
        UpdatePoints();
    }

    VBAPVisualization::~VBAPVisualization()
    {
        mModel->TargetChannelMap.RemoveChangeCallback(this);
        mModel->SourceChannelMap.RemoveChangeCallback(this);

        if (mModel->VBAPModel)
        {
            mModel->VBAPModel->RemoveListener(this);
        }
    }

    void VBAPVisualization::SetVBAPModel(std::shared_ptr<JPL::VBAPModel> newVBAPModel)
    {
        if (mModel->VBAPModel)
        {
            mModel->VBAPModel->RemoveListener(this);
        }

        mModel->VBAPModel = newVBAPModel;

        mModel->VBAPModel->AddListener(this);

        OnTargetChannelsChanged(mModel->TargetChannelMap.Get());
        OnSourceChannelsChanged(mModel->SourceChannelMap.Get());
        UpdatePoints();
    }

    template<class DrawContentCb>
    static void DrawSquareCanvas(const char* label, ImVec2 size, const DrawContentCb& drawContentCallback)
    {
        JPL::ImGuiEx::ScopedColour childBg(ImGuiCol_ChildBg, ImGui::GetColorU32(ImGuiCol_FrameBg));

        const float width = size.x;
        const float height = size.y;
        ImVec2 canvasSize{ width, height };

        //if (bRoomSizeChanged)
        {
            // Forcing square canvas
            const float aspectRatio = 1.0f;

            canvasSize.y = canvasSize.x * aspectRatio;

            if (canvasSize.y > height)
                canvasSize *= height / canvasSize.y;

            if (canvasSize.x > width)
                canvasSize *= width / canvasSize.x;
        }

        JPL::ImGuiEx::Child(label, [&]
        {
            drawContentCallback();
        }, { .Size = canvasSize, .ChildFlags = ImGuiChildFlags_Borders });

    }

    void VBAPVisualization::Draw()
    {
        using namespace JPL::ImGuiEx;

        static bool bDrawSpeakers = true;

        LayoutHorizontal("Properties", [&]
        {
            const float halfWidth = ImGui::GetContentRegionAvail().x * 0.5f;
            const ImVec2 halfSize(halfWidth, 0.0f);

            LayoutVertical("Vis Properties", halfSize, 0.0f, [&]
            {
                ScopedItemWidth width(200.0f);

                const SliderConfig sliderConfig{ .Fmt = "%.2f" };

                GUI::PropertySlider("Focus", Undoable(mModel->VBAPModel, &VBAPModel::Focus), 0.0f, 1.0f, sliderConfig);

                {
                    // If set to calculate spread from source size,
                    // just display the current value computed from the source size
                    if (mModel->VBAPModel->SpreadFromSourceSize.Get())
                    {
                        const JPL::MinimalVec3 sourcePosition = mModel->VBAPModel->SourcePosition.Get();
                        
                        float distance;
                        const JPL::MinimalVec3 direction =
                            sourcePosition.SafeNormal(distance, /* fallback direction */ JPL::MinimalVec3(0.0f, 0.0f, -1.0f));

                        ScopedDisable _(true);

                        float spread = JPL::GetSpreadFromSourceSize(mModel->VBAPModel->SourceSize.Get(), distance);
                        Slider("Spread", spread, 0.0f, 1.0f, sliderConfig);
                    }
                    else
                    {
                        GUI::PropertySlider("Spread", Undoable(mModel->VBAPModel, &VBAPModel::Spread), 0.0f, 1.0f, sliderConfig);
                    }
                }

                //! Should the source size be in visualization model or room model?
                {
                    //ScopedDisable disableIf(not mModel->VBAPModel->SpreadFromSourceSize.Get());
                    GUI::PropertySlider("Source Size", Undoable(mModel->VBAPModel, &VBAPModel::SourceSize), 0.1f, 100.0f, SliderConfig{ .Fmt = "%.1f" });
                }

                ImGui::Spacing();
                
                GUI::PropertyCheckbox("Spread From Source Size", Undoable(mModel->VBAPModel, &VBAPModel::SpreadFromSourceSize));
                GUI::PropertyCheckbox("Spread From Height", Undoable(mModel->VBAPModel, &VBAPModel::HeightSpread));

            }); // LayoutVertical

            LayoutVertical("Extra props", halfSize, 0.0f, [&]
            {
                ScopedItemWidth width(200.0f);

                GUI::PropertyCheckbox("Source Orientation", Undoable(mModel->VBAPModel, &VBAPModel::UseSourceOrientation));
                
                ImGui::Spacing();

                const bool bIsConnectedToAudioPlayer = mModel->ConnectToAudioPlayer.Get();

                {
                    ScopedDisable disable(bIsConnectedToAudioPlayer);

                    int selectedSet =
                        std::ranges::find(
                            detail::ValidSourceMasks,
                            mModel->SourceChannelMap.Get()) - std::ranges::begin(detail::ValidSourceMasks);

                    auto getSourceSetNameCb = [](const JPL::NamedChannelMask& channelMask, int /*index*/) -> const char*
                    {
                        return channelMask.Name.data();
                    };

                    GUI::PropertyCombo("Source Channel Set",
                                       Undoable(mModel, &VBAPVisualizationModel::SourceChannelMap),
                                       selectedSet,
                                       std::span(detail::ValidSourceMasks),
                                       getSourceSetNameCb);
                }

                {
                    ScopedDisable disable(bIsConnectedToAudioPlayer);

                    int selectedOutputSet =
                        std::ranges::find(
                            detail::ValidTrargetMasks,
                            mModel->TargetChannelMap.Get()) - std::ranges::begin(detail::ValidTrargetMasks);
                    
                    auto getOutputSetNameCb = [](const JPL::NamedChannelMask& channelMask, int /*index*/) -> const char*
                    {
                        return channelMask.Name.data();
                    };

                    GUI::PropertyCombo("Output Channel Set",
                                       Undoable(mModel, &VBAPVisualizationModel::TargetChannelMap),
                                       selectedOutputSet,
                                       std::span(detail::ValidTrargetMasks),
                                       getOutputSetNameCb);
                }

                GUI::PropertyCheckbox("Connect to Audio Player", Undoable(mModel, &VBAPVisualizationModel::ConnectToAudioPlayer));

                SetTooltip("If enebaled, visualization is drawn for the Source & Output sets\n"
                           "currently used by the Audio Player.\n"
                           "\n"
                           "Disabling is meant for checking out visualization for the channel\n"
                           "layouts not available on the user system.");

                // TODO: do we want undo/redo for this?
                ImGuiEx::Checkbox("Show Speakers", bDrawSpeakers);
            });
        });

        Layout<Spacer, Spacer>();

        const ImVec2 availableSpace = ImGui::GetContentRegionAvail();
        const float axisSize = ImMax((availableSpace.x - ImGui::GetStyle().WindowPadding.x) * 0.5f, 4.0f);
        const ImVec2 canvasSize(axisSize, ImMin(axisSize, availableSpace.y));

        LayoutHorizontal("Canvases", [&]
        {
            DrawSquareCanvas("Top-down View", canvasSize, [&]
            {

                DrawPoints(mChannelPoints.Points);

                if (bDrawSpeakers)
                {
                    DrawSpeakers(mSpeakers.Points, mLFEIndex);
                }
            });

            DrawSquareCanvas("Left-side View", canvasSize, [&]
            {
                // View rotation
                static const JPL::Basis<JPL::MinimalVec3> rotation = []
                {
                    static constexpr JPL::MinimalVec3 cForward(0.0f, 0.0f, 1.0f);
                    static constexpr JPL::MinimalVec3 cUp(0.0f, 1.0f, 0.0f);

                    // TODO: we can play around with tilt offset if we want to slightly tilt the flat plane
                    static constexpr float tiltOffsetMultiplier = 0.9f;
                    return (JPL::Math::QuatFromUpAndForward(cUp, JPL::MinimalVec3(1.0f, 0.0f, 0.0f)) *
                            JPL::Math::QuatRotation(cForward, -JPL::JPL_HALF_PI * tiltOffsetMultiplier))
                        .ToBasis();
                }();

                DrawPointsTransformed(mChannelPoints.Points, rotation);

                if (bDrawSpeakers)
                {
                    DrawSpeakersTransformed(mSpeakers.Points, rotation);
                }
            });
        });
    }

    static ImVec2 GetPositionOnCanvas(const ImRect& bounds, ImVec2 direction)
    {
        direction += ImVec2(1, 1);
        direction *= ImVec2(0.5f, 0.5f);
        return direction * bounds.GetSize() + bounds.Min;
    }

    void VBAPVisualization::DrawPoints(std::span<const GroupedPoint> points)
    {
        for (const GroupedPoint& point : points)
        {
            DrawPoint(point);
        }
    }

    void VBAPVisualization::DrawSpeakers(std::span<const IntencityPoint> points, uint32_t lfeIndex)
    {
        for (uint32_t i = 0; i < points.size(); ++i)
        {
            if (i == lfeIndex)
                continue; // skip LFE

            const IntencityPoint& point = points[i];
            DrawSpeaker(point);
        }
    }

    void VBAPVisualization::DrawPointsTransformed(std::span<const GroupedPoint> points, const JPL::Basis<JPL::MinimalVec3>& rotation)
    {
        // Copy GroupedPoints to a buffer with depths
        struct PointWithDepth : GroupedPoint { float Depth; };

        static std::vector<PointWithDepth> pointsTransformed;
        pointsTransformed.resize(points.size());

        stdr::transform(points, pointsTransformed.begin(), [](const GroupedPoint& gp)
        {
            return PointWithDepth{ GroupedPoint{ gp.Point, gp.Group }, 0.0f };
        });

        // Z-sort poinns (depth-sorting based on the camera view axis)
        stdr::sort(pointsTransformed, [](const PointWithDepth& lhs, const PointWithDepth& rhs)
        {
            return lhs.Point.X > rhs.Point.X;
        });

        // Transform points to our view rotation
        // and assign depth value
        for (uint32_t i = 0; i < pointsTransformed.size(); ++i)
        {
            PointWithDepth& point = pointsTransformed[i];
            point.Depth = JPL::Math::FMA(-point.Point.X, 0.5f, 0.5f);
            point.Point = rotation.Transform(point.Point);
        }

        // Draw points, dimming the ones farther away from the camera
        for (const PointWithDepth& point : pointsTransformed)
        {
            const float brightnessMultiplier = JPL::Math::FMA(point.Depth, 0.5f, 0.5f);
            DrawPoint(point, brightnessMultiplier);
        }
    }

    void VBAPVisualization::DrawSpeakersTransformed(std::span<const IntencityPoint> points, const JPL::Basis<JPL::MinimalVec3>& rotation)
    {
        // Copy GroupedPoints to a buffer with depths
        struct PointWithDepth : IntencityPoint { float Depth; };

        static std::vector<PointWithDepth> pointsTransformed;
        pointsTransformed.resize(points.size());

        stdr::transform(points, pointsTransformed.begin(), [](const IntencityPoint& gp)
        {
            return PointWithDepth{ IntencityPoint{ gp.Direction, gp.Intensity}, 0.0f };
        });

        // Z-sort poinns (depth-sorting based on the camera view axis)
        stdr::sort(pointsTransformed, [](const PointWithDepth& lhs, const PointWithDepth& rhs)
        {
            return lhs.Direction.X > rhs.Direction.X;
        });

        // Transform points to our view rotation
        // and assign depth value
        for (uint32_t i = 0; i < pointsTransformed.size(); ++i)
        {
            PointWithDepth& point = pointsTransformed[i];
            point.Depth = JPL::Math::FMA(-point.Direction.X, 0.5f, 0.5f);
            point.Direction = rotation.Transform(point.Direction);
        }

        // Draw points, dimming the ones farther away from the camera
        for (const PointWithDepth& point : pointsTransformed)
        {
            const float brightnessMultiplier = JPL::Math::FMA(point.Depth, 0.5f, 0.5f);
            DrawSpeaker(point, brightnessMultiplier);
        }
    }

    void VBAPVisualization::DrawPoint(const GroupedPoint& point, float brightnessMultiplier)
    {
        static constexpr float pointRadius = 5.0f;
        static constexpr float margin = pointRadius * 2.0f;

        ImRect bounds = ImGui::GetCurrentWindow()->Rect();
        bounds.Expand(bounds.GetWidth() >= margin ? -margin : 0.0f);

        ImVec2 center = bounds.GetCenter();

        ImVec2 sourcePosAbs = GetPositionOnCanvas(bounds, { point.Point.X, point.Point.Z });

        ImVec2 sourceSize = ImVec2(pointRadius, pointRadius);
        ImVec2 sourceMin = sourcePosAbs - sourceSize;
        ImVec2 sourceMax = sourcePosAbs + sourceSize;
        ImRect sourceBounds = { sourceMin, sourceMax };

        static constexpr ImU32 colours[]
        {
            //'b', 'r', 'y', 'g', 'c', 'm', 'k'
            IM_COL32(50, 50, 255, 255),
            IM_COL32(255, 50, 50, 255),
            IM_COL32(255, 255, 50, 255),
            IM_COL32(50, 255, 50, 255),
            IM_COL32(50, 255, 255, 255),
            IM_COL32(255, 50, 255, 255),
            IM_COL32(255, 150, 50, 255),
            IM_COL32(50, 100, 255, 255),

            IM_COL32(255, 255, 255, 255),
            IM_COL32(255, 255, 255, 255),
            IM_COL32(255, 255, 255, 255),
            IM_COL32(255, 255, 255, 255),
            IM_COL32(255, 255, 255, 255),
            IM_COL32(255, 255, 255, 255),
            IM_COL32(255, 255, 255, 255),
            IM_COL32(255, 255, 255, 255)
        };

        // Hover outline
        //if (hovered || held)
          //  drawList->AddCircle(sourcePosAbs, mSource.Radius + 1.0f, colourHovered, 0, 2.0f);

        // Fill

        auto colour = ImGui::ColorConvertU32ToFloat4(colours[point.Group]);
        colour.x *= brightnessMultiplier;
        colour.y *= brightnessMultiplier;
        colour.z *= brightnessMultiplier;

        auto* drawList = ImGui::GetWindowDrawList();
        drawList->AddCircleFilled(sourcePosAbs, pointRadius, ImGui::ColorConvertFloat4ToU32(colour));
    }

    void VBAPVisualization::DrawSpeaker(const IntencityPoint& point, float brightnessMultiplier)
    {
        const float pointRadius = 5.0f + 5.0f * (brightnessMultiplier);
        const float margin = pointRadius * 3.0f;

        ImRect bounds = ImGui::GetCurrentWindow()->Rect();
        bounds.Expand(bounds.GetWidth() >= margin ? -margin : 0.0f);

        ImVec2 sourcePosAbs = GetPositionOnCanvas(bounds, { point.Direction.X, point.Direction.Z });

        ImVec2 sourceSize = ImVec2(pointRadius, pointRadius);
        ImVec2 sourceMin = sourcePosAbs - sourceSize;
        ImVec2 sourceMax = sourcePosAbs + sourceSize;
        ImRect sourceBounds = { sourceMin, sourceMax };

        auto* drawList = ImGui::GetWindowDrawList();

        const auto spekaerColour = IM_COL32(40 * brightnessMultiplier, 40 * brightnessMultiplier, 40 * brightnessMultiplier, 255);
        drawList->AddRectFilled(sourceMin, sourceMax, spekaerColour, 3.0f);

        const auto woferColour = IM_COL32(10 * brightnessMultiplier, 10 * brightnessMultiplier, 10 * brightnessMultiplier, 255);
        const float woferRadius = pointRadius * 0.8f;
        drawList->AddCircleFilled(sourcePosAbs, woferRadius, woferColour);

        if (point.Intensity > JPL::JPL_FLOAT_EPS) // avoid division by 0 in ImGui
        {
            const auto intencityColour = IM_COL32(200 * brightnessMultiplier, 200 * brightnessMultiplier, 200 * brightnessMultiplier, 255);
            drawList->AddCircleFilled(sourcePosAbs, woferRadius * point.Intensity, intencityColour);
        }
    }

    void VBAPVisualization::OnChange(JPL::GenericChangeBroadcaster* broadcaster)
    {
        if (broadcaster == mModel->VBAPModel.get())
        {
            UpdatePoints();
        }
    }

    void VBAPVisualization::OnTargetChannelsChanged(JPL::NamedChannelMask channelSet)
    {
        if (mTargetChannelSet == channelSet || not channelSet.Layout.IsValid())
        {
            return;
        }

        // Source layout is only valid for a specific target layout,
        // so we need to invalidate it when target changes
        mSourceLayout = {};

        // TODO: do we want to reset panner, or keep the last valid?
        mPanner = std::make_unique<JPLPanner>();

        if (not mPanner->Initialize(channelSet.Layout))
        {
            // TODO: report error
            mPanner = nullptr;
        }

        mTargetChannelSet = channelSet;

        // If panner was updated, we need to also update source layout
        if (mPanner && mSourceChannelSet.Layout.IsValid())
        {
            mSourceLayout = mPanner->InitializeSourceLayout(mSourceChannelSet.Layout);
            // TODO: report error if failed
        }

        mLFEIndex = mTargetChannelSet.Layout.GetChannelIndex(JPL::EChannel::LFE);
        mSpeakers.SetLayout(mTargetChannelSet.Layout);

        UpdatePoints();
    }

    void VBAPVisualization::OnSourceChannelsChanged(JPL::NamedChannelMask channelSet)
    {
        if (mSourceChannelSet == channelSet || not channelSet.Layout.IsValid())
        {
            return;
        }

        if (mPanner)
        {
            if (mSourceLayout = mPanner->InitializeSourceLayout(channelSet.Layout))
            {
                mSourceChannelSet = channelSet;
            }
            // TODO: report error if failed
        }
        else
        {
            mSourceChannelSet = channelSet;
            mSourceLayout = {};
        }

        UpdatePoints();
    }

    void VBAPVisualization::UpdatePoints()
    {
        mChannelPoints.Reset();

        if (mPanner && mSourceLayout)
        {
            JPLPanUpdateData panData;
            const float distance = mModel->VBAPModel->ComputePanUpdateData(mPanner->GetChannelMap().HasTopChannels(), panData);

            const uint32_t numSourceChannels = mSourceChannelSet.Layout.GetNumChannels() - mSourceChannelSet.Layout.HasLFE();
            const uint32_t numTargetChannels = mPanner->GetNumChannels();
            const uint32_t channelMixMapSize = numSourceChannels * numTargetChannels;

            static constexpr std::size_t cBufferCapacity = JPL::VBAPStandartTraits::MAX_CHANNEL_MIX_MAP_SIZE;

            JPL::StaticArray<float, cBufferCapacity> channelMixMap(channelMixMapSize, 0.0f);

            auto collectPoints = [](void* collector, std::span<const JPL::MinimalVec3> channelVSs, uint32_t channelID)
            {
                static_cast<SourcePanningVisualizationCallback*>(collector)->operator()(channelVSs, channelID);
            };

            if (mModel->VBAPModel->UseSourceOrientation.Get())
            {
                const auto orientation = JPL::OrientationData<JPL::MinimalVec3>::IdentityForward();
                /* const auto orientation = JPL::OrientationData<JPL::MinimalVec3>
                 {
                     .Up = JPL::MinimalVec3(0.0f, 1.0f, 0.0f),
                     .Forward = JPL::MinimalVec3(-1.0f, 0.0f, 0.0f)
                 };*/
                mPanner->ProcessVBAP(mSourceLayout, panData, orientation, channelMixMap, collectPoints, &mChannelPoints);
            }
            else
            {
                mPanner->ProcessVBAP(mSourceLayout, panData, channelMixMap, collectPoints, &mChannelPoints);
            }

            mSpeakers.SetChannelGains(channelMixMap);
        }
    }

    //==============================================================================
    ChannelPoints SourcePanningVisualizationCallback::AssignGroup(std::span<const Vec3> vectors, int group)
    {
        ChannelPoints points;
        points.reserve(vectors.size());
        for (const Vec3& v : vectors)
            points.emplace_back(v, group);
        return points;
    }

    void SourcePanningVisualizationCallback::operator()(std::span<const Vec3> channelVSs, uint32_t channelID)
    {
        auto p = AssignGroup(channelVSs, channelID);
        Points.insert(Points.end(), p.begin(), p.end());
    }

    void SourcePanningVisualizationCallback::Reset()
    {
        Points.clear();
    }

    //==============================================================================
    void SpeakerVisualization::SetLayout(JPL::ChannelMap targetLayout)
    {
        Reset();

        targetLayout.ForEachChannel([this](JPL::EChannel channel/*, uint32 index*/)
        {
            //if (channel != JPL::EChannel::LFE)
            Points.emplace_back(JPL::VBAPStandartTraits::GetChannelVector(channel), 0.0f);
        });
    }

    void SpeakerVisualization::AddChannelGains(std::span<const float> sourceGains)
    {
        // For each source channel from the mixing map, copy each output channel gains into Points.Intencity
        for (uint32_t sch = 0; sch < sourceGains.size(); sch += Points.size())
        {
            for (uint32_t ch = 0; ch < Points.size(); ++ch)
                Points[ch].Intensity += sourceGains[sch + ch];
        }
    }

    void SpeakerVisualization::SetChannelGains(std::span<const float> sourceGains)
    {
        ResetChannelGains();
        AddChannelGains(sourceGains);
    }

    void SpeakerVisualization::ResetChannelGains()
    {
        for (IntencityPoint& speaker : Points)
        {
            speaker.Intensity = 0.0f;
        }
    }

    void SpeakerVisualization::Reset()
    {
        Points.clear();
    }
} // namespace JPL
