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

#include "SpectrogramGenerator.h"

#include "SpectrogramColourMap.h"

#include <JPLSpatial/Math/DecibelsAndGain.h>

#include <AudioFFT.h>

#include <algorithm>
#include <cmath>

namespace JPL::GUI
{
	//==========================================================================
	SpectrogramGenerator::SpectrogramGenerator()
		: mFFT(std::make_unique<audiofft::AudioFFT>())
	{}

	SpectrogramGenerator::~SpectrogramGenerator() = default;

	SpectrogramGenerator::SpectrogramGenerator(const SpectrogramParams& params)
		: mFFT(std::make_unique<audiofft::AudioFFT>())
	{
		Init(params);
	}

	void SpectrogramGenerator::Init(const SpectrogramParams& params)
	{
		mParams = params;
		mBinCount = audiofft::AudioFFT::ComplexSize(params.FFTSize);

		mFFT->init(params.FFTSize);

		mRealOutput.resize(mBinCount);
		mImagOutput.resize(mBinCount);
		mMagnitudes.resize(mBinCount);
		mWindowedBuffer.resize(params.FFTSize);
		mHannWindow.resize(params.FFTSize);

		// Precalculate Hann window
		float windowSum = 0.0f;
		
		for (uint32 i = 0; i < params.FFTSize; ++i)
		{
			mHannWindow[i] = 0.5f * (1.0f - std::cos(JPL_TWO_PI * i / (params.FFTSize - 1)));
			windowSum += mHannWindow[i];
		}

		mInvWindowSum = 1.0f / windowSum;
	}

	float SpectrogramGenerator::GetMagnitudeAtBin(float bin) const
	{
		bin = std::clamp(bin, 0.0f, static_cast<float>(mBinCount - 1));

		const uint32 lower = static_cast<uint32>(bin);
		const uint32 upper = std::min<uint32>(lower + 1, mBinCount - 1);
		const float fraction = bin - static_cast<float>(lower);
		return Math::Lerp(mMagnitudes[lower], mMagnitudes[upper], fraction);
	}

	simd SpectrogramGenerator::GetMagnitudeAtBin(simd bin) const
	{
		bin = clamp(bin, 0.0f, static_cast<float>(mBinCount - 1));

		const simd_mask lower = bin.to_mask();
		const simd_mask upper = min(lower + 1, mBinCount - 1);
		const simd fraction = bin - lower.to_simd();

		uint32 lowerIdx[4]{};
		uint32 upperIdx[4]{};
		lower.store(lowerIdx);
		upper.store(upperIdx);

		const simd magLower(
			mMagnitudes[lowerIdx[0]],
			mMagnitudes[lowerIdx[1]],
			mMagnitudes[lowerIdx[2]],
			mMagnitudes[lowerIdx[3]]
		);

		const simd magUpper(
			mMagnitudes[upperIdx[0]],
			mMagnitudes[upperIdx[1]],
			mMagnitudes[upperIdx[2]],
			mMagnitudes[upperIdx[3]]
		);

		return Math::Lerp(magLower, magUpper, fraction);
	}

#if JPL_SPECTROGRAM_RANGE_BIN
	float SpectrogramGenerator::GetMaxMagnitudeInRange(float lowerBin, float upperBin) const
	{
		lowerBin = std::clamp(lowerBin, 0.0f, static_cast<float>(mBinCount - 1));
		upperBin = std::clamp(upperBin, 0.0f, static_cast<float>(mBinCount - 1));

		if (lowerBin > upperBin)
			std::swap(lowerBin, upperBin);

		// Preserve interpolation when the row covers less than one FFT bin.
		float maxMagnitude = std::max(GetMagnitudeAtBin(lowerBin), GetMagnitudeAtBin(upperBin));

		const uint32 firstWholeBin = static_cast<uint32>(std::ceil(lowerBin));
		const uint32 lastWholeBin = static_cast<uint32>(std::floor(upperBin));

		for (uint32 bin = firstWholeBin; bin <= lastWholeBin and bin < mBinCount; ++bin)
		{
			maxMagnitude = std::max(maxMagnitude, mMagnitudes[bin]);
		}

		return maxMagnitude;
	}

	simd SpectrogramGenerator::GetMaxMagnitudeInRange(simd lowerBin, simd upperBin) const
	{
		const simd minBin(0.0f);
		const simd maxBin(static_cast<float>(mBinCount - 1));
		lowerBin = clamp(lowerBin, minBin, maxBin);
		upperBin = clamp(upperBin, minBin, maxBin);

		const simd lbCopy = lowerBin;
		lowerBin = min(lbCopy, upperBin);
		upperBin = max(lbCopy, upperBin);

		// Preserve interpolation when the row covers less than one FFT bin.
		float maxMagnitude[4]{};
		max(GetMagnitudeAtBin(lowerBin), GetMagnitudeAtBin(upperBin)).store(maxMagnitude);

		uint32 firstWholeBin[4]{}; ceil(lowerBin).to_mask().store(firstWholeBin);
		uint32 lastWholeBin[4]{}; floor(upperBin).to_mask().store(lastWholeBin);

		auto findMaxBin = [&](uint32 i)
		{
			for (uint32 bin = firstWholeBin[i]; bin <= lastWholeBin[i]; ++bin)
			{
				JPL_ASSERT(bin < mBinCount);

				maxMagnitude[i] = std::max(maxMagnitude[i], mMagnitudes[bin]);
			}
		};

		// TODO: probably not the most efficient way
		findMaxBin(0);
		findMaxBin(1);
		findMaxBin(2);
		findMaxBin(3);

		return maxMagnitude;
	}
#endif

