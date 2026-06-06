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

#include <algorithm>
#include <concepts>
#include <coroutine>
#include <future>
#include <type_traits>

// TODO: consider moving all Coroutine headers to JPL Spatial if there is a use for it

//==============================================================================
/// Basic coroutine utilities.
/// 
/// Task and TaskPromise are borrowed from cppcoro (MIT) (https://github.com/lewissbaker/cppcoro),
/// which seems to be no longer maintained.
namespace JPL::Coro
{
	namespace Detail
	{
		class TaskPromiseBase;

		template<typename T>
		class TaskPromise;
	}

	//==========================================================================
	template<class T = void>
	class [[nodiscard]] Task
	{
	public:
		using promise_type = Detail::TaskPromise<T>;
		using value_type = T;
		
	private:
		struct AwaitableBase;

	public:
		Task() noexcept = default;
		explicit Task(std::coroutine_handle<promise_type> h) noexcept : mCoro(h) {}

		Task(Task&& other) noexcept : mCoro(std::exchange(other.mCoro, {})) {}
		Task& operator=(Task&& other) noexcept;

		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;

		~Task() { if (mCoro) mCoro.destroy(); }

		/// Query if the task result is complete.
		bool IsReady() const noexcept { return not mCoro or mCoro.done(); }

		bool Resume(); //? might want to remove this API

		auto operator co_await() const& noexcept;

		auto operator co_await() const&& noexcept;

		/// Returns an awaitable that will await completion of the task without
		/// attempting to retrieve the result.
		auto WhenReady() const noexcept;
	
	private:
		std::coroutine_handle<promise_type> mCoro{ nullptr };
	};

	//==========================================================================
	class SwitchToAsync
	{
	public:
		SwitchToAsync() = default;
		explicit SwitchToAsync(std::jthread& threadStorage)
			: mThread(&threadStorage) {}

		constexpr bool await_ready() const { return false; }
		
		void await_suspend(std::coroutine_handle<> handle) noexcept
		{
			if (mThread)
				*mThread = std::jthread([handle] { handle.resume(); });
			else
				std::thread([handle] { handle.resume(); }).detach(); // TODO: this can be application-specific Task Graph
		}

		void await_resume() noexcept {}

	private:
		std::jthread* mThread = nullptr;
	};

	//==========================================================================
	template<class T>
	class DataUpdate
	{
	public:
		DataUpdate() = default;

		void SetData(T newData)
		{
			bDirty = true;
			mData = std::move(newData);
			if (mWaitingCoro)
			{
				mWaitingCoro.resume();
			}
		}

		[[nodiscard]] JPL_INLINE bool IsDataReady() const noexcept { return bDirty; }
		[[nodiscard]] JPL_INLINE T GetData() noexcept
		{
			mWaitingCoro = {};
			bDirty = false;
			return std::move(mData);
		}

#if 0	//! we may or may not want proxy awaiter for this simple DataUpdate type
		bool await_ready() noexcept { return bDirty; }
		bool await_suspend(std::coroutine_handle<> h) noexcept
		{
			mWaitingCoro = h;
			return not bDirty;
		}

		T await_resume() noexcept
		{
			mWaitingCoro = {};
			bDirty = false;
			return std::move(mData);
		}

#else	
		struct Awaiter;
		Awaiter operator co_await() { return Awaiter{ *this }; }

	protected:
		struct Awaiter
		{
			DataUpdate& Data;

			bool await_ready() noexcept { return Data.IsDataReady(); }
			bool await_suspend(std::coroutine_handle<> h) noexcept
			{
				Data.mWaitingCoro = h;
				return not Data.IsDataReady();
			}

			T await_resume() noexcept
			{
				return Data.GetData();
			}
		};
#endif

	private:
		std::coroutine_handle<> mWaitingCoro;
		T mData;
		bool bDirty = false;
	};

	//==========================================================================
	class Notification
	{
	public:
		Notification() = default;

		void Notify()
		{
			if (mWaitingCoro)
				mWaitingCoro.resume();
		}

		struct Awaiter;
		Awaiter operator co_await() { return Awaiter{ *this }; }

	protected:
		struct Awaiter
		{
			Notification& Parent;

			JPL_INLINE bool await_ready() noexcept { return false; }
			JPL_INLINE void await_suspend(std::coroutine_handle<> h) noexcept { Parent.mWaitingCoro = h; }
			JPL_INLINE void await_resume() noexcept { Parent.mWaitingCoro = {}; }
		};

	private:
		std::coroutine_handle<> mWaitingCoro;
	};

	//==========================================================================
	class Flag
	{
	public:
		Flag() = default;

		JPL_INLINE void Set()
		{
			if (not bSet)
			{
				bSet = true;
				if (mWaitingCoro)
					mWaitingCoro.resume();
			}
		}

