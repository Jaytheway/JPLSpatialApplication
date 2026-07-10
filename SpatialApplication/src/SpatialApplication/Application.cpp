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

#include "Application.h"

#include "ImGui/ImGui.h"
#include "Config.h"

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"

#include <MiniaudioCpp/MiniaudioWrappers.h>

#include "GUI/DirectoryDisplay.h"
#include "GUI/VBAPVisualization.h"
#include "GUI/PerformanceGUI.h"
#include "GUI/ConsoleLogGUI.h"

#include "Model/VBAPVisualizationModel.h"
#include "Model/DirectSoundModel.h"
#include "Model/LateReverbModel.h"

#include "Layers/AudioPlaybackLayer.h"
#include "Layers/RoomLayer.h"

#include "Utility/PerformanceMetering.h"
#include "Systems/EventsLoop.h"

#include "choc/text/choc_Files.h"
#include "choc/text/choc_JSON.h"

#include <implot.h>

#include <filesystem>
#include <memory>
#include <string>

namespace JPL
{
	namespace Embedded
	{
		#include "GUI/Embed/JPLSpatialApplication-Logo.embed"
		#include "GUI/Embed/JPLSpatialApplication-Icon.embed"
		#include "GUI/Embed/DefaultImGuiSettings.embed"
	} // namespace Embedded

	// TODO: this may become modifiable (i.e. to save in OS's AppData directory)
	constexpr const char* cAppDataFilename = "jplsa_app_data.json";
	
	//==========================================================================
	void JPLSpatialApplication::SerializeAppData(std::string_view filepath)
	{
		sAppData->Serialize(filepath);
	}

	const std::shared_ptr<JPLSpatialApplicationData>& JPLSpatialApplication::DeserializeAppData(std::string_view filepath)
	{
		sAppData = JPLSpatialApplicationData::Load(filepath);
		return sAppData;
	}

	void JPLSpatialApplication::RegisterWindow(const char* name)
	{
		JPLSpatialApplicationData::sWindowStates.try_emplace(name, /* bOpen */ true);
	}

	void JPLSpatialApplication::ExecuteCommand(IUndoableCommand* command)
	{
		if (command)
		{
			command->Execute();
			sCommandHistory.Add(std::unique_ptr<IUndoableCommand>(command));
		}
	}

	//==========================================================================
	bool JPLSpatialApplicationData::IsValidObject(const choc::value::Value& value)
	{
		return value.isObject() and value.hasObjectMember("Type") and value["Type"].getString() == cClassName;
	}

	choc::value::Value JPLSpatialApplicationData::ToValue() const
	{
		return choc::json::create(
			"Type", cClassName,
			"Version", 1,
			"WindowWidth", WindowWidth,
			"WindowHeight", WindowHeight,
			"WindowIsMaximized", WindowIsMaximized
		);
	}

	void JPLSpatialApplicationData::FromValue(const choc::value::Value& value)
	{
		if (not IsValidObject(value))
			return;

		// Ensure we deserialize valid values from application settings .json
		const int width = value["WindowWidth"].get<int>();
		const int height = value["WindowHeight"].get<int>();
		if (width > 0) WindowWidth = width;
		if (height > 0) WindowHeight = height;
		
		WindowIsMaximized = value["WindowIsMaximized"].getBool();
			 
		if (value.hasObjectMember(cWindowStatesType))
			DeserializeWindowStates(value[cWindowStatesType]);
	}

	choc::value::Value JPLSpatialApplicationData::TryParse(std::string_view settingsJSON)
	{
		choc::value::Value settings = choc::json::parse(settingsJSON);

		if (JPL_ENSURE(IsValidObject(settings)))
		{
			const int version = settings["Version"].get<int>();
			if (version == cVersion)
			{
				return settings;
			}
			else
			{
				// TODO: handle version updates
				Log::Warn("Loaded JPL Application Settings with older version {}, current version {}.", version, cVersion);
			}
		}

		return {};
	}

