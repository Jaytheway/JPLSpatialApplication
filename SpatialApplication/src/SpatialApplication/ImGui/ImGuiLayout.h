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

#pragma once

#include "ImGui/ImGui.h"

#include <JPLSpatial/Utilities/TypeUtilities.h>

#include <concepts>
#include <ranges>
#include <type_traits>

namespace JPL::ImGuiEx
{
    //======================================================================
    /// Layout

    template<class IDType>
    concept CValidLayoutIDType =
        std::same_as<IDType, const char*> ||
        std::same_as <IDType, const void*> ||
        std::same_as <IDType, int>;

    template<class DrawFunction, CValidLayoutIDType IDType>
    void LayoutHorizontal(IDType id, const DrawFunction& draw, const ImVec2& size = ImVec2(0, 0), float align = -1.0f)
    {
        ImGui::BeginHorizontal(id, size, align);
        draw();
        ImGui::EndHorizontal();
    }

    template<class DrawFunction, CValidLayoutIDType IDType>
    void LayoutVertical(IDType id, const DrawFunction& draw, const ImVec2& size = ImVec2(0, 0), float align = -1.0f)
    {
        ImGui::BeginVertical(id, size, align);
        draw();
        ImGui::EndVertical();
    }

    template<class DrawFunction, CValidLayoutIDType IDType>
    void LayoutHorizontal(IDType id, const ImVec2& size, float align, const DrawFunction& draw)
    {
        LayoutHorizontal(id, draw, size, align);
    }

    template<class DrawFunction, CValidLayoutIDType IDType>
    void LayoutVertical(IDType id, const ImVec2& size, float align, const DrawFunction& draw)
    {
        LayoutVertical(id, draw, size, align);
    }

    //======================================================================
    /// Windows

    template<class IDType>
    concept CValidWindowIDType =
        std::same_as<IDType, const char*> ||
        std::same_as <IDType, ImGuiID>;

    struct ChildConfig
    {
        ImVec2 Size{ 0.0f, 0.0f };
        ImVec2 MinSize{ 0.0f, 0.0f };
        ImVec2 MaxSize{ FLT_MAX, FLT_MAX };

        inline bool ConstrainsSet() const noexcept
        {
            return MinSize.x != 0.0f
                || MinSize.y != 0.0f
                || MaxSize.x != FLT_MAX
                || MaxSize.y != FLT_MAX;
        }

        ImGuiChildFlags ChildFlags = 0;
        ImGuiWindowFlags WindowFlags = 0;
    };

    template<class DrawFunction, CValidWindowIDType IDType>
    void Child(IDType id, const DrawFunction& draw, const ChildConfig& config = {})
    {
        if (config.ConstrainsSet())
        {
            ImGui::SetNextWindowSizeConstraints(config.MinSize, config.MaxSize);
        }

        if (ImGui::BeginChild(id, config.Size, config.ChildFlags, config.WindowFlags))
        {
            draw();
        }
        ImGui::EndChild();
    }

    template<class DrawFunction, CValidWindowIDType IDType>
    void Child(IDType id, const ChildConfig& config, const DrawFunction& draw)
    {
        Child(id, draw, config);
    }

    struct WindowConfig
    {
        ImVec2 Size{ 0.0f, 0.0f };
        ImGuiCond SizeCond = 0;

        ImVec2 MinSize{ 0.0f, 0.0f };
        ImVec2 MaxSize{ FLT_MAX, FLT_MAX };

        inline bool SizeSet() const noexcept
        {
            return Size.x != 0.0f || Size.y != 0.0f;
        }

        inline bool ConstrainsSet() const noexcept
        {
            return MinSize.x != 0.0f
                || MinSize.y != 0.0f
                || MaxSize.x != FLT_MAX
                || MaxSize.y != FLT_MAX;
        }

        ImGuiWindowFlags Flags = 0;
        ImGuiDockNodeFlags DockFlags = 0;
    };

    namespace Impl
    {
        void RenderCustomTitleBarDecorations(ImGuiWindow* window, const char* name, bool* p_open);
    } // namespace Impl

