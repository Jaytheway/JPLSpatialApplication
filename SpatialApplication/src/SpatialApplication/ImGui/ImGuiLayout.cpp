#include "ImGuiLayout.h"

namespace JPL::ImGuiEx
{
	void Impl::PushMenuStyle()
	{
		auto& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.WindowPadding.x, 4.0f));
		// When ImGui Draws MenuItem, for some reason FramePadding and ItemSpacing semantics are swapped,
		// specifically when drawing the highlightable background.
		// We want tighter spacing, but larger item rectangle
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 8.0f));
	}
	void Impl::PopMenuStyle()
	{
		ImGui::PopStyleVar(3);
	}

	void Impl::RenderCustomTitleBarDecorations(ImGuiWindow* window, const char* name, bool* p_open)
	{
        ImGuiContext& g = *GImGui;
        ImGuiStyle& style = g.Style;
        ImGuiWindowFlags flags = window->Flags;

        if (window->DockIsActive)
            return;

        // Note: we let ImGui draw title bar background and title, and only draw our custom buttons and UX subtleties

        const ImRect titleBarRect = window->TitleBarRect();
        const float titleBarHeight = titleBarRect.GetHeight();

        // ImGui::Begin clips titlebar area out
        ImGui::PushClipRect(titleBarRect.Min, titleBarRect.Max + ImVec2(0.0f, 4.0f), false);

        // Background fill and outlines
        {
#if 0       
            const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
            const bool title_bar_is_highlight = /*want_focus ||*/
                (window_to_highlight && (window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight));

            ImU32 title_bar_col = ImGui::GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
            if (window->ViewportOwned)
                title_bar_col |= IM_COL32_A_MASK; // No alpha

            window->DrawList->AddRectFilled(
                titleBarRect.Min,
                titleBarRect.Max,
                title_bar_col,
                style.WindowRounding,
                ImDrawFlags_RoundCornersTop);

			window->DrawList->AddRect(
				titleBarRect.Min,
				titleBarRect.Max,
				ImGui::GetColorU32(ImGuiCol_Border),
				IM_COL32(255, 180, 0, 255),
				style.WindowRounding,
				ImDrawFlags_RoundCornersTop,
				style.WindowBorderSize);
#endif
            // Subtle shadow underline
            window->DrawList->AddLine(
                titleBarRect.GetBL() + ImVec2(0.0f, 1.0f),
                titleBarRect.GetBR() + ImVec2(0.0f, 1.0f),
                Colour::BlackWithAlpha(0.2f));
        }

        const bool bHasCollapseButton = !(flags & ImGuiWindowFlags_NoCollapse);
        const bool bHasCloseButton = (p_open != NULL);

        // Close & Collapse button are on the Menu NavLayer and don't default focus (unless there's nothing else on that layer)
        // FIXME-NAV: Might want (or not?) to set the equivalent of ImGuiButtonFlags_NoNavFocus so that mouse clicks on standard title bar items don't necessarily set nav/keyboard ref?
        const ImGuiItemFlags itemFlagsBackup = g.CurrentItemFlags;
        g.CurrentItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
        window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

        // Layout buttons
        const float buttonSize = 36.0f;

        // Collapse button (submitting first so it gets priority when choosing a navigation init fallback)
        if (bHasCollapseButton)
        {
            const ImVec2 collapseButtonPos = ImVec2(titleBarRect.Min.x, titleBarRect.Min.y);
            const ImVec2 collapseButtonSize(buttonSize, titleBarHeight);
            const ImRect collapseButtonBB(collapseButtonPos, collapseButtonPos + collapseButtonSize);

            if (ImGuiEx::CollapseButton("#COLLAPSE_JPL", collapseButtonBB, window->Collapsed))
                window->WantCollapseToggle = true;
        }

        // Close button
        if (bHasCloseButton)
        {
            const ImGuiItemFlags backupItemFlags = g.CurrentItemFlags;
            g.CurrentItemFlags |= ImGuiItemFlags_NoFocus;

            const ImVec2 closeButtonPos(titleBarRect.Max.x - buttonSize, titleBarRect.Min.y);
            const ImVec2 closeButtonSize(buttonSize, titleBarHeight);
            const ImRect closeButtonBB(closeButtonPos, closeButtonPos + closeButtonSize);

            if (ImGuiEx::CloseButton("#CLOSE_JPL", closeButtonBB))
                *p_open = false;

            g.CurrentItemFlags = backupItemFlags;
        }

        window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
        g.CurrentItemFlags = itemFlagsBackup;
        
        ImGui::PopClipRect();
	}
}
