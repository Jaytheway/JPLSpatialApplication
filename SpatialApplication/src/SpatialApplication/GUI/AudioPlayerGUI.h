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

#pragma once

#include "Controller/AudioPlayer.h"
#include "GUI/AudioPreview.h"
#include "GUI/Waveform/WaveformRendering.h"
#include "GUI/Spectrogram/SpectrogramRendering.h"

#include <filesystem>

namespace JPL
{
	class AudioPlayerGUI : public ChangeListener<AudioPlayer>
	{
	public:
		explicit AudioPlayerGUI(AudioPlayer& audioPlayer);
		~AudioPlayerGUI();

		void Draw();

		// ~ Begin ChangeListener<AudioPlayer> interface
		void OnAudioFileChanged(const std::filesystem::path& file) override;
		// ~ End ChangeListener<AudioPlayer> interface

	private:
		AudioPlayer& mAudioPlayer;
		GUI::AudioFileWaveformDataSource mWaveformDataSource;
		GUI::AudioPreview mAudioPreview;
	};
} // namespace JPL
