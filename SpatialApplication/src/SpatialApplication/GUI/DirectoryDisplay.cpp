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

#include "Application.h"
#include "Controller/AudioPlayer.h"
#include "ImGui/ImGui.h"

#include <JPLSpatial/Core.h>
#include <JPLSpatial/ErrorReporting.h>

#include <ImGuiFileDialog.h>

#include <algorithm>
#include <system_error>

namespace JPL
{
	void DirectoryDisplay::Draw(Directory& directory)
	{
		using namespace JPL::ImGuiEx;

		Child("##DirectoryDisplay", [&]
		{
			Layout<Spacer, Spacer>();

			LayoutHorizontal("##directory", [&]
			{
				ImGui::Spacing();
				{
					//ScopedFont titleFont(JPL::GUI::GetBoldFont());
					ScopedColour textColour(ImGuiCol_Text, GUI::Colours::Theme::TextSlightlyDarker);
					ScopedColour bgColour(ImGuiCol_FrameBg, IM_COL32_BLACK_TRANS);
					ScopedColour borderColour(ImGuiCol_Border, IM_COL32_BLACK_TRANS);

					// Give some space to the "Browse..." button
					const float width = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f - 100.0f;
					ScopedItemWidth itemWidth(width);
					
					std::string pathString = directory.GetPath().string();
					const int flags = ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_ElideLeft;

					// Draw actual text widget
					ImGui::InputText("##PathString", pathString.data(), pathString.size() + 1, flags);

					const ImVec2 textSize = ImGui::CalcTextSize(pathString.data(), pathString.data() + pathString.size() + 1);

					// Draw subtle shadow edge if the entire path string doesn't fit
					if (textSize.x > width)
					{
						ImGuiEx::DrawLeftShadowEdge(
							*ImGui::GetWindowDrawList(),
							ImGui::GetItemRectMin(),
							ImGui::GetItemRectMax(),
							ShadowEdgeStyle{ .ShadowSize = 20.0f, .AcrossSegments = 6 });
					}
				}

				ImGui::Spring();

				if (ImGuiEx::IconButton(ICON_jplsa_FOLDER"  Browse...", IconStyle::cIconLabelBgHovered))
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

			Layout<Separator, Spacer>();
			{
				// TODO: maybe wrap this into something like Layout<ShadowSeparator>, or straight to Layout<Separator>
				const ImVec2 bbMin = ImGui::GetItemRectMin() + ImVec2(0.0f, 1.0f);
				const ImVec2 bbMax = ImGui::GetItemRectMax() + ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 4);
				DrawTopShadowEdge(*ImGui::GetWindowDrawList(), bbMin, bbMax,
								  ShadowEdgeStyle{
									  .ShadowSize = 14.0f,
									  .FalloffPower = 3.0f,
									  .Colour = IM_COL32(0, 0, 0, 70)});
			}

			Child("##FilesList", [&]
			{
				const auto& selectedFile = directory.GetSelectedFile();
				const auto selectedRelative = std::filesystem::relative(selectedFile, directory.GetPath());

				// ImGui extends item size by about half spacing,
				// which can cause first item's upper extent to be clipped
				ShiftCursorY(ImGui::GetStyle().ItemSpacing.y * 0.5f);

				// ImGui ignores frame padding for ImGui::Selectable
				// so we add it ourselves
				ImGui::Indent(ImGui::GetStyle().FramePadding.x);

				auto drawEntry = [&](const std::filesystem::path& path)
				{
					bool selected = path == selectedRelative;

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
		: mSelectedFile(std::make_shared<Property<std::filesystem::path>>())
	{
		SetDirectory(directoryPath);

		mSelectedFile->AddChangeCallback(this, [](Directory* self, const std::filesystem::path& selectedFile)
		{
			// If directory was changed, don't accept file it doesn't contain
			if (selectedFile.empty() or self->CurrentDirectoryContains(selectedFile))
				self->Broadcast<&ChangeListenerType::OnSelectedFilePathChanged>(selectedFile);
		});
	}

	void Directory::SetDirectory(const std::filesystem::path& newDirectory)
	{
		if (not std::filesystem::is_directory(newDirectory) || newDirectory == mPath)
			return;

		// Directory change will be not undoable,
		// because it triggers selected file change,
		// which would not be valid to undo unless undoing
		// the directory change as well.

		mPath = newDirectory;
		mWatcher = std::make_unique<choc::file::Watcher>(mPath, [this](const choc::file::Watcher::Event& ev)
		{
			OnChange(ev);
		});

		ParseDirectory();
	}

	bool Directory::SetSelectedFile(const std::filesystem::path& fileRelative)
	{
		if (mPath.empty() or not JPL_ENSURE(mSelectedFile)) [[unlikely]]
			return false;

		auto fileAbs = mPath / fileRelative;

		if (fileAbs == mSelectedFile->Get())
			return false;

		if (fileRelative.empty()) [[unlikely]]
		{
			CommitSelectedFileChange({});
			return true;
		}
		else if (std::find(mFiles.begin(), mFiles.end(), fileRelative) != mFiles.end()) [[likely]]
		{
			CommitSelectedFileChange(fileAbs);
			return true;
		}
		else [[unlikely]]
		{
			return false;
		}
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
			Log::Error("Directory: {}", errorCode.message());
		}

		if (not JPL_ENSURE(mSelectedFile))
			return;
		
		const std::filesystem::path oldPath = mSelectedFile->Get();

		if (not oldPath.empty())
		{
			// Clear selected file if it's not in the new directory
			if (not CurrentDirectoryContains(oldPath))
			{
				// Selected file change is not undoable
				// when triggered by directory change
				mSelectedFile->Set({});
			}
		}
	}

	bool Directory::CurrentDirectoryContains(const std::filesystem::path& absolutePath) const
	{
		auto relative = std::filesystem::relative(absolutePath, mPath);
		return std::find(mFiles.begin(), mFiles.end(), relative) != mFiles.end();
	}

	void Directory::CommitSelectedFileChange(const std::filesystem::path& newSelectedFileAbs)
	{
		const std::filesystem::path oldPath = mSelectedFile->Get();

		mSelectedFile->Set(newSelectedFileAbs);

		JPLSpatialApplication::GetCommandHistory().PropertyEdited(Undoable(mSelectedFile), OldValue(oldPath), "Selected Source");
	}
} // namespace JPL
