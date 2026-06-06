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

#include "AudioPlayer.h"

#include <algorithm>
#include <ranges>
#include <string_view>

namespace JPL
{

	AudioPlayer::AudioPlayer()
	{
	}

	AudioPlayer::~AudioPlayer()
	{
	}

	void AudioPlayer::Play()
	{
		if (!IsValidAudioFile(m_File))
			return;

		if (!mSound)
			return;

		if (mSound.IsPlaying())
		{
			const bool bSeeked = mSound.SeekToFrame(0);
			JPL_ASSERT(bSeeked);
		}

		mSound.SetLooping(bLooping);
		const bool bStarted = mSound.Start();
		JPL_ASSERT(bStarted);
	}

	void AudioPlayer::Stop()
	{
		if (!IsValidAudioFile(m_File))
			return;

		if (!mSound)
			return;

		if (mSound.Stop())
		{
			const bool bSeeked = mSound.SeekToFrame(0);
			JPL_ASSERT(bSeeked);
		}
	}

	void AudioPlayer::Pause()
	{
		if (!IsValidAudioFile(m_File))
			return;

		if (!mSound)
			return;

		const bool bStopped = mSound.Stop();
		JPL_ASSERT(bStopped);
	}

	void AudioPlayer::SetFile(const std::filesystem::path& file)
	{
		if (!IsValidAudioFile(file))
		{
			m_File.clear();
			Broadcast<&ChangeListenerType::OnAudioFileChanged>(file);
			return;
		}

		m_File = file;

		// Broadcast file change before resetting the source,
		// this will allow other systems to clean up while
		// the source is still valid.
		Broadcast<&ChangeListenerType::OnAudioFileChanged>(file);

		mSound = {};

		// mSound.Init can block the GUI thread for a while, if decoding the entire files for some formats,
		// though it's barely noticeable in Release/Dist build, we still want load the source asynchronously
		// (using MA_SOUND_FLAG_ASYNC, or MA_SOUND_FLAG_STREAM)
		const ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
		if (not JPL_ENSURE(mSound.Init(file.string().c_str(), flags, /* use source's out channel count*/ true)))
		{
			return;
		}

		if (onSoundCreated)
		{
			onSoundCreated(mSound);
		}

		mSound.SetLooping(bLooping);
		JPL_ENSURE(mSound.Start());
	}

	void AudioPlayer::SetLooping(bool shouldLoop)
	{
		if (bLooping != shouldLoop)
		{
			if (mSound)
				mSound.SetLooping(shouldLoop);

			bLooping = shouldLoop;
		}
	}

	float AudioPlayer::GetCursorPosition() const
	{
		if (!mSound)
		{
			return -1.0f;
		}

		if (ma_sound_at_end(mSound) || !GetSeekFrame())
		{
			return -1.0f;
		}

		//? This is annoying, but miniaudio's internal functions are not const
		// even though they don't modify the object
		MA::Sound& s = const_cast<MA::Sound&>(mSound);

		return s.GetCursorInSeconds() / s.GetLengthInSeconds();
	}

	void AudioPlayer::SeekToPosition(float position)
	{
		if (mSound)
		{
			position = std::clamp(position, 0.0f, 1.0f);
			mSound.SeekToFrame(mSound.GetLengthInFrames() * position);
		}
	}

	float AudioPlayer::GetCursorInSeconds() const
	{
		if (!mSound)
		{
			return 0.0f;
		}

		MA::Sound& s = const_cast<MA::Sound&>(mSound);
		return s.GetCursorInSeconds();
	}

	float AudioPlayer::GetTotalLengthInSeconds() const
	{
		if (!mSound)
		{
			return 0.0f;
		}

		MA::Sound& s = const_cast<MA::Sound&>(mSound);
		return s.GetLengthInSeconds();
	}

	uint32_t AudioPlayer::GetNumChannels() const
	{
		return mSound ? mSound.GetNumOutputChannels(0) : 0;
	}

	bool AudioPlayer::IsValidAudioFile(const std::filesystem::path& filepath)
	{
		if (not std::filesystem::exists(filepath)) [[unlikely]]
		{
			return false;
		}
		else
		{
			static constexpr std::string_view cAllowedFileExtensions[]
			{
				".wav", ".WAV", ".ogg", ".mp3", ".flac"
			};

			return std::ranges::find(cAllowedFileExtensions, filepath.extension()) != std::ranges::end(cAllowedFileExtensions);
		}
	}

	uint64_t AudioPlayer::GetSeekFrame() const
	{
		return mSound ? mSound->seekTarget : 0;
	}
} // namespace JPL
