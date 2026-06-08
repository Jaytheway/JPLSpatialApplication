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

#include "ImGui/ImGui.h"
#include "Config.h"

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>
#include <MiniaudioCpp/MiniaudioWrappers.h>

#include "GUI/DirectoryDisplay.h"
#include "GUI/VBAPVisualization.h"

#include "Model/VBAPVisualizationModel.h"
#include "Model/DirectSoundModel.h"
#include "Model/LateReverbModel.h"

#include "Layers/AudioPlaybackLayer.h"
#include "Layers/RoomLayer.h"

#include "Systems/EventsLoop.h"
#include <implot.h>

#include <filesystem>
#include <memory>
#include <cassert>
#include <unordered_map>

namespace JPL
{
	namespace Embedded
	{
		#include "GUI/Embed/JPLSpatialApplication-Logo.embed"
		#include "GUI/Embed/JPLSpatialApplication-Icon.embed"
		#include "GUI/Embed/DefaultImGuiSettings.embed"
	} // namespace Embedded

	class JPLSpatialApplicationLayer : public Walnut::Layer
		, public ChangeListener<AudioPlaybackLayer>
		, public ChangeListener<RoomLayer>
	{
	public:
		JPLSpatialApplicationLayer()
			: mEventsLoop(&EventsLoop::Get())
		{
			mDirectSoundModel = std::make_shared<DirectSoundModel>();
			mLateReverbModel = std::make_shared<LateReverbModel>();
			
			mAudioPlaybackLayer = std::make_shared<AudioPlaybackLayer>(mDirectSoundModel, mLateReverbModel);
			mRoomLayer = std::make_shared<RoomLayer>(mDirectSoundModel, mLateReverbModel);

			mAudioPlaybackLayer->SetVBAPModel(mVBAPVis.VBAPModel);

			mAudioPlaybackLayer->AddListener(this);
			mRoomLayer->AddListener(this);

			mRoomLayer->GetModel().EnableDirect.AddChangeCallback<&AudioPlaybackLayer::SetEnableDirectSound>(mAudioPlaybackLayer.get());
			mVBAPVis.ConnectToAudioPlayer.AddChangeCallback<&JPLSpatialApplicationLayer::ConnectVBAPVisToAudioPlayer>(this);

			mVBAPVis.VBAPModel->SourceSize.AddChangeCallback<&RoomLayer::SetSourceSize>(mRoomLayer.get());
			mRoomLayer->SetSourceSize(mVBAPVis.VBAPModel->SourceSize.Get());
		}

		virtual void OnAttach() override
		{
			mEventsLoop = &EventsLoop::Get();

			sLogoImage = std::make_shared<Walnut::Image>(JPLSA_APP_LOGO_WIDTH,
														 JPLSA_APP_LOGO_HEIGHT,
														 Walnut::ImageFormat::RGBA,
														 JPL::Embedded::jplsa_app_logo);
			JPL::GUI::SetImGuiStyle();

			ImPlot::CreateContext();
		}

		virtual void OnDetach() override
		{
			mEventsLoop = nullptr;

			ImPlot::DestroyContext();

			sLogoImage.reset();
		}

		virtual void OnUIRender() override
		{
			if (mEventsLoop)
			{
				mEventsLoop->Process();
			}


#ifndef WL_DIST // just to be sure
			ImGui::ShowDemoWindow();
#endif
			ImGuiEx::Window("VBAP", [&]
			{
				mVBAPVisView.Draw();
			}, nullptr, { .Flags = ImGuiWindowFlags_NoCollapse });
		}

		void PushLayers(Walnut::Application& app)
		{
			app.PushLayer(mAudioPlaybackLayer);
			app.PushLayer(mRoomLayer);
		}

		virtual void OnSoundChanged(const JPL::Sound& newSound) override
		{
			mRoomLayer->SetNumChannelsForERs(newSound.GetNumOutputChannels(0));

			mSourceChannelSet = JPL::ChannelMap::FromNumChannels(newSound.GetNumOutputChannels(0));

			if (mVBAPVis.ConnectToAudioPlayer.Get())
			{
				mVBAPVis.SourceChannelMap.Set(JPL::NamedChannelMask(mSourceChannelSet.GetChannelMask()));
			}
		}

		// RoomLayer listener
		virtual void OnTapsUpdated(const std::vector<typename JPL::ERBus::ERUpdateData>& taps) override
		{
			mAudioPlaybackLayer->SetTaps(taps);
		}

		// RoomLayer listener
		virtual void OnSourceChanged(const JPL::MinimalVec3& newPosition) override
		{
			mVBAPVis.VBAPModel->SourcePosition.Set(newPosition);
		}

		// RoomLayer listener
		virtual void OnRoomSizeChanged(const JPL::MinimalVec3& /*newRoomSize*/) override
		{
			// Just let playback layer update the entire VBAP when the room changes.
			// This may be called after OnTapsUpdated, or not, it doesn't matter.
			mAudioPlaybackLayer->OnChange(mAudioPlaybackLayer->GetVBAPModel().get());
		}

		virtual void OnReverbTimeUpdated(const simd& newRT60) override
		{
			// Update late reverb processor parameters
			mAudioPlaybackLayer->GetLateReverbModel().lock()->T60.Set(newRT60);
		}


		static void DrawJPLSpatialApplicationLogo(ImVec2 titlebarMin, ImVec2 titlebarMax)
		{
			if (sLogoImage == nullptr)
			{
				return;
			}

			const bool isMaximized = Walnut::Application::Get().IsMaximized();
			float titlebarVerticalOffset = isMaximized ? -6.0f : 0.0f;

			const ImVec2 windowPadding = ImGui::GetCurrentWindow()->WindowPadding;
			auto* fgDrawList = ImGui::GetForegroundDrawList();

			const int logoWidth = 48;
			const int logoHeight = 48;
			const ImVec2 logoOffset(10.0f + windowPadding.x, 3.0f + windowPadding.y + titlebarVerticalOffset * 0.5f);

			const ImVec2 logoRectStart = { ImGui::GetItemRectMin().x + logoOffset.x, ImGui::GetItemRectMin().y + logoOffset.y };
			const ImVec2 logoRectMax = { logoRectStart.x + logoWidth, logoRectStart.y + logoHeight };

			// If the logo provided is smaller, shrink the image rectangle
			ImRect logoRect(logoRectStart, logoRectMax);
			logoRect.Expand(ImVec2(
				ImMin((int(sLogoImage->GetWidth()) - logoWidth) / 2, 0),
				ImMin((int(sLogoImage->GetHeight()) - logoHeight) / 2, 0)
			));

			fgDrawList->AddImage(sLogoImage->GetDescriptorSet(), logoRect.Min, logoRect.Max);
		}

	private:
		void ConnectVBAPVisToAudioPlayer(bool bShouldBeConnected)
		{
			if (bShouldBeConnected)
			{
				//! Note: this won't parse correctly layours with/without LFE, since it just uses number of channels
				//! (e.g. 5 channels may return Surround 4.1 channel set)
				//! However for the sake of the example application, it's fine, most of the users would be on stereo systems.
				const JPL::ChannelMap outputChannelSet = JPL::ChannelMap::FromNumChannels(JPL::GetMiniaudioEngine(nullptr).GetEndpointBus().GetNumChannels());

				mVBAPVis.TargetChannelMap.Set(JPL::NamedChannelMask(outputChannelSet.GetChannelMask()));
				mVBAPVis.SourceChannelMap.Set(JPL::NamedChannelMask(mSourceChannelSet.IsValid() ? mSourceChannelSet.GetChannelMask() : JPL::ChannelMask::Stereo));
			}
		}

	private:
		EventsLoop* mEventsLoop = nullptr;

		JPL::ChannelMap mSourceChannelSet;

		std::shared_ptr<DirectSoundModel> mDirectSoundModel;
		std::shared_ptr<LateReverbModel> mLateReverbModel;
		std::shared_ptr<AudioPlaybackLayer> mAudioPlaybackLayer;
		std::shared_ptr<RoomLayer> mRoomLayer;

		VBAPVisualizationModel mVBAPVis;
		VBAPVisualization mVBAPVisView{ mVBAPVis };

		static inline std::shared_ptr<Walnut::Image> sLogoImage{};
	};

/* TODO:
	- integrate FFT view with filter contrls
*/
} // namespace JPL

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "JPL Spatial Application";
	
	// this is the icon for taskbar and native titlebar if not using custom
	spec.IconData = std::span(JPL::Embedded::jplsa_app_icon, JPLSA_APP_ICON_WIDTH * JPLSA_APP_ICON_HEIGHT);
	spec.CustomTitlebar = true;
	
	spec.SettingsFile = "jplsa_settings.ini";
	spec.DefaultSettingsData = JPL::Embedded::gDefaultImGuiLayout;

	Walnut::Application* app = new Walnut::Application(spec);

	app->SetDrawLogoCallback(JPL::JPLSpatialApplicationLayer::DrawJPLSpatialApplicationLogo);
	app->SetDrawWindowButtonsCallback(JPL::ImGuiEx::DrawMainWindowButtons);

	auto editorLayer = std::make_shared<JPL::JPLSpatialApplicationLayer>();

	app->PushLayer(editorLayer);
	editorLayer->PushLayers(*app);

	// We dnon't need menubar for now
	/*app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});*/
	return app;
}