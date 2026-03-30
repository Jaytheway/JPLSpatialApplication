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

#include "RoomLayer.h"

#include "Utility/PerformanceMetering.h"
#include "ImGui/ImGui.h"

#include <JPLSpatial/ChannelMap.h>

#include <format>

namespace JPL
{
	struct RayTracinMetering
	{
		static inline const char* Label = "Ray Tracing";
	};
	using PerfMeterRayTracing = PerformanceMetering<RayTracinMetering>;

	RoomLayer::RoomLayer(std::shared_ptr<JPL::DirectSoundModel> directSoundModel)
		: mDirectSoundModel(directSoundModel)
	{
		JPL_ASSERT(directSoundModel);
		mRoom.DirectSound = mDirectSoundModel;
	}

	RoomLayer::~RoomLayer() = default;

	void RoomLayer::OnAttach()
	{
		mRoom.Listener.AddChangeCallback<&RoomLayer::OnListenerChanged>(this);
		mRoom.Source.AddChangeCallback<&RoomLayer::OnSourceChanged>(this);
		mRoom.RoomSize.AddChangeCallback<&RoomLayer::OnRoomSizeChanged>(this);
		mRoom.SurfaceMaterial.AddChangeCallback<&RoomLayer::OnSurfaceMaterialChanged>(this);

		JPL::AddChangeListener<&RoomLayer::UpdateTaps>(*this,
													   mRoom.EnableSpecular,
													   mRoom.EnableDirect,
													   mRoom.NumPrimaryRays,
													   mRoom.MaxOrder);

		mERPanner = std::make_unique <JPLPanner>();
		mERPanner->Initialize(JPL::ChannelMap::FromNumChannels(2));

		JPL_ASSERT(mRoom.SurfaceMaterial.Get() != nullptr);
		mERTracer.SetSurfaceMaterial(*mRoom.SurfaceMaterial.Get());
		mERTracer.OnListenerChanged(mRoom.GetListenerAbsPosition());
		mERTracer.OnSourceChanged(mRoom.GetSourceAbsPosition());
		mERTracer.OnRoomSizeChanged(mRoom.RoomSize.Get().Size);

		UpdateTaps();
		Broadcast<&ChangeListenerType::OnRoomSizeChanged>(mRoom.RoomSize.Get().Size);
		Broadcast<&ChangeListenerType::OnSourceChanged>(mRoom.GetSourceAbsPosition() - mRoom.GetListenerAbsPosition());
	}

	void RoomLayer::OnDetach()
	{
		mERPanner = nullptr;
	}

	void RoomLayer::OnUpdate(float ts)
	{
	}

	void RoomLayer::OnUIRender()
	{
		const JPL::ImGuiEx::WindowConfig config
		{
			.Size = ImVec2(600.0f, 600.0f),
			.MinSize = ImVec2(600.0f, 600.0f),
			.Flags = ImGuiWindowFlags_NoCollapse,
			.DockFlags = ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther
		};


		JPL::ImGuiEx::Window("RoomWindow", [&]
		{
			mRoomView.DrawProperties();

			ImGui::Spacing();
			ImGui::Spacing();
			
			const ImVec2 roomCanvasPosition = ImGui::GetCursorScreenPos();
			mRoomView.DrawEnvironment();
			
			// Draw info text
			char numERLabelText[64]{};
			std::format_to_n(numERLabelText, 64, "Specular Reflections Count: {}", mTaps.size());
			
			auto* fgDrawList = ImGui::GetForegroundDrawList();
			const ImU32 textColour = IM_COL32(255, 255, 255, 60);
			fgDrawList->AddText(roomCanvasPosition + ImGui::GetStyle().ItemSpacing, textColour, numERLabelText);

		}, nullptr, config);

	}

	void RoomLayer::SetNumChannelsForERs(uint32_t numChannels)
	{
		JPL_ASSERT(mERPanner);
		if (mERPanner->GetNumChannels() != numChannels)
		{
			mERPanner->Initialize(JPL::ChannelMap::FromNumChannels(numChannels));

			// Number of channels changed, we need to update taps (mainly panning)
			UpdateTaps();
		}
	}

	void RoomLayer::SetSourceSize(float newSize)
	{
		mRoomView.SetSourceSize(newSize);
	}

	void RoomLayer::OnListenerChanged(const typename RoomModel::ListenerData& /*listener*/)
	{
		mERTracer.OnListenerChanged(mRoom.GetListenerAbsPosition());
		UpdateTaps();

		const auto sourcePosition = (mRoom.GetSourceAbsPosition() - mRoom.GetListenerAbsPosition());
		Broadcast<&ChangeListenerType::OnSourceChanged>(sourcePosition);
	}

	void RoomLayer::OnSourceChanged(const typename RoomModel::SourceData& /*source*/)
	{
		mERTracer.OnSourceChanged(mRoom.GetSourceAbsPosition());
		UpdateTaps();

		const auto sourcePosition = (mRoom.GetSourceAbsPosition() - mRoom.GetListenerAbsPosition());
		Broadcast<&ChangeListenerType::OnSourceChanged>(sourcePosition);
	}

	void RoomLayer::OnRoomSizeChanged(const typename RoomModel::RoomSizeData& room)
	{
		mERTracer.OnListenerChanged(mRoom.GetListenerAbsPosition());
		mERTracer.OnSourceChanged(mRoom.GetSourceAbsPosition());
		mERTracer.OnRoomSizeChanged(room.Size);
		UpdateTaps();

		Broadcast<&ChangeListenerType::OnRoomSizeChanged>(room.Size);
	}

	void RoomLayer::OnSurfaceMaterialChanged(const JPL::AcousticMaterial* newMaterial)
	{
		mERTracer.SetSurfaceMaterial(*newMaterial);
		UpdateTaps();
	}

	void RoomLayer::UpdateTaps()
	{
		mTaps.clear();

		// Trace specular reflections
		{
			auto timer = PerfMeterRayTracing::MakeScopedTimer();
			mERTracer.ClearCache();
			mERTracer.Trace(mRoom.NumPrimaryRays.Get(), mRoom.MaxOrder.Get());
		}

		const uint32_t numChannels = mERPanner->GetNumChannels();

		if (mRoom.EnableSpecular.Get())
		{
			const auto& cache = mERTracer.GetCache();

			const auto sourcePosition = mRoom.GetSourceAbsPosition();

			for (auto&& [pathId, path] : cache.GetValidPaths())
			{
				auto& newTap = mTaps.emplace_back();

				// WHen we do backtracing the last IS is the listerer to the source, so we flip the direction
				const MinimalVec3 imageSource = sourcePosition - path.ImageSource;

				const float pathLength = Length(imageSource);
				const float invDistance = 1.0f / pathLength;

				const float pathTime = pathLength * JPL::JPL_INV_SPEAD_OF_SOUND;
				const auto direction = imageSource * invDistance;

				newTap.Gains.resize(numChannels);
				mERPanner->GetSpeakerGains(direction, newTap.Gains);
				for (float& g : newTap.Gains) // apply distance attenuation
					g *= invDistance;

				newTap.FilterGains = dBToGain(-path.Energy);
				newTap.Delay = pathTime;
				newTap.Id = pathId.Id;
			}
		}

		Broadcast<&ChangeListenerType::OnTapsUpdated>(mTaps);
	}
} // namespace JPL

