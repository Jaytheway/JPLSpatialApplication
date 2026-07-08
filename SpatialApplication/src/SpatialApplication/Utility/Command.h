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

#include "CoreInclude.h"
#include "MVCUtils.h"
#include "Log.h"

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_iostream.hpp>

#include <concepts>
#include <ios>
#include <memory>
#include <string_view>
#include <vector>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace JPL
{
	//======================================================================
	/// Command that can be executed
	class ICommand
	{
	public:
		virtual ~ICommand() noexcept = default;

		/// Execute the command
		virtual void Execute() = 0;

		/// @returns command name
		virtual std::string_view GetName() const = 0;
	};

	//======================================================================
	/// Command that can be executed and undone
	class IUndoableCommand : public ICommand
	{
	public:
		/// Execute undo operation
		virtual void Undo() = 0;

		/// Optionally print state of an undoable command
		/// @returns true, if something was printed to the output stream
		virtual bool PrintTo(std::ostream& output) const { output << GetName(); return true; };
	};

	//======================================================================
	namespace EUndoableType
	{
		struct Value{};
		struct Property{};
		struct ModelValue{};
		struct ModelProperty{};
	}

	template<class T>
	concept CUndoableDataType =
		std::same_as<T, EUndoableType::Value> or
		std::same_as<T, EUndoableType::Property> or
		std::same_as<T, EUndoableType::ModelValue> or
		std::same_as<T, EUndoableType::ModelProperty>;

	//======================================================================
	/// Undoable is an access wrapper for some kind of object with shared
	/// lifetime, which can be safely used in undo/redo Command History.
	/// 
	/// Used usually as a function parameter like:
	/// 
	/// CommandHistory::PropertyEdited(Undoable(sharedPtr), OldValue(oldValue))
	/// 
	/// ...Undoable internally stores a weak_ptr to the property owner,
	/// or the property itself, depending on the type of Undoable.
	template<class T, CUndoableDataType Type, class Model = void*>
	struct Undoable;

	/// Undoable std::shared_ptr<T>
	template<class T>
	struct Undoable<T, EUndoableType::Value>;

	/// Undoable std::shared_ptr<Property<T>>
	template<class T>
	struct Undoable<T, EUndoableType::Property>;

	/// Undoable member poitner T of std::shared_ptr<ModelType>
	template<class T, class Model>
	struct Undoable<T, EUndoableType::ModelValue, Model>;

	/// Undoable member poitner Property<T> of std::shared_ptr<ModelType>
	template<class T, class Model>
	struct Undoable<T, EUndoableType::ModelProperty, Model>;

	//======================================================================
	/// Deduction guides for cleaner calling syntax Undoable(Ts..)

	template<class T>
	Undoable(const std::shared_ptr<T>&) -> Undoable<T, EUndoableType::Value>;

	template<class T>
	Undoable(const std::shared_ptr<Property<T>>&) -> Undoable<T, EUndoableType::Property>;

	template<class T, class Model>
	Undoable(const std::shared_ptr<Model>&, T Model::*) -> Undoable<T, EUndoableType::ModelValue, Model>;

	template<class T, class Model>
	Undoable(const std::shared_ptr<Model>&, Property<T> Model::*) -> Undoable<T, EUndoableType::ModelProperty, Model>;

	//======================================================================
	/// Interface for property undo command where the undo point has to be
	/// captured at end of a continuous edit and the undo action would undo
	/// to the initial value before the edit began.
	class IPropertyEdit
	{
	public:
		virtual ~IPropertyEdit() noexcept = default;

		/// Check if this IPropertyEdit holds given undoable object
		template<class T, class...Args>
		bool IsA(const Undoable<T, Args...>& undoable)
		{
			return undoable and undoable.GetDataAddress() == GetPropertyAddress();
		}

		virtual std::unique_ptr<IUndoableCommand> CreateCommandIfModified() const = 0;

	private:
		virtual void* GetPropertyAddress() const = 0;
	};

	//======================================================================
	// Forcing stongly typed undoable command parameters
	template<class T> struct OldValue { T Value; };
	template<class T> struct NewValue { T Value; };

	//======================================================================
	/// History of Undoable property changes.
	/// Changes can be undone and redone.
	class CommandHistory
	{
	public:
		CommandHistory();

		/// Add undoable command to the history
		void Add(std::unique_ptr<IUndoableCommand> command);

		/// Add Undoable modification to the history
		template<class T, CUndoableDataType Type, class Model>
		void PropertyEdited(const Undoable<T, Type, Model>& undoable, OldValue<T>&& initialValue, std::string_view propertyLabel = {});

		/// Begin a continuous edit of an Undoable
		template<class T, CUndoableDataType Type, class Model>
		void BeginPropertyEdit(const Undoable<T, Type, Model>& undoable, std::string_view propertyLabel = {});

		/// End a continuous edit of an Undoable and add a record
		/// to the history if the value actually changed.
		template<class T, CUndoableDataType Type, class Model>
		void EndPropertyEdit(const Undoable<T, Type, Model>& undoable);

		/// Clear the history and any pending edits
		void Clear();

		/// Undo the last Undoable modification in the history
		void Undo();

		/// Redo the last undone modification in the history
		void Redo();
		
	private:
		void CommitCurrentEdit();
		inline void PrintUndo(IUndoableCommand& command);
		inline void PrintRedo(IUndoableCommand& command);

	private:
		std::vector<std::unique_ptr<IUndoableCommand>> mUndoStack;
		std::vector<std::unique_ptr<IUndoableCommand>> mRedoStack;

		std::unique_ptr<IPropertyEdit> mCurrentEdit;

		// Reusing this buffer for printing commands undo/redo
		std::ostringstream mPrintBuffer;
	};

} // namespace JPL

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

