//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright 2026 Jaroslav Pevno, JPL Spatial Application is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "SpeakerPolarPattern.h"

#include "Processing/Panner.h"

#include <JPLSpatial/Math/Math.h>
#include <JPLSpatial/Math/MinimalVec3.h>
#include <JPLSpatial/Math/MinimalVec2.h>
#include <JPLSpatial/Panning/PannerBase.h>
#include <JPLSpatial/Containers/StaticArray.h>

#include "ImGui/ImGui.h"

#include <array>
#include <vector>
#include <utility>

namespace JPL::GUI
{
	// TODO: move this somewhere to reuse with VBAPVisualization
	static constexpr ImU32 cChannelColour[]
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

		// In case more channels used
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80),
		IM_COL32(255, 255, 255, 80)
	};

	//==========================================================================
	class SpeakerPolarPattern::Pimpl
	{
	public:
		using Vec3 = MinimalVec3;

		static constexpr std::size_t cMaxOutChannels = VBAPBaseTraits<Vec3>::MAX_CHANNELS;
		static constexpr float cAngleStep = Math::ToRadians(1.0f);

		// TODO: make constexpr when constexpr SinCos is available
		inline static const std::array<Vec2, 360> cDirections = []
		{
			std::array<Vec2, 360> dirs;
			for (uint32 i = 0; i < dirs.size(); ++i)
			{
				const auto [s, c] = Math::SinCos(i * cAngleStep);
				dirs[i] = Vec2(s, -c);
			}
			return dirs;
		}();

		struct ChannelPlot
		{
			std::vector<ImVec2> Points;
			ImU32 Col = IM_COL32(255, 255, 255, 40);
		};

		using GainsArray = StaticArray<float, cMaxOutChannels>;

	public:

		void Draw(const JPLPanner& panner)
		{
			// TODO(?): params to visualize { panning strength (nominal-pan-blend / omni-blend) }

			const ChannelMap newChannelMap = panner.GetChannelMap();
			if (not JPL_ENSURE(newChannelMap.GetNumGroundChannels() > 1))
				return; // shouldn't happen

			const bool bChannelMapChanged = newChannelMap != mChannelMap;
			if (bChannelMapChanged)
				UpdateGains(panner);

			const auto [newCentre, newRadius] = GetCentreAndRadius();
			const bool bCanvasChanged = newCentre != mCentre or newRadius != mRadius;

			if (bChannelMapChanged or bCanvasChanged)
				UpdatePlot(newCentre, newRadius);

			mChannelMap = newChannelMap;
			mCentre = newCentre;
			mRadius = newRadius;

			DrawChannelPlot(mChannels);
		}

		void SetStyle(const Style& newStyle)
		{
			mStyle = newStyle;
			for (uint32 ch = 0; ch < mChannels.size(); ++ch)
				mChannels[ch].Col = mStyle.Colour != 0 ? mStyle.Colour : cChannelColour[ch];
		}

	private:
		void UpdateGains(const JPLPanner& panner)
		{
			// We only visualize ground channels,
			// since we sweep the ground plane circle
			const uint32 numGroundChannels = panner.GetChannelMap().GetNumGroundChannels();
			mChannels.resize(numGroundChannels);

			const uint32 numChannels = panner.GetNumChannels();
			StaticArray<float, cMaxOutChannels> panGains(numChannels, 0.0f);

			for (uint32 i = 0; i < cDirections.size(); ++i)
			{
				panner.GetSpeakerGains(Vec3(cDirections[i].X, 0.0f, cDirections[i].Y), panGains);
				
				mPanGains[i].resize(numGroundChannels);
				std::ranges::copy(panGains | std::views::take(numGroundChannels), mPanGains[i].begin());
			}

			// Update colours
			SetStyle(mStyle);
		}

		void UpdatePlot(ImVec2 centre, float radius)
		{
			if (not JPL_ENSURE(mPanGains.size() == cDirections.size()))
				return;

			for (auto& chp : mChannels)
			{
				chp.Points.clear();
				chp.Points.reserve(360);
			}

			for (uint32 i = 0; i < cDirections.size(); ++i)
			{
				// Scale 2D direction vector by the mix level
				const Vec2 vector = cDirections[i] * radius;
				for (uint32 ch = 0; ch < mPanGains[i].size(); ++ch)
					mChannels[ch].Points.push_back(ImVec2(vector * mPanGains[i][ch]) + centre);
			}
		}

		static std::pair<ImVec2, float> GetCentreAndRadius()
		{
			const ImRect bb(ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImGui::GetContentRegionAvail());
			const ImVec2 centre = bb.GetCenter();
			const float radius = std::min(bb.GetWidth(), bb.GetHeight()) * 0.5f;
			return { centre, radius };
		}

		static void DrawChannelPlot(const std::vector<ChannelPlot>& channels)
		{
			auto* drawList = ImGui::GetWindowDrawList();
			for (const auto& channel : channels)
			{
				// Simple outline looks cleaner than filled polygon
				drawList->PathClear();
				for (const ImVec2& p : channel.Points)
					drawList->PathLineTo(p);

				static constexpr float cLineThickness = 2.0f;
				drawList->PathStroke(channel.Col, ImDrawFlags_Closed, cLineThickness);
			}
		}

	private:
		Style mStyle;
		ChannelMap mChannelMap = ChannelMap::FromChannelMask(ChannelMask::Invalid);
		std::vector<ChannelPlot> mChannels;
		std::vector<GainsArray> mPanGains{ cDirections.size() };

		ImVec2 mCentre{ -1.0f, -1.0f };
		float mRadius{ -1.0f };
	};

	//==========================================================================
	SpeakerPolarPattern::SpeakerPolarPattern(const Style& style)
	{
		mPimpl = std::make_unique<Pimpl>();
		mPimpl->SetStyle(style);
	}

	SpeakerPolarPattern::~SpeakerPolarPattern() = default;

	void SpeakerPolarPattern::Draw(const std::weak_ptr<JPLPanner>& pannerWeak)
	{
		if (auto panner = pannerWeak.lock())
			mPimpl->Draw(*panner);
	}

	void SpeakerPolarPattern::SetStyle(const Style& newStyle)
	{
		mPimpl->SetStyle(newStyle);
	}

} // namespace JPL::GUI
