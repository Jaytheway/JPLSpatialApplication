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

#include "DirectoryDisplay.h"

#include "Controller/AudioPlayer.h"

#include "ImGui/ImGui.h"

#include <ImGuiFileDialog.h>

namespace JPL
{
	void DirectoryDisplay::Draw(Directory& directory)
	{
		using namespace JPL::ImGuiEx;

		Child("##DirectoryDisplay", [&]
		{
			ImGui::Spacing();
			ImGui::Spacing();

			LayoutHorizontal("##directory", [&]
			{
				ImGui::Spacing();
				{
					ScopedFont titleFont(JPL::GUI::GetBoldFont());

					// Give some space to the "Browse..." button
					const float width = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 4.0f - 20.0f;
					ScopedTextWrap wrapAt(width);

					ImGui::Text(directory.GetPath().string().c_str());
				}

				ImGui::Spring();

				if (ImGui::Button((const char*)ICON_jplsa_FOLDER"  Browse..."))
				{
					const int flags = ImGuiFileDialogFlags_DontShowHiddenFiles
						| ImGuiFileDialogFlags_DisableCreateDirectoryButton
						| ImGuiFileDialogFlags_Modal
						| ImGuiFileDialogFlags_DisableThumbnailMode
						| ImGuiFileDialogFlags_ShowDevicesButton;

					IGFD::FileDialogConfig config;
					config.path = directory.GetPath().string();
					config.flags = flags;
					ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDlgKey", "Select Sources Folder", nullptr, config);
				}

				{
					// ImGuiDialogue forces alternating colours for row,
					// to make things more readable, we set that colour to same as non-alternating
					ScopedColour tableRowAlt(ImGuiCol_TableRowBgAlt, ImGui::GetColorU32(ImGuiCol_TableRowBg));

					const ImVec2 minSize(400.0f, 300.0f);
					if (ImGuiFileDialog::Instance()->Display("ChooseDirDlgKey", 32, minSize))
					{
						if (ImGuiFileDialog::Instance()->IsOk())
						{
							directory.SetDirectory(ImGuiFileDialog::Instance()->GetCurrentPath());
						}

						ImGuiFileDialog::Instance()->Close();
					}
				}

				ImGui::Dummy({ ImGui::GetStyle().ItemSpacing.x * 0.5f, 0.0f });

			}, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
			ImGui::Separator();
			ImGui::Spacing();

			Child("##FilesList", [&]
			{
				const auto& selectedFile = directory.GetSelectedFile();

				auto drawEntry = [&](const std::filesystem::path& path)
				{
					bool selected = path == selectedFile;

					if (ImGui::Selectable(path.string().c_str(), &selected))
						directory.SetSelectedFile(path);
				};

				for (const auto& entry : directory.GetFiles())
				{
					ScopedColour itemBg(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_TabSelectedOverline, 0.5f));
					drawEntry(entry);
				}
			});
		});
	}

	Directory::Directory(const std::filesystem::path& directoryPath)
	{
		SetDirectory(directoryPath);
	}

	void Directory::SetDirectory(const std::filesystem::path& newDirectory)
	{
		if (not std::filesystem::is_directory(newDirectory) || newDirectory == mPath)
			return;

		mPath = newDirectory;
		mWatcher = std::make_unique<choc::file::Watcher>(mPath, [this](const choc::file::Watcher::Event& ev)
		{
			OnChange(ev);
		});

		ParseDirectory();
	}

	void Directory::SetSelectedFile(const std::filesystem::path& file)
	{
		if (mPath.empty()) [[unlikely]]
			return;

		if (file.empty()) [[unlikely]]
		{
			mSelectedFile.clear();
		}
		else if (std::find(mFiles.begin(), mFiles.end(), file) != mFiles.end()) [[likely]]
		{
			mSelectedFile = file;
		}
		else [[unlikely]]
		{
			return;
		}

		if (onSelectionChanged)
			onSelectionChanged(mPath / mSelectedFile);
	}

	void Directory::ParseDirectory()
	{
		mFiles.clear();

		std::error_code errorCode = {};

		static constexpr std::filesystem::directory_options iterOptions
			= std::filesystem::directory_options::skip_permission_denied;

		for (auto& entry : std::filesystem::recursive_directory_iterator(mPath, iterOptions, errorCode))
		{
			if (errorCode)
				break;

			const auto& path = entry.path();

			if (AudioPlayer::IsValidAudioFile(path))
			{
				mFiles.push_back(std::filesystem::relative(path, mPath));
			}
		}

		if (errorCode)
		{
			JPL_ERROR_TAG("Directory", errorCode.message());
		}

		if (std::find(mFiles.begin(), mFiles.end(), mSelectedFile.filename()) == mFiles.end())
			mSelectedFile.clear();
	}
} // namespace JPL
