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

#include "audio/choc_SampleBuffers.h"
#include <JPLSpatial/Containers/StaticArray.h>
#include <JPLSpatial/Utilities/RealtimeObject.h>

#include <atomic>
#include <vector>
#include <array>
#include <map>

namespace JPL
{
    //==========================================================================
    /// Tracks and reports per-channel RMS and peak levels for multichannel audio,
    /// maintaining short- and long-term accumulators and providing thread-safe access
    /// between realtime and non-realtime contexts.
    class RMSMeter
    {
    public:
        inline static constexpr uint32 MAX_CHANNELS = 32;

    public:
        RMSMeter();
        ~RMSMeter();

        RMSMeter(const RMSMeter&) = delete;
        RMSMeter& operator=(const RMSMeter&) = delete;

        void Prepare(double sampleRate,
                           int numberOfInputChannels,
                           int estimatedSamplesPerBlock,
                           int expectedRequestRate);

        void ProcessBlock(const choc::buffer::ChannelArrayView<const float>& buffer);

        const std::vector<float>& GetRMSForIndividualChannels();
        const std::vector<float>& GetMaxRMSForIndividualChannels();

        const std::vector<float>& GetPeakForIndividualChannels();
        const std::vector<float>& GetMaxPeakForIndividualChannels();

        void SetFreezeLoudnessRangeOnSilence(bool freeze);

        // Must be called from non-realtime thread and only when ProcessBlock is not running!
        void Reset();

    private:
        int mNumberOfBins;
        int mNumberOfSamplesPerBin;
        int mNumberOfSamplesInAllBins;
        double mInvNumberOfSamplesInAllBins;

        int mNumberOfBinsToCoverPeakMs;

        /** After the samples are squared, they need to be accumulated.
             For the measurement of the RMS, the previous 'measure time'
             of samples need to be accumulated. For the other
             measurements shorter windows are used.
         */
        std::vector<std::vector<double>> mBin;
        std::vector<std::vector<double>> mPeakBin;

        int mCurrentBin;
        int mNumberOfSamplesInTheCurrentBin;
        int mCurrentPeakBin;

        struct ChannelLevels
        {
            StaticArray<float, MAX_CHANNELS> Levels;
            StaticArray<float, MAX_CHANNELS> MaxLevels;
        };
        // Thread-safe atomic object to change levels from realtime thread and read from non-realtime thread.
        using RealtimeLevels = RealtimeObject<ChannelLevels, RealtimeObjectOptions::realtimeMutatable>;

        using LevelsRealtimeAccess = RealtimeObject<RMSMeter::ChannelLevels,
            RealtimeObjectOptions::realtimeMutatable>::ScopedAccess<ThreadType::realtime>;

        using LevelsNonRealtimeAccess = RealtimeObject<RMSMeter::ChannelLevels,
            RealtimeObjectOptions::realtimeMutatable>::ScopedAccess<ThreadType::nonRealtime>;

        // The average of the squared samples of the last cTimeOfAccumulationForRMS second(s).
        // A value for each channel.
        RealtimeLevels mLastRMSMeasureTime;
        std::vector<float> mAverageRMSForIndividualChannelsReturn;
        std::vector<float> mMaxRMSForIndividualChannelsReturn;

        RealtimeLevels mLastPeakMeasure;
        std::vector<float> mPeakForIndividualChannelsReturn;
        std::vector<float> mMaxPeakForIndividualChannelsReturn;

        /** If there is no signal at all, the methods GetRMSForIndividualChannels() and
             GetPeakForIndividualChannels() would perform a log10(0) which would result
             in a value -nan. To avoid this, the return value of this methods will be
             set to sMinimalReturnValue.
         */
        inline static const float sMinimalReturnValue{ 0.0f };

        // Time to accumulate RMS values (in seconds):
        // 300 ms - average (standard)
        // 125 ms - fast
        // 1 s - slow
        // 50 ms - very fast, peak sensitive
        inline static const double cTimeOfAccumulationForRMS = 0.3;


        std::atomic<bool> mFreezeLoudnessRangeOnSilence;
        bool mCurrentBlockIsSilent;
    };
} // namespace JPL
