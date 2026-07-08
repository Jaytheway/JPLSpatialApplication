//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatial
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
#include "Utility/MVCUtils.h"

#include "ImGui/ImGui.h"

#include <Walnut/Image.h>

#include <JPLSpatial/Math/SIMD.h>
#include <JPLSpatial/Math/MinimalVec3.h>

#include <concepts>
#include <span>
#include <format>
#include <string>
#include <optional>

namespace JPL::ImGuiEx
{
    bool Checkbox(const char* label, bool& value);

    bool Checkbox(const char* label, Property<bool>& property);

	//==========================================================================
	/// Button Image

	void DrawButtonImage(const Walnut::Image& imageNormal, const Walnut::Image& imageHovered, const Walnut::Image& imagePressed,
						 ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax,
						 ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f));

	inline void DrawButtonImage(const Walnut::Image& imageNormal, const Walnut::Image& imageHovered, const Walnut::Image& imagePressed,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max, uv0, uv1);
	};

	inline void DrawButtonImage(const Walnut::Image& image,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
								ImVec2 rectMin, ImVec2 rectMax,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectMin, rectMax, uv0, uv1);
	};

	inline void DrawButtonImage(const Walnut::Image& image,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
								ImRect rectangle,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max, uv0, uv1);
	};


	inline void DrawButtonImage(const Walnut::Image& imageNormal, const Walnut::Image& imageHovered, const Walnut::Image& imagePressed,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), uv0, uv1);
	};

	inline void DrawButtonImage(const Walnut::Image& image,
								ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
								ImVec2 uv0 = ImVec2(0.0f, 0.0f), ImVec2 uv1 = ImVec2(1.0f, 1.0f))
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), uv0, uv1);
	};

	//==========================================================================
	bool PathButton(const char* label, ImVec2 size = ImVec2(0.0f, 0.0f));

	//==========================================================================
	/// Window Buttons
	
	void DrawMainWindowButtons(ImVec2 titlebarMin, ImVec2 titlebarMax);

	// Render window close button
	bool CloseButton(const char* id, const ImVec2& size);

	//==========================================================================
	/// Icon Button

	struct IconButtonStyle
	{
		ImU32 ColourNormal = GUI::Colours::Theme::Text;
		ImU32 ColourHovered = GUI::Colours::Theme::TextBrighter;
		ImU32 ColourPressed = GUI::Colours::Theme::TextDarker;

		ImU32 BgColourNormal = IM_COL32_BLACK_TRANS;
		ImU32 BgColourHovered = IM_COL32_BLACK_TRANS;
		ImU32 BgColourPressed = IM_COL32_BLACK_TRANS;

		bool bDrawBorder = false;
		ImU32 BorderColour = GUI::Colours::Theme::Border;

		ImVec2 ButtonSize = ImVec2(0.0f, 0.0f);
	};

	// Predefined IconButton styles
	namespace IconStyle
	{
		extern const IconButtonStyle cIconLabelBgHovered;
		extern const IconButtonStyle cIconLabelBgAlways;
	} // namespace IconStyle

	bool IconButton(const char* label, const IconButtonStyle& style = {});
	inline bool IconButton(const char8_t* label, const IconButtonStyle& style = {}) { return IconButton((const char*)(label), style); }

	//==========================================================================
	/// Button

	/// Pretty much identical to ImGui::Button, but with label offset when pressed
	bool Button(const char* label, const ImVec2& size = ImVec2(0, 0));

	//==========================================================================
	/// Graphic EQ widget with vertical sliders for frequency band gains
	/// @param frequencies if size = values.size -> treated as frequency band
	/// centers; if the size = values.size - 1 -> treated as split frequencies;
	/// other sizes are invalid.
	/// 
	/// @returns band that was modified (index + 1), 0 if not modified
	uint32 DrawGEQ(const char* itemId, std::span<float> values, std::span<const float> frequencies, float minValue, float maxValue, const ImVec2& size = ImVec2{ 0.0f, 160.0f });
	
	uint32 DrawGEQ(const char* itemId, simd& values, const simd& frequencies, float minValue, float maxValue, const ImVec2& size = ImVec2{ 0.0f, 160.0f });
	
	uint32 DrawGEQ(const char* itemId, Property<simd>& values, const simd& frequencies, float minValue, float maxValue, const ImVec2& size = ImVec2{ 0.0f, 160.0f });

	//==========================================================================
	/// Text

	template <class... Types>
	void TextFormatted(std::string& buffer, const std::format_string<Types...> fmt, Types&&...args)
	{
		buffer.clear();
		std::format_to(std::back_inserter(buffer), fmt, std::forward<Types>(args)...);
		ImGui::TextUnformatted(buffer.c_str());
	}

	// Simple utility to draw formatted text.
	// It reuses its own string buffer to avoid allocatoins,
	// and is ment to be used as a function local static variable.
	struct TextPrinter
	{
		template <class... Types>
		void Print(const std::format_string<Types...> fmt, Types&&...args)
		{
			TextFormatted(Buffer, fmt, std::forward<Types>(args)...);
		}

		std::string Buffer{ 64, 0 };
	};


	//==========================================================================
	/// Sliders
	
	template<class T>
	concept CSliderVType = std::floating_point<T> or std::integral<T>;

	template<CSliderVType T>
	constexpr const char* GetDefaultFmt();
	
	struct SliderConfig
	{
		const char* Fmt = "%.3f";
		ImGuiSliderFlags Flags = 0;

		// We won't put min/max in config for now,
		// because it would make it easy to forget
		// to specify the values, which are required Slider widget
	};

	template<CSliderVType T>
	bool Slider(const char* label, T& value, T minV, T maxV, const SliderConfig& config = { .Fmt = GetDefaultFmt<T>() });

	template<CSliderVType T>
	bool Slider(const char* label, Property<T>& property, T minV, T maxV, const SliderConfig& config = { .Fmt = GetDefaultFmt<T>() });

	//==========================================================================
	/// Input
	
	template<class T>
	struct InputConfig
	{
		T Step{ 0 };
		T StepFast{ 0 };
		const char* Fmt = "%.3f";
		ImGuiInputTextFlags Flags = 0;

		std::optional<T> Min;
		std::optional<T> Max;
	};

	template<std::floating_point T>
	bool Input(const char* label, T& v, const InputConfig<T>& config = {});

	template<std::integral T>
	inline bool Input(const char* label, T& v, const InputConfig<T>& config = { .Step = 1, .StepFast = 100, .Fmt = "%d" });

	template<std::floating_point T>
	bool Input(const char* label, Property<T>& property, const InputConfig<T>& config = {});

	template<std::integral T>
	inline bool Input(const char* label, Property<T>& property, const InputConfig<T>& config = { .Step = 1, .StepFast = 100, .Fmt = "%d" });


	//==========================================================================
	/// Vec3 widgets

	struct PropertyVec3Config
	{
		const char* Fmt = "%.1f";

		std::optional<MinimalVec3> Min;
		std::optional<MinimalVec3> Max;
	};

	struct InputVec3Config
	{
		PropertyVec3Config Base;
		ImGuiInputTextFlags Flags = 0;
	};

	struct DragVec3Config
	{
		PropertyVec3Config Base;
		float VSpead = 1.0f;
		ImGuiSliderFlags Flags = 0;
	};

	bool InputVec3(const char* label, MinimalVec3& value, const ImGuiEx::InputVec3Config& config = {});
	bool InputVec3(const char* label, Property<MinimalVec3>& property, const ImGuiEx::InputVec3Config& config = {});
	bool DragVec3(const char* label, MinimalVec3& value, const ImGuiEx::DragVec3Config& config = {});
	bool DragVec3(const char* label, Property<MinimalVec3>& property, const ImGuiEx::DragVec3Config& config = {});

	
	//==========================================================================
	/// Spacers & Separators

	struct Spacer {};
	struct Separator {};

	template<class T>
	concept CLayoutItem = std::same_as<T, Spacer> or std::same_as<T, Separator>;

	template<CLayoutItem...Ts>
	void Layout()
	{
		auto drawElement = []<typename T>()
		{
			if constexpr (std::same_as<T, Spacer>)
			{
				ImGui::Spacing();
			}
			else
			{
				ImGui::Separator();
			}
		};

		(drawElement.operator()<Ts>(),...);
	}


	//==========================================================================
	/// Combo boxes

	template<class Callback, class DataType>
	concept CComboGetNameCb = std::is_invocable_r_v<const char*, Callback, const DataType&, int>;

	template<class T, class GetNameCb, class...Args> requires (CComboGetNameCb<GetNameCb, T>)
	bool Combo(const char* label,
				int& selectedItemIndex,
				std::span<const T> items,
				const GetNameCb& getNameCb,
				int popupMaxHeightInItems = -1);

} // namespace JPL::ImGuiEx

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