	void JPLSpatialApplicationData::Deserialize(const std::filesystem::path& filepath)
	{
		if (std::filesystem::exists(filepath)) // Load application data from file
		{
			try
			{
				const std::string settingsStr = choc::file::loadFileAsString(filepath.string());
				const choc::value::Value jsonObject = JPLSpatialApplicationData::TryParse(settingsStr);
				if (not jsonObject.isVoid())
					FromValue(jsonObject);
			}
			catch (const choc::file::Error& error)
			{
				Log::Error("Failed to deserialize Application Data from file '{}': {}", filepath.string(), error.what());

			}
			catch (const choc::json::ParseError& error)
			{
				Log::Error("Failed to deserialize Application Data from file '{}': {}", filepath.string(), error.what());
			}
		}
		else // Create default application data and save to file
		{
			try
			{
				choc::value::Value jsonObject = ToValue(); // default value
				const std::string settingsStr = choc::json::toString(jsonObject, /* useLineBreaks */ true);
				if (not settingsStr.empty())
					choc::file::replaceFileWithContent(filepath, settingsStr);
			}
			catch (const choc::file::Error& error)
			{
				Log::Error("Failed to create default Application Data for file '{}': {}", filepath.string(), error.what());
			}
		}
	}

	void JPLSpatialApplicationData::Serialize(const std::filesystem::path& filepath) const
	{
		try
		{
			choc::value::Value jsonObject = ToValue();
			const std::string settingsStr = choc::json::toString(jsonObject, /* useLineBreaks */ true);
			if (not settingsStr.empty())
				choc::file::replaceFileWithContent(filepath, settingsStr);

			SerializeWindowStates();
		}
		catch (const choc::file::Error& error)
		{
			Log::Error("Failed to serialize Application Data: {}", error.what());
		}
	}

	void JPLSpatialApplicationData::DeserializeWindowStates(const choc::value::ValueView& data)
	{
		if (not data.hasObjectMember("Type") or data["Type"].getString() != cWindowStatesType)
			return;

		if (data["Version"].get<int>() != cWindowStateVersion)
			return; // TODO: log version errror, or implement conversion

		const auto& states = data["Windows"];

		sWindowStringTable.clear();
		sWindowStringTable.reserve(states.size());

		states.visitObjectMembers([&](std::string_view name, const choc::value::ValueView& value)
		{
			if (not name.empty())
			{
				const auto& nameStr = sWindowStringTable.emplace_back(name);
				sWindowStates[nameStr.c_str()] = { value.getBool() };
			}
		});
	}

	choc::value::Value JPLSpatialApplicationData::SerializeWindowStates()
	{
		choc::value::Value data = choc::json::create(
			"Type", cWindowStatesType,
			"Version", cWindowStateVersion
		);

		choc::value::Value states = choc::value::createObject("");

		//! For now only serializing oppennes state
		for (auto&& [label, state] : JPLSpatialApplicationData::sWindowStates)
			states.addMember(label, state.bOpen);

		data.addMember("Windows", states);

		return data;
	}

	std::shared_ptr<JPLSpatialApplicationData> JPLSpatialApplicationData::Load(const std::filesystem::path& filepath)
	{
		std::shared_ptr<JPLSpatialApplicationData> appData = std::make_shared<JPLSpatialApplicationData>();
		appData->Deserialize(filepath);
		return appData;
	}

	//==========================================================================
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
			mRoomLayer = std::make_shared<RoomLayer>(mDirectSoundModel, mLateReverbModel, mVBAPVisView);

			mAudioPlaybackLayer->SetVBAPModel(mVBAPVis->VBAPModel);

			mAudioPlaybackLayer->AddListener(this);
			mRoomLayer->AddListener(this);

