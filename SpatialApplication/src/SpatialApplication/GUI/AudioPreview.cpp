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

#include "AudioPreview.h"
#include "Application.h"

#include "fonts/FontIcons.h"

namespace JPL::GUI
{
	AudioPreview::AudioPreview(WaveformDataSource& dataSource, EAudioPreviewMode mode)
		: mDataSource(dataSource)
		, mWaveform(dataSource)
		, mSpectrogram(dataSource)
		, mMode(std::make_shared<EAudioPreviewMode>(mode))
	{
	}

	bool AudioPreview::Draw(const char* itemIdStr)
	{
		// Store current cursor position and bounds available
		const ImVec2 start = ImGui::GetCursorScreenPos();
		const ImVec2 end = start + ImGui::GetContentRegionAvail();

		const ImGuiID itemID = ImGui::GetID(itemIdStr);

		const char* popupIDStr = "Audio Preview Popup";
		const ImGuiID popupID = ImGui::GetID(popupIDStr);

		const ImGuiPopupFlags popupFlags = 0;

		ImGuiEx::ScopedGroup group(itemID);

		switch (*mMode)
		{
		case EAudioPreviewMode::Waveform:
		{
			if (mWaveform.Draw(itemIdStr))
			{
				if (ImGui::BeginPopup(popupIDStr, popupFlags))
				{
					if (ImGui::Button("Display Spectrogram"))
						SetMode(EAudioPreviewMode::Spectrogram);

					//ImGui::Separator();

					//! currently waveform doesn't have any properties
					//mWaveform.DrawProperties();

					ImGui::EndPopup();
				}
			}
		}
		break;
		case EAudioPreviewMode::Spectrogram:
		{
			if (mSpectrogram.Draw(itemIdStr))
			{
				if (ImGui::BeginPopup(popupIDStr, popupFlags))
				{
					if (ImGui::Button("Display Waveform"))
						SetMode(EAudioPreviewMode::Waveform);

					ImGui::Separator();

					mSpectrogram.DrawProperties();

					ImGui::EndPopup();
				}
			}
		}
		break;
		default:
			return false;
		}

		// Handle right-click and settings button for widget settings popup
		const ImGuiID hoveredID = ImGui::GetHoveredID();
		const ImGuiID settingsButtonID = ImGui::GetID((const char*)(ICON_jplsa_GEAR));

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			ImGui::OpenPopupEx(popupID, popupFlags);

		bool bWasLeftClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

		// Draw settings button if preview widget is hovered, or if settings popup is already open
		if (ImGui::IsItemHovered() or hoveredID == settingsButtonID or ImGui::IsPopupOpen(popupID, popupFlags))
		{
			ImGui::SetCursorScreenPos({ end.x - 35.0f, start.y + 5.0f });

			const Colour colourH(GUI::Colours::Theme::Text);
			const Colour colourN = colourH.WithMultipliedValue(0.8f);
			const Colour colourP = colourH.WithMultipliedValue(1.2f);
			const ImGuiEx::IconButtonStyle style
			{
				.ColourNormal = colourN,
				.ColourHovered = colourH,
				.ColourPressed = colourP
			};

			ImGui::SetNextItemAllowOverlap();

			if (ImGuiEx::IconButton(ICON_jplsa_GEAR, ImGuiEx::IconStyle::cIconLabelBgHovered))
			{
				ImGui::OpenPopupEx(popupID, popupFlags);
			}

			// This seems to be the only way to prevent click on settings button
			// from propagating outside as click on the main widget
			if (ImGui::IsItemClicked())
				bWasLeftClicked = false;
		}

		return bWasLeftClicked;
	}

	void AudioPreview::SetMode(EAudioPreviewMode newMode)
	{
		if (*mMode != newMode)
		{
			const EAudioPreviewMode oldValue = *mMode;
			*mMode = newMode;

			JPLSpatialApplication::GetCommandHistory()
				.PropertyEdited(Undoable(mMode), OldValue(oldValue), "Audio Preview Mode");
		}
	}

} // namespace JPL::GUI
