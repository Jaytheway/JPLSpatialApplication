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

#include "GUI/Waveform/WaveformDataSource.h"
#include "Controller/AudioPlayer.h"
#include "Systems/EventsLoop.h"

#include <JPLSpatial/ErrorReporting.h>
#include <JPLSpatial/Containers/StaticArray.h>

#define STB_VORBIS_HEADER_ONLY
#include "miniaudio/extras/stb_vorbis.c"
#include <miniaudio/miniaudio.h>

namespace JPL::GUI
{
	//==========================================================================
	static [[nodiscard]] uint64 ReadSampleData(const std::filesystem::path& filepath, std::vector<float>& outSampleData)
	{
		ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);

		uint64 numFramesRead = 0;
		void* framesPtr = nullptr;
		const ma_result result = ma_decode_file(filepath.string().c_str(), &config, &numFramesRead, &framesPtr);

		if (not JPL_ENSURE(result == MA_SUCCESS) || numFramesRead == 0 || framesPtr == nullptr)
		{
			if (framesPtr != nullptr)
			{
				ma_free(framesPtr, nullptr);
			}

			numFramesRead = 0;
		}
		else
		{
			ma_uint16 channels = config.channels;
			ma_uint32 sampleRate = config.sampleRate;
			double duration = (double)numFramesRead / (double)sampleRate;

			//auto dataSize = wav.dataChunkDataSize;
			//auto pos = wav.dataChunkDataPos;
			//auto fileSize = dataSize + pos;
			////auto sizestr = Utils::BytesToString(fileSize);
			//fileInfo = { duration, sampleRate, bitDepth, channels, wav.totalPCMFrameCount, fileSize };

			outSampleData.resize(numFramesRead * channels);

			JPL::StaticArray<void*, MA_MAX_CHANNELS> channelPtrs(channels);
			for (uint32_t i = 0; i < channels; ++i)
			{
				channelPtrs[i] = &outSampleData[i * numFramesRead];
			}

			ma_deinterleave_pcm_frames(ma_format_f32, channels, numFramesRead, framesPtr, (void**)channelPtrs.data());

			// miniaudio doesn't deallocated allocated frame storage,
			// we need to do it ourselves
			ma_free(framesPtr, nullptr);
		}

		return numFramesRead;
	}

	//==========================================================================
	AudioFileWaveformDataSource::AudioFileWaveformDataSource()
		: mSelectedFileUpdate(mFilepath)
	{
		mWaveformUpdateRoutine = ReadSampleDataRoutine();
		mWaveformUpdateRoutine.Resume(); // start the routine
	}

	void AudioFileWaveformDataSource::SetFile(const std::filesystem::path& filepath)
	{
		mFilepath.Set(filepath);
	}

	Coro::Task<> AudioFileWaveformDataSource::ReadSampleDataRoutine()
	{
		std::jthread backgroundThread;
		SampleData sampleData;

		for (;;)
		{
			// Wait for the filepath to be updated
			const std::filesystem::path newFilePath = co_await mSelectedFileUpdate;

			Broadcast<&ChangeListenerType::OnSourceChanged>();

			if (AudioPlayer::IsValidAudioFile(newFilePath))
			{
				co_await Coro::SwitchToAsync(backgroundThread); // --- Read file on background thread
				{
					sampleData.NumFrames = ReadSampleData(mFilepath.Get(), sampleData.Samples);
				}
				co_await Coro::SwitchToMainThread(); // --- Update waveform data on main thread
			}
			// ...we still want to broadcast if filepath was invalid and our sample data is empty

			Broadcast<&ChangeListenerType::OnNewDataAwailable>(sampleData);
			sampleData.NumFrames = 0;
			sampleData.Samples.clear();
		}
	}

	//==========================================================================
	void WaveformDataAwaitable::OnSourceChanged()
	{
		mSourceChangedFlag.Set();
		bNewDataAwailable = false;
	}

	void WaveformDataAwaitable::OnNewDataAwailable(const SampleData& newSampleData)
	{
		// TODO: if new data becomes awailable, while no coroutine weaiting for it,
		// when a coroutine starts awaiting, it will skip the new data,
		// even though this is the first time it asks awaitable for it.
		// This can happne only if new data comes in from a different thread than
		// the coroutine is executing on.

		mSourceChangedFlag.Reset();

		bNewDataAwailable = true;
		mCachedSampleData = newSampleData;

		for (auto& coro : mWaitingCoros)
		{
			if (coro)
				coro.resume();
		}

		mWaitingCoros.clear();
		bNewDataAwailable = false;
	}


	WaveformDataAwaitable::WaveformDataAwaitable(WaveformDataSource& source)
		: mSource(source)
	{
		mSource.AddListener(this);
	}

	WaveformDataAwaitable::~WaveformDataAwaitable()
	{
		mSource.RemoveListener(this);
	}

	WaveformDataAwaitable::Awaiter WaveformDataAwaitable::operator co_await()
	{
		return Awaiter{ *this };
	}
} // namespace JPL::GUI
