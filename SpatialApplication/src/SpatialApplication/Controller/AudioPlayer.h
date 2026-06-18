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

#include "CoreInclude.h"

#include "Utility/MVCUtils.h"

#include <MiniaudioCpp/MiniaudioWrappers.h>

#include <filesystem>
#include <functional>

struct ma_engine;
struct ma_sound;

namespace JPL
{
	class AudioPlayer;

	template<>
	class ChangeListener<AudioPlayer>
	{
	public:
		virtual ~ChangeListener() = default;
		virtual void OnAudioFileChanged(const std::filesystem::path& file) {}
	};

	class AudioPlayer : public ChangeBroadcaster<AudioPlayer>
	{
	public:
		AudioPlayer();
		virtual ~AudioPlayer();

		void Play();
		void Stop();
		void Pause();

		void SetFile(const std::filesystem::path& file);
		const std::filesystem::path& GetFile() const { return m_File; }

		MA::Sound& GetSound() { return mSound; }
		const MA::Sound& GetSound() const { return mSound; }

		bool IsLooping() const { return bLooping; }
		void SetLooping(bool shouldLoop);

		/** Get playback cursor position relative to the total length of the source */
		float GetCursorPosition() const;
		void SeekToPosition(float position);

		float GetCursorInSeconds() const;
		float GetTotalLengthInSeconds() const;

		uint32_t GetNumChannels() const;

		static bool IsValidAudioFile(const std::filesystem::path& filepath);

		std::function<void(MA::Sound&)> onSoundCreated = nullptr;

	private:
		uint64_t GetSeekFrame() const;

	private:
		std::filesystem::path m_File;
		MA::Sound mSound;

		bool bLooping = false;
	};
} //namespace JPL
