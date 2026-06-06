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

#include "Coroutine/Coroutine.h"
#include "GUI/Spectrogram/SpectrogramGenerator.h"
#include "GUI/Waveform/WaveformDataSource.h"

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>
#include <JPLSpatial/Math/SIMD.h>
#include <JPLSpatial/Memory/Memory.h>

#include <memory>
#include <memory_resource>
#include <limits>
#include <vector>

namespace Walnut
{
	class Image;
}

namespace JPL::GUI
{
	//==========================================================================
	/// Provides spectrogram drawing routine
	class Spectrogram : ChangeListener<SampleData>
	{
	public:
		struct ImageChunk
		{
			uint64 DataOffset;
			uint64 DataSize;
			uint32 Width;
			uint32 Height;
		};

		/// Pixel data that represent a spectrogram
		/// of a single audio channel
		class ChannelData
		{
		public:
			using allocator_type = std::pmr::polymorphic_allocator<>;

			explicit ChannelData(const allocator_type& allocator);
			ChannelData(const ChannelData& other, const allocator_type& alloc);
			ChannelData(ChannelData&& other, const allocator_type& alloc);

			~ChannelData() noexcept = default;

			std::pmr::vector<uint32> Pixels;
			std::pmr::vector<ImageChunk> Chunks;
		};

	public:
		explicit Spectrogram(WaveformDataSource& waveformDataSorce);

		~Spectrogram() noexcept;

		// @returns false if the spectrogram ImGui Item bounds are clipped
		bool Draw(const char* itemId);
		void DrawProperties();

	private:
		Coro::Task<> SpectrogramUpdateRoutine();

		// ~ Begin ChangeListener<SampleData> interface
		void OnSourceChanged() override;
		void OnNewDataAwailable(const SampleData& newSampleData) override;
		// ~ End ChangeListener<SampleData> interface

	private:
		enum class EUpdateState
		{
			UpToDate,
			UpdatingProperties,
			UpdatingSource
		} mUpdateState;

		WaveformDataSource& mDataSource;
		SampleData mSampleDataCache;
	
		// Note: this must not be used outside of the
		// spectrogram update routine, which handles
		// thread synchronization.
		std::pmr::unsynchronized_pool_resource mMemoryResource;

		Coro::Task<> mSpectrogramUpdateRoutine;
		Coro::Flag mSpectrogramNeedsUpdate;

		std::pmr::vector<ChannelData> mChannelData;
		int mSpectrogramWidthPx = 0;
		int mSpectrogramHeightPx = 0;

		SpectrogramGenerator mGenerator;
		SpectrogramParams mParams; // TODO: properties

		using ChannelAtlas = std::pmr::vector<std::shared_ptr<Walnut::Image>>;
		std::pmr::vector<ChannelAtlas> mChannelAtlases;

		//? debug/profile
		float mSpectrogramGenTime = 0.0f;
	};
} // namespace JPL::GUI
