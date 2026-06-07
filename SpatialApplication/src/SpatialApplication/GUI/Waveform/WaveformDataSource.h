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

#include <JPLSpatial/Core.h>

#include "Coroutine/Coroutine.h"
#include "Utility/MVCUtils.h"

#include <coroutine>
#include <filesystem>
#include <vector>

namespace JPL
{
	namespace GUI
	{
		//======================================================================
		struct SampleData
		{
			/// Number of frames in the Samples buffer.
			/// (number of channels is inferred from Samples.size() / NumFrames)
			uint64 NumFrames = 0;

			/// Deinterleaved sample data,
			/// channels are stored contiguously one after another.
			std::vector<float> Samples;
		};
	}

	//==========================================================================
	template<>
	class ChangeListener<GUI::SampleData>
	{
	public:
		virtual ~ChangeListener() = default;

		/// Should be called when underlying source of sample data changed
		virtual void OnSourceChanged() {}

		/// Should be called when new sample data is available freom the underlying source
		virtual void OnNewDataAwailable(const GUI::SampleData& sampleData) {}
	};
} // namespace JPL

namespace JPL::GUI
{
	using WaveformDataSource = ChangeBroadcaster<SampleData>;

	//==========================================================================
	/// Implementation of WaveformDataSource that reads sample data from a file
	class AudioFileWaveformDataSource : public WaveformDataSource
	{
	public:
		AudioFileWaveformDataSource();

		void SetFile(const std::filesystem::path& filepath);

		const std::filesystem::path& GetCurrentFile() const { return mFilepath.Get(); }

	private:
		[[nodiscard]] Coro::Task<> ReadSampleDataRoutine();

	private:
		Property<std::filesystem::path> mFilepath;
		Coro::PropertyUpdate<std::filesystem::path> mSelectedFileUpdate;
		Coro::Task<> mWaveformUpdateRoutine;
	};

	//==========================================================================
#if 0
	// Just an example use-case. Anything can inherit from WaveformDataSource
	// and Waveform class will be able to use it to draw sample data as waveform
	class SynthesizedWaveformDataSource : public WaveformDataSource
	{
	public:
	};
#endif

	//==========================================================================
	/// Utility that provides coroutine awaiters for WaveformDataSource.
	/// 
	/// For now WaveformDataAwaitable supports only one awaiting coroutine at a time,
	/// but multiple WaveformDataAwaitables can be used for the same WaveformDataSource
	class WaveformDataAwaitable : protected ChangeListener<SampleData>
	{
	public:
		explicit WaveformDataAwaitable(WaveformDataSource& source);

		~WaveformDataAwaitable();

		[[nodiscard]] JPL_INLINE const SampleData& GetSampleData() const { return mCachedSampleData; }

		// co_await source change flag
		[[nodiscard]] JPL_INLINE auto WhenSourceChanged() { return mSourceChangedFlag.operator co_await(); }

		struct Awaiter;

		// co_await new waveform data awailable (same as calling co_await WaveformDataAwaitable)
		[[nodiscard]] JPL_INLINE Awaiter NewSampleData() { return operator co_await(); }

		// co_await new waveform data awailable (same as calling NewSampleData())
		Awaiter operator co_await();

	private:
		void OnSourceChanged() override;
		void OnNewDataAwailable(const SampleData& newSampleData) override;

	private:
		struct Awaiter
		{
			WaveformDataAwaitable& DataSource;

			bool await_ready() noexcept { return DataSource.bNewDataAwailable; }
			bool await_suspend(std::coroutine_handle<> h) noexcept
			{
				DataSource.mWaitingCoros.push_back(h);
				return not DataSource.bNewDataAwailable;
			}
			const SampleData* await_resume() noexcept { return &DataSource.mCachedSampleData; }
		};

	private:
		WaveformDataSource& mSource;
		SampleData mCachedSampleData;

		std::vector<std::coroutine_handle<>> mWaitingCoros;
		Coro::Flag mSourceChangedFlag;
		bool bNewDataAwailable = false;
	};
} // namespace JPL::GUI