    template<class DrawFunction>
    void Window(const char* name, const DrawFunction& draw, bool* p_open = nullptr, const WindowConfig& config = {})
    {
        if (config.ConstrainsSet())
        {
            ImGui::SetNextWindowSizeConstraints(config.MinSize, config.MaxSize);
        }

        if (config.SizeSet())
        {
            ImGui::SetNextWindowSize(config.Size, config.SizeCond);
        }

        ImGuiWindowClass windowClass;
        windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar | config.DockFlags;

        ImGui::SetNextWindowClass(&windowClass);

        if (p_open == nullptr or (*p_open) == true)
        {
            const bool bDrawCustomTitleBar = !(config.Flags & ImGuiWindowFlags_NoTitleBar);

            // Increase the title bar height and set bold font
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { ImGui::GetStyle().FramePadding.x, 6.0f });
            ImGui::PushFont(GUI::GetBoldFont(), ImGui::GetFontSize());
            
            // This will prevent ImGui from drawing collapse button
            // and we can draw our own
            auto& style = ImGui::GetStyle();
            const auto windowMenuButtonPosBckp = style.WindowMenuButtonPosition;
            style.WindowMenuButtonPosition = ImGuiDir_None;

            if (ImGui::Begin(name, nullptr, config.Flags))
            {
                ImGui::PopStyleVar(); // FramePadding
                ImGui::PopFont();
                style.WindowMenuButtonPosition = windowMenuButtonPosBckp;

                if (bDrawCustomTitleBar)
                    Impl::RenderCustomTitleBarDecorations(ImGui::GetCurrentWindow(), name, p_open);

                // Draw window contents
                draw();
            }
            else
            {
                // Window is either not visible or collapsed,
                // we still want to draw our stuff if it's collapsed
                if (bDrawCustomTitleBar)
                    Impl::RenderCustomTitleBarDecorations(ImGui::GetCurrentWindow(), name, p_open);

                style.WindowMenuButtonPosition = windowMenuButtonPosBckp;
                ImGui::PopStyleVar(); // FramePadding
                ImGui::PopFont();
            }
            ImGui::End();
        }
    }

    template<class DrawFunction>
    void Window(const char* name, const WindowConfig& config, const DrawFunction& draw, bool* p_open = NULL)
    {
        Window(name, draw, p_open, config);
    }

    template<class DrawFunction>
    void TabBar(const char* label, const DrawFunction& draw, ImGuiTabBarFlags flags = 0)
    {
        if (ImGui::BeginTabBar(label, flags | ImGuiTabBarFlags_DrawSelectedOverline))
        {
            draw();
            ImGui::EndTabBar();
        }
    }

    //======================================================================
    /// Tab Bar

    template<class DrawFunction>
    void TabBar(const char* label, ImGuiTabBarFlags flags, const DrawFunction& draw)
    {
        TabBar(label, draw, flags);
    }

    template<class DrawFunction>
    void TabItem(const char* label, const DrawFunction& draw, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0)
    {
        if (ImGui::BeginTabItem(label, p_open, flags))
        {
            draw();
            ImGui::EndTabItem();
        }
    }

    template<class DrawFunction>
    void TabItem(const char* label, bool* p_open, ImGuiTabItemFlags flags, const DrawFunction& draw)
    {
        TabItem(label, draw, flags);
    }


    //==========================================================================
    /// Menu Bar & Menu Items
    
    struct MenuItemSelectable
    {
        const char* Label = nullptr;
        bool& bSelected;

        inline void Draw() const
        {
            ImGui::AlignTextToFramePadding();
            const char* iconOffset = "   ";
            if (ImGui::MenuItemEx(Label, iconOffset, nullptr, /*bSelected*/ false))
                bSelected = !bSelected;

            if (bSelected)
                RenderCheckMark();
        }

        static void RenderCheckMark();
    };

    template<class CommandType> requires (std::is_invocable_v<CommandType>)
    struct MenuItem
    {
        const char* Label = nullptr;
        CommandType Command;
        bool bSelected = false;

        void Draw() const
        {
            ImGui::AlignTextToFramePadding();
            const char* iconOffset = "   ";
            if (ImGui::MenuItemEx(Label, iconOffset, nullptr, /*bSelected*/ false)) // TODO: icon, chortcut, enabled
                std::invoke(Command);

            if (bSelected)
                MenuItemSelectable::RenderCheckMark();
        }
    };

    template<class T, class RangeType>
    concept CMenuItemLabelCallback = std::ranges::input_range<RangeType> and std::is_invocable_r_v<const char*, T, std::ranges::range_value_t<RangeType>>;

    template<class T, class RangeType>
    concept CMenuItemValueCallback = std::ranges::input_range<RangeType> and std::is_invocable_r_v<bool&, T, std::ranges::range_value_t<RangeType>>;

	template<
		std::ranges::input_range RangeType,
		CMenuItemLabelCallback<RangeType> LabelProjection,
        CMenuItemValueCallback<RangeType> ValueProjection
	>
    struct MenuItemSelectableList
    {
        RangeType& Range;
        LabelProjection GetLabelCb;
        ValueProjection ValueCb;

        inline void Draw() const
        {
            for (auto&& pair : Range)
            {
                MenuItemSelectable{
                    .Label = std::invoke(GetLabelCb, pair),
                    .bSelected = std::invoke(ValueCb, pair),
                }.Draw();
            }
        }
    };


    // TODO: MenuItem with command ID instead of functor (maybe also with context/target)

    struct MenuSeparator
    {
        inline void Draw() const { ImGui::Separator(); }
    };

    template<class T>
    concept CMenuItem =
        Type::CIsSpecializationOf<T, MenuItem> or
        std::same_as<T, MenuItemSelectable> or
        Type::CIsSpecializationOf<T, MenuItemSelectableList> or
        std::same_as<T, MenuSeparator>;

    // We have to do this Push/Pop instead of using our ScopedXXX utilities
    // to avoid circular dependencies
    namespace Impl
    {
        void PushMenuStyle();
        void PopMenuStyle();
    }

    template<CMenuItem ...MenuItemArgs>
    void Menu(const char* label, MenuItemArgs...items)
    {
        Impl::PushMenuStyle();

        if (ImGui::BeginMenu(label))
        {
            (items.Draw(), ...);

            ImGui::EndMenu();
        }

        Impl::PopMenuStyle();
    }

} // namespace JPL::ImGuiEx
