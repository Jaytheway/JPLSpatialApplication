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

#include "MVCUtils.h"
#include "Log.h"

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>

#include <magic_enum/magic_enum.hpp>

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
		virtual bool PrintTo(std::ostream& output) const { return false; };
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
		command.PrintTo(mPrintBuffer);
		Log::Trace("Undo: {}", mPrintBuffer.view());
	}

	inline void JPL::CommandHistory::PrintRedo(IUndoableCommand& command)
	{
		mPrintBuffer.str("");
		mPrintBuffer.clear();
		command.PrintTo(mPrintBuffer);
		Log::Trace("Redo: {}", mPrintBuffer.view());
	}
	
	//======================================================================
	template<class T>
	struct Undoable<T, EUndoableType::Value> final
	{
		Undoable(const std::shared_ptr<T>& value)
			: SharedValue(value)
		{}

		void* GetDataAddress() const
		{
			const auto sharedValue = SharedValue.lock();
			return sharedValue ? std::addressof(*sharedValue) : nullptr;
		}

		std::optional<T> GetValue() const
		{
			const auto sharedValue = SharedValue.lock();
			return sharedValue ? *sharedValue : std::optional<T>{};
		}

		bool SetValue(const T& newValue) const
		{
			if (const auto property = SharedValue.lock())
			{
				*property = newValue;
				return true;
			}
			return false;
		}

		inline operator bool() const { return GetDataAddress() != nullptr; }

		std::weak_ptr<T> SharedValue;
	};

	template<class T>
	struct Undoable<T, EUndoableType::Property> final
	{
		Undoable(const std::shared_ptr<Property<T>>& property)
			: SharedProperty(property)
		{}

		void* GetDataAddress() const
		{
			const auto sharedValue = SharedProperty.lock();
			return sharedValue ? std::addressof(*sharedValue) : nullptr;
		}

		std::optional<T> GetValue() const
		{
			const auto sharedValue = SharedProperty.lock();
			return sharedValue ? sharedValue->Get() : std::optional<T>{};
		}

		bool SetValue(const T& newValue) const
		{
			if (const auto property = SharedProperty.lock())
			{
				property->Set(newValue);
				return true;
			}
			return false;
		}

		inline operator bool() const { return GetDataAddress() != nullptr; }

		std::weak_ptr<Property<T>> SharedProperty;
	};

	template<class T, class Model>
	struct Undoable<T, EUndoableType::ModelValue, Model> final
	{
		Undoable(const std::shared_ptr<Model>& model, T Model::* valuePtr)
			: Model(valuePtr ? model : nullptr)
			, ValuePtr(model ? valuePtr : nullptr)
		{}

		void* GetDataAddress() const
		{
			const auto model = Model.lock();
			return model ? std::addressof((*model).*ValuePtr) : nullptr;
		}

		std::optional<T> GetValue() const
		{
			const auto model = Model.lock();
			return model ? (*model).*ValuePtr : std::optional<T>{};
		}

		bool SetValue(const T& newValue) const
		{
			if (const auto model = Model.lock())
			{
				(*model).*ValuePtr = newValue;
				return true;
			}
			return false;
		}

		inline operator bool() const { return GetDataAddress() != nullptr; }

		std::weak_ptr<Model> Model;
		T Model::* ValuePtr;
	};

	template<class T, class Model>
	struct Undoable<T, EUndoableType::ModelProperty, Model> final
	{
		Undoable(const std::shared_ptr<Model>& model, Property<T> Model::* propertyPtr)
			: Model(propertyPtr ? model : nullptr)
			, PropertyPtr(model ? propertyPtr : nullptr)
		{}

		void* GetDataAddress() const
		{
			const auto model = Model.lock();
			return model ? std::addressof((*model).*PropertyPtr) : nullptr;
		}

		std::optional<T> GetValue() const
		{
			const auto model = Model.lock();
			return model ? ((*model).*PropertyPtr).Get() : std::optional<T>{};
		}

		bool SetValue(const T& newValue) const
		{
			if (const auto model = Model.lock())
			{
				((*model).*PropertyPtr).Set(newValue);
				return true;
			}
			return false;
		}

		inline operator bool() const { return GetDataAddress() != nullptr; }

		std::weak_ptr<Model> Model;
		Property<T> Model::* PropertyPtr;
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

			auto printValue = [&]
			{
				if (mPropertyLabel.empty())
					output << "Property edit" << " [" << mOldValue << "] -> [" << *mNewValue << ']';
				else
					output << "Set '" << mPropertyLabel << "' [" << mOldValue << "] -> [" << *mNewValue << ']';
			};

			if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>)
			{
				// If either is a nullptr, print as pointer
				if (mOldValue == nullptr or mNewValue.value() == nullptr)
				{
					printValue();
				}
				else // if both pointers are valid, assume we need to print value
				{
					if (mPropertyLabel.empty())
						output << "Property edit" << " [" << *mOldValue << "] -> [" << *(mNewValue.value()) << ']';
					else
						output << "Set '" << mPropertyLabel << "' [" << *mOldValue << "] -> [" << *(mNewValue.value()) << ']';
				}
			}
			else
			{
				printValue();
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