namespace JPL::ImGuiEx
{
	template<class T>
	constexpr ImGuiDataType GetImGuiDataTypeFrom()
	{
		if constexpr (std::same_as<T, float>)
		{
			return ImGuiDataType_Float;
		}
		else if constexpr (std::same_as<T, int>)
		{
			return ImGuiDataType_S32;
		}
		else if constexpr (std::same_as<T, int8>)
		{
			return ImGuiDataType_S8;
		}
		else if constexpr (std::same_as<T, uint8>)
		{
			return ImGuiDataType_U8;
		}
		else if constexpr (std::same_as<T, int16>)
		{
			return ImGuiDataType_S16;
		}
		else if constexpr (std::same_as<T, uint16>)
		{
			return ImGuiDataType_U16;
		}
		else if constexpr (std::same_as<T, uint32>)
		{
			return ImGuiDataType_U32;
		}
		else if constexpr (std::same_as<T, int64>)
		{
			return ImGuiDataType_S64;
		}
		else if constexpr (std::same_as<T, uint64>)
		{
			return ImGuiDataType_U64;
		}
		else if constexpr (std::same_as<T, double>)
		{
			return ImGuiDataType_Double;
		}
		else
		{
			static_assert(false && "Unsupported data type.");
			return 0;
		}
	}

