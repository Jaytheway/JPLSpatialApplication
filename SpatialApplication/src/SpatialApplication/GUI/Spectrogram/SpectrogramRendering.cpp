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

#include "SpectrogramRendering.h"

#include "ImGui/ImGui.h"
#include "Systems/EventsLoop.h"

#include <JPLSpatial/Math/Math.h>
#include <JPLSpatial/Math/MinimalVec2.h>
#include <Walnut/Image.h>

#define JPL_PROFILE_SPECTROGRAM_GEN 0

#if JPL_PROFILE_SPECTROGRAM_GEN 
#include "Utility/Timer.h"
#endif

#include <algorithm>
#include <array>
#include <format>
#include <iterator>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace JPL::GUI
{
	//==========================================================================
	Spectrogram::ChannelData::ChannelData(const allocator_type& allocator)
		: Pixels(allocator.resource())
		, Chunks(allocator.resource())
	{}

	Spectrogram::ChannelData::ChannelData(const ChannelData& other, const allocator_type& alloc)
		: Pixels(other.Pixels, alloc.resource())
		, Chunks(other.Chunks, alloc.resource())
	{}

	Spectrogram::ChannelData::ChannelData(ChannelData&& other, const allocator_type& alloc)
		: Pixels(std::move(other.Pixels), alloc.resource())
		, Chunks(std::move(other.Chunks), alloc.resource())
	{}


	//==========================================================================
	// Forward declaration
	static [[nodiscard]] std::pmr::vector<typename Spectrogram::ChannelData>
		GenerateFFTImageData(const std::vector<float>& sampleData,
						SpectrogramGenerator& generator,
						uint64 numFrames,
						uint32 targetWidth,
						uint32 targetHeight,
						uint32 maxChunkWidth);

	//==========================================================================
	struct FFTSizeEntry
	{
		const char* Label;
		uint32 Value;
	};
	constexpr auto cFFTSizeEntries = std::to_array(
	{
		//FFTSizeEntry{ "32", 32 },
		FFTSizeEntry{ "64", 64 },
		FFTSizeEntry{ "128", 128 },
		FFTSizeEntry{ "256", 256 },
		FFTSizeEntry{ "512", 512 },
		FFTSizeEntry{ "1024", 1024 },
		FFTSizeEntry{ "2048", 2048 },
		FFTSizeEntry{ "4096", 4096 },
		FFTSizeEntry{ "8192", 8192 },
		//FFTSizeEntry{ "16384", 16384 }
	});
	
	//==========================================================================
	Spectrogram::Spectrogram(WaveformDataSource& waveformDataSorce)
		: mDataSource(waveformDataSorce)
		, mMemoryResource(std::pmr::get_default_resource())
		, mChannelData(&mMemoryResource)
		, mChannelAtlases(&mMemoryResource)
	{
		mDataSource.AddListener(this);

		mSpectrogramUpdateRoutine = SpectrogramUpdateRoutine();
		mSpectrogramUpdateRoutine.Resume(); // start the routine
	}

	Spectrogram::~Spectrogram() noexcept
	{
		mDataSource.RemoveListener(this);
	}
	
	void Spectrogram::OnSourceChanged()
	{
		// Set the update state to display message
		// to the user immediately when source changes.
		// 
		// OnNewDataAwailable will trigger the actual update,
		// when new sample data is loaded.
		mUpdateState = EUpdateState::UpdatingSource;
	}

	void Spectrogram::OnNewDataAwailable(const SampleData& newSampleData)
	{
		mSampleDataCache = newSampleData;
		mUpdateState = EUpdateState::UpdatingSource;
		mSpectrogramNeedsUpdate.Set();
	}

	bool Spectrogram::Draw(const char* itemId)
	{
		ImGuiEx::ScopedGroup group;

		const uint32_t numChannels = mChannelData.size();

		const float borderSize = ImGui::GetStyle().ChildBorderSize;

		// When we have multiple subplots, ImGui adds a scrollbard, even uf item spacing set to 0
		const ImVec2 plotSpace =
			Vec2(ImGui::GetContentRegionAvail()) -		// content region
			Vec2(borderSize * 2.0f, borderSize * 2.0f);	// subtract border

		// If we have valid data source, but the spectrogram widget was never draw,
		// the width in pixels would be 0, and no spectrogram generated for the valid source.
		// Therefore we need to manually trigger update.
		if (mSpectrogramWidthPx == 0 or mSpectrogramHeightPx == 0)
		{
			mSpectrogramWidthPx = static_cast<int>(plotSpace.x);
			mSpectrogramHeightPx = static_cast<int>(plotSpace.y);

			if (not mSampleDataCache.Samples.empty())
			{
				mSpectrogramNeedsUpdate.Set();
			}
		}

		// TODO: we may want to also update the waveform if the display width changes
		mSpectrogramWidthPx = static_cast<int>(plotSpace.x);
		mSpectrogramHeightPx = static_cast<int>(plotSpace.y);

		auto drawSpectrogram = [&]
		{
			if (not mChannelAtlases.empty() and mChannelData.size() == mChannelAtlases.size())
			{
				auto* drawList = ImGui::GetWindowDrawList();
				Vec2 pos = ImGui::GetCursorScreenPos() + Vec2(borderSize, borderSize);

				const float channelHeight = plotSpace.y / numChannels;
				const Vec2 channelSize(plotSpace.x, channelHeight);

				// Total channel atlas width in pixels might be different from our current widget width,
				// we need to scale the offsets accordingly
				const uint64 sourcePixelWidth = mChannelData[0].Pixels.size() / mChannelData[0].Chunks[0].Height;

				const float scale = static_cast<float>(channelSize.X) / sourcePixelWidth;

				//? debug
				/*constexpr std::array<uint32, 5> colours
				{
					IM_COL32(0, 255, 0, 120),
					IM_COL32(0, 0, 255, 120),
					IM_COL32(255, 0, 255, 120),
					IM_COL32(0, 255, 255, 120),
					IM_COL32(255, 0, 255, 120),
				};*/

				for (const ChannelAtlas& channelAtlas : mChannelAtlases)
				{
					Vec2 chunkPos = pos;

					for (uint32 i = 0; i < channelAtlas.size(); ++i)
					{
						const auto& image = channelAtlas[i];
						const Vec2 chunkSize(image->GetWidth() * scale, channelHeight);

						drawList->AddImage(image->GetDescriptorSet(), chunkPos, chunkPos + chunkSize);

						//? debug
						//ImGui::GetForegroundDrawList()->AddRect(chunkPos, chunkPos + chunkSize, colours[i % colours.size()], 0.0f, 0, 3.0f);

						// Offset image chunk to its X position on the timeline
						chunkPos.X += chunkSize.X;
					}

					// Offset channel to its Y position
					pos.Y += channelHeight;
				}

#if JPL_PROFILE_SPECTROGRAM_GEN
				char buffer[64]{};
				std::format_to_n(buffer, 64, "Gen time: {:.3f} ms", mSpectrogramGenTime);
				drawList->AddText(ImGui::GetCursorScreenPos(), IM_COL32(0, 255, 0, 255), buffer);
#endif
			}
		};

		// TODO: draw a nice animation isntead of just boring text
		auto drawPropsUpdateMessage = [&]
		{
			// TODO: maybe wrap this in a function like "OverlayText"

			ImVec2 bbMin = ImGui::GetCursorScreenPos();
			ImVec2 bbMax = bbMin + ImGui::GetContentRegionAvail();
			const ImVec2 bbCenter = (bbMax - bbMin) * 0.5f + bbMin;
			
			const ImVec2 textSize = ImGui::CalcTextSize("Updating spectrogram...");

			const float desiredWidth =
				textSize.x +
				ImGui::GetStyle().FramePadding.x * 2.0f +
				ImGui::GetStyle().WindowPadding.x * 2.0f;

			const float desiredHeight =
				ImGui::GetTextLineHeight() +
				ImGui::GetStyle().WindowPadding.y * 2.0f;

			const ImVec2 desiredSize(desiredWidth, desiredHeight);
			const ImVec2 desiredHalfSize = desiredSize * 0.5f;

			bbMin = bbCenter - desiredHalfSize;
			bbMax = bbCenter + desiredHalfSize;

			auto* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(bbMin,
									bbMax,
									IM_COL32(0, 0, 0, 100),
									ImGui::GetStyle().FrameRounding);

			const ImVec2 textPos = bbCenter - textSize * 0.5f;
			drawList->AddText(textPos,
							  ImGui::GetColorU32(ImGuiCol_Text),
							  "Updating spectrogram...");
		};

		auto drawSourceUpdateMessage = [&]
		{
			// TODO: draw a nice animation isntead of just boring text

			ImGuiEx::LayoutHorizontal("text_align", ImGui::GetContentRegionAvail(), 0.5f, [&]
			{
				ImGuiEx::ScopedDisable greyColour;
				ImGui::TextAligned(0.5f, mSpectrogramWidthPx, "Generating spectrogram...");
			});
		};
		
		// Use unique item ID
		const ImGuiID itemID = ImGui::GetCurrentWindow()->GetID(itemId);

		const ImVec2 bbMin = ImGui::GetCursorScreenPos();
		const ImVec2 bbMax = bbMin + ImGui::GetContentRegionAvail();
		const ImRect itemBB{ bbMin, bbMax };

		if (not ImGui::ItemAdd(itemBB, itemID))
			return false;

		// Draw border outline
		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(bbMin, bbMax, ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().FrameRounding);

		switch (mUpdateState)
		{
		case Spectrogram::EUpdateState::UpToDate:
		{
			drawSpectrogram();
		}
		break;
		case Spectrogram::EUpdateState::UpdatingProperties:
		{
			drawSpectrogram();
			drawPropsUpdateMessage();
		}
		break;
		case Spectrogram::EUpdateState::UpdatingSource:
		{
			drawSourceUpdateMessage();
		}
		break;
		default:
			break;
		}

		return true;
	}

	void Spectrogram::DrawProperties()
	{
		if (mChannelAtlases.empty())
		{
			ImGui::TextUnformatted("No sound source loaded");
			return;
		}

		ImGui::TextUnformatted("Spectrogram Parameters");

		bool bParamsChanged = false;

		static std::string buffer; buffer.clear();
		std::format_to(std::back_inserter(buffer), "{}", mParams.FFTSize);

		if (ImGui::BeginCombo("FFT Size", buffer.c_str()))
		{
			for (const auto& [label, value] : cFFTSizeEntries)
			{
				bool bSelected = mParams.FFTSize == value;

				if (ImGui::Selectable(label, &bSelected))
				{
					if (value != mParams.FFTSize)
					{
						mParams.FFTSize = value;
						bParamsChanged = true;
					}

				}
			}
			ImGui::EndCombo();
		}

		bParamsChanged |= ImGui::InputFloat("dB Range Offset", &mParams.DisplayDb, 3.0f, 0.0f, "%.0f");
		
		if (ImGui::InputFloat("dB Range Min", &mParams.MinDb, 3.0f, 0.0f, "%.0f"))
		{
			mParams.MinDb = std::min(mParams.MinDb, mParams.MaxDb - 6.0f);
			bParamsChanged = true;
		}

		if (ImGui::InputFloat("dB Range Max", &mParams.MaxDb, 3.0f, 0.0f, "%.0f"))
		{
			mParams.MaxDb = std::max(mParams.MaxDb, mParams.MinDb + 6.0f);
			bParamsChanged = true;
		}

		if (bParamsChanged)
		{
			mUpdateState = EUpdateState::UpdatingProperties;
			mSpectrogramNeedsUpdate.Set();
		}
	}

	Coro::Task<> Spectrogram::SpectrogramUpdateRoutine()
	{
		// The memory won't be deallocated until the Task is destroyed
		std::pmr::vector<ChannelData> outChannelData(&mMemoryResource);
		SampleData newSampleData; // TODO: we might want constructor to take allocator

		// Hold on to the background thread handle to not let it dangle away
		std::jthread backgroundThread;

		for (;;)
		{
			// Wait for new sample data or parameters change
			co_await mSpectrogramNeedsUpdate;

			// Take a copy, keep the cache valid
			newSampleData = mSampleDataCache;

			const uint32 numChannels =
				newSampleData.NumFrames > 0
				? newSampleData.Samples.size() / newSampleData.NumFrames
				: 0;

			// Get the latest parameters
			mGenerator.Init(mParams);

			// Get the latest required width in pixels
			const uint64 spectrogramWPx = mSpectrogramWidthPx;
			const uint64 spectrogramHPx = mSpectrogramHeightPx;

			float spectrogramGenTime = 0.0f;

			if (numChannels > 0)
			{
				const uint32 channelHeight = spectrogramHPx / numChannels;

				// Get the image dimension limit that hardware can render
				static const uint32 cMaxPixelChunkWidth = Walnut::Image::GetDimensionLimit();

				co_await Coro::SwitchToAsync(backgroundThread); // --- Process file on background thread
				{
#if JPL_PROFILE_SPECTROGRAM_GEN
					Timer timer;
#endif

					outChannelData = GenerateFFTImageData(newSampleData.Samples,
														  mGenerator,
														  newSampleData.NumFrames,
														  spectrogramWPx,
														  channelHeight,
														  cMaxPixelChunkWidth);

#if JPL_PROFILE_SPECTROGRAM_GEN
					spectrogramGenTime = timer.ElapsedMillis();
#endif
				}
				co_await Coro::SwitchToMainThread(); // --- Update waveform data on main thread
			}

			mChannelData = std::move(outChannelData);
			outChannelData = {};
		
			if (mChannelData.empty())
			{
				mChannelAtlases.clear();
			}
			else
			{
				mChannelAtlases.resize(numChannels);

				for (uint32 channel = 0; channel < mChannelData.size(); ++channel)
				{
					ChannelAtlas& channelAtlas = mChannelAtlases[channel];
					ChannelData& channelData = mChannelData[channel];

					channelAtlas.clear();
					channelAtlas.reserve(channelData.Chunks.size());

					// Create images for spectrogram chunks
					for (const ImageChunk& chunk : channelData.Chunks)
					{
						channelAtlas.emplace_back(
							make_pmr_shared<Walnut::Image>( // TODO: this doesn't actually use our allocator yet
								chunk.Width, chunk.Height,
								Walnut::ImageFormat::RGBA,
								&mChannelData[channel].Pixels[chunk.DataOffset]));
							
					}
				}
			}

			mSpectrogramGenTime = spectrogramGenTime;

			// Let the GUI know the update is no longer in progress
			mUpdateState = EUpdateState::UpToDate;
		}
	}

	std::pmr::vector<typename Spectrogram::ChannelData> GenerateFFTImageData(const std::vector<float>& sampleData,
																			 SpectrogramGenerator& generator,
																			 uint64 numFrames,
																			 uint32 targetWidth,
																			 uint32 targetHeight,
																			 uint32 maxChunkWidth)
	{
		if (not JPL_ENSURE(not sampleData.empty()))
			return {};

		if (numFrames == 0 or targetWidth == 0 or targetHeight == 0)
			return {};

		const std::size_t fftSize = generator.GetParams().FFTSize;
		
		// Hop size based on FFT size.
		// Here we're hoping by a fraction of FFT window,
		// therefore we get the overlap and not missing any data.
		const std::size_t framesPerPixel = std::max<std::size_t>((numFrames + targetWidth - 1) / targetWidth, 1);
		const std::size_t halfWindow = fftSize >> 1;
		const std::size_t hopSize = std::min(halfWindow, framesPerPixel);
		
		// Image dimensions
		const std::size_t width = (numFrames + hopSize - 1) / hopSize;
		const std::size_t height = targetHeight;
		
		//! Note: we could generate full resolution and let GPU
		//! scale the resulting image, but it still slows generation
		//! down by about 2x and the result is not tha much better
		//! for simple audio preview.
		// However, currently we're "skipping" some data in frequency
		// domain, since we're not computing magnitude for a frequency
		// range that fits into pixel row, but instead just giving the
		// value of frequency at the pixel center position.
		// (This is only the case if (heigh < num frequency bins)
		//const std::size_t height = fftSize / 2 + 1;
		
		const std::size_t totalDataSize = width * height;

		using ChannelData = typename Spectrogram::ChannelData;
		using Chunk = typename Spectrogram::ImageChunk;

		const uint32 numChannels = sampleData.size() / numFrames;

		std::pmr::vector<ChannelData> outChannelData(numChannels); // TODO: use allocator?

		// Splitting entire image data into chunks
		const uint32 numChunks = std::max<uint32>((width + maxChunkWidth - 1) / maxChunkWidth, 1);
		const uint64 chunkSize = maxChunkWidth * height;

		for (auto& channel : outChannelData)
			channel.Chunks.resize(numChunks);

		// Pixel column scratch buffer
		std::vector<uint32> pixelBuffer(height);

		auto writePixelBuffer = [&pixelBuffer](uint32 currentColumn, std::span<uint32> outPixels, uint32 stride)
		{
			uint32 pixelIndex = currentColumn;
			for (uint32 rowPixel : pixelBuffer)
			{
				outPixels[pixelIndex] = rowPixel;
				pixelIndex += stride;
			}
		};

		// TODO: avoid this copy (?)
		std::vector<float> paddedSamples(halfWindow + numFrames + halfWindow, 0.0f);

		std::size_t channelDataOffset = 0;
		for (ChannelData& channel : outChannelData)
		{
			std::ranges::copy_n(sampleData.begin() + channelDataOffset, numFrames, paddedSamples.begin() + halfWindow);

			auto& outPixels = channel.Pixels;
			outPixels.resize(totalDataSize);

			std::size_t pixelDataOffset = 0;
			std::size_t timestamp = 0;

			for (Chunk& chunk : channel.Chunks)
			{
				const std::size_t currentChunkSize = std::min(chunkSize, totalDataSize - pixelDataOffset);
				const std::size_t currentChunkWidth = currentChunkSize / height;
				
				chunk.DataOffset = pixelDataOffset;
				chunk.DataSize = currentChunkSize;
				chunk.Width = currentChunkWidth;
				chunk.Height = height;

				std::span<uint32> outPixelsChunk(outPixels.data() + pixelDataOffset, currentChunkSize);

				for (std::size_t column = 0; column < currentChunkWidth; ++column, timestamp += hopSize)
				{
					std::span<const float> window(paddedSamples.data() + timestamp, fftSize);
					
					generator.ProcessFrame(window);

					// For display purposes, any samplerate will do
					static constexpr float cSampleRate = 44100.0f;
					generator.WriteMelScaledColumn(pixelBuffer, cSampleRate);

					const uint32 stride = currentChunkWidth;
					writePixelBuffer(column, outPixelsChunk, stride);
				}

				pixelDataOffset += currentChunkSize;
			}

			channelDataOffset += numFrames;
		}

		return outChannelData;
	}

} // namespace JPL::GUI
