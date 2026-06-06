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
#include <JPLSpatial/Math/SIMD.h>

#include <imgui.h> // IM_COL32

#include <array>

namespace JPL::GUI
{
	//==========================================================================
	/// Colour map used to render spectrogram
	class SpectrogramColourMap
	{
	public:
		static constexpr uint32 cColourRange = std::numeric_limits<uint8>::max();

	public:
		// Sample colour map used by Spectrogram class
		[[nodiscard]] static JPL_INLINE uint32 SampleColourMap(float t);

		// Sample colour map used by Spectrogram class
		[[nodiscard]] static JPL_INLINE simd_mask SampleColourMap(simd t);

		// All index values within uint8 range are valid
		[[nodiscard]] static JPL_INLINE uint32 SampleColourMap(uint8 colourMapIndex) { return cPixelColours[colourMapIndex]; }

	private:
		// Note: Spectrogram Colour Map is queried in a very hot paths
		// we want it to be inlined

		struct Keyframe
		{
			float Pos;
			uint8 r, g, b;
		};

		// High-resolution nodes for the Matplotlib Inferno colormap
		static constexpr auto keyframes =
			std::to_array({
				Keyframe{ 0.000f,   0,   0,   4 },
				Keyframe{ 0.078f,  20,   3,  49 },
				Keyframe{ 0.157f,  46,   1,  92 },
				Keyframe{ 0.235f,  76,   1, 115 },
				Keyframe{ 0.314f, 108,  15, 117 },
				Keyframe{ 0.392f, 140,  32, 107 },
				Keyframe{ 0.471f, 171,  50,  90 },
				Keyframe{ 0.549f, 200,  71,  69 },
				Keyframe{ 0.627f, 225,  96,  45 },
				Keyframe{ 0.706f, 243, 127,  20 },
				Keyframe{ 0.784f, 252, 161,   2 },
				Keyframe{ 0.863f, 252, 199,  22 },
				Keyframe{ 0.941f, 247, 237,  93 },
				Keyframe{ 1.000f, 255, 255, 228 }
			});

		// Build a full range colour table
		static constexpr std::array<uint32, cColourRange> cPixelColours = []()
		{
			std::array<uint32, cColourRange> colours;

			// Fallback for maximum edge case
			const auto fallbackColour = IM_COL32(keyframes.back().r, keyframes.back().g, keyframes.back().b, 255);

			colours.fill(fallbackColour);

			for (uint32 c = 0; c < colours.size(); ++c)
			{
				const float t = c / static_cast<float>(cColourRange - 1);

				for (uint32 i = 0; i < keyframes.size() - 1; ++i)
				{
					// Locate the active interpolation segment
					if (t <= keyframes[i + 1].Pos)
					{
						// Interpolate within segment
						const auto& start = keyframes[i];
						const auto& end = keyframes[i + 1];
						const float segmentT = (t - start.Pos) / (end.Pos - start.Pos);

						colours[c] = IM_COL32(
							static_cast<uint8>(Math::Lerp(start.r, end.r, segmentT)),
							static_cast<uint8>(Math::Lerp(start.g, end.g, segmentT)),
							static_cast<uint8>(Math::Lerp(start.b, end.b, segmentT)),
							255
						);

						break;
					}
				}
			}
			return colours;
		}();
	};
} // namespace JPL::GUI

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

namespace JPL::GUI
{
	JPL_INLINE uint32 SpectrogramColourMap::SampleColourMap(float t)
	{
		//t = std::clamp(t, 0.0f, 1.0f);
		return SampleColourMap(static_cast<uint8>(t * (cColourRange - 1)));
	}

	JPL_INLINE simd_mask SpectrogramColourMap::SampleColourMap(simd t)
	{
		//t = clamp(t, 0.0f, 1.0f);

		// buffer of indices
		uint32 values[4]{};
		(t * (cColourRange - 1)).to_mask().store(values);

		// index colour map table and reuse the same buffer for result
		values[0] = SampleColourMap(static_cast<uint8>(values[0]));
		values[1] = SampleColourMap(static_cast<uint8>(values[1]));
		values[2] = SampleColourMap(static_cast<uint8>(values[2]));
		values[3] = SampleColourMap(static_cast<uint8>(values[3]));

		return simd_mask(values);
	}
} // namespace JPL::GUI

