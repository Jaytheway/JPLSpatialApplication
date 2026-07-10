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

#include "AirAbsorptionGUI.h"

#include "ImGui/ImGui.h"
#include "GUI/PropertyWidgets.h"
#include "GUI/Window.h"

#include <JPLSpatial/AirAbsorption.h>
#include <JPLSpatial/Math/SIMD.h>

#include <implot.h>

#include <vector>

namespace JPL::GUI
{
	//==========================================================================
	// TODO: move this somewhere to be reused with BDPTGUI
	namespace Util
	{
		template<uint32 Lane, class Callback>
		ImPlotPoint SimdGetter(int index, void* data)
		{
			const auto [t, value] = std::invoke(*static_cast<const Callback*>(data), index);
			return ImPlotPoint(t, value.get_lane<Lane>());
		}
	} // namespace Util

	//==========================================================================
	struct AirAbsorptionGUI::AirAbsorptionProperties : public GenericChangeBroadcaster
	{
		float AirTemperatureC = 20.0f;
		float RelativeHumidityPercent = 50.0f;
		float AtmosphericPressureKPa = 101.325f;

		inline AirAbsorptionProperties()
		{
			Reset();
		}

		inline void Reset(const AirAbsorptionParams params = {})
		{
			AirTemperatureC = params.AirTemperatureC;
			RelativeHumidityPercent = params.RelativeHumidityPercent;
			AtmosphericPressureKPa = params.AtmosphericPressureKPa;
		}

		inline AirAbsorptionParams ToParams() const
		{
			return AirAbsorptionParams{
				.AirTemperatureC = AirTemperatureC,
				.RelativeHumidityPercent = RelativeHumidityPercent,
				.AtmosphericPressureKPa = AtmosphericPressureKPa
			};
		}
	};

	//==========================================================================
	// TODO: mabye make "snapshot" undo/redo utilities
	class AirAbsorptionGUI::ResetAAPropertiesCommand : public IUndoableCommand
	{
	public:
		ResetAAPropertiesCommand(const std::shared_ptr<AirAbsorptionProperties>& params)
			: mParams(params)
		{
		}

		std::string_view GetName() const final { return "Reset Air Absorption parameters."; }

		void Execute() final
		{
			if (auto params = mParams.lock())
			{
				mSnapshot = params->ToParams();
				params->Reset();
			}
		}

		void Undo() final
		{
			if (auto params = mParams.lock())
				params->Reset(mSnapshot);
		}

	private:
		AirAbsorptionParams mSnapshot;
		std::weak_ptr<AirAbsorptionProperties> mParams;
	};

	//==========================================================================
	AirAbsorptionGUI::AirAbsorptionGUI()
		: mParams(std::make_shared<AirAbsorptionProperties>())
		, mAirAbsorption(std::make_unique<AirAbsorption>(mParams->ToParams()))
		, mAACache(std::make_unique<AirAbsorptionCache>(
			AirAbsorption::CacheParameters(
				cDefaultAirAbsCache.FrequencyBandCenters, mParams->ToParams())))
		, mPTFrequency(std::make_shared<Property<float>>(1000.0f))
		, mPTDistance(std::make_shared<Property<float>>(1.0f))
	{
		mParams->AddListener(this);

		mPTFrequency->AddChangeCallback<&AirAbsorptionGUI::UpdatePureToneInfo>(this);
		mPTDistance->AddChangeCallback<&AirAbsorptionGUI::UpdatePureToneInfo>(this);
	}

