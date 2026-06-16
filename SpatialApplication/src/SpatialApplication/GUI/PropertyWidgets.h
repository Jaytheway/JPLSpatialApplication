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

#include "Application.h"
#include "ImGui/ImGui.h"

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>
#include <JPLSpatial/Math/SIMD.h>
#include <JPLSpatial/Math/MinimalVec3.h>

#include <functional>
#include <span>
#include <type_traits>

namespace JPL::GUI
{
	template<ImGuiEx::CSliderVType T, class...Args>
	bool PropertySlider(const char* label,
				const Undoable<T, Args...>& undoable,
				T minV, T maxV,
				const ImGuiEx::SliderConfig& config = { .Fmt = ImGuiEx::GetDefaultFmt<T>() });


	template<class ...Args>
	inline bool PropertyCheckbox(const char* label, const Undoable<bool, Args...>& undoable);

	// TODO: maybe rename to PropertySIMD but with some config options to draw as GEQ
	template<class...Args>
	uint32 PropertyGEQ(const char* label,
					   const Undoable<simd, Args...>& undoable,
					   const simd& frequencies,
					   float minValue, float maxValue,
					   const ImVec2& size = ImVec2{ 0.0f, 160.0f });

	template<std::floating_point T, class...Args>
	bool PropertyInput(const char* label,
					   const Undoable<T, Args...>& undoable,
					   const ImGuiEx::InputConfig<T>& config = {});

	template<std::integral T, class...Args>
	bool PropertyInput(const char* label,
					   const Undoable<T, Args...>& undoable,
					   const ImGuiEx::InputConfig<T>& config = { .Step = 1, .StepFast = 100, .Fmt = "%d" });

	template<class TransformPred, class InvTransformPred>
	struct PropertyTransform
	{
		TransformPred Transform;
		InvTransformPred InvTransform;
	};

	template<class Vec3Type, class...Args> requires (std::same_as<Vec3Type, MinimalVec3> or std::same_as<Vec3Type, Property<MinimalVec3>>)
	bool PropertyInputVec3(const char* label,
						   const Undoable<Vec3Type, Args...>& undoable,
						   const ImGuiEx::InputVec3Config& config = {});

	template<
		class Vec3Type,
		class TransformPred = std::identity,
		class InvTransformPred = std::identity,
		class...Args
	> requires (std::same_as<Vec3Type, MinimalVec3> or std::same_as<Vec3Type, Property<MinimalVec3>>)
	bool PropertyDragVec3(const char* label,
						  const Undoable<Vec3Type, Args...>& undoable,
						  const ImGuiEx::DragVec3Config& config = {},
						  const PropertyTransform<TransformPred, InvTransformPred>& transform = {});


	
	template<class T, class GetNameCb, class...Args> requires (ImGuiEx::CComboGetNameCb<GetNameCb, T>)
	bool PropertyCombo(const char* label,
					   const Undoable<T, Args...>& undoable,
					   int& selectedItemIndex,
					   std::span<const T> items,
					   const GetNameCb& getNameCb,
					   int popupMaxHeightInItems = -1);

} // namespace JPL::GUI

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

namespace JPL::GUI
{
	template<ImGuiEx::CSliderVType T, class...Args>
	bool PropertySlider(const char* label,
						const Undoable<T, Args...>& undoable,
						T minV, T maxV,
						const ImGuiEx::SliderConfig& config)
	{
		if (not JPL_ENSURE(undoable))
			return false;

		// Copy current value to modifiable cache
		std::optional<T> value = undoable.GetValue();

		if (not value)
			return false;

		// Actual slider widget
		const bool bModified = ImGuiEx::Slider(label, *value, minV, maxV, config);

		auto& history = JPLSpatialApplication::GetCommandHistory();

		// Begin property gesture edit before modifying property
		if (ImGui::IsItemActivated())
			history.BeginPropertyEdit(undoable, label);

		if (bModified)
			undoable.SetValue(*value);

		// End property edit gesture after modifying property the last time
		if (ImGui::IsItemDeactivated())
			history.EndPropertyEdit(undoable);

		return bModified;
	}

	template<class ...Args>
	inline bool PropertyCheckbox(const char* label, const Undoable<bool, Args...>& undoable)
	{
		std::optional<bool> value = undoable.GetValue();

		if (not value.has_value())
			return false;

		if (ImGuiEx::Checkbox(label, *value))
		{
			undoable.SetValue(*value);

			JPLSpatialApplication::GetCommandHistory()
				.PropertyEdited(Undoable(undoable), OldValue(not (*value)), label);

			return true;
		}
		else
		{
			return false;
		}
	}

	template<class...Args>
	uint32 PropertyGEQ(const char* label,
					   const Undoable<simd, Args...>& undoable,
					   const simd& frequencies,
					   float minValue, float maxValue,
					   const ImVec2& size)
	{
		if (not undoable)
			return 0;

		// Copy current value to modifiable cache
		std::optional<simd> value = undoable.GetValue();

		if (not value.has_value())
			return 0;

		const uint32 modifiedBand = ImGuiEx::DrawGEQ(label, *value, frequencies, minValue, maxValue, size);

		auto& history = JPLSpatialApplication::GetCommandHistory();

		// Begin property gesture edit before modifying property
		if (ImGui::IsItemActivated())
			history.BeginPropertyEdit(undoable, label);

		if (modifiedBand)
			undoable.SetValue(*value);

		// End property edit gesture after modifying property the last time
		if (ImGui::IsItemDeactivated())
			history.EndPropertyEdit(undoable);

		return modifiedBand;
	}

