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

#pragma once

#include "spdlog/spdlog.h"

#include <concurrentqueue.h>
#include <magic_enum/magic_enum.hpp>

#include <cstdint>
#include <format>
#include <memory>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace JPL
{
	class RingBufferSink;

	enum class ELogLevel : uint8_t
	{
		Trace = 1 << 0,
		Debug = 1 << 1,
		Info = 1 << 2,
		Warn = 1 << 3,
		Error = 1 << 4,
		Critical = 1 << 5
		
		//Off, // 'Off' value is used to disable spdlog sinks, we don't use it yet
	};

	// Imporing operators into JPL namespace for convenience
	namespace EnumFlags
	{
		using magic_enum::bitwise_operators::operator~;
		using magic_enum::bitwise_operators::operator|;
		using magic_enum::bitwise_operators::operator&;
		using magic_enum::bitwise_operators::operator^;
		using magic_enum::bitwise_operators::operator|=;
		using magic_enum::bitwise_operators::operator&=;
		using magic_enum::bitwise_operators::operator^=;
	}
}

// Lets magic_enum treat ELogLevel as flags
template <>
struct magic_enum::customize::enum_range<JPL::ELogLevel>
{
	static constexpr bool is_flags = true;
};

namespace JPL
{
	//==========================================================================
	/// Data we store about messages comming into log.
	/// (may be extended later with timestamp and other info)
	struct LogMessage
	{
		ELogLevel Level;
		std::string Message;
	};

	//==========================================================================
	/// Simple logger, using spdlog internally and lock-free concurrent queue.
	/// Safe to use from multiple threads.
	class Log
	{
	public:
		static void Init();
		static void Uninit();

		static void Print(ELogLevel level, std::string_view text);
		static void Trace(std::string_view text);
		static void Error(std::string_view text);
		static void Warn(std::string_view text);
		static void Info(std::string_view text);

		template<class... Args> requires(sizeof...(Args) > 0)
		static void Print(ELogLevel level, std::format_string<Args...> format, Args&&... args);

		template<class... Args> requires(sizeof...(Args) > 0)
		static void Trace(std::format_string<Args...> format, Args&&... args);

		template<class... Args> requires(sizeof...(Args) > 0)
		static void Error(std::format_string<Args...> format, Args&&... args);

		template<class... Args> requires(sizeof...(Args) > 0)
		static void Warn(std::format_string<Args...> format, Args&&... args);

		template<class... Args> requires(sizeof...(Args) > 0)
		static void Info(std::format_string<Args...> format, Args&&... args);

		/// Extract all the messages from the queue
		static void ExtractLastMessages(std::vector<LogMessage>& outMessages);

	private:
		static void LogCallback(const spdlog::details::log_msg& message);

		static ELogLevel TranslateLogLevel(spdlog::level::level_enum level);
		static spdlog::level::level_enum TranslateLogLevel(ELogLevel level);

	private:
		inline static std::shared_ptr<::spdlog::logger> sLogger;

		static constexpr std::size_t cInitialQueueSize = 1024;
		inline static moodycamel::ConcurrentQueue<LogMessage> sQueue{ cInitialQueueSize };
	};

	inline std::string_view LogLevelToString(ELogLevel level)
	{
		return magic_enum::enum_name(level);
	}
} // namespace JPL

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

namespace JPL
{
	template<class ...Args> requires(sizeof...(Args) > 0)
	inline void Log::Print(ELogLevel level, std::format_string<Args...> format, Args && ...args)
	{
		sLogger->log(TranslateLogLevel(level), std::format(format, std::forward<Args>(args)...));
	}

	template<class ...Args> requires(sizeof...(Args) > 0)
	inline void Log::Trace(std::format_string<Args...> format, Args&& ...args)
	{
		sLogger->trace(std::format(format, std::forward<Args>(args)...));
	}

	template<class ...Args> requires(sizeof...(Args) > 0)
	inline void Log::Error(std::format_string<Args...> format, Args && ...args)
	{
		sLogger->error(std::format(format, std::forward<Args>(args)...));
	}

	template<class ...Args> requires(sizeof...(Args) > 0)
	inline void Log::Warn(std::format_string<Args...> format, Args && ...args)
	{
		sLogger->warn(std::format(format, std::forward<Args>(args)...));
	}

	template<class ...Args> requires(sizeof...(Args) > 0)
	inline void Log::Info(std::format_string<Args...> format, Args && ...args)
	{
		sLogger->info(std::format(format, std::forward<Args>(args)...));
	}

} // namespace JPL