//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatial
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
#include "Utility/MVCUtils.h"

#include "platform/choc_FileWatcher.h"

#include <functional>
#include <filesystem>
#include <memory>
#include <vector>

namespace JPL
{
	struct DirectoryDisplay;
	struct Directory;

	template<>
	class ChangeListener<Directory>
	{
	public:
		virtual ~ChangeListener() noexcept = default;

		virtual void OnSelectedFilePathChanged(const std::filesystem::path& newFilePath) const = 0;
	};

	struct Directory : public ChangeBroadcaster<Directory>
	{
	public:
		explicit Directory(const std::filesystem::path& directoryPath);

		const std::filesystem::path& GetPath() const { return mPath; }

		const std::vector<std::filesystem::path>& GetFiles() const { return mFiles; }
		const std::filesystem::path& GetSelectedFile() const { return mSelectedFile->Get(); }

		void SetDirectory(const std::filesystem::path& newDirectory);
		bool SetSelectedFile(const std::filesystem::path& fileRelative);

	private:
		inline void OnChange(const choc::file::Watcher::Event& event) { ParseDirectory(); }
		void ParseDirectory();

		bool CurrentDirectoryContains(const std::filesystem::path& absolutePath) const;

		void CommitSelectedFileChange(const std::filesystem::path& newSelectedFileAbs);

	private:
		// Absolute path to selected file
		std::shared_ptr<Property<std::filesystem::path>> mSelectedFile;

		std::filesystem::path mPath; // directory path
		std::unique_ptr<choc::file::Watcher> mWatcher = nullptr;

		std::vector<std::filesystem::path> mFiles;
	};

	struct DirectoryDisplay
	{
		static void Draw(Directory& directory);
	};
} // namespace JPL

