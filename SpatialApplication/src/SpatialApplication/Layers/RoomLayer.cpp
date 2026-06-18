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

#include "Config.h"

#include "Utility/PerformanceMetering.h"
#include "ImGui/ImGui.h"
#include "GUI/PropertyWidgets.h"

#include <JPLSpatial/AirAbsorption.h>
#include <JPLSpatial/Auralization/ReverbUtilities.h>
#include <JPLSpatial/ChannelMap.h>
#include <JPLSpatial/Math/SIMD.h>

#include <implot.h>

#include <algorithm>
#include <format>

// TODO: move this somewhere reasonable and reuse here and in BDPTGUI.cpp
template <>
struct std::formatter<JPL::simd>
{
	std::formatter<float, char> FloatFormatter;

	constexpr auto parse(std::format_parse_context& ctx)
	{
		return FloatFormatter.parse(ctx);
	}

	auto format(const JPL::simd& v, std::format_context& ctx) const
	{
		float data[4];
		v.store(data);

		auto out = FloatFormatter.format(data[0], ctx);

		out = std::format_to(out, ", ");
		ctx.advance_to(out);

		out = FloatFormatter.format(data[1], ctx);

		out = std::format_to(out, ", ");
		ctx.advance_to(out);

		out = FloatFormatter.format(data[2], ctx);

		out = std::format_to(out, ", ");
		ctx.advance_to(out);

		return FloatFormatter.format(data[3], ctx);
	}
};

namespace JPL
{
	struct RayTracinMetering
	{
		static inline const char* Label = "Ray Tracing";
	};
	using PerfMeterRayTracing = PerformanceMetering<RayTracinMetering>;


	//==========================================================================
	RoomLayer::RoomLayer(const std::shared_ptr<DirectSoundModel>& directSoundModel,
						 const std::shared_ptr<LateReverbModel>& lateReverbModel,
						 const std::shared_ptr<VBAPVisualization>& vbapVisualization)
		: mRoomView(mRoom, vbapVisualization)
		, mDirectSoundModel(directSoundModel)
		, mLateReverbModel(lateReverbModel)
		, mLateReverbGUI(mLateReverbModel)
	{
		JPL_ASSERT(directSoundModel);
		mRoom->DirectSound = mDirectSoundModel;

		mLinkMaterialEditBands = std::make_shared<bool>(false);

		// Initialize default custom material
		mCustomMaterialAbsorption =
			std::make_shared<Property<AbsorptionCoeffs>>(AbsorptionCoeffs(0.5f));

		mCustomMaterialAbsorption->AddChangeCallback(this, [](RoomLayer* self, const AbsorptionCoeffs& newCoeffs)
		{
			AcousticMaterial::SetMaterial("< CUSTOM >", newCoeffs);

			// mRoom->SurfaceMaterial holds pointer to AcousticMaterial("< CUSTOM >")
			self->mRoom->SurfaceMaterial.BroadcastUpdate();
		});
		
		// Set initial value
		AcousticMaterial::SetMaterial("< CUSTOM >", mCustomMaterialAbsorption->Get());
	}

	RoomLayer::~RoomLayer() = default;