	void SpectrogramGenerator::WriteMelScaledColumn(std::span<uint32> outImageColumn, float sampleRate)
	{
		if (outImageColumn.size() < 2)
			return;

		const float minHz = 0.0f;
		const float maxHz = sampleRate * 0.5f;

		const float minMel = HzToMel(minHz);
		const float maxMel = HzToMel(maxHz);

		const float invHeight = 1.0f / static_cast<float>(outImageColumn.size());
		const float fftBinFactor = static_cast<float>(mParams.FFTSize) / sampleRate;
		const float invDbRange = 1.0f / (mParams.MaxDb - mParams.MinDb);

		const std::size_t simdCount = FloorToSIMDSize(outImageColumn.size());
		const simd stride(4.0f);
		const simd invHeightV(invHeight);
		const simd invDbRangeV(invDbRange);
		const simd minMelV(minMel);
		const simd maxMelV(maxMel);

#if JPL_SPECTROGRAM_DBG_PRINT
		float maxMagnitude = 0.0f;
		std::size_t maxBin = 0;
#endif

#if not JPL_SPECTROGRAM_RANGE_BIN
		simd rowIdx(0.5f, 1.5f, 2.5f, 3.5f); // +0.5f offset to center pixel row

		for (std::size_t row = 0; row < simdCount; row += simd::size(), rowIdx += stride)
		{
			// Get mel-scaled row
			const simd normalizedY = simd(1.0f) - rowIdx * invHeightV;
			const simd mel = Math::Lerp(minMelV, maxMelV, normalizedY);
			const simd hz = MelToHz(mel);

			// Get magnitued at the row
			const simd fftBin = hz * fftBinFactor;
			const simd magnitude = GetMagnitudeAtBin(fftBin);

			// Convert to dB and normalize
			const simd db = -GainTodB(magnitude + 1e-7f) + mParams.DisplayDb;
			const simd normalized = clamp((db - mParams.MinDb) * invDbRangeV, 0.0f, 1.0f);

			// Write pixel colour for the normalized magnitude
			SpectrogramColourMap::SampleColourMap(normalized).store(&outImageColumn[row]);

#if JPL_SPECTROGRAM_DBG_PRINT
			const float m = magnitude.reduce_max();
			if (m > maxMagnitude)
			{
				maxMagnitude = m;
				maxBin = row;
			}
#endif
		}

		for (std::size_t row = simdCount; row < outImageColumn.size(); ++row)
		{
			const float normalizedY = 1.0f - (static_cast<float>(row) + 0.5f) * invHeight;

			// Get mel-scaled row
			const float mel = Math::Lerp(minMel, maxMel, normalizedY);
			const float hz = MelToHz(mel);

			// Get magnitued at the row
			const float fftBin = hz * fftBinFactor;
			const float magnitude = GetMagnitudeAtBin(fftBin);

			// Convert to dB and normalize
			const float db = -GainTodB(magnitude + 1e-7f) + mParams.DisplayDb;
			const float normalized = std::clamp((db - mParams.MinDb) * invDbRange, 0.0f, 1.0f);

			// Write pixel colour for the normalized magnitude
			outImageColumn[row] = SpectrogramColourMap::SampleColourMap(normalized);

#if JPL_SPECTROGRAM_DBG_PRINT
			if (magnitude > maxMagnitude)
			{
				maxMagnitude = magnitude;
				maxBin = row;
			}
#endif
		}
#else
		simd rowIdx(0.5f, 1.5f, 2.5f, 3.5f);

		for (std::size_t row = 0; row < simdCount; row += simd::size(), rowIdx += stride)
		{
			const simd topNormalizedY = simd(1.0f) - rowIdx * invHeight;
			const simd bottomNormalizedY = simd(1.0f) - (rowIdx + 1.0f) * invHeight;

			const simd topMel = Math::Lerp(minMel, maxMel, topNormalizedY);
			const simd bottomMel = Math::Lerp(minMel, maxMel, bottomNormalizedY);

			const simd topHz = MelToHz(topMel);
			const simd bottomHz = MelToHz(bottomMel);

			const simd upperBin = topHz * fftBinFactor;
			const simd lowerBin = bottomHz * fftBinFactor;

			// Reduce the FFT magnitudes covered by this pixel row.
			const simd magnitude = GetMaxMagnitudeInRange(lowerBin, upperBin) * mWindowScalingFactor;
			const simd db = -GainTodB(magnitude + 1e-7f) + mParams.DisplayDb;
			const simd normalized = clamp((db - mParams.MinDb) * invDbRange, 0.0f, 1.0f);

			SpectrogramColourMap::SampleColourMap(normalized).store(&outImageColumn[row]);

#if JPL_SPECTROGRAM_DBG_PRINT
			const float m = magnitude.reduce_max();
			if (m > maxMagnitude)
			{
				maxMagnitude = m;
				maxBin = row;
			}
#endif
		}

		for (std::size_t row = simdCount; row < outImageColumn.size(); ++row)
		{
			const float topNormalizedY = 1.0f - static_cast<float>(row) * invHeight;
			const float bottomNormalizedY = 1.0f - static_cast<float>(row + 1) * invHeight;

			const float topMel = Math::Lerp(minMel, maxMel, topNormalizedY);
			const float bottomMel = Math::Lerp(minMel, maxMel, bottomNormalizedY);

			const float topHz = MelToHz(topMel);
			const float bottomHz = MelToHz(bottomMel);

			const float upperBin = topHz * fftBinFactor;
			const float lowerBin = bottomHz * fftBinFactor;

			// Reduce the FFT magnitudes covered by this pixel row.
			const float magnitude = GetMaxMagnitudeInRange(lowerBin, upperBin) * mWindowScalingFactor;
			const float db = -GainTodB(magnitude + 1e-7f) + mParams.DisplayDb;
			const float normalized = std::clamp((db - mParams.MinDb) * invDbRange, 0.0f, 1.0f);

			outImageColumn[row] = SpectrogramColourMap::SampleColourMap(normalized);

#if JPL_SPECTROGRAM_DBG_PRINT
			if (magnitude > maxMagnitude)
			{
				maxMagnitude = magnitude;
				maxBin = row;
			}
#endif
		}
#endif

#if JPL_SPECTROGRAM_DBG_PRINT
		const float normalizedMagnitude =
			maxMagnitude;// *2.0f / mWindowSum;

		const float db =
			-GainTodB(normalizedMagnitude + 1e-7f);

		std::cout << std::format(
			"Peak bin: {}, normalized magnitude: {}, dB: {}",
			maxBin,
			normalizedMagnitude,
			db) << '\n';
#endif
	}