	template<CSliderVType T>
	constexpr const char* GetDefaultFmt()
	{
		if constexpr (std::same_as<T, float>)
		{
			return "%.3f";
		}
		else if constexpr (std::same_as<T, int>)
		{
			return "%d";
		}
		else
		{
			return nullptr;
		}
	}

	template<CSliderVType T>
	bool Slider(const char* label, T& value, T minV, T maxV, const SliderConfig& config)
	{
		ScopedItemOutline outline(label);

		if constexpr (std::same_as<T, float>)
		{
			return ImGui::SliderFloat(label, &value, minV, maxV, config.Fmt, config.Flags);
		}
		else if constexpr (std::same_as<T, int>)
		{
			return ImGui::SliderInt(label, &value, minV, maxV, config.Fmt, config.Flags);
		}
		else
		{
			return ImGui::SliderScalar(label, GetImGuiDataTypeFrom<T>(), &value, &minV, &maxV, config.Fmt, config.Flags);
		}
	}

	template<CSliderVType T>
	bool Slider(const char* label, Property<T>& property, T minV, T maxV, const SliderConfig& config)
	{
		T value = property.Get();

		if (Slider(label, value, minV, maxV, config))
		{
			property.Set(value);
			return true;
		}
		else
		{
			return false;
		}
	}