	void RoomLayer::OnAttach()
	{
		mRoom->ListenerPosition.AddChangeCallback<&RoomLayer::OnListenerChanged>(this);
		mRoom->SourcePosition.AddChangeCallback<&RoomLayer::OnSourceChanged>(this);
		mRoom->RoomSize.AddChangeCallback<&RoomLayer::OnRoomSizeChanged>(this);
		mRoom->SurfaceMaterial.AddChangeCallback<&RoomLayer::OnSurfaceMaterialChanged>(this);

		AddChangeListener<&RoomLayer::UpdateTaps>(*this,
												  mRoom->EnableSpecular,
												  mRoom->EnableDirect,
												  mRoom->NumPrimaryRays,
												  mRoom->MaxOrder);
		mERPanner = std::make_unique <JPLPanner>();
		mERPanner->Initialize(ChannelMap::FromNumChannels(2));

		JPL_ASSERT(mRoom->SurfaceMaterial.Get() != nullptr);
		mERTracer.SetSurfaceMaterial(*mRoom->SurfaceMaterial.Get());
		mERTracer.OnListenerChanged(mRoom->GetListenerAbsPosition());
		mERTracer.OnSourceChanged(mRoom->GetSourceAbsPosition());
		mERTracer.OnRoomSizeChanged(mRoom->RoomSize.Get());
		UpdateTaps();
		UpdateReverbTime();
		Broadcast<&ChangeListenerType::OnRoomSizeChanged>(mRoom->RoomSize.Get());
		Broadcast<&ChangeListenerType::OnSourceChanged>(mRoom->GetSourceAbsPosition() - mRoom->GetListenerAbsPosition());
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
		using namespace JPL::ImGuiEx;

		const WindowConfig config
		{
			.Size = ImVec2(600.0f, 600.0f),
			.MinSize = ImVec2(600.0f, 600.0f),
			.Flags = ImGuiWindowFlags_NoCollapse,
			.DockFlags = ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther
		};

		Window("RoomWindow", [&]
		{
			Child("Properties", ChildConfig{ .Size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 6.0f) }, [&]
			{
				// Draw Room properties first
				mRoomView.DrawProperties();

				// Then draw our various environment/propagation properties
				ImGui::SameLine();

				auto drawPropsMaterial = [&]
				{
					const AcousticMaterial* selectedMaterial = mRoom->SurfaceMaterial.Get();

					uint32 selectedMaterialId = selectedMaterial->ID;
					const AcousticMaterial* customMaterial = AcousticMaterial::Get("< CUSTOM >");
					JPL_ASSERT(customMaterial);

					ScopedItemWidth width(210.0f);
					{
						ScopedItemOutline outline("Surface Material");

						std::string_view currentMaterial = selectedMaterial->Name;

						// TODO: maybe implement GUI::PropertyCombo with item callback
						if (ImGui::BeginCombo("Surface Material", currentMaterial.data()))
						{
							const auto& acousticMaterials = AcousticMaterial::GetListOfMaterials();

							for (const auto& [id, material] : acousticMaterials)
							{
								bool bSelected = id == selectedMaterialId;
								if (ImGui::Selectable(material.Name.data(), &bSelected))
								{
									// Selectable triggers even when clicked on already selected item
									if (id != selectedMaterialId)
									{
										selectedMaterialId = id;

										mRoom->SurfaceMaterial.Set(&material);

										// Add material change to Undo history
										JPLSpatialApplication::GetCommandHistory()
											.PropertyEdited(Undoable(mRoom, &RoomModel::SurfaceMaterial),
															OldValue(selectedMaterial),
															"Surface Material");
									}
								}
							}
							ImGui::EndCombo();
						}
						ImGuiEx::SetTooltip(currentMaterial);
					}

					// Selected material properties
					if (selectedMaterial)
					{
						static constexpr float minAbsorption = 0.01f;
						static constexpr float maxAbsorption = 0.99f;

						float absorptionBands[4]{}; selectedMaterial->Coeffs.store(absorptionBands);
						float bandCenters[4]{}; cBandCenters.store(bandCenters);

						const bool bSelectedCustomMaterial = selectedMaterial == customMaterial;

						// We don't want to modify default materials
						ScopedDisable disable(not bSelectedCustomMaterial);

						const uint32 modifiedBand = ImGuiEx::DrawGEQ("Absorption",
																	 absorptionBands, bandCenters,
																	 minAbsorption, maxAbsorption,
																	 ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 2.0f));

						// We need to capture these right after our GEQ,
						// other widgets may override the flags
						const bool bActivated = ImGui::IsItemActivated();
						const bool bDeactivated = ImGui::IsItemDeactivated();

						if (bSelectedCustomMaterial)
						{
							ImGui::SameLine();
							GUI::PropertyCheckbox("Link", Undoable(mLinkMaterialEditBands));

							auto& history = JPLSpatialApplication::GetCommandHistory();

							// Begin property gesture edit before modifying property
							if (bActivated)
								history.BeginPropertyEdit(Undoable(mCustomMaterialAbsorption), "Custom Material Absorption");

							if (modifiedBand)
							{
								const simd modifiedAbsorption = *mLinkMaterialEditBands
									? simd(absorptionBands[modifiedBand - 1])
									: simd(absorptionBands);

								// This is going to modify AcousticMaterial("< CUSTOM >")
								mCustomMaterialAbsorption->Set(modifiedAbsorption);
							}

							// End property edit gesture after modifying property the last time
							if (bDeactivated)
								history.EndPropertyEdit(Undoable(mCustomMaterialAbsorption));
						}
					}
				};

				auto drawPropsER = [&]
				{
					GUI::PropertyCheckbox("Enable Specular Reflections", Undoable(mRoom, &RoomModel::EnableSpecular));

					ImGui::Spacing();

					ScopedDisable disable(not mRoom->EnableSpecular.Get());
					ScopedItemWidth width(210.0f);

					GUI::PropertyInput("Num Prim. Rays", Undoable(mRoom, &RoomModel::NumPrimaryRays), InputConfig<uint32>{.Step = 1, .StepFast = 100, .Fmt = "%d" });
					GUI::PropertyInput("Max Spec. Order", Undoable(mRoom, &RoomModel::MaxOrder), InputConfig<uint32>{.Step = 1, .StepFast = 100, .Fmt = "%d", .Max = uint32(SpecularRayTracing::cMaxOrder) });
				};

				auto drawPropsDirectSound = [&]
				{
					GUI::PropertyCheckbox("Enable Direct Sound", Undoable(mRoom, &RoomModel::EnableDirect));

					ImGui::Spacing();

					ScopedDisable disable(not mRoom->EnableDirect.Get());

					GUI::PropertyCheckbox("Air Absorption", Undoable(mRoom->DirectSound, &DirectSoundModel::EnableAirAbsorption)); ImGui::SameLine();
					GUI::PropertyCheckbox("Distance Attenuation", Undoable(mRoom->DirectSound, &DirectSoundModel::EnableDistanceAttenuation));
					GUI::PropertyCheckbox("Propagaion Delay", Undoable(mRoom->DirectSound, &DirectSoundModel::EnablePropagationDelay));
				};

				auto drawPropsLateReverb = [&]
				{
					// TODO: mixing mode selection (?)
					mLateReverbGUI.Draw();
				};
				
				const auto autoResizeY = ImGuiChildFlags_AutoResizeY;

				Child("Props", ChildConfig{ .ChildFlags = autoResizeY }, [&]
				{
					ImGuiEx::TabBar("Properties", [&]
					{
						ImGuiEx::TabItem("Early Reflections", drawPropsER);
						ImGuiEx::TabItem("Direct Sound", drawPropsDirectSound);
						ImGuiEx::TabItem("Surface Material", drawPropsMaterial);
						ImGuiEx::TabItem("Late Reverb", drawPropsLateReverb);
					});

					// We want LateReverbGUI to retain the IR preview window,
					// if it was requested, even if LateReverbGUI properties
					// are not drawn.
					// (separating the two draw functions like this is a bit of a hack, but works)	
					mLateReverbGUI.DrawPreview();
				});
			});

			Layout<Spacer>();
			{
				ImRect rect = GetItemRect();
				const ImVec2 bbMin = rect.GetBL();
				const ImVec2 bbMax = rect.GetBR() + ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 4);
				DrawTopShadowEdge(*ImGui::GetWindowDrawList(), bbMin, bbMax,
								  ShadowEdgeStyle{
									  .ShadowSize = 14.0f,
									  .FalloffPower = 3.0f,
									  .Colour = IM_COL32(0, 0, 0, 70) });
			}
			
