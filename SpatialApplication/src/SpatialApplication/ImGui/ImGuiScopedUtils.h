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

#include "ImGui/ImGui.h"

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>

#include <memory>
#include <cstddef>

namespace JPL::ImGuiEx
{
	//=========================================================================================
	/// Scoped Utilities

	class NonCopyable
	{
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	protected:
		NonCopyable() = default;
	};

	template<class ScopedType>
	class Conditional : private NonCopyable
	{
		alignas(ScopedType) std::byte storage[sizeof(ScopedType)];
		ScopedType* ptr;
	public:
		template<class...Args>
		explicit Conditional(bool condition, Args&&...args)
			: ptr(nullptr)
		{
			if (condition)
			{
				ptr = std::construct_at(reinterpret_cast<ScopedType*>(storage),
										std::forward<Args>(args)...);
			}
		}

		~Conditional()
		{
			if (ptr)
			{
				std::destroy_at(ptr);
			}
		}
	};

	class ScopedStyle : private NonCopyable
	{
	public:
		template<typename T>
		ScopedStyle(ImGuiStyleVar styleVar, T value) { ImGui::PushStyleVar(styleVar, value); }
		~ScopedStyle() { ImGui::PopStyleVar(); }
	};

	class ScopedColour : private NonCopyable
	{
	public:
		template<typename T>
		ScopedColour(ImGuiCol colourId, T colour) { ImGui::PushStyleColor(colourId, ImColor(colour).Value); }
		~ScopedColour() { ImGui::PopStyleColor(); }
	};

	class ScopedFont : private NonCopyable
	{
	public:
		ScopedFont(ImFont* font, float fontSizePx = 0.0f)
		{
			ImGui::PushFont(font, fontSizePx);
		}
		~ScopedFont()
		{
			ImGui::PopFont();
		}
	};

	class ScopedFontSize : private NonCopyable
	{
	public:
		ScopedFontSize(float fontSizePx)
		{
			ImGui::PushFont(GImGui->Font, fontSizePx);
		}
		~ScopedFontSize()
		{
			ImGui::PopFont();
		}
	};

	class ScopedTextWrap : private NonCopyable
	{
	public:
		ScopedTextWrap(float warpPosition) { ImGui::PushTextWrapPos(warpPosition); }
		~ScopedTextWrap() { ImGui::PopTextWrapPos(); }
	};


	class ScopedID : private NonCopyable
	{
	public:
		template<typename T>
		ScopedID(T id) { ImGui::PushID(id); }
		~ScopedID() { ImGui::PopID(); }
	};

	class ScopedColourStack : private NonCopyable
	{
	public:

		template <typename ColourType, typename... OtherColours>
		ScopedColourStack(ImGuiCol firstColourID, ColourType firstColour, OtherColours&& ... otherColourPairs)
			: m_Count((sizeof... (otherColourPairs) / 2) + 1)
		{
			static_assert ((sizeof... (otherColourPairs) & 1u) == 0,
						   "ScopedColourStack constructor expects a list of pairs of colour IDs and colours as its arguments");

			PushColour(firstColourID, firstColour, std::forward<OtherColours>(otherColourPairs)...);
		}

		~ScopedColourStack() { ImGui::PopStyleColor(m_Count); }

	private:
		int m_Count;

		template <typename ColourType, typename... OtherColours>
		void PushColour(ImGuiCol colourID, ColourType colour, OtherColours&& ... otherColourPairs)
		{
			if constexpr (sizeof... (otherColourPairs) == 0)
			{
				ImGui::PushStyleColor(colourID, ImColor(colour).Value);
			}
			else
			{
				ImGui::PushStyleColor(colourID, ImColor(colour).Value);
				PushColour(std::forward<OtherColours>(otherColourPairs)...);
			}
		}
	};

	class ScopedStyleStack : private NonCopyable
	{
	public:

		template <typename ValueType, typename... OtherStylePairs>
		ScopedStyleStack(ImGuiStyleVar firstStyleVar, ValueType firstValue, OtherStylePairs&& ... otherStylePairs)
			: m_Count((sizeof... (otherStylePairs) / 2) + 1)
		{
			static_assert ((sizeof... (otherStylePairs) & 1u) == 0,
						   "ScopedStyleStack constructor expects a list of pairs of colour IDs and colours as its arguments");

			PushStyle(firstStyleVar, firstValue, std::forward<OtherStylePairs>(otherStylePairs)...);
		}

		~ScopedStyleStack() { ImGui::PopStyleVar(m_Count); }

	private:
		int m_Count;

		template <typename ValueType, typename... OtherStylePairs>
		void PushStyle(ImGuiStyleVar styleVar, ValueType value, OtherStylePairs&& ... otherStylePairs)
		{
			if constexpr (sizeof... (otherStylePairs) == 0)
			{
				ImGui::PushStyleVar(styleVar, value);
			}
			else
			{
				ImGui::PushStyleVar(styleVar, value);
				PushStyle(std::forward<OtherStylePairs>(otherStylePairs)...);
			}
		}
	};

	class ScopedItemWidth : private NonCopyable
	{
	public:
		ScopedItemWidth(float width) { ImGui::PushItemWidth(width); }
		~ScopedItemWidth() { ImGui::PopItemWidth(); }
	};

	class ScopedItemFlags : private NonCopyable
	{
		bool bSkip = false;
	public:
		ScopedItemFlags(const ImGuiItemFlags flags, const bool enable = true)
		{
			if (not JPL_ENSURE(!(flags & ImGuiItemFlags_Disabled), "We shouldn't use ImGuiItemFlags_Disabled! Use ImGuiEx::BeginDisabled / UI::EndDisabled instead. It will handle visuals for you."))
			{
				bSkip = true;
			}
			else
			{
				ImGui::PushItemFlag(flags, enable);
			}
		}
		~ScopedItemFlags() { if (not bSkip) ImGui::PopItemFlag(); }
	};

	class ScopedWindowMinSize : private NonCopyable
	{
		ImVec2 mRestoreSize; 
	public:
		explicit ScopedWindowMinSize(ImVec2 size)
		{
			mRestoreSize = ImGui::GetStyle().WindowMinSize;
			ImGui::GetStyle().WindowMinSize = size;
		}

		~ScopedWindowMinSize()
		{
			ImGui::GetStyle().WindowMinSize = mRestoreSize;
		}

	};

	class ScopedDisable : private NonCopyable
	{
	public:
		ScopedDisable(bool disabled = true);
		~ScopedDisable();
	};

	// Adds ImGui::Spacing at the beginning and the end of the scope
	class ScopedSpacing : private NonCopyable
	{
	public:
		ScopedSpacing() { ImGui::Spacing(); }
		~ScopedSpacing() { ImGui::Spacing(); }
	};

	// User ID can be provided for the group to be set as LastItemID
	// to be able to do hover/click checks more reliably.
	class ScopedGroup : private NonCopyable
	{
		ImGuiID ID = 0;
	public:
		ScopedGroup(ImGuiID id = 0) : ID(id) { ImGui::BeginGroup(); }
		~ScopedGroup();
	};

	class ScopedTextWrapPos : private NonCopyable
	{
	public:
		ScopedTextWrapPos(float wrapPosition = 0.0f) { ImGui::PushTextWrapPos(wrapPosition); }
		~ScopedTextWrapPos() { ImGui::PopTextWrapPos(); }
	};

} // namespace JPL::ImGuiEx
