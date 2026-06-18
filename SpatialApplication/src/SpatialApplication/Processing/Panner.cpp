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

#include "Panner.h"

#include <JPLSpatial/Panning/VBAPanning2D.h>
#include <JPLSpatial/Panning/VBAPanning3D.h>
#include <JPLSpatial/Math/Position.h>

#include <cstdint>
#include <vector>
#include <utility>

namespace JPL
{
	template<class InternalPanner>
	class JPLPannerInternalBase : public JPLPanner::IImpl
	{
	public:
		using InternalPannerType = InternalPanner;
		using LayoutType = typename InternalPannerType::SourceLayoutType;
		using PanUpdateData = typename InternalPannerType::PanUpdateData;
		using PanUpdateDataWithOrientation = typename InternalPannerType::PanUpdateDataWithOrientation;
		using PairType = std::pair<uint32, LayoutType>;

		explicit JPLPannerInternalBase(JPL::ChannelMap ChannelMap)
		{
			Initialize(ChannelMap);
		}

		~JPLPannerInternalBase() = default;

		JPL_INLINE void Initialize(JPL::ChannelMap ChannelMap)
		{
			[[maybe_unused]] const bool bInitialized = Panner.InitializeLUT(ChannelMap);
			JPL_ASSERT(bInitialized, "Failed to initialize internal panner LUT.");
		}

		// TODO: consider removing this overload, as it handles presense of LFE differntly
		JPL_INLINE JPLSourceLayoutHandle InitializeSourceLayout(uint32 NumSourceChannels) final
		{
			for (int32_t i = 0; i < SourceLayouts.size(); ++i)
			{
				if (SourceLayouts[i].first == NumSourceChannels)
				{
					return { .Id = i };
				}
			}

			PairType NewLayout{ NumSourceChannels, LayoutType{} };

			if (Panner.InitializeSourceLayout(JPL::ChannelMap::FromNumChannels(NumSourceChannels), NewLayout.second))
			{
				SourceLayouts.emplace_back(std::move(NewLayout));
				return { .Id = static_cast<int>(SourceLayouts.size()) - 1 };
			}

			return { .Id = -1 };
		}

		JPL_INLINE JPLSourceLayoutHandle InitializeSourceLayout(JPL::ChannelMap channelSet) final
		{
			const uint32 numSourceChannels = (channelSet.GetNumChannels() - channelSet.HasLFE());

			for (int32_t i = 0; i < SourceLayouts.size(); ++i)
			{
				if (SourceLayouts[i].first == numSourceChannels)
				{
					return { .Id = i };
				}
			}

			PairType NewLayout{ numSourceChannels, LayoutType{} };

			if (Panner.InitializeSourceLayout(channelSet, NewLayout.second))
			{
				SourceLayouts.emplace_back(std::move(NewLayout));
				return { .Id = static_cast<int>(SourceLayouts.size()) - 1 };
			}

			return { .Id = -1 };
		}


		JPL_INLINE void GetSpeakerGains(const JPL::MinimalVec3& Direction, std::span<float> OutChannelGains) const final
		{
			Panner.GetSpeakerGains(Direction, OutChannelGains);
		}

		JPL_INLINE uint32 GetNumChannels() const final
		{
			return Panner.GetNumChannels();
		}

		JPL_INLINE ChannelMap GetChannelMap() const final
		{
			return Panner.GetChannelMap();
		}

		JPL_INLINE virtual bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
											const JPLPanUpdateData& UpdateData,
											std::span<float> OutChannelGains) const final
		{
			if (SourceLayout.Id == -1)
			{
				return false;
			}

			const auto& [NumChannels, Layout] = SourceLayouts[SourceLayout.Id];

			const PanUpdateData UpdatDataInternal
			{
				.SourceDirection = UpdateData.Direction,
				.Focus = UpdateData.Focus,
				.Spread = UpdateData.Spread
			};
			static_assert(sizeof(PanUpdateData) == sizeof(JPLPanUpdateData));

			Panner.ProcessVBAPData(Layout, UpdatDataInternal, OutChannelGains);

			return true;
		}