namespace JPL
{
	// Implements IUndoableCommand for any kind of Undoable property edit.
	// Used internally when adding a record to Command History.
	template<class T, CUndoableDataType Type, class Model = void*>
	class PropertyEditCommand;

	// Implements IPropertyEdit for Undoables
	template<class T, CUndoableDataType Type, class Model = void*>
	class PropertyEdit;

	//======================================================================
	inline CommandHistory::CommandHistory()
	{
		mPrintBuffer << std::boolalpha; // print true/false instead of 1/0
	}

	inline void CommandHistory::Add(std::unique_ptr<IUndoableCommand> command)
	{
		if (command)
		{
			mUndoStack.emplace_back(std::move(command));
			mRedoStack.clear();
		}
	}

	template<class T, CUndoableDataType Type, class Model>
	inline void CommandHistory::PropertyEdited(const Undoable<T, Type, Model>& undoable, OldValue<T>&& initialValue, std::string_view propertyLabel)
	{
		Add(std::unique_ptr<IUndoableCommand>(new PropertyEditCommand(undoable, std::move(initialValue), propertyLabel)));
	}

	template<class T, CUndoableDataType Type, class Model>
	inline void CommandHistory::BeginPropertyEdit(const Undoable<T, Type, Model>& undoable, std::string_view propertyLabel)
	{
		// We may want to reconsider whether the old edit should be committed
		// or cancelled if the situation ever occurs when we start a new edit
		// while having pending old edit
		CommitCurrentEdit();
		mCurrentEdit.reset(new PropertyEdit(undoable, propertyLabel));
	}

	template<class T, CUndoableDataType Type, class Model>
	inline void CommandHistory::EndPropertyEdit(const Undoable<T, Type, Model>& undoable)
	{
		if (mCurrentEdit and JPL_ENSURE(mCurrentEdit->IsA(undoable)))
			CommitCurrentEdit();
	}

	inline void CommandHistory::Clear()
	{
		mCurrentEdit.reset();
		mUndoStack.clear();
		mRedoStack.clear();
	}

	inline void CommandHistory::Undo()
	{
		CommitCurrentEdit();

		if (not mUndoStack.empty() and mUndoStack.back())
		{
			auto& lastCommand = mUndoStack.back();

			// First print
			PrintUndo(*lastCommand);

			// Then undo
			lastCommand->Undo();

			// Move to redo stack
			mRedoStack.emplace_back(std::move(lastCommand));
			mUndoStack.pop_back();
		}
	}

	inline void CommandHistory::Redo()
	{
		if (not mRedoStack.empty() and mRedoStack.back())
		{
			auto& lastCommand = mRedoStack.back();
			
			// Print
			PrintRedo(*lastCommand);
			
			// Redo
			lastCommand->Execute();

			// Move to undo stack
			mUndoStack.emplace_back(std::move(lastCommand));
			mRedoStack.pop_back();
		}
	}

