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

#include "CoreInclude.h"

#include <JPLSpatial/Math/MinimalVec3.h>
#include <JPLSpatial/ChannelMap.h>

#include <span>

namespace JPL
{
	template<CVec3 T>
	struct OrientationData;

	struct JPLPanUpdateData
	{
		JPL::MinimalVec3 Direction;
		float Focus;
		float Spread;
	};

	struct JPLSourceLayoutHandle
	{
		int32_t Id = -1;

		JPL_INLINE operator bool() const noexcept { return Id != -1; }
	};

	class JPLPanner
	{
	public:
		JPLPanner();
		~JPLPanner();

		explicit JPLPanner(uint32 NumChannels);
		explicit JPLPanner(JPL::ChannelMap channelSet);

		bool Initialize(JPL::ChannelMap channelSet);
		bool Initialize(uint32 NumChannels);

		JPLSourceLayoutHandle InitializeSourceLayout(uint32 NumSourceChannels);
		JPLSourceLayoutHandle InitializeSourceLayout(JPL::ChannelMap channelSet);

		bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout, const JPLPanUpdateData& UpdateData, std::span<float> OutChannelGains);
		bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout, const JPLPanUpdateData& UpdateData, const JPL::OrientationData<JPL::MinimalVec3>& orientation, std::span<float> OutChannelGains);

		using ChannelPointsGeneratedCb = void(*)(void* userData, std::span<const JPL::MinimalVec3> channelVSs, uint32 channelID);

		bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
						 const JPLPanUpdateData& UpdateData,
						 std::span<float> OutChannelGains,
						 ChannelPointsGeneratedCb callback, void* userData);

		bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
						 const JPLPanUpdateData& UpdateData,
						 const JPL::OrientationData<JPL::MinimalVec3>& orientation,
						 std::span<float> OutChannelGains,
						 ChannelPointsGeneratedCb callback, void* userData);


		void GetSpeakerGains(const JPL::MinimalVec3& Direction, std::span<float> OutChannelGains) const;
		uint32 GetNumChannels() const;
		ChannelMap GetChannelMap() const;

		static float GetSpreadFromSourceSize(float sourceSize, float distance);

	private:
		template<class T>
		friend class JPLPannerInternalBase;

		struct IImpl
		{
			virtual ~IImpl() = default;
			virtual JPLSourceLayoutHandle InitializeSourceLayout(uint32 NumSourceChannels) = 0;
			virtual JPLSourceLayoutHandle InitializeSourceLayout(JPL::ChannelMap channelSet) = 0;
			virtual void GetSpeakerGains(const JPL::MinimalVec3& Direction, std::span<float> OutChannelGains) const = 0;
			virtual uint32 GetNumChannels() const = 0;
			virtual ChannelMap GetChannelMap() const = 0;

			virtual bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
									 const JPLPanUpdateData& UpdateData,
									 std::span<float> OutChannelGains) const = 0;

			virtual bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
									 const JPLPanUpdateData& UpdateData,
									 const JPL::OrientationData<JPL::MinimalVec3>& orientation,
									 std::span<float> OutChannelGains) const = 0;

			virtual bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
									 const JPLPanUpdateData& UpdateData,
									 std::span<float> OutChannelGains,
									 ChannelPointsGeneratedCb callback,
									 void* userData) const = 0;

			virtual bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
									 const JPLPanUpdateData& UpdateData,
									 const JPL::OrientationData<JPL::MinimalVec3>& orientation,
									 std::span<float> OutChannelGains,
									 ChannelPointsGeneratedCb callback,
									 void* userData) const = 0;
		};

		std::unique_ptr<IImpl> Impl;
	};
} // namespace JPL