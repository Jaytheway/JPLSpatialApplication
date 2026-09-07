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

#include "Utility/MVCUtils.h"

#include <JPLSpatial/Math/SIMD.h>

namespace JPL
{
	enum EReverbSource : int
	{
		Geometry = 0,	// Static geometry is used to precompute reverb parameters
		RayTracer		// Dynamic ray tracer is used to suply reverb parameters
	};

	class LateReverbModel
	{
	public:
		LateReverbModel() = default;

		static constexpr float cMinReverbTime = 0.03f;
		static constexpr float cMaxReverbTime = 10.0f;

		Property<simd> T60{ 2.0f };

		Property<float> DryLevel{ 1.0f };	// ER level bypassing reveb
		Property<float> WetLevel{ 0.75f };	// Late Reverb output level

		// GUI-specific
		Property<EReverbSource> ReverbSource{ EReverbSource::Geometry };
	};
} // namespace JPL
