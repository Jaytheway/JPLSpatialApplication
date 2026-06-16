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

#include "Utility/MVCUtils.h"
#include "Model/DirectSoundModel.h"

#include <JPLSpatial/AcousticMaterial.h>
#include <JPLSpatial/Math/MinimalVec3.h>

#include <cstdint>

namespace JPL
{
	class RoomModel
	{
	public:
		RoomModel() = default;
		~RoomModel() = default;

		inline MinimalVec3 GetListenerAbsPosition() const
		{
			return ListenerPosition.Get() * RoomSize.Get();
		}

		inline MinimalVec3 GetSourceAbsPosition() const
		{
			return SourcePosition.Get() * RoomSize.Get();
		}

		Property<MinimalVec3 > ListenerPosition
		{
			{ 11.0f / 24.0f, 1.8f / 5.0f, 14.0f / 24.0f } // % of canvas size
			//{ 0.5f, 0.5f, 0.5f } //? temp for testing
		};

		Property<MinimalVec3 > SourcePosition
		{
			{ 13.0f / 24.0f, 2.6 / 5.0f, 8.0f / 24.0f } // % of canvas size
			//{ 0.5f, 0.5f, 0.5f } //? temp for testing
		};


		Property<MinimalVec3> RoomSize
		{
			{ 24.0f, 5.0f, 24.0f }
			//{ 12.0f, 12.0f, 12.0f } //? temp for testing
		};

		Property<bool> EnableSpecular{ true };
		Property<bool> EnableDirect{ true };

		std::shared_ptr<DirectSoundModel> DirectSound;
		// TODO: add BDPT model here as well

		Property<const JPL::AcousticMaterial*> SurfaceMaterial{ JPL::AcousticMaterial::Get("ConcreteBlockRough") };

		Property<uint32_t> NumPrimaryRays{ 100 };
		Property<uint32_t> MaxOrder{ 4 };
	};

} // namespace JPL