	inline void CommandHistory::CommitCurrentEdit()
	{
		if (mCurrentEdit)
		{
			auto edit = std::move(mCurrentEdit);
			Add(edit->CreateCommandIfModified());
		}
	}

	inline void CommandHistory::PrintUndo(IUndoableCommand& command)
	{
		mPrintBuffer.str(""); // clear buffer
		mPrintBuffer.clear();
		if (command.PrintTo(mPrintBuffer));
			Log::Trace("Undo: {}", mPrintBuffer.view());
	}

	inline void CommandHistory::PrintRedo(IUndoableCommand& command)
	{
		mPrintBuffer.str("");
		mPrintBuffer.clear();
		if (command.PrintTo(mPrintBuffer))
			Log::Trace("Redo: {}", mPrintBuffer.view());
	}
	
	//======================================================================
	template<class T>
	struct Undoable<T, EUndoableType::Value> final
	{
		using ValueType = T;

		Undoable(const std::shared_ptr<T>& value)
			: mSharedValue(value)
		{}

		void* GetDataAddress() const
		{
			const auto sharedValue = mSharedValue.lock();
			return sharedValue ? std::addressof(*sharedValue) : nullptr;
		}

		std::optional<T> GetValue() const
		{
			const auto sharedValue = mSharedValue.lock();
			return sharedValue ? *sharedValue : std::optional<T>{};
		}

		bool SetValue(const T& newValue) const
		{
			if (const auto property = mSharedValue.lock())
			{
				*property = newValue;
				return true;
			}
			return false;
		}

		inline operator bool() const { return GetDataAddress() != nullptr; }

	private:
		std::weak_ptr<T> mSharedValue;
	};

	template<class T>
	struct Undoable<T, EUndoableType::Property> final
	{
		using ValueType = T;

		Undoable(const std::shared_ptr<Property<T>>& property)
			: mSharedProperty(property)
		{}

		void* GetDataAddress() const
		{
			const auto sharedValue = mSharedProperty.lock();
			return sharedValue ? std::addressof(*sharedValue) : nullptr;
		}

		std::optional<T> GetValue() const
		{
			const auto sharedValue = mSharedProperty.lock();
			return sharedValue ? sharedValue->Get() : std::optional<T>{};
		}

		bool SetValue(const T& newValue) const
		{
			if (const auto property = mSharedProperty.lock())
			{
				property->Set(newValue);
				return true;
			}
			return false;
		}

		inline operator bool() const { return GetDataAddress() != nullptr; }

	private:
		std::weak_ptr<Property<T>> mSharedProperty;
	};

	template<class T, class Model>
	struct Undoable<T, EUndoableType::ModelValue, Model> final
	{
		using ValueType = T;

		Undoable(const std::shared_ptr<Model>& model, T Model::* valuePtr)
			: mModel(valuePtr ? model : nullptr)
			, mValuePtr(model ? valuePtr : nullptr)
		{}

		void* GetDataAddress() const
		{
			const auto model = mModel.lock();
			return model ? std::addressof((*model).*mValuePtr) : nullptr;
		}

		std::optional<T> GetValue() const
		{
			const auto model = mModel.lock();
			return model ? (*model).*mValuePtr : std::optional<T>{};
		}

		bool SetValue(const T& newValue) const
		{
			if (const auto model = mModel.lock())
			{
				(*model).*mValuePtr = newValue;

				// If model implements change broadcasting interface, like GenericChangeBroadcaster,
				// we can let listeners know that value was modified.
				// 
				// This may be undesireable in some cases,
				// if those cases arise, we may switch to more of an "opt-in" approach,
				// or even broadcast all undo/redo operations from CommandHistory
				// passing property address as parameters to let listeners
				// determine if they want to react to that change.
				// However, that could result in many "empty" calls to listeners
				// that don't care about given undo/redo operation.
				if constexpr (requires(Model& object) { object.BroadcastChange(); })
				{
					model->BroadcastChange();
				}

				return true;
			}
			return false;
		}

		inline operator bool() const { return GetDataAddress() != nullptr; }
	
	private:
		std::weak_ptr<Model> mModel;
		T Model::* mValuePtr;
	};

	template<class T, class Model>
	struct Undoable<T, EUndoableType::ModelProperty, Model> final
	{
		using ValueType = T;

		Undoable(const std::shared_ptr<Model>& model, Property<T> Model::* propertyPtr)
			: mModel(propertyPtr ? model : nullptr)
			, mPropertyPtr(model ? propertyPtr : nullptr)
		{}

