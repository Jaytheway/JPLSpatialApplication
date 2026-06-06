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

#pragma once

#include <Walnut/Layer.h>

#include "GUI/RoomView.h"
#include "Model/RoomModel.h"
#include "Model/DirectSoundModel.h"
#include "Model/LateReverbModel.h"
#include "Systems/ERTracing.h"
#include "Utility/MVCUtils.h"
#include "Processing/Panner.h"

#include <JPLSpatial/Auralization/EarlyReflectionsBus.h>

#include <vector>
#include <memory>

namespace JPL
{
	class RoomLayer;

	template<>
	class ChangeListener<RoomLayer>
	{
	public:
		virtual ~ChangeListener() = default;
		virtual void OnTapsUpdated(const std::vector<typename JPL::ERBus::ERUpdateData>& taps) {}
		virtual void OnSourceChanged(const JPL::MinimalVec3& sourcePosition) {}
		virtual void OnRoomSizeChanged(const JPL::MinimalVec3& newRoomSize) {}
		virtual void OnReverbTimeUpdated(const simd& newRT60) {}
	};


	class RoomLayer : public Walnut::Layer
					, public ChangeBroadcaster<RoomLayer>
	{
	public:
		explicit RoomLayer(std::shared_ptr<JPL::DirectSoundModel> directSoundModel);
		~RoomLayer();

		// ~ Begin Walnut::Layer interface
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnUIRender() override;
		// ~ End Walnut::Layer interface

		void SetNumChannelsForERs(uint32_t numChannels);

		RoomModel& GetModel() { return mRoom; }
		void SetSourceSize(float newSize);

		const simd& GetReverbTime() const { return mRT60; }

	private:
		void OnListenerChanged(const typename RoomModel::ListenerData& listener);
		void OnSourceChanged(const typename RoomModel::SourceData& source);
		void OnRoomSizeChanged(const typename RoomModel::RoomSizeData& room);
		void OnSurfaceMaterialChanged(const JPL::AcousticMaterial* newMaterial);
		void UpdateTaps();
		void UpdateReverbTime();
	private:
		RoomModel mRoom;
		RoomView mRoomView{ mRoom };

		std::shared_ptr<JPL::DirectSoundModel> mDirectSoundModel;

		ERTracer mERTracer;
		std::vector<typename JPL::ERBus::ERUpdateData> mTaps;
		std::unique_ptr<JPLPanner> mERPanner{ nullptr };
		simd mRT60{ 1.0f };
	};
} // namespace JPL
