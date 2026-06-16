//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application**
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

#include <concurrentqueue.h>

#include <coroutine>
#include <functional>
#include <queue>
#include <mutex>

namespace JPL
{
	class EventsLoop
	{
	public:
		using Event = std::function<void()>;

	public:
		static EventsLoop& Get() { static EventsLoop sEL; return sEL; }

		template<class Function>
		static void Schedule(Function&& function) { Get().Enqueue(std::forward<Function>(function)); }

		/// Has to be called periodically to process
		/// scheduled events
		void Process()
		{
			// Dequeue only what's currently in the queue
			// to avoid looping endlessesly over possible recursive enqueues
			std::size_t numEvents = mEventQueue.size_approx();

			Event event;
			while (numEvents-- and mEventQueue.try_dequeue(event))
				std::invoke(event);
		}

	private:
		template<class Function>
		void Enqueue(Function&& function)
		{
			mEventQueue.enqueue(std::forward<Function>(function));
		}

	private:
		static constexpr std::size_t cInitialQueueSize = 1024;
		moodycamel::ConcurrentQueue<Event> mEventQueue{ cInitialQueueSize };
	};

	namespace Coro
	{
		struct SwitchToMainThread
		{
			constexpr bool await_ready() const { return false; }
			void await_suspend(std::coroutine_handle<> handle) noexcept
			{
				EventsLoop::Schedule([handle] { handle.resume(); });
			}
			void await_resume() noexcept {}
		};
	}
} // namespace JPL
