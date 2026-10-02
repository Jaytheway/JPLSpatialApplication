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
#include "ImGui/ImGui.h"

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
					if (ImGuiEx::Button("Display Spectrogram"))
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
					if (ImGuiEx::Button("Display Waveform"))
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
	
		bool bWasLeftClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

		// This seems to be the only way to prevent click on settings button
		// from propagating outside as click on the main widget
		if (ImGuiEx::SettingsButtonOnHover(popupID, start, end, /* trigger on item right click */ true, popupFlags))
			bWasLeftClicked = false;

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
