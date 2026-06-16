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

#include "Model/VBAPVisualizationModel.h"
#include "Processing/Panner.h"
#include "Utility/MVCUtils.h"

#include <JPLSpatial/ChannelMap.h>
#include <JPLSpatial/Math/MinimalVec3.h>
#include <JPLSpatial/Math/MinimalBasis.h>

#include <cstdint>
#include <memory>
#include <vector>
#include <span>

namespace JPL
{
	struct Panner2D;
	struct SourceLayout2D;

	using Vec3 = JPL::MinimalVec3;
	struct GroupedPoint { Vec3 Point; int Group; };
	using ChannelPoints = std::vector<GroupedPoint>;
	struct IntencityPoint { Vec3 Direction; float Intensity; };

	struct SourcePanningVisualizationCallback
	{
		static ChannelPoints AssignGroup(std::span<const Vec3> vectors, int group);

		void operator()(std::span<const Vec3> channelVSs, uint32_t channelID);

		void Reset();

		ChannelPoints Points;
	};

	struct SpeakerVisualization
	{
		void SetLayout(JPL::ChannelMap targetLayout);

		// Accumulate channel intencities per output channel
		void AddChannelGains(std::span<const float> sourceGains);

		// Set channel intencities per output channel
		void SetChannelGains(std::span<const float> sourceGains);

		// Reset channel intencities to 0
		void ResetChannelGains();

		void Reset();

		std::vector<IntencityPoint> Points;
	};

	class VBAPVisualization : public JPL::GenericChangeListener
	{
	public:
		explicit VBAPVisualization(const std::shared_ptr<VBAPVisualizationModel>& model);
		~VBAPVisualization();

		void Draw();

	private:
		static void DrawPoints(std::span<const GroupedPoint> points);
		static void DrawSpeakers(std::span<const IntencityPoint> points, uint32_t lfeIndex);
		static void DrawPointsTransformed(std::span<const GroupedPoint> points, const JPL::Basis<JPL::MinimalVec3>& rotation);
		static void DrawSpeakersTransformed(std::span<const IntencityPoint> points, const JPL::Basis<JPL::MinimalVec3>& rotation);
		static void DrawPoint(const GroupedPoint& point, float brightnessMultiplier = 1.0f);
		static void DrawSpeaker(const IntencityPoint& point, float brightnessMultiplier = 1.0f);

		virtual void OnChange(JPL::GenericChangeBroadcaster*) final;

		void OnTargetChannelsChanged(JPL::NamedChannelMask);
		void OnSourceChannelsChanged(JPL::NamedChannelMask);

		void UpdatePoints();

		void SetVBAPModel(std::shared_ptr<JPL::VBAPModel> newVBAPModel);

	private:
		std::shared_ptr<VBAPVisualizationModel> mModel;

		std::unique_ptr<JPLPanner> mPanner;
		JPLSourceLayoutHandle mSourceLayout;

		JPL::NamedChannelMask mSourceChannelSet;
		JPL::NamedChannelMask mTargetChannelSet;

		SourcePanningVisualizationCallback mChannelPoints;
		SpeakerVisualization mSpeakers;
		int mLFEIndex = -1;
	};
} // namespace JPL