	template<std::floating_point T, class...Args>
	bool PropertyInput(const char* label,
					   const Undoable<T, Args...>& undoable,
					   const ImGuiEx::InputConfig<T>& config)
	{
		if (not undoable)
			return false;

		// Copy current value to modifiable cache
		std::optional<T> oldValue = undoable.GetValue();

		if (not oldValue.has_value())
			return false;

		T value = *oldValue;

		if (ImGuiEx::Input(label, value, config))
		{
			undoable.SetValue(value);

			JPLSpatialApplication::GetCommandHistory()
				.PropertyEdited(undoable, OldValue(*oldValue), label);

			return true;
		}
		else
		{
			return false;
		}
	}
	
	template<std::integral T, class...Args>
	bool PropertyInput(const char* label,
					   const Undoable<T, Args...>& undoable,
					   const ImGuiEx::InputConfig<T>& config)
	{
		if (not undoable)
			return false;

		// Copy current value to modifiable cache
		std::optional<T> oldValue = undoable.GetValue();

		if (not oldValue.has_value())
			return false;

		T value = *oldValue;

		if (ImGuiEx::Input(label, value, config))
		{
			undoable.SetValue(value);

			JPLSpatialApplication::GetCommandHistory()
				.PropertyEdited(undoable, OldValue(*oldValue), label);

			return true;
		}
		else
		{
			return false;
		}
	}

	template<class Vec3Type, class...Args> requires (std::same_as<Vec3Type, MinimalVec3> or std::same_as<Vec3Type, Property<MinimalVec3>>)
	bool PropertyInputVec3(const char* label,
						   const Undoable<Vec3Type, Args...>& undoable,
						   const ImGuiEx::InputVec3Config& config)
	{
		if (not undoable)
			return false;

		using T = typename Undoable<Vec3Type, Args...>::ValueType;

		// Copy current value to modifiable cache
		std::optional<T> oldValue = undoable.GetValue();

		if (not oldValue.has_value())
			return false;

		T value = *oldValue;

		if (ImGuiEx::InputVec3(label, value, config))
		{
			undoable.SetValue(value);

			JPLSpatialApplication::GetCommandHistory()
				.PropertyEdited(undoable, OldValue(*oldValue), label);

			return true;
		}
		else
		{
			return false;
		}
	}

	template<class Vec3Type, class TransformPred, class InvTransformPred, class...Args> requires (std::same_as<Vec3Type, MinimalVec3> or std::same_as<Vec3Type, Property<MinimalVec3>>)
	bool PropertyDragVec3(const char* label,
						  const Undoable<Vec3Type, Args...>& undoable,
						  const ImGuiEx::DragVec3Config& config,
						  const PropertyTransform<TransformPred, InvTransformPred>& transform)
	{
		if (not undoable)
			return false;

		using T = typename Undoable<Vec3Type, Args...>::ValueType;

		// Copy current value to modifiable cache
		std::optional<T> oldValue = undoable.GetValue();

		if (not oldValue.has_value())
			return false;

		// TODO: we might want property transform in base ImGuiEx widget
		T value = std::invoke(transform.Transform, *oldValue);

		const bool bModified = ImGuiEx::DragVec3(label, value, config);
		
		auto& history = JPLSpatialApplication::GetCommandHistory();

		// Begin property gesture edit before modifying property
		if (ImGui::IsItemActivated())
			history.BeginPropertyEdit(undoable, label);

		if (bModified)
			undoable.SetValue(std::invoke(transform.InvTransform, value));

		// End property edit gesture after modifying property the last time
		if (ImGui::IsItemDeactivated())
			history.EndPropertyEdit(undoable);

		return bModified;
	}

	template<class T, class GetNameCb, class...Args> requires (ImGuiEx::CComboGetNameCb<GetNameCb, T>)
	bool PropertyCombo(const char* label,
					   const Undoable<T, Args...>& undoable,
					   int& selectedItemIndex,
					   std::span<const T> items,
					   const GetNameCb& getNameCb,
					   int popupMaxHeightInItems)
	{
		if (not undoable)
			return false;

		// Cache old index before possible modification
		const int oldIndex = selectedItemIndex;

		const bool bModified = ImGuiEx::Combo(label,
											  selectedItemIndex,
											  items,
											  getNameCb,
											  popupMaxHeightInItems);

		if (bModified)
		{
			// Copy current value to modifiable cache
			std::optional<T> oldValue = undoable.GetValue();

			undoable.SetValue(items[selectedItemIndex]);

			JPLSpatialApplication::GetCommandHistory()
				.PropertyEdited(undoable, OldValue(oldValue ? *oldValue : items[oldIndex]), label);
		}

		return bModified;
	}
} // namespace JPL::GUI