			mRoomLayer->GetModel().EnableDirect.AddChangeCallback<&AudioPlaybackLayer::SetEnableDirectSound>(mAudioPlaybackLayer.get());
			mVBAPVis->ConnectToAudioPlayer.AddChangeCallback<&JPLSpatialApplicationLayer::ConnectVBAPVisToAudioPlayer>(this);

			mVBAPVis->VBAPModel->SourceSize.AddChangeCallback<&RoomLayer::SetSourceSize>(mRoomLayer.get());
			mRoomLayer->SetSourceSize(mVBAPVis->VBAPModel->SourceSize.Get());
		}

		~JPLSpatialApplicationLayer()
		{
			Log::Uninit();
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
			// Serialize application window state
			{
				const auto& app = Walnut::Application::Get();
				const auto [width, height] = app.GetWindowClientSize();

				auto appData = JPLSpatialApplication::GetAppData();
				appData->WindowWidth = width;
				appData->WindowHeight = height;
				appData->WindowIsMaximized = app.IsMaximized();

				JPLSpatialApplication::SerializeAppData(cAppDataFilename);
			}

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
				mVBAPVisView->Draw();
			}, nullptr, { .Flags = ImGuiWindowFlags_NoCollapse });

#if 0
			ImGuiEx::Window("Perf.", [&]
			{
				//const uint32_t sampleFrequencyMs =
				//	static_cast<uint32_t>(GetMAEngine().GetProcessingSizeInFrames() / mSampleRate * 1000.0f);
				//PerformanceMeterGUI<PerfMeterAudioCallback>::Draw(sampleFrequencyMs);

				PerformanceMeterGUI<PerfMeterUpdateTaps>::Draw(0);

				PerformanceMeterGUI<PerfMeterRayTracing>::Draw(0);
			});
#endif

			// TODO: temporarily placing global shortcuts here
			// Note: _RouteGlobal won't hijack active text input widgets
			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
				JPLSpatialApplication::Undo();

			if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
				JPLSpatialApplication::Redo();

			mLogGUI.Draw();
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

			if (mVBAPVis->ConnectToAudioPlayer.Get())
			{
				mVBAPVis->SourceChannelMap.Set(JPL::NamedChannelMask(mSourceChannelSet.GetChannelMask()));
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
			mVBAPVis->VBAPModel->SourcePosition.Set(newPosition);
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

				mVBAPVis->TargetChannelMap.Set(JPL::NamedChannelMask(outputChannelSet.GetChannelMask()));
				mVBAPVis->SourceChannelMap.Set(JPL::NamedChannelMask(mSourceChannelSet.IsValid() ? mSourceChannelSet.GetChannelMask() : JPL::ChannelMask::Stereo));
			}
		}

	private:
		EventsLoop* mEventsLoop = nullptr;

		JPL::ChannelMap mSourceChannelSet;

		std::shared_ptr<DirectSoundModel> mDirectSoundModel;
		std::shared_ptr<LateReverbModel> mLateReverbModel;
		std::shared_ptr<AudioPlaybackLayer> mAudioPlaybackLayer;
		std::shared_ptr<RoomLayer> mRoomLayer;

		std::shared_ptr<VBAPVisualizationModel> mVBAPVis{ std::make_shared<VBAPVisualizationModel>() };
		std::shared_ptr<VBAPVisualization> mVBAPVisView{ std::make_shared<VBAPVisualization>(mVBAPVis) };

		static inline std::shared_ptr<Walnut::Image> sLogoImage{};

		GUI::ConsoleLogGUI mLogGUI;
	};

/* TODO:
	- integrate FFT view with filter contrls
*/
} // namespace JPL


