//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright Jaroslav Pevno 2026, JPL Spatial Application is offered under the terms of the ISC license:
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

#include "Log.h"

#include "ImGui/ImGui.h"

#include <magic_enum/magic_enum_containers.hpp>

#include <limits>
#include <string>
#include <vector>

namespace JPL::GUI
{
	class ConsoleLogGUI
	{
	public:
		ConsoleLogGUI();
		~ConsoleLogGUI() noexcept = default;

		void Draw();
		void ClearLog();

	private:
		void GetLatestLog();

		void DrawControlsPanel();
		void DrawSettingsPopup();
		void DrawFiltersPopup();
		void DrawTextFilterWidget();

		void DrawLog();

		static uint32 GetLogColour(ELogLevel level);

		void DrawLevelToggle(ELogLevel level);

		bool PassesFilter(const LogMessage& message) const;

	private:
		struct Properties
		{
			bool bWordWrap = false;
			bool bAutoScroll = true;

			magic_enum::containers::bitset<ELogLevel> LogLevelFlags;
		};

	private:
		std::vector<LogMessage> mCurrentLog;
		std::vector<LogMessage> mBuffer;
		
		// Temporary flag set to manually scroll to the bottom
		// e.g. when important message/event appears in the log
		bool bScrollToBottom = false;

		// Undoable
		std::shared_ptr<Properties> mProps;
		
		ImGuiTextFilter mTextFilter;
	};
} // namespace JPL::GUI