	void SpectrogramGenerator::ProcessFrame(std::span<const float> audioFrame)
	{
		// bin count > 0 means we're initialzied
		if (not JPL_ENSURE(audioFrame.size() == mParams.FFTSize and mBinCount > 0))
			return;

		// Apply window
		{
			const uint32 simdCount = FloorToSIMDSize(mParams.FFTSize);

			for (uint32 i = 0; i < simdCount; i += simd::size())
				(simd(&audioFrame[i]) * simd(&mHannWindow[i])).store(&mWindowedBuffer[i]);

			for (uint32 t = simdCount; t < mParams.FFTSize; ++t)
				mWindowedBuffer[t] = audioFrame[t] * mHannWindow[t];
		}

		// Compute FFT
		mFFT->fft(mWindowedBuffer.data(), mRealOutput.data(), mImagOutput.data());

		// TODO: if outImageColumn.size() is < mBinCount,
		//		it may be more efficient to comnpute and normalize magnitudes
		//		only for the requested rows in WriteMelScaledColumn()

		// Compute magnitudes
		{
			const uint32 simdCount = FloorToSIMDSize(mBinCount);

			for (uint32 i = 0; i < simdCount; i += simd::size())
			{
				const simd real(&mRealOutput[i]);
				const simd imag(&mImagOutput[i]);
				const simd magnitude = Math::Sqrt(real * real + imag * imag);
				magnitude.store(&mMagnitudes[i]);
			}

			for (uint32 t = simdCount; t < mBinCount; ++t)
			{
				const float real = mRealOutput[t];
				const float imag = mImagOutput[t];
				mMagnitudes[t] = Math::Sqrt(real * real + imag * imag);
			}
		}

		// Normalzie FFT magnitudes for Hann window
		{
			// Skip DC bin 0
			float* magnitude = mMagnitudes.data() + 1;

			// Skip Nyquist bin
			const uint32 count = mMagnitudes.size() - 2;
			const uint32 simdOps = GetNumSIMDOps(count);
			const uint32 tail = GetSIMDTail(count);

			const float windowScalingFactor = 2.0f * mInvWindowSum;
			const simd normFactor(windowScalingFactor);

			for (uint32 i = 0; i < simdOps; ++i)
			{
				(simd(magnitude) * normFactor).store(magnitude);
				magnitude += simd::size();
			}

			for (uint32 t = 0; t < tail; ++t)
				magnitude[t] *= windowScalingFactor;

			// DC and Nyquist bins we don't need to multiply by 2
			mMagnitudes.front() *= mInvWindowSum;
			mMagnitudes.back() *= mInvWindowSum;
		}
	}
} // namespace JPL::GUI