	void AirAbsorptionGUI::Draw()
	{
		const ImGuiEx::WindowConfig windowConfig
		{
			.Size = ImVec2(500.0f, 700.0f),
			.SizeCond = ImGuiCond_FirstUseEver,
			.MinSize = ImVec2(500.0f, 400.0f),
			.Flags = ImGuiWindowFlags_NoCollapse
		};

		GUI::Window("Air Absorption", windowConfig, [this]
		{
			using namespace ImGuiEx;

			ImGui::TextDisabled("Currently this window is only for demonstration/diagnostic purposes.\n"
								"The properties set here are NOT YET used for sound propagation.\n");

			Layout<Spacer, Spacer>();

			// TODO: move this to some utility/theme header
			auto drawTitle = [](const char* text)
			{
				ScopedFont font(GUI::GetLightFont(), 20.0f);
				ScopedColour textColour(ImGuiCol_Text, GUI::Colours::Theme::TextDarker);

				ImGui::TextUnformatted(text);

				Layout<Spacer>();
			};

			const ChildConfig config
			{
				.ChildFlags = ImGuiChildFlags_AutoResizeY
			};

			Child("Air Properties", config, [&]
			{
				bool bParamsChanged = false;

				bParamsChanged |= PropertySlider("Temperature", Undoable(mParams, &AirAbsorptionProperties::AirTemperatureC), -273.15f, 1000.0f, SliderConfig{ .Fmt = "%.0f \xC2\xB0", .Flags = ImGuiSliderFlags_AlwaysClamp });
				bParamsChanged |= PropertySlider("Humidity", Undoable(mParams, &AirAbsorptionProperties::RelativeHumidityPercent), 0.0f, 100.0f, SliderConfig{ .Fmt = "%.0f %%", .Flags = ImGuiSliderFlags_AlwaysClamp });
				bParamsChanged |= PropertySlider("Atm. Pressure", Undoable(mParams, &AirAbsorptionProperties::AtmosphericPressureKPa), 1e-17f, 9300.0f, SliderConfig{ .Fmt = "%.0f kPa", .Flags = ImGuiSliderFlags_AlwaysClamp });

				Layout<Spacer>();

				if (ImGuiEx::Button("Reset to Default"))
				{
					JPLSpatialApplication::ExecuteCommand(new ResetAAPropertiesCommand(mParams));
					bParamsChanged = true;
				}

				if (bParamsChanged)
					mParams->BroadcastChange();
			});

			Layout<Spacer, Spacer, Separator, Spacer>();

			Child("Pure-tone Test", config, [&]
			{
				drawTitle("Pure-Tone Attenuation");

				SliderConfig config
				{
					.Fmt = "%.0f Hz",
					.Flags = ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic
				};
				
				PropertySlider("Frequency", Undoable(mPTFrequency), 20.0f, 20000.0f, config);

				config.Fmt = "%.0f m";

				PropertySlider("Distance", Undoable(mPTDistance), 1.0f, 3000.0f, config);

				ImGui::Text("Attenuation of %.0f Hz pure-tone at %.0f m: %.8f dB",
							mPTFrequency->Get(),
							mPTDistance->Get(),
							mAttenuationAtDistance);
			});

			Layout<Spacer, Spacer, Separator, Spacer>();

			Child("Multi-Band Info Table", config, [&]
			{
				drawTitle("Multi-Band Info");
				ScopedFont font(GUI::GetConsoleFont(), 14.0f);

				// TODO: ImGuiEx wrapper for table, maybe use builder pattern
				if (ImGui::BeginTable("MBInfo", 5, ImGuiTableFlags_SizingFixedFit))
				{
					char str[16]{};

					auto getFrequencyStr = [&](float freq)
					{
						std::format_to_n(str, 15, "{:.0f}", freq);
						return str;
					};

					float freq[4]{};
					mAACache->FrequencyBandCenters.store(freq);

					// Setup header row
					ImGui::TableSetupColumn("       FREQUENCY BAND:");
					ImGui::TableSetupColumn(getFrequencyStr(freq[0]), ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn(getFrequencyStr(freq[1]), ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn(getFrequencyStr(freq[2]), ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn(getFrequencyStr(freq[3]), ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableHeadersRow();

					auto drawSIMDRow = [](const char* label, const char* format, const simd& sdata)
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						{
							ScopedColour textColour(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
							ImGui::TextUnformatted(label);
						}

						float data[4]{}; sdata.store(data);

						ImGui::TableNextColumn(); ImGui::Text(format, data[0]);
						ImGui::TableNextColumn(); ImGui::Text(format, data[1]);
						ImGui::TableNextColumn(); ImGui::Text(format, data[2]);
						ImGui::TableNextColumn(); ImGui::Text(format, data[3]);
					};

					drawSIMDRow("     Attenuation dB/m:", "%.5f", mAACache->CenterFrequencyAbsorption_dB);
					drawSIMDRow("HF-weighted Att. dB/m:", "%.5f", mAACache->HighFreqWeightedAbsorption_dB);
					drawSIMDRow("     Decay Slope dB/s:", "%.3f", mAACache->FrequencyDecaySlope);

					ImGui::EndTable();
				}
			});

			Layout<Spacer, Spacer, Separator, Spacer>();

			// Draw deviation plot for air absorption using center frequencies (default)
			// vs air absorption with correction applied (corrected)
			Child("High-Frequency Correction Plot", [&]
			{
				drawTitle("HF Correction Plot");

				static constexpr uint32 numValues = 5'000;
				static std::vector<simd> defaultDbAttenuation;
				static std::vector<simd> correctedDbAttenuation;

				if (bAbsorptionChanged)
				{
					defaultDbAttenuation.clear();
					defaultDbAttenuation.reserve(numValues);
					correctedDbAttenuation.clear();
					correctedDbAttenuation.reserve(numValues);
					for (uint32 m = 1; m <= numValues; ++m)
					{
						const float dist = m;

						defaultDbAttenuation.push_back(
							dist * mAirAbsorption->ComputeAbsorptionPerMeter(mAACache->FrequencyBandCenters));

						correctedDbAttenuation.push_back(AirAbsorption::ComputeForDistance(dist, *mAACache));
					}

					bAbsorptionChanged = false;
				}

				auto getDefaultAA = [&](int index) { return std::pair(index, defaultDbAttenuation[index]); };
				using GetDefaultAACb = decltype(getDefaultAA);

				auto getCorrectedAA = [&](int index) { return std::pair(index, correctedDbAttenuation[index]); };
				using GetCorrectedAACb = decltype(getCorrectedAA);

				ImPlotSpec spec;

				auto drawBandPlot = [&](const char* bandLabel, Colour colour, auto defaultGetter, auto correctedGetter)
				{
					spec.LineColor = colour;
					spec.FillColor = colour.WithAlpha(0.5f);

					ImPlot::PlotShadedG(bandLabel,
										defaultGetter, &getDefaultAA,
										correctedGetter, &getCorrectedAA,
										numValues,
										spec);

					ImPlot::PlotLineG(bandLabel,
									  defaultGetter, &getDefaultAA,
									  numValues,
									  spec);

					ImPlot::PlotLineG(bandLabel,
									  correctedGetter, &getCorrectedAA,
									  numValues,
									  spec);
				};

				if (ImPlot::BeginPlot("Deviation of AA at Center Freq. vs Corrected", ImVec2(-1, -1)))
				{
					const ImPlotAxisFlags flags = 0;

					ImPlot::SetupAxes(nullptr, nullptr, flags, ImPlotAxisFlags_NoTickMarks/* | ImPlotAxisFlags_AutoFit*/);
					ImPlot::SetupLegend(ImPlotLocation_NorthWest);

					ImPlot::SetupAxisFormat(ImAxis_X1, "%g m");	// meters
					ImPlot::SetupAxisFormat(ImAxis_Y1, "%g dB"); // absorption dB

					ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 1.0f, static_cast<float>(numValues));
					ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0f, 200.0f); // 200 dB range should be enough for visualization

					spec.FillAlpha = 0.5f;

					drawBandPlot("Band 1", Colour(168, 50, 60), &Util::SimdGetter<0, GetDefaultAACb>, &Util::SimdGetter<0, GetCorrectedAACb>);
					drawBandPlot("Band 2", Colour(62, 168, 50), &Util::SimdGetter<1, GetDefaultAACb>, &Util::SimdGetter<1, GetCorrectedAACb>);
					drawBandPlot("Band 3", Colour(50, 133, 168), &Util::SimdGetter<2, GetDefaultAACb>, &Util::SimdGetter<2, GetCorrectedAACb>);
					drawBandPlot("Band 4", Colour(144, 50, 168), &Util::SimdGetter<3, GetDefaultAACb>, &Util::SimdGetter<3, GetCorrectedAACb>);

					ImPlot::EndPlot();
				}
			});
		});
	}

	void AirAbsorptionGUI::OnChange(GenericChangeBroadcaster* source)
	{
		if (source == &(*mParams))
		{
			mAirAbsorption.reset(new AirAbsorption(mParams->ToParams()));
			mAACache.reset(new AirAbsorptionCache(AirAbsorption::CacheParameters(cDefaultAirAbsCache.FrequencyBandCenters, mParams->ToParams())));
			bAbsorptionChanged = true;

			UpdatePureToneInfo();
		}
	}

	void AirAbsorptionGUI::UpdatePureToneInfo()
	{
		mAttenuationAtDistance = mAirAbsorption->ComputeAbsorptionPerMeter(mPTFrequency->Get()) * mPTDistance->Get();
	}

} // namespace JPL::GUI
