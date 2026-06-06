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

#include "AudioPlayerGUI.h"

#include "ImGui/ImGui.h"

#include "fonts/FontIcons.h"

namespace JPL
{

	AudioPlayerGUI::AudioPlayerGUI(AudioPlayer& audioPlayer)
		: mAudioPlayer(audioPlayer)
		, mAudioPreview(mWaveformDataSource)
	{
		mAudioPlayer.AddListener(this);
	}

	AudioPlayerGUI::~AudioPlayerGUI()
	{
		mAudioPlayer.RemoveListener(this);
	}

	void AudioPlayerGUI::Draw()
	{
		using namespace JPL::ImGuiEx;

		JPL::ImGuiEx::Child("Audio Player Child", [&]
		{
			const ImVec2 infoPanelSize(ImGui::GetContentRegionAvail().x, 0.0f);
			const float infoPanelAlignment = 0.0f;

			JPL::ImGuiEx::LayoutHorizontal("##InfoAndControlsPanel", infoPanelSize, infoPanelAlignment, [&]
			{
				ImGui::Spacing();

				const IconButtonStyle style
				{
					.ColourNormal = ImGui::GetColorU32(ImGuiCol_Button),
					.ColourHovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered),
					.ColourPressed = ImGui::GetColorU32(ImGuiCol_ButtonActive)
				};

				{
					ScopedFontSize fontSize(25.0f);

					if (ImGuiEx::IconButton((const char*)ICON_jplsa_PLAY, style))
					{
						mAudioPlayer.Play();
					}

					if (ImGuiEx::IconButton((const char*)ICON_jplsa_PAUSE, style))
					{
						mAudioPlayer.Pause();
					}

					if (ImGuiEx::IconButton((const char*)ICON_jplsa_STOP, style))
					{
						mAudioPlayer.Stop();
					}
				}

				ImGui::Spacing();

				// Looping checkbox
				{
					ScopedItemOutline outline((const char*)ICON_jplsa_ARROWS_ROTATE, OutlineFlags_NoOutlineInactive, ImColor(60, 60, 60));

					ShiftCursorY(2.0f);
					bool bLooping = mAudioPlayer.IsLooping();
					if (ImGui::Checkbox("##looping", &bLooping))
					{
						mAudioPlayer.SetLooping(bLooping);

					}
					SetTooltip("Set playback looping");
					{
						ScopedColour iconColour(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
						ScopedFontSize fontSize(20.0f);
						ShiftCursor(-6.0f, 0.0f);
						ImGui::TextUnformatted((const char*)ICON_jplsa_ARROWS_ROTATE);
					}
				}

				ImGui::Spacing();

				// Playback position / duration
				{
					const float totalLength = mAudioPlayer.GetTotalLengthInSeconds();
					if (totalLength > 0.0f)
					{
						const float currentPosition = mAudioPlayer.GetCursorInSeconds();

						ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 6.0f));
						
						ImGui::Text("%.3f s", currentPosition);
						ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(GUI::Colours::Theme::TextDarker), " | ");
						ImGui::Text("%.3f s", totalLength);
					}

				}

				// Dilename
				{
					ImGui::Spring();
					ShiftCursorY(2.0f);

					static std::string filename; // just to have a local buffer and avoid allocations on every frame
					filename = mAudioPlayer.GetFile().filename().string();

					ImGui::Text(filename.c_str());
				}

				ImGui::Dummy(ImGui::GetStyle().ItemSpacing);
			}); // InfoAndControlsPanel

			static ImColor cursorColour = IM_COL32(255, 255, 255, 128);
#if 0 // Enable to play around with playback cursor colour
			JPL::ImGuiEx::Window("Cursor Col", [&]
			{
				ImGui::ColorPicker4("Cursor Colour", &cursorColour.Value.x);
			});
#endif

			JPL::ImGuiEx::Child("Audio Preview", [&]
			{
				// Store current cursor position and bounds available
				const ImVec2 start = ImGui::GetCursorScreenPos();
				const ImVec2 end = start + ImGui::GetContentRegionAvail();

				// Draw audio preview
				const bool wasLeftClicked = mAudioPreview.Draw("Audio Player Audio Preview");

				// Draw playback cursor
				const float playbackCursor = mAudioPlayer.GetCursorPosition();
				if (playbackCursor >= 0.0f)
				{
					auto* drawList = ImGui::GetWindowDrawList();

					const float cursorThickness = 3.0f;
					const ImVec2 min = ImLerp(start, ImVec2(end.x, start.y), playbackCursor) - ImVec2(1.0f, 0.0f);
					const ImVec2 max = ImVec2(min.x + cursorThickness, end.y);

					drawList->AddRectFilled(min, max, cursorColour);
				}

				// If clicked on the audio preview item
				if (wasLeftClicked)
				{
					// Setup click to seek functionality
					const ImVec2 waveformRectSize = end - start;
					const ImVec2 clickedPos = ImGui::GetMousePos() - start;
					const float position = clickedPos.x / waveformRectSize.x;

					mAudioPlayer.SeekToPosition(position);
				}
			}); // Audio Preview
		}); // Audio Player
	}

	void AudioPlayerGUI::OnAudioFileChanged(const std::filesystem::path& file)
	{
		mWaveformDataSource.SetFile(file);
	}
} // namespace JPL