		void* GetDataAddress() const
		{
			const auto model = mModel.lock();
			return model ? std::addressof((*model).*mPropertyPtr) : nullptr;
		}

		std::optional<T> GetValue() const
		{
			const auto model = mModel.lock();
			return model ? ((*model).*mPropertyPtr).Get() : std::optional<T>{};
		}

		bool SetValue(const T& newValue) const
		{
			if (const auto model = mModel.lock())
			{
				((*model).*mPropertyPtr).Set(newValue);
				return true;
			}
			return false;
		}

		inline operator bool() const { return GetDataAddress() != nullptr; }

	private:
		std::weak_ptr<Model> mModel;
		Property<T> Model::* mPropertyPtr;
	};
	
	//======================================================================
	template<class T, CUndoableDataType Type, class Model>
	class PropertyEditCommand final : public IUndoableCommand
	{
	public:
		using UndoableType = Undoable<T, Type, Model>;

		PropertyEditCommand(Undoable<T, Type, Model> undoable, OldValue<T>&& oldValue, std::string_view propertyLabel = "")
			: mDataEdit(std::move(undoable))
			, mOldValue(std::move(oldValue.Value))
			, mNewValue(mDataEdit.GetValue())
			, mPropertyLabel(propertyLabel)
		{}

		void Execute() override { if (mNewValue) mDataEdit.SetValue(*mNewValue); }
		void Undo() override { if (mNewValue) mDataEdit.SetValue(mOldValue); }

		std::string_view GetName() const override { return "Property Edit Command"; }

		bool PrintTo(std::ostream& output) const override
		{
			if (not mNewValue)
				return false;

			// Lets us print enums
			using magic_enum::ostream_operators::operator<<;

			if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>)
			{
				auto printValueOrNULL = [&output](const auto* ptr)
				{
					if (ptr) output << *ptr;
					else output << "NULL";
				};

				if (mPropertyLabel.empty())
				{
					// "Property edit [old value] -> [new value]"
					output << "Property edit" << " [";
					printValueOrNULL(mOldValue);
					output << "] -> [";
					printValueOrNULL(mNewValue.value());
					output << ']';
				}
				else
				{
					// "Set `Property Label` [old value] -> [new value]"
					output << "Set '" << mPropertyLabel << "' [";
					printValueOrNULL(mOldValue);
					output << "] -> [";
					printValueOrNULL(mNewValue.value());
					output << ']';
				}
			}
			else
			{
				if (mPropertyLabel.empty())
				{
					output << "Property edit"
						<< " ["
						<< mOldValue
						<< "] -> ["
						<< *mNewValue
						<< ']';
				}
				else
				{
					output << "Set '"
						<< mPropertyLabel
						<< "' ["
						<< mOldValue
						<< "] -> ["
						<< *mNewValue
						<< ']';
				}
			}

			return true;
		}

	private:
		UndoableType mDataEdit;
		T mOldValue;
		std::optional<T> mNewValue;
		// we can't use view, because stack may be clear before we print the command
		std::string mPropertyLabel;
	};

	//======================================================================
	template<class T, CUndoableDataType Type, class Model>
	class PropertyEdit final : public IPropertyEdit
	{
	public:
		using UndoableType = Undoable<T, Type, Model>;

		PropertyEdit(Undoable<T, Type, Model> undoable, std::string_view propertyLabel = "")
			: mUndoable(std::move(undoable))
			, mInitialValue(mUndoable ? mUndoable.GetValue() : std::optional<T>{})
			, mPropertyLabel(propertyLabel)
		{}

		std::unique_ptr<IUndoableCommand> CreateCommandIfModified() const override
		{
			if (not mInitialValue.has_value())
				return {};

			if (std::optional<T> currentValue = mUndoable.GetValue())
			{
				if (not std::equal_to<T>{}(*currentValue, *mInitialValue))
				{
					return std::unique_ptr<IUndoableCommand>(
						new PropertyEditCommand(mUndoable, OldValue(*mInitialValue), mPropertyLabel));
				}
			}

			return {};
		}

	private:
		void* GetPropertyAddress() const override
		{
			return mUndoable.GetDataAddress();
		}

	private:
		UndoableType mUndoable;
		std::optional<T> mInitialValue;
		// we can't use view, because stack may be clear before we print the command
		std::string mPropertyLabel;
	};
} // namespace JPL