		JPL_INLINE void Reset() { bSet = false; }

		struct Awaiter;
		Awaiter operator co_await() { return Awaiter{ *this }; }

	protected:
		struct Awaiter
		{
			Flag& Parent;

			bool await_ready() noexcept { return Parent.bSet; }
			bool await_suspend(std::coroutine_handle<> h) noexcept
			{
				Parent.mWaitingCoro = h;
				return not Parent.bSet;
			}

			void await_resume() noexcept
			{
				Parent.bSet = false;
				Parent.mWaitingCoro = {};
			}
		};

	private:
		std::coroutine_handle<> mWaitingCoro;
		bool bSet = false;
	};

	//==========================================================================
	class MultiNotification
	{
	public:
		MultiNotification() = default;

		void Notify()
		{
			for (auto& coro : mWaitingCoros)
			{
				if (coro)
					coro.resume();
			}

			mWaitingCoros.clear();
		}

		struct Awaiter;
		Awaiter operator co_await() { return Awaiter{ *this }; }

	protected:
		struct Awaiter
		{
			MultiNotification& Parent;

			JPL_INLINE bool await_ready() noexcept { return false; }
			JPL_INLINE void await_suspend(std::coroutine_handle<> h) noexcept { Parent.mWaitingCoros.push_back(h); }
			JPL_INLINE void await_resume() noexcept {}
		};

	private:
		std::vector<std::coroutine_handle<>> mWaitingCoros;
	};

} // namespace JPL::Coro

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

namespace JPL::Coro
{
	namespace Detail
	{
		class TaskPromiseBase
		{
			friend struct FinalAwaitable;

			struct FinalAwaitable
			{
				bool await_ready() const noexcept { return false; }

				template<typename PromiseType>
				std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType> coro) noexcept
				{
					return coro.promise().mContinuation;
				}
				void await_resume() noexcept {}
			};

		public:
			TaskPromiseBase() noexcept = default;

			std::suspend_always initial_suspend() noexcept { return {}; }
			FinalAwaitable final_suspend() noexcept { return {}; }

			void set_continuation(std::coroutine_handle<> continuation) noexcept
			{
				mContinuation = continuation;
			}

		private:
			std::coroutine_handle<> mContinuation;
		};

		template<typename T>
		class TaskPromise final : public TaskPromiseBase
		{
		public:
			TaskPromise() noexcept = default;

			~TaskPromise()
			{
				switch (mResultType)
				{
				case EResultType::Value:
					mValue.~T();
					break;
				case EResultType::Exception:
					mException.~exception_ptr();
					break;
				default:
					break;
				}
			}

			Task<T> get_return_object() noexcept;

			void unhandled_exception() noexcept
			{
				::new (static_cast<void*>(std::addressof(mException))) std::exception_ptr(std::current_exception());
				mResultType = EResultType::Exception;
			}

			template<typename ValueType> requires (std::is_convertible_v<ValueType&&, T>)
				void return_value(ValueType&& value) noexcept(std::is_nothrow_constructible_v<T, ValueType&&>)
			{
				::new (static_cast<void*>(std::addressof(mValue))) T(std::forward<ValueType>(value));
				mResultType = EResultType::Value;
			}

			T& result()&
			{
				if (mResultType == EResultType::Exception)
				{
					std::rethrow_exception(mException);
				}

				JPL_ASSERT(mResultType == EResultType::Value);

				return mValue;
			}

			// HACK: Need to have co_await of task<int> return prvalue rather than
			// rvalue-reference to work around an issue with MSVC where returning
			// rvalue reference of a fundamental type from await_resume() will
			// cause the value to be copied to a temporary. This breaks the
			// sync_wait() implementation.
			// See https://github.com/lewissbaker/cppcoro/issues/40#issuecomment-326864107
			using rvalue_type = std::conditional_t<
				std::is_arithmetic_v<T> || std::is_pointer_v<T>,
				T,
				T&&>;

			rvalue_type result()&&
			{
				if (mResultType == EResultType::Exception)
				{
					std::rethrow_exception(mException);
				}

				JPL_ASSERT(mResultType == EResultType::Value);

				return std::move(mValue);
			}

		private:

			enum class EResultType { Empty, Value, Exception };
			EResultType mResultType = EResultType::Empty;

			union
			{
				T mValue;
				std::exception_ptr mException;
			};

		};

		template<>
		class TaskPromise<void> : public TaskPromiseBase
		{
		public:

			TaskPromise() noexcept = default;

			Task<void> get_return_object() noexcept;

			void return_void() noexcept {}

			void unhandled_exception() noexcept
			{
				mException = std::current_exception();
			}

			void result()
			{
				if (mException)
				{
					std::rethrow_exception(mException);
				}
			}

