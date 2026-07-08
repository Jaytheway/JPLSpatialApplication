#include "ImGuiLayout.h"

namespace JPL::ImGuiEx
{

	void RenderCustomTitleBar(ImGuiWindow* window, const char* name, bool* p_open)
	{
        ImGuiContext& g = *GImGui;
        ImGuiStyle& style = g.Style;
        ImGuiWindowFlags flags = window->Flags;

        if (window->DockIsActive)
            return;

        ScopedFont titleBarFont(GUI::GetBoldFont(), 17.0f);

        ImRect title_bar_rect = window->TitleBarRect();
        title_bar_rect.Expand(ImVec2(-window->WindowBorderSize, 0.0f));

        const float titleBarHeight = ImMax(30.0f, g.FontSize + g.Style.FramePadding.y * 2.0f);
        title_bar_rect.Max.y = title_bar_rect.Min.y + titleBarHeight;

        // Background fill
        {
            const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
            const bool title_bar_is_highlight = /*want_focus ||*/
                (window_to_highlight && (window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight));

            ImU32 title_bar_col = ImGui::GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
            if (window->ViewportOwned)
                title_bar_col |= IM_COL32_A_MASK; // No alpha

            window->DrawList->AddRectFilled(
                title_bar_rect.Min,
                title_bar_rect.Max,
                title_bar_col,
                style.WindowRounding,
                ImDrawFlags_RoundCornersTop);
        }

        const bool has_close_button = (p_open != NULL);

        //! See the note below
        //const bool has_collapse_button = !(flags & ImGuiWindowFlags_NoCollapse) && (style.WindowMenuButtonPosition != ImGuiDir_None);

        // Close & Collapse button are on the Menu NavLayer and don't default focus (unless there's nothing else on that layer)
        // FIXME-NAV: Might want (or not?) to set the equivalent of ImGuiButtonFlags_NoNavFocus so that mouse clicks on standard title bar items don't necessarily set nav/keyboard ref?
        const ImGuiItemFlags item_flags_backup = g.CurrentItemFlags;
        g.CurrentItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
        window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

        // Layout buttons
        // FIXME: Would be nice to generalize the subtleties expressed here into reusable code.
        float pad_l = style.FramePadding.x;
        float pad_r = 0.0f;// style.FramePadding.x;
        float button_sz = 36.0f;// titleBarHeight;// g.FontSize;
        //ImVec2 close_button_pos;
        //ImVec2 collapse_button_pos;
        if (has_close_button)
        {
            //close_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - button_sz, title_bar_rect.Min.y + style.FramePadding.y);
            pad_r += button_sz + style.ItemInnerSpacing.x;
        }

        //! Note (JP): ImGui resets Collapse flags in ImGui::Begin if NoTitleBar flag is set,
        //! which is what we do to draw our own title bar, therefore we're not going deal with it,
        //! the utility of collapsing windows is rather questionable, especially when you have
        //! a menu from which windows can be enabled and disabled.
#if 0
        if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Right)
        {
            collapse_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - button_sz, title_bar_rect.Min.y + style.FramePadding.y);
            pad_r += button_sz + style.ItemInnerSpacing.x;
        }
        if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Left)
        {
            collapse_button_pos = ImVec2(title_bar_rect.Min.x + pad_l, title_bar_rect.Min.y + style.FramePadding.y);
            pad_l += button_sz + style.ItemInnerSpacing.x;
        }

        // Collapse button (submitting first so it gets priority when choosing a navigation init fallback)
        if (has_collapse_button)
        {
            if (ImGui::CollapseButton(window->GetID("#COLLAPSE"), collapse_button_pos, NULL))
                ImGui::SetWindowCollapsed(window, true);
        }
#endif

        // Close button
        if (has_close_button)
        {
            ImGuiItemFlags backup_item_flags = g.CurrentItemFlags;
            g.CurrentItemFlags |= ImGuiItemFlags_NoFocus;

            const ImVec2 closeButtonPos(title_bar_rect.Max.x - button_sz, title_bar_rect.Min.y);
            const ImVec2 closeButtonSize(button_sz, titleBarHeight);

            ImGui::SetCursorScreenPos(closeButtonPos);

            if (ImGuiEx::CloseButton("#CLOSE", closeButtonSize))
                *p_open = false;
            g.CurrentItemFlags = backup_item_flags;
        }

        window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
        g.CurrentItemFlags = item_flags_backup;

        // Title bar text (with: horizontal alignment, avoiding collapse/close button, optional "unsaved document" marker)
        // FIXME: Refactor text alignment facilities along with RenderText helpers, this is WAY too much messy code..
        const float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? button_sz * 0.80f : 0.0f;
        const ImVec2 text_size = ImGui::CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);

        // As a nice touch we try to ensure that centered title text doesn't get affected by visibility of Close/Collapse button,
        // while uncentered title text will still reach edges correctly.
        if (pad_l > style.FramePadding.x)
            pad_l += g.Style.ItemInnerSpacing.x;
        if (pad_r > style.FramePadding.x)
            pad_r += g.Style.ItemInnerSpacing.x;
        if (style.WindowTitleAlign.x > 0.0f && style.WindowTitleAlign.x < 1.0f)
        {
            float centerness = ImSaturate(1.0f - ImFabs(style.WindowTitleAlign.x - 0.5f) * 2.0f); // 0.0f on either edges, 1.0f on center
            float pad_extend = ImMin(ImMax(pad_l, pad_r), title_bar_rect.GetWidth() - pad_l - pad_r - text_size.x);
            pad_l = ImMax(pad_l, pad_extend * centerness);
            pad_r = ImMax(pad_r, pad_extend * centerness);
        }

        ImRect layout_r(title_bar_rect.Min.x + pad_l, title_bar_rect.Min.y, title_bar_rect.Max.x - pad_r, title_bar_rect.Max.y);
        ImRect clip_r(layout_r.Min.x, layout_r.Min.y, ImMin(layout_r.Max.x + g.Style.ItemInnerSpacing.x, title_bar_rect.Max.x), layout_r.Max.y);
        if (flags & ImGuiWindowFlags_UnsavedDocument)
        {
            ImVec2 marker_pos;
            marker_pos.x = ImClamp(layout_r.Min.x + (layout_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x + text_size.x, layout_r.Min.x, layout_r.Max.x);
            marker_pos.y = (layout_r.Min.y + layout_r.Max.y) * 0.5f;
            if (marker_pos.x > layout_r.Min.x)
            {
                ImGui::RenderBullet(window->DrawList, marker_pos, ImGui::GetColorU32(ImGuiCol_UnsavedMarker));
                clip_r.Max.x = ImMin(clip_r.Max.x, marker_pos.x - (int)(marker_size_x * 0.5f));
            }
        }
        //if (g.IO.KeyShift) window->DrawList->AddRect(layout_r.Min, layout_r.Max, IM_COL32(255, 128, 0, 255)); // [DEBUG]
        //if (g.IO.KeyCtrl) window->DrawList->AddRect(clip_r.Min, clip_r.Max, IM_COL32(255, 128, 0, 255)); // [DEBUG]
        ImGui::RenderTextClipped(layout_r.Min, layout_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_r);
        
        // Advance cursor
        if (not has_close_button)
            ImGui::ItemSize(title_bar_rect);
	}
}