		JPL_INLINE virtual bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
											const JPLPanUpdateData& UpdateData,
											const JPL::OrientationData<JPL::MinimalVec3>& orientation,
											std::span<float> OutChannelGains) const final
		{
			if (SourceLayout.Id == -1)
			{
				return false;
			}

			const auto& [NumChannels, Layout] = SourceLayouts[SourceLayout.Id];

			const PanUpdateDataWithOrientation UpdatDataInternal
			{
				.Pan = {
					.SourceDirection = UpdateData.Direction,
					.Focus = UpdateData.Focus,
					.Spread = UpdateData.Spread
				},
				.Orientation = orientation
			};
			static_assert(sizeof(PanUpdateData) == sizeof(JPLPanUpdateData));

			Panner.ProcessVBAPData(Layout, UpdatDataInternal, OutChannelGains);

			return true;
		}

		JPL_INLINE virtual bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
											const JPLPanUpdateData& UpdateData,
											std::span<float> OutChannelGains,
											JPLPanner::ChannelPointsGeneratedCb callback,
											void* userData) const final
		{
			if (SourceLayout.Id == -1)
			{
				return false;
			}

			const auto& [NumChannels, Layout] = SourceLayouts[SourceLayout.Id];

			const PanUpdateData UpdatDataInternal
			{
				.SourceDirection = UpdateData.Direction,
				.Focus = UpdateData.Focus,
				.Spread = UpdateData.Spread
			};
			static_assert(sizeof(PanUpdateData) == sizeof(JPLPanUpdateData));

			auto callbackWrapper = [userData, callback](std::span<const JPL::MinimalVec3> channelVSs, uint32 channelID)
			{
				callback(userData, channelVSs, channelID);
			};

			Panner.ProcessVBAPData(Layout, UpdatDataInternal, OutChannelGains, callbackWrapper);

			return true;
		}

		JPL_INLINE virtual bool ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
											const JPLPanUpdateData& UpdateData,
											const JPL::OrientationData<JPL::MinimalVec3>& orientation,
											std::span<float> OutChannelGains,
											JPLPanner::ChannelPointsGeneratedCb callback,
											void* userData) const final
		{
			if (SourceLayout.Id == -1)
			{
				return false;
			}

			const auto& [NumChannels, Layout] = SourceLayouts[SourceLayout.Id];

			const PanUpdateDataWithOrientation UpdatDataInternal
			{
				.Pan = {
					.SourceDirection = UpdateData.Direction,
					.Focus = UpdateData.Focus,
					.Spread = UpdateData.Spread
				},
				.Orientation = orientation
			};
			static_assert(sizeof(PanUpdateData) == sizeof(JPLPanUpdateData));

			auto callbackWrapper = [userData, callback](std::span<const JPL::MinimalVec3> channelVSs, uint32 channelID)
			{
				callback(userData, channelVSs, channelID);
			};

			Panner.ProcessVBAPData(Layout, UpdatDataInternal, OutChannelGains, callbackWrapper);

			return true;
		}

	private:
		InternalPannerType Panner;
		std::vector<PairType> SourceLayouts;
	};

	using JPLPanner2D = JPLPannerInternalBase<JPL::VBAPanner2D<>>;
	using JPLPanner3D = JPLPannerInternalBase<JPL::VBAPanner3D<>>;

	JPLPanner::JPLPanner() = default;
	JPLPanner::~JPLPanner() = default;

	JPLPanner::JPLPanner(uint32 NumChannels)
	{
		[[maybe_unused]] const bool bSuccess = Initialize(NumChannels);
		JPL_ASSERT(bSuccess);
	}

	JPLPanner::JPLPanner(JPL::ChannelMap channelSet)
	{
		[[maybe_unused]] const bool bSuccess = Initialize(channelSet);
		JPL_ASSERT(bSuccess);
	}

	bool JPLPanner::Initialize(JPL::ChannelMap channelSet)
	{
		if (Impl)
		{
			// TODO: pring error: panner cannot be reinitialized
			return false;
		}

		if (!channelSet.IsValid())
		{
			return false;
		}

		if (channelSet.HasTopChannels())
		{
			Impl.reset(new JPLPanner3D(channelSet));
		}
		else
		{
			Impl.reset(new JPLPanner2D(channelSet));
		}

		return true;
	}

	bool JPLPanner::Initialize(uint32 NumChannels)
	{
		return Initialize(JPL::ChannelMap::FromNumChannels(NumChannels));
	}

	JPLSourceLayoutHandle JPLPanner::InitializeSourceLayout(uint32 NumSourceChannels)
	{
		JPL_ASSERT(Impl);
		return Impl->InitializeSourceLayout(NumSourceChannels);
	}

	JPLSourceLayoutHandle JPLPanner::InitializeSourceLayout(JPL::ChannelMap channelSet)
	{
		JPL_ASSERT(Impl);
		return Impl->InitializeSourceLayout(channelSet);
	}

	bool JPLPanner::ProcessVBAP(JPLSourceLayoutHandle SourceLayout, const JPLPanUpdateData& UpdateData, std::span<float> OutChannelGains)
	{
		JPL_ASSERT(Impl);
		return Impl->ProcessVBAP(SourceLayout, UpdateData, OutChannelGains);
	}

	bool JPLPanner::ProcessVBAP(JPLSourceLayoutHandle SourceLayout, const JPLPanUpdateData& UpdateData, const JPL::OrientationData<JPL::MinimalVec3>& orientation, std::span<float> OutChannelGains)
	{
		JPL_ASSERT(Impl);
		return Impl->ProcessVBAP(SourceLayout, UpdateData, orientation, OutChannelGains);
	}

	bool JPLPanner::ProcessVBAP(JPLSourceLayoutHandle SourceLayout,
								const JPLPanUpdateData& UpdateData,
								std::span<float> OutChannelGains,
								ChannelPointsGeneratedCb callback, void* userData)
	{
		JPL_ASSERT(Impl);
		return Impl->ProcessVBAP(SourceLayout, UpdateData, OutChannelGains, callback, userData);
	}

	bool JPLPanner::ProcessVBAP(JPLSourceLayoutHandle SourceLayout, const JPLPanUpdateData& UpdateData, const JPL::OrientationData<JPL::MinimalVec3>& orientation, std::span<float> OutChannelGains, ChannelPointsGeneratedCb callback, void* userData)
	{
		JPL_ASSERT(Impl);
		return Impl->ProcessVBAP(SourceLayout, UpdateData, orientation, OutChannelGains, callback, userData);
	}

	void JPLPanner::GetSpeakerGains(const JPL::MinimalVec3& Direction, std::span<float> OutChannelGains) const
	{
		JPL_ASSERT(Impl);
		Impl->GetSpeakerGains(Direction, OutChannelGains);
	}

	uint32 JPLPanner::GetNumChannels() const
	{
		JPL_ASSERT(Impl);
		return Impl->GetNumChannels();
	}

	ChannelMap JPLPanner::GetChannelMap() const
	{
		JPL_ASSERT(Impl);
		return Impl->GetChannelMap();
	}

	float JPLPanner::GetSpreadFromSourceSize(float sourceSize, float distance)
	{
		return JPL::GetSpreadFromSourceSize(sourceSize, distance);
	}
} // namespace JPL
