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

#include "CoreInclude.h"
#include "Utility/Command.h"

#include "choc/containers/choc_Value.h"

#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace JPL
{
	struct WindowState
	{
		bool bOpen = true;
	};

	struct CStrLess
	{
		inline bool operator()(const char* lhs, const char* rhs) const { return std::strcmp(lhs, rhs) < 0; }
	};

	//==========================================================================
	class JPLSpatialApplicationData
	{
		static constexpr int cVersion = 1;
		static constexpr std::string_view cClassName = "JPL Application Settings";

		inline static constexpr std::string_view cWindowStatesType = "Application Window States";
		static constexpr int cWindowStateVersion = 1;
	public:
		void Deserialize(const std::filesystem::path& filepath);
		void Serialize(const std::filesystem::path& filepath) const;

		static std::shared_ptr<JPLSpatialApplicationData> Load(const std::filesystem::path& filepath);

	private:
		static bool IsValidObject(const choc::value::Value& value);
		static choc::value::Value TryParse(std::string_view settingsJSON);

		choc::value::Value ToValue() const;
		void FromValue(const choc::value::Value& value);

		static choc::value::Value SerializeWindowStates();
		static void DeserializeWindowStates(const choc::value::ValueView& data);

	public:
		int WindowWidth = 1600;
		int WindowHeight = 900;
		bool WindowIsMaximized = false;

	private:
		friend class JPLSpatialApplication;

		// For now window state map is static to keep JPLSpatialApplicationData object lightweight
		inline static std::map<const char*, WindowState, CStrLess> sWindowStates;
		// We need to use a string table if we want the states to be serializable
		inline static std::vector<std::string> sWindowStringTable;
	};

	//==========================================================================
	class JPLSpatialApplication
	{
	public:
		static CommandHistory& GetCommandHistory() { return sCommandHistory; }

		/// Execute command and add it to command history
		static void ExecuteCommand(IUndoableCommand* command);

		/// Undo the last command from the global command history
		inline static void Undo() { sCommandHistory.Undo(); }

		/// Redo the last command undone from the global command history
		inline static void Redo() { sCommandHistory.Redo(); }

		/// Register window that can be opened and closed from a menu
		static void RegisterWindow(const char* name);

		static WindowState& GetWindowState(const char* windowName) { return JPLSpatialApplicationData::sWindowStates[windowName]; }
		static std::map<const char*, WindowState, CStrLess>& GetAllWindowStates() { return JPLSpatialApplicationData::sWindowStates; }

		static const std::shared_ptr<JPLSpatialApplicationData>& GetAppData() { return sAppData; }
		static const std::shared_ptr<JPLSpatialApplicationData>& DeserializeAppData(std::string_view filepath);
		static void SerializeAppData(std::string_view filepath);

	private:
		inline static CommandHistory sCommandHistory;
		inline static std::shared_ptr<JPLSpatialApplicationData> sAppData{ std::make_shared<JPLSpatialApplicationData>()};
	};
} // namespace JPL