// Overrides both, JPL Spatial and MiniaudioCpp trace callback
template<JPL::ELogLevel LogLevel>
static void JPLLog(std::string_view tag, std::string_view message)
{
	if (tag == "miniaudio")
	{
		// Parse logs comming from miniaudio, formatted by MiniaudioCpp

		//! Note: we may or may not want to print log level in the log,
		//! for now leaving it in to know what to disable in GUI
		auto trimMessage = [](std::string_view message, int miniaudioLogLevel)
		{
#if 0
			std::string_view maLL = ma_log_level_to_string(miniaudioLogLevel);
			message.remove_prefix(maLL.length());
			if (message.starts_with(": "))
				message.remove_prefix(2);
#endif
			return message;
		};

		if (message.starts_with(ma_log_level_to_string(MA_LOG_LEVEL_DEBUG)))
		{
			JPL::Log::Print(JPL::ELogLevel::Debug, "JPL: miniaudio: {}", trimMessage(message, MA_LOG_LEVEL_DEBUG));
		}
		else if (message.starts_with(ma_log_level_to_string(MA_LOG_LEVEL_INFO)))
		{
			JPL::Log::Print(JPL::ELogLevel::Trace, "JPL: miniaudio: {}", trimMessage(message, MA_LOG_LEVEL_INFO)); // miniaudio info -> trace
		}
		else if (message.starts_with(ma_log_level_to_string(MA_LOG_LEVEL_WARNING)))
		{
			JPL::Log::Print(JPL::ELogLevel::Warn, "JPL: miniaudio: {}", trimMessage(message, MA_LOG_LEVEL_WARNING));
		}
		else
		{
			JPL::Log::Print(JPL::ELogLevel::Error, "JPL: miniaudio: {}", trimMessage(message, MA_LOG_LEVEL_ERROR));
		}
	}
	else
	{
		JPL::Log::Print(LogLevel, "JPL: {}: {}", tag, message);
	}
}
JPL::TaggedTraceFunction JPL::JPLTraceTaggedTrace = JPLLog<JPL::ELogLevel::Trace>;
JPL::TaggedTraceFunction JPL::JPLTraceTaggedInfo = JPLLog<JPL::ELogLevel::Info>;
JPL::TaggedTraceFunction JPL::JPLTraceTaggedWarn = JPLLog<JPL::ELogLevel::Warn>;
JPL::TaggedTraceFunction JPL::JPLTraceTaggedError = JPLLog<JPL::ELogLevel::Error>;

// Overrides both, JPL Spatial and MiniaudioCpp assertion failed callback
#if defined(JPL_ENABLE_ASSERTS) || defined(JPL_ENABLE_ENSURE)
static bool JPLAssertionFailedCb(const char* inExpression, const char* inMessage, const std::source_location location)
{
	// Print assertion details to the log
	const auto messageString = std::format(
		"ASSERT FAILED in file '{}' at line {}\n"
		"\n"
		"  Function: {}.\n"
		"Expression: {}"
		"{}{}", // message if provided
		location.file_name(),
		location.line(),
		location.function_name(),
		inExpression,
		inMessage ? "\n   Message: " : "",
		inMessage ? inMessage : "");

	JPL::Log::Print(JPL::ELogLevel::Critical, messageString);

	return true; // Trigger breakpoint
};
JPL::AssertFailedFunction JPL::AssertFailed = JPLAssertionFailedCb;
#endif

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	JPL::Log::Init(); // Initialize logger as soon as possible (unitit in app layer destructor)

	// Deserialize application window state
	auto appData = JPL::JPLSpatialApplication::DeserializeAppData(JPL::cAppDataFilename);

	Walnut::ApplicationSpecification spec;
	spec.Name = "JPL Spatial Application";
	spec.Width = appData->WindowWidth;
	spec.Height = appData->WindowHeight;
	
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
	// Maximize async, in case some glfw code needs to run after this function returns
	if (appData->WindowIsMaximized and not app->IsMaximized())
	{
		JPL::EventsLoop::Schedule([]
		{
			auto& app = Walnut::Application::Get();
			if (not app.IsMaximized())
				app.ToggleMaximize();
		});
	}
	return app;
}