	template<std::floating_point T>
	bool Input(const char* label, T& v, const InputConfig<T>& config)
	{
		ScopedItemOutline outline(label);

		bool bModified = false;

		T originalValue = v;

		int flags = config.Flags;

		// ImGui doesn't support this flag for input scalar natively
		bool bTrueOnEnter = false;
		if ((flags & ImGuiInputTextFlags_EnterReturnsTrue))
		{
			flags ^= ImGuiInputTextFlags_EnterReturnsTrue;
			bTrueOnEnter = true;
		}

		if constexpr (std::same_as<T, float>)
		{
			bModified = ImGui::InputFloat(label, &v, config.Step, config.StepFast, config.Fmt, flags);
		}
		else if constexpr (std::same_as<T, double>)
		{
			bModified = ImGui::InputDouble(label, &v, config.Step, config.StepFast, config.Fmt, flags);
		}
		else
		{
			static_assert(false, "Unsupported floating point type.");
		}

		if (bModified)
		{
			if (bTrueOnEnter and not ImGui::IsItemDeactivatedAfterEdit())
			{
				v = originalValue;
				return false;
			}

			if (config.Min)
				v = std::max(v, *config.Min);

			if (config.Max)
				v = std::min(v, *config.Max);
		}

		return bModified;
	}

	template<std::integral T>
	bool Input(const char* label, T& v, const InputConfig<T>& config)
	{
		ScopedItemOutline outline(label);

		T originalValue = v;

		int flags = config.Flags;

		// ImGui doesn't support this flag for input scalar natively
		bool bTrueOnEnter = false;
		if ((flags & ImGuiInputTextFlags_EnterReturnsTrue))
		{
			flags ^= ImGuiInputTextFlags_EnterReturnsTrue;
			bTrueOnEnter = true;
		}

		const bool bModified = ImGui::InputScalar(label,
												  GetImGuiDataTypeFrom<T>(),
												  (void*)&v,
												  (void*)(config.Step > 0 ? &config.Step : nullptr),
												  (void*)(config.StepFast > 0 ? &config.StepFast : nullptr),
												  config.Fmt,
												  flags);

		if (bModified)
		{
			if (bTrueOnEnter and not ImGui::IsItemDeactivatedAfterEdit())
			{
				v = originalValue;
				return false;
			}

			if (config.Min)
				v = std::max(v, *config.Min);

			if (config.Max)
				v = std::min(v, *config.Max);
		}

		return bModified;
	}

	template<std::floating_point T>
	bool Input(const char* label, Property<T>& property, const InputConfig<T>& config)
	{
		T value = property.Get();

		if (Input(label, value, config))
		{
			property.Set(value);
			return true;
		}
		else
		{
			return false;
		}
	}

	template<std::integral T>
	bool Input(const char* label, Property<T>& property, const InputConfig<T>& config)
	{
		T value = property.Get();

		if (Input(label, value, config))
		{
			property.Set(value);
			return true;
		}
		else
		{
			return false;
		}
	}

	template<class T, class GetNameCb, class ...Args>  requires (CComboGetNameCb<GetNameCb, T>)
	bool Combo(const char* label, int& selectedItemIndex, std::span<const T> items, const GetNameCb& getNameCb, int popupMaxHeightInItems)
	{
		// Combo boxes have different outline colour,
		// because we don't want the widget be distracting
		// in the background while its popup is open.
		ImGuiEx::ScopedItemOutline outline(label, ImGuiEx::OutlineFlags_NoOutlineInactive, ImColor(60, 60, 60));

		auto getNameCbImpl = [&](int index) -> const char*
		{
			return getNameCb(items[index], index);
		};
		using GetNameCbImpl = decltype(getNameCbImpl);
		
		auto getNameCbInternal = [](void* cb, int index) -> const char*
		{
			return (*static_cast<GetNameCbImpl*>(cb))(index);
		};

		const bool bModified = ImGui::Combo(label,
											&selectedItemIndex,
											getNameCbInternal,
											&getNameCbImpl,
											static_cast<int>(items.size()),
											popupMaxHeightInItems);
		return bModified;
	}
} // namespace JPL::ImGuiEx
