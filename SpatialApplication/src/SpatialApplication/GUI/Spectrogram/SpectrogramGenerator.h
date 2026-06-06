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
#include <JPLSpatial/ErrorReporting.h>
#include <JPLSpatial/Math/SIMD.h>
#include <JPLSpatial/Math/SIMDMath.h>

#include <memory>
#include <vector>

// Whether to output max bin for a bin range within single pixel row
// or just the interpolated value between adjacent bins
#define JPL_SPECTROGRAM_RANGE_BIN 0
#define JPL_SPECTROGRAM_DBG_PRINT 0

namespace audiofft
{
	class AudioFFT;
}

namespace JPL::GUI
{
	//==========================================================================
	struct SpectrogramParams
	{
		std::size_t FFTSize = 1024;
		float MinDb = -96.0f;
		float MaxDb = 0.0f;

		float DisplayDb = 0.0f; // dB offset for display purposes
	};

	//==========================================================================
	// TODO: maybe move these somewhere else
	inline float HzToMel(float hz)
	{
		static constexpr float d = 1.0f / 700.0f;
		return 2595.0f * std::log10(1.0f + hz * d);
	}

	inline simd HzToMel(simd hz)
	{
		static const simd d(1.0f / 700.0f);
		return 2595.0f * log10(simd(1.0f) + hz * d);
	}

	inline float MelToHz(float mel)
	{
		static constexpr float d = 1.0f / 2595.0f;
		return 700.0f * (std::pow(10.0f, mel * d) - 1.0f);
	}

	inline simd MelToHz(simd mel)
	{
		static const simd d(1.0f / 2595.0f);
		return 700.0f * (pow(10.0f, mel * d) - 1.0f);
	}

	//==========================================================================
	class SpectrogramGenerator
	{
	public:
		SpectrogramGenerator();
		~SpectrogramGenerator();
		explicit SpectrogramGenerator(const SpectrogramParams& params);

		void Init(const SpectrogramParams& params);

		// @param audioFrame must have size of 'params.FFTSize'
		void ProcessFrame(std::span<const float> audioFrame);
		
		// Write mel-scaled frequency magnitudes of the last processed frame
		void WriteMelScaledColumn(std::span<uint32> outImageColumn, float sampleRate);

		const SpectrogramParams& GetParams() const { return mParams; }

	private:
		float GetMagnitudeAtBin(float bin) const;
		simd GetMagnitudeAtBin(simd bin) const;

#if JPL_SPECTROGRAM_RANGE_BIN
		float GetMaxMagnitudeInRange(float lowerBin, float upperBin) const;
		simd GetMaxMagnitudeInRange(simd lowerBin, simd upperBin) const;
#endif

	private:
		SpectrogramParams mParams;
		std::size_t mBinCount = 0;
		std::unique_ptr<audiofft::AudioFFT> mFFT;

		float mInvWindowSum;
		std::vector<float> mWindowedBuffer;
		std::vector<float> mHannWindow;
		std::vector<float> mRealOutput;
		std::vector<float> mImagOutput;
		std::vector<float> mMagnitudes;
	};
} // namespace JPL::GUI
