//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright Jaroslav Pevno 2026, JPL Spatial Application is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "CoreInclude.h" // contains Log.h

#include <spdlog/sinks/callback_sink.h>

#include <iterator>

#define JPL_HAS_CONSOLE !WL_DIST

#if JPL_HAS_CONSOLE
#include <spdlog/sinks/ansicolor_sink.h>
#include <choc/threading/choc_TaskThread.h>

//==============================================================================
/// Wrapper for a thread periodically pumping log messages from lock-free
/// concurrent queue and pushing them to stdout colour sink.
class ConsolePump
{
public:
    // Receive log messages from various threads
    void operator()(const spdlog::details::log_msg& msg)
    {
        if (mStdoutSink) // push message to the queue only if the pump is active
            mQueue.enqueue(spdlog::details::log_msg_buffer{ msg });
    }

    void Start()
    {
        mStdoutSink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_st>();
        mStdoutSink->set_pattern("%^[%T] %n: %v%$");

        static constexpr uint32_t cPollIntervalMs = 32;

        // Pump log message queue from our thread
        mThread.start(cPollIntervalMs, [this]() mutable
        {
            static moodycamel::ConsumerToken consumerToken(mQueue);

            spdlog::details::log_msg_buffer message;
            while (mQueue.try_dequeue(consumerToken, message))
                mStdoutSink->log(message);
        });
    }

    void Stop()
    {
        mThread.stop();
        mStdoutSink = nullptr;
    }

private:
    std::shared_ptr<spdlog::sinks::ansicolor_stdout_sink_st> mStdoutSink;
    choc::threading::TaskThread mThread;
    
    static constexpr std::size_t cQueueInitialSize = 512;
    moodycamel::ConcurrentQueue<spdlog::details::log_msg_buffer> mQueue{ cQueueInitialSize };
};

ConsolePump gConsolPump;

#endif // !JPL_HAS_CONSOLE

//==============================================================================
namespace JPL
{
    ELogLevel Log::TranslateLogLevel(spdlog::level::level_enum level)
    {
        switch (level)
        {
        case spdlog::level::trace:      return ELogLevel::Trace;
        case spdlog::level::debug:      return ELogLevel::Debug;
        case spdlog::level::info:       return ELogLevel::Info;
        case spdlog::level::warn:       return ELogLevel::Warn;
        case spdlog::level::err:        return ELogLevel::Error;
        case spdlog::level::critical:   return ELogLevel::Critical;
       // case spdlog::level::off:        return ELogLevel::Off;
        default:
            return ELogLevel::Trace;
        }
    }

    spdlog::level::level_enum Log::TranslateLogLevel(ELogLevel level)
    {
        switch (level)
        {
        case ELogLevel::Trace:      return spdlog::level::trace;
        case ELogLevel::Debug:      return spdlog::level::debug;
        case ELogLevel::Info:       return spdlog::level::info;
        case ELogLevel::Warn:       return spdlog::level::warn;
        case ELogLevel::Error:      return spdlog::level::err;
        case ELogLevel::Critical:   return spdlog::level::critical;
       // case ELogLevel::Off:        return spdlog::level::off;
        default:
            return spdlog::level::trace;
        }
    }

    moodycamel::ConsumerToken* gLogConsumerToken = nullptr;

    void Log::Init()
    {
        if (gLogConsumerToken != nullptr)
        {
            if (sLogger != nullptr)
                Log::Error("Log: Trying to initialize Log which was already initialized!");

            return;
        }

        gLogConsumerToken = new moodycamel::ConsumerToken(sQueue);

        // Using single-theraded sink, since we use our own buffer and mutex
        auto sink = std::make_shared<spdlog::sinks::callback_sink_st>(&LogCallback);

        sLogger = std::make_shared<spdlog::logger>("JPLSpatialApplication", sink);
        sLogger->set_level(spdlog::level::trace); // receive all messages

#if JPL_HAS_CONSOLE
        auto consoleSink = std::make_shared<spdlog::sinks::callback_sink_st>([](const spdlog::details::log_msg& msg)
        {
            gConsolPump(msg); // simply forward the message to console pump
        });
        sLogger->sinks().push_back(consoleSink);
        gConsolPump.Start();
#endif
    }

    void Log::Uninit()
    {
#if JPL_HAS_CONSOLE
        gConsolPump.Stop();
#endif
        sLogger.reset();
        // We don't need to call spdlog::drop_all(), Walnut does it

        delete gLogConsumerToken;
        gLogConsumerToken = nullptr;
    }

    void Log::Trace(std::string_view text)
    {
        sLogger->trace(text);
    }

    void Log::Print(ELogLevel level, std::string_view text)
    {
        sLogger->log(TranslateLogLevel(level), text);
    }

    void Log::Error(std::string_view text)
    {
        sLogger->error(text);
    }

    void Log::Warn(std::string_view text)
    {
        sLogger->warn(text);
    }

    void Log::Info(std::string_view text)
    {
        sLogger->info(text);
    }

    void Log::ExtractLastMessages(std::vector<LogMessage>& outMessages)
	{
        outMessages.reserve(outMessages.size() + sQueue.size_approx());

        size_t dequeued = 0;
        do
        {
            static constexpr size_t numToDequeue = 64;

            dequeued = sQueue.try_dequeue_bulk(*gLogConsumerToken,
                                               std::back_inserter(outMessages),
                                               numToDequeue);
        } while (dequeued > 0);
        
	}

    void Log::LogCallback(const spdlog::details::log_msg& message)
    {
        //! Note: if we want patterns/formatting, we need to do it manually
        //! since we're using simple callback sink
        sQueue.enqueue(LogMessage(TranslateLogLevel(message.level),
                                  std::string(message.payload.begin(), message.payload.end())));
    }

} // namespace JPL