		private:

			std::exception_ptr mException;
		};

		template<typename T>
		class TaskPromise<T&> : public TaskPromiseBase
		{
		public:

			TaskPromise() noexcept = default;

			Task<T&> get_return_object() noexcept;

			void unhandled_exception() noexcept
			{
				mException = std::current_exception();
			}

			void return_value(T& value) noexcept
			{
				mValue = std::addressof(value);
			}

			T& result()
			{
				if (mException)
				{
					std::rethrow_exception(mException);
				}

				return *mValue;
			}

		private:

			T* mValue = nullptr;
			std::exception_ptr mException;
		};

		template<typename T>
		Task<T> TaskPromise<T>::get_return_object() noexcept
		{
			return Task<T>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
		}

		inline Task<void> TaskPromise<void>::get_return_object() noexcept
		{
			return Task<void>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
		}
		
		template<typename T>
		Task<T&> TaskPromise<T&>::get_return_object() noexcept
		{
			return Task<T&>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
		}

		// TODO: move to type traits header
		template<typename T>
		struct remove_rvalue_reference { using type = T; };
		template<typename T> struct remove_rvalue_reference<T&&> { using type = T; };
		template<typename T>
		using remove_rvalue_reference_t = typename remove_rvalue_reference<T>::type;

		template<class HandleType>
		concept CCoroutineHandle = requires (HandleType h)
		{
			{ h.promise() };
			{ HandleType::from_promise(h.promise()) } -> std::same_as<std::coroutine_handle<decltype(h.promise())>>;
		};

		template<class T>
		concept CValidAwaitSuspendReturnValue = std::is_void_v<T> or std::same_as<bool, T> or CCoroutineHandle<T>;


		template<class T>
		concept CAwaiter = requires (T a)
		{
			{ a.await_ready() };
			{ a.await_suspend(std::declval<std::coroutine_handle<>>()) };
			{ a.await_resume() };
			std::is_constructible_v<bool, decltype(a.await_ready())>;
			CValidAwaitSuspendReturnValue<decltype(a.await_suspend(std::declval<std::coroutine_handle<>>()))>;
		};

		template<CAwaiter T>
		struct AwaitableTraits
		{
			using awaiter_t = decltype(std::declval<T&&>().operator co_await()); // TODO: this might not work
			using await_result_t = decltype(std::declval<awaiter_t>().await_resume());
		};

	} // namespace Detail
	

	template<class T>
	struct Task<T>::AwaitableBase
	{
		std::coroutine_handle<promise_type> mCoro;

		AwaitableBase(std::coroutine_handle<promise_type> coroutine) noexcept
			: mCoro(coroutine)
		{}

		bool await_ready() const noexcept { return not mCoro || mCoro.done(); }

		std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
		{
			mCoro.promise().set_continuation(awaitingCoroutine);
			return mCoro;
		}
	};

	template<class T>
	inline Task<T>& Task<T>::operator=(Task<T>&& other) noexcept
	{
		if (this != &other)
		{
			if (mCoro)
				mCoro.destroy();
			mCoro = std::exchange(other.mCoro, {});
		}
		return *this;
	}

	template<class T>
	inline bool Task<T>::Resume()
	{
		if (mCoro and not mCoro.done())
		{
			mCoro.resume();
			return true;
		}
		return false;
	}

	template<class T>
	inline auto Task<T>::operator co_await() const& noexcept
	{
		struct Awaitable : AwaitableBase
		{
			using AwaitableBase::AwaitableBase;

			decltype(auto) await_resume()
			{
				if (not this->mCoro)
				{
					throw std::logic_error("broken promise");
				}

				return this->mCoro.promise().result();
			}
		};

		return Awaitable{ mCoro };
	}

	template<class T>
	inline auto Task<T>::operator co_await() const&& noexcept
	{
		struct Awaitable : AwaitableBase
		{
			using AwaitableBase::AwaitableBase;

			decltype(auto) await_resume()
			{
				if (not this->mCoro)
				{
					throw std::logic_error("broken promise");
				}

				return std::move(this->mCoro.promise()).result();
			}
		};

		return Awaitable{ mCoro };
	}

	template<class T>
	inline auto Task<T>::WhenReady() const noexcept
	{
		struct Awaitable : AwaitableBase
		{
			using AwaitableBase::AwaitableBase;

			void await_resume() const noexcept {}
		};

		return Awaitable{ mCoro };
	}

	template<typename AwaitableType>
	auto make_task(AwaitableType awaitable)
		-> Task<Detail::remove_rvalue_reference_t<typename Detail::AwaitableTraits<AwaitableType>::await_result_t>>
	{
		co_return co_await static_cast<AwaitableType&&>(awaitable);
	}
} // namespace JPL::Coro