			const ImVec2 roomCanvasPosition = ImGui::GetCursorScreenPos();

			// Draw info text over the Room View
			if (ImDrawList* canvasDrawList = mRoomView.DrawEnvironment())
			{
				// Draw info text
				char numERLabelText[64]{};
				std::format_to_n(numERLabelText, 64, "Specular Reflections Count: {}", mTaps.size());

				const ImU32 textColour = IM_COL32(255, 255, 255, 60);
				canvasDrawList->AddText(roomCanvasPosition + ImGui::GetStyle().ItemSpacing, textColour, numERLabelText);
			}

		}, nullptr, config);
		{
			{
				{

#endif

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

	void RoomLayer::OnListenerChanged(const MinimalVec3& /*listenerPosition*/)
	{
		mERTracer.OnListenerChanged(mRoom->GetListenerAbsPosition());
		UpdateTaps();
		UpdateReverbTime();

		const auto sourcePosition = (mRoom->GetSourceAbsPosition() - mRoom->GetListenerAbsPosition());
		Broadcast<&ChangeListenerType::OnSourceChanged>(sourcePosition);
	}

	void RoomLayer::OnSourceChanged(const MinimalVec3& /*sourcePosition*/)
	{
		mERTracer.OnSourceChanged(mRoom->GetSourceAbsPosition());
		UpdateTaps();
		UpdateReverbTime();

		const auto sourcePosition = (mRoom->GetSourceAbsPosition() - mRoom->GetListenerAbsPosition());
		Broadcast<&ChangeListenerType::OnSourceChanged>(sourcePosition);
	}

	void RoomLayer::OnRoomSizeChanged(const MinimalVec3& roomSize)
	{
		mERTracer.OnListenerChanged(mRoom->GetListenerAbsPosition());
		mERTracer.OnSourceChanged(mRoom->GetSourceAbsPosition());
		mERTracer.OnRoomSizeChanged(roomSize);
		UpdateTaps();
		UpdateReverbTime();

		Broadcast<&ChangeListenerType::OnRoomSizeChanged>(roomSize);
	}

	void RoomLayer::OnSurfaceMaterialChanged(const JPL::AcousticMaterial* newMaterial)
	{
		mERTracer.SetSurfaceMaterial(*newMaterial);
		UpdateTaps();
		UpdateReverbTime();
	}

	void RoomLayer::UpdateTaps()
	{
		mTaps.clear();

		// Trace specular reflections
		{
			auto timer = PerfMeterRayTracing::MakeScopedTimer();
			mERTracer.ClearCache();
			mERTracer.Trace(mRoom->NumPrimaryRays.Get(), mRoom->MaxOrder.Get());
		}

		const uint32_t numChannels = mERPanner->GetNumChannels();

		if (mRoom->EnableSpecular.Get())
		{
			const auto& cache = mERTracer.GetCache();

			const auto listenerPosition = mRoom->GetListenerAbsPosition();

			for (auto&& [pathId, path] : cache.GetValidPaths())
			{
				auto& newTap = mTaps.emplace_back();

				const MinimalVec3 imageSource = path.ImageSource - listenerPosition;

				const float pathLength = Length(imageSource);
				const float invDistance = 1.0f / pathLength;

				const float pathTime = pathLength * JPL::JPL_INV_SPEAD_OF_SOUND;
				const auto direction = imageSource * invDistance;

				newTap.Gains.resize(numChannels);
				mERPanner->GetSpeakerGains(direction, newTap.Gains);
				for (float& g : newTap.Gains) // apply distance attenuation
					g *= invDistance;

				// Energy loss due to air absorption depends on image source position
				// relative to listener position, therefore cannot be part of the path
				// like energy loss due to material absorption.
				const simd energyLoss = path.Energy + AirAbsorption::ComputeForDistance(pathLength, cDefaultAirAbsCache);
				// TODO: get environment properties from the user

				newTap.FilterGains = dBToGain(-energyLoss);
				newTap.Delay = pathTime;
				newTap.Id = pathId.Id;
			}
		}

		Broadcast<&ChangeListenerType::OnTapsUpdated>(mTaps);
	}

	void RoomLayer::UpdateReverbTime()
	{

		// Estimate reverberation time using Eyring equation
		const AcousticMaterial* surfaceMaterial = mRoom->SurfaceMaterial.Get();
		if (JPL_ENSURE(surfaceMaterial))
		{
			const MinimalVec3 roomSize = mRoom->RoomSize.Get();

			mRT60 = EstimateRT60_Eyring(roomSize.X, roomSize.Z, roomSize.Y,
										surfaceMaterial->Coeffs,
										cDefaultAirAbsCache.HighFreqWeightedAbsorption_dB);

			Broadcast<&ChangeListenerType::OnReverbTimeUpdated>(mRT60);
		}
	}

} // namespace JPL

