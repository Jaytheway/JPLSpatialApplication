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

#include "Coroutine/Coroutine.h"

#include <algorithm>
#include <concepts>
#include <iterator>
#include <vector>

namespace JPL
{
	namespace Intenral
	{
		template<class>
		inline constexpr bool always_false_v = false;
	}

	template<class T>
	class ChangeListener
	{
		static_assert(Intenral::always_false_v<T> && "Change Listener must be specialized.");
	};
	//======================================================================
	/// Generic Listener is a simple specialization of
	/// the templated ChangeListener<T> that doesn't have
	/// any specific callbacks, instead just being
	/// notified of OnChange(GenericChangeBroadcaster*)
	using GenericChangeListener = ChangeListener<void>;

	/// Generic Broadcaster is a simple specialization of
	/// the templated ChangeBroadcaster<T> whose listeners
	/// don't need any specific callbacks, instead just being
	/// notified of some change is sufficient
	class GenericChangeBroadcaster;

	template<>
	class ChangeListener<void>
	{
	public:
		virtual ~ChangeListener() = default;

		virtual void OnChange(GenericChangeBroadcaster* broadcaster) = 0;
	};

	//==========================================================================
	template<class Type>
	class ChangeBroadcaster
	{
	public:
		using ChangeListenerType = ChangeListener<Type>;

	public:
		virtual ~ChangeBroadcaster() = default;

		void AddListener(ChangeListenerType* listener)
		{
			if (listener == nullptr)
				return;

			if (std::ranges::find(mListeners, listener) == std::ranges::end(mListeners))
				mListeners.push_back(listener);
		}

		void RemoveListener(ChangeListenerType* listener)
		{
			if (listener != nullptr)
				std::erase(mListeners, listener);
		}

	protected:
		template<auto Function, class ...Args>
		void Broadcast(Args&&...args)
		{
			for (ChangeListenerType* listener : mListeners)
				std::invoke(Function, listener, std::forward<Args>(args)...);
		}

	protected:
		std::vector<ChangeListenerType*> mListeners;
	};

	//==========================================================================
	class GenericChangeBroadcaster : public ChangeBroadcaster<void>
	{
	public:
		inline void BroadcastChange()
		{
			ChangeBroadcaster<void>::Broadcast<&ChangeListenerType::OnChange>(this);
		}
	};

	//==========================================================================
	template<class T>
	class PropertyChangeBroadcaster
	{
	public:
		using CallbackFunction = void(*)(void* object, const T& propertyValue);
		
		struct Callback
		{
			void* Obj;
			CallbackFunction Function;

			inline bool operator==(const Callback& other) const
			{
				return Obj == other.Obj && Function == other.Function;
			}
		};

		void AddChangeCallback(Callback callback)
		{
			if (callback.Obj == nullptr || callback.Function == nullptr)
				return;

			if (std::ranges::find(mListeners, callback) == std::ranges::end(mListeners))
				mListeners.push_back(callback);
		}

		/// Add change callback function that takes pointer to the object as first argument
		/// and value T as second.
		template<class ObjectType, class CallbackFuntionType> requires (std::invocable<CallbackFuntionType, ObjectType*, const T&>)
		void AddChangeCallback(ObjectType* object, CallbackFuntionType callbackFunction)
		{
			if (object == nullptr || callbackFunction == nullptr)
				return;

			Callback callback{
				.Obj = object,
				.Function = [](void* obj, const T& property)
				{
					std::invoke(CallbackFuntionType{}, static_cast<ObjectType*>(obj), property);
				}
			};

			if (std::ranges::find(mListeners, callback) == std::ranges::end(mListeners))
				mListeners.push_back(callback);
		}

		template<auto MemberFunction, class ObjectType>
		void AddChangeCallback(ObjectType* obj)
		{
			AddChangeCallback(Callback{
				.Obj = obj,
				.Function = [](void* obj, const T& property)
				{
					ObjectType* p = static_cast<ObjectType*>(obj);
					return (p->*MemberFunction)(property);
				}
							  });
		}

		void RemoveChangeCallback(Callback callback)
		{
			std::erase(mListeners, callback);
		}

		template<class ObjectType>
		void RemoveChangeCallback(ObjectType* obj)
		{
			std::erase_if(mListeners, [obj](const Callback& callback) { return callback.Obj == obj; });
		}

	protected:
		void Broadcast(const T& propertyValue) const
		{
			for (const Callback& callback : mListeners)
				std::invoke(callback.Function, callback.Obj, propertyValue);
		}

	protected:
		std::vector<Callback> mListeners;
	};

	//==========================================================================
	template<class T>
	template<class T, class Equal = std::equal_to<T>>
	class Property : public PropertyChangeBroadcaster<T>
	{
	public:
		using EqualType = Equal;

		Property() = default;
		Property(const T& value) : mValue(value) {}
		//Property& operator=(const T& value) { Set(newValue); }
		//Property& operator=(const Property<T>& value) { Set(value.Get()); }

		void Set(const T& newValue)
		{
			if (not Equal{}(newValue, mValue))
			{
				mValue = newValue;
				PropertyChangeBroadcaster<T>::Broadcast(mValue);
			}
		}

		const T& Get() const { return mValue; }

		// Sometimes underlying value object may be changed outside of this Property,
		// (e.g. if the value is pointer and pointed to object's internal stange has 
		// changed)
		// This should be used cautiously, since it retriggers the update listeners
		// even when the value hasn't changed.
		void BroadcastUpdate() const { PropertyChangeBroadcaster<T>::Broadcast(mValue); }

		Coro::PropertyAwaiter<T> operator co_await() { return Coro::PropertyAwaiter<T>{ .Parent = *this }; }

	private:
		T mValue;
	};

	//======================================================================
	/// Add change listener to a set of properties.
	/// The calback function must take no arguments.
	template<auto MemberFunction, class ...Ts, class Listener>
	void AddChangeListener(Listener& listener, Property<Ts>& ...property)
	{
		auto onChangeOpaque = []<typename T>(void* self, const T&) { std::invoke(MemberFunction, static_cast<Listener*>(self)); };

		(property.AddChangeCallback({ &listener, onChangeOpaque }), ...);
	}

	/// Remove change listener from a set of properties.
	template<auto MemberFunction, class ...Ts, class Listener>
	void RemoveChangeListener(Listener& listener, Property<Ts>& ...property)
	{
		(property.RemoveChangeCallback(&listener), ...);
	}

	namespace Coro
	{
		/// Coroutine Awaiter for property update.
		/// Returns if the data was updated even if awaiter constructed after the udpate.
		/// Uses DataUpdate<T> "dirty" flag
		/// 
		/// Make sure the property outlives the awaitable object.
		template<class T>
		class PropertyUpdate final : public DataUpdate<T>
		{
		public:
			explicit PropertyUpdate(Property<T>& property)
				: mProperty(&property)
			{
				mProperty->AddChangeCallback<&PropertyUpdate<T>::SetData>(this);
				DataUpdate<T>::SetData(mProperty->Get()); // set initial data
			}

			~PropertyUpdate()
			{
				mProperty->RemoveChangeCallback(this);
			}

		private:
			Property<T>* mProperty;
		};
	} // namespace Coro
} // namespace JPL
