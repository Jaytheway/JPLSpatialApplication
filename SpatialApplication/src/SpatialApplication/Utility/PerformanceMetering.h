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

#include "Utility/Timer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <span>

namespace JPL
{

	// TODO: add custom scope profiler

	template<class T>
	class ScopedMeterTimer : JPL::Timer
	{
	public:
		ScopedMeterTimer() = default;
		~ScopedMeterTimer() { T::WriteFrameTime(JPL::Timer::ElapsedMillis()); }
	};


	template<class Class>
	class PerformanceMetering : private Class
	{
		// Must be power of 2
		static constexpr std::size_t cCapacity = 2 << 19; // 4 MB is plenty
		static constexpr std::size_t cMeasureFrequencyPerSecond = 100; // 10 ms at 480 samples blocks 48 kHz samplerate
		static constexpr float cHoursCapacity = cCapacity / (3600.0f * cMeasureFrequencyPerSecond); // rough capacity

	public:
		using Class::Label;

		// Get all data available
		static inline std::span<const float> GetFrameTimeData()
		{
			return { sFrameTime.data(), sWriteIndex.load(std::memory_order_acquire) };
		}

		// Get available data count
		static inline std::size_t GetFrameTimeCount()
		{
			return sWriteIndex.load(std::memory_order_acquire);
		}

		static inline void Clear() { sWriteIndex.store(0, std::memory_order_release); }

		// Get numLastValues written to data
		static inline std::span<const float> GetFrameTimeData(std::size_t numLastValues)
		{
			const std::size_t available = sWriteIndex.load(std::memory_order_acquire);
			numLastValues = std::min(available, numLastValues);
			const std::size_t offset = available - numLastValues;
			return { sFrameTime.data() + offset, numLastValues };
		}

		// Get a window into the available data
		static inline std::span<const float> GetFrameTimeDataWindow(std::size_t offset, std::size_t numValues)
		{
			const std::size_t available = sWriteIndex.load(std::memory_order_acquire);
			numValues = std::min(available, numValues);
			offset = (offset + numValues) > available ? 0 : offset;
			return { sFrameTime.data() + offset, numValues };
		}

		// Write frame time into the data
		static inline void WriteFrameTime(float frameTime)
		{
			std::size_t index = sWriteIndex.load(std::memory_order_acquire);
			sFrameTime[index] = frameTime;

			// Udpate with fast modulo
			sWriteIndex.store((index + 1) & (cCapacity - 1), std::memory_order_release);
		}

		static inline ScopedMeterTimer<PerformanceMetering<Class>> MakeScopedTimer()
		{
			return {};
		}

	private:
		// TODO: we might want to use abstract index with some contiguous window size
		static inline std::atomic<std::size_t> sWriteIndex{ 0 };
		static inline std::array<float, cCapacity> sFrameTime;
	};

	struct AudioCallbackMetering
	{
		inline static const char* Label = "Audio Callback";
	};

	// TODO: if I want it to be scoped/local, it has to be added to some sort of registry
	struct UpdateTapsMetering
	{
		inline static const char* Label = "Update ER Taps";
	};

	struct RayTracinMetering
	{
		static inline const char* Label = "Ray Tracing";
	};

	using PerfMeterAudioCallback = PerformanceMetering<AudioCallbackMetering>;
	using PerfMeterUpdateTaps = PerformanceMetering<UpdateTapsMetering>;
	using PerfMeterRayTracing = PerformanceMetering<RayTracinMetering>;
} // namespace JPL