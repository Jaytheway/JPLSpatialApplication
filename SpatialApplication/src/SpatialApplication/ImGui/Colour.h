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

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>

#include <algorithm>

namespace JPL
{
	/// Utility for more declarative way to setup colours
	struct Colour
	{
		ImVec4 Value;

		constexpr Colour() {}
		constexpr Colour(float r, float g, float b, float a = 1.0f) : Value(r, g, b, a) {}
		constexpr Colour(const ImVec4& col) : Value(col) {}
		constexpr Colour(int r, int g, int b, int a = 255) : Value((float)r* (1.0f / 255.0f), (float)g* (1.0f / 255.0f), (float)b* (1.0f / 255.0f), (float)a* (1.0f / 255.0f)) {}
		constexpr Colour(ImU32 rgba) : Value((float)((rgba >> IM_COL32_R_SHIFT) & 0xFF)* (1.0f / 255.0f), (float)((rgba >> IM_COL32_G_SHIFT) & 0xFF)* (1.0f / 255.0f), (float)((rgba >> IM_COL32_B_SHIFT) & 0xFF)* (1.0f / 255.0f), (float)((rgba >> IM_COL32_A_SHIFT) & 0xFF)* (1.0f / 255.0f)) {}
		constexpr Colour(ImColor imColor) : Value(imColor.Value) {}
		inline operator ImU32() const { return ImGui::ColorConvertFloat4ToU32(Value); }
		inline operator ImVec4() const { return Value; }
		inline operator ImColor() const { return Value; }

		inline Colour WithValue(float value) const
		{
			float hue, sat, val;
			ImGui::ColorConvertRGBtoHSV(Value.x, Value.y, Value.z, hue, sat, val);
			return ImColor::HSV(hue, sat, std::min(value, 1.0f));
		}

		inline Colour WithSaturation(float saturation) const
		{
			float hue, sat, val;
			ImGui::ColorConvertRGBtoHSV(Value.x, Value.y, Value.z, hue, sat, val);
			return ImColor::HSV(hue, std::min(saturation, 1.0f), val);
		}

		inline Colour WithHue(float hue) const
		{
			float h, s, v;
			ImGui::ColorConvertRGBtoHSV(Value.x, Value.y, Value.z, h, s, v);
			return ImColor::HSV(std::min(hue, 1.0f), s, v);
		}


		inline Colour WithMultipliedValue(float multiplier) const
		{
			float hue, sat, val;
			ImGui::ColorConvertRGBtoHSV(Value.x, Value.y, Value.z, hue, sat, val);
			return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
		}

		inline Colour WithMultipliedSaturation(float multiplier) const
		{
			float hue, sat, val;
			ImGui::ColorConvertRGBtoHSV(Value.x, Value.y, Value.z, hue, sat, val);
			return ImColor::HSV(hue, std::min(sat * multiplier, 1.0f), val);
		}

		inline Colour WithMultipliedHue(float multiplier) const
		{
			float hue, sat, val;
			ImGui::ColorConvertRGBtoHSV(Value.x, Value.y, Value.z, hue, sat, val);
			return ImColor::HSV(std::min(hue * multiplier, 1.0f), sat, val);
		}

		inline Colour WithRed(float value) const { return { value, Value.y, Value.z, Value.w }; }
		inline Colour WithGreen(float value) const { return { Value.x, value, Value.z, Value.w }; }
		inline Colour WithBlue(float value) const { return { Value.x, Value.y, value, Value.w }; }
		inline Colour WithAlpha(float value) const { return { Value.x, Value.y, Value.z, value }; }
		inline Colour WithMultipliedAlpha(float multiplier) const
		{
			return { Value.x, Value.y, Value.z, Value.w * std::min(multiplier, 1.0f) };
		}

		static inline Colour WhiteWithAlpha(float alpha) { return Colour(1.0f, 1.0f, 1.0f, alpha); }
		static inline Colour WhiteWithAlpha(int alpha) { return Colour(1.0f, 1.0f, 1.0f, (float)alpha / 255.0f); }

		static inline Colour BlackWithAlpha(float alpha) { return Colour(0.0f, 0.0f, 0.0f, alpha); }
		static inline Colour BlackWithAlpha(int alpha) { return Colour(0.0f, 0.0f, 0.0f, (float)alpha / 255.0f); }
	};
} // namespace JPL
