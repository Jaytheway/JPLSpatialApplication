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

#include "ImGui/ImGuiScopedUtils.h"
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
    
    //==========================================================================
    /// Flexbox-like layout.
    /// 
    /// Works by "placing elements" with simple "fixed" vs "grow" sizing policy.
    /// The elements are not actually placed, instead ImGui cursor is shifted.
    /// 
    /// Element defines Size or Weight it takes in parent Layout bounds.
    /// It can contain a user draw function, or a nested Layout.
    /// 
    /// Layout defines the direction (or Axis) of Element placement and Spacing.
    /// The root layout is used to ComputeSizes() and actually Draw() the Elements.
    namespace Flex
    {
        enum class EAxis { Horizontal, Vertical };

        //======================================================================
        template<class LayoutOrDrawCb>
        struct Element;

        template<class T>
        concept CElement = Type::CIsSpecializationOf<T, Element>;

        template<CElement...Elements>
        struct Layout;

        template<class T>
        concept CLayout = Type::CIsSpecializationOf<T, Layout>;

        // Item draw callback can take: no parameters, item size float, or item screen position ImVec2 and item size float
        template<class T>
        concept CDrawCb = std::is_invocable_v<T> or std::is_invocable_v<T, float> or std::is_invocable_v<T, ImVec2, float>;

        template<class T>
        concept CLayoutOrDrawCb = CLayout<T> or CDrawCb<T>;

        //======================================================================
        /// Factory functions for Elements

        template<CLayoutOrDrawCb LayoutOrDrawCb>
        Element<LayoutOrDrawCb> Fixed(float size, LayoutOrDrawCb&& content)
        {
            return Element<LayoutOrDrawCb>{.Weight = 0.0f, .Size = size, .Content = std::forward<LayoutOrDrawCb>(content) };
        }

        template<CLayoutOrDrawCb LayoutOrDrawCb>
        Element<LayoutOrDrawCb> Grow(float weight, LayoutOrDrawCb&& content)
        {
            return Element<LayoutOrDrawCb>{.Weight = weight, .Size = 0.0f, .Content = std::forward<LayoutOrDrawCb>(content) };
        }

        template<CLayoutOrDrawCb LayoutOrDrawCb>
        Element<LayoutOrDrawCb> Grow(LayoutOrDrawCb&& content)
        {
            return Grow(1.0f, std::forward<LayoutOrDrawCb>(content));
        }

        //======================================================================
        template<class LayoutOrDrawCb>
        struct Element
        {
            // Cannot use concept in class template due to the order of declarations,
            // therefore using static assert.
            static_assert(CLayoutOrDrawCb<LayoutOrDrawCb>);

            float Weight = 0.0f; // 0: fixed size, > 0: flex grow weight
            float Size = 0.0f;   // Computed or assigned fixed pixel value

            [[no_unique_address]] LayoutOrDrawCb Content;
        };

         /// Special kind of Element that simply adds empty space
		auto Spacing(float size = 0.0f)
        {
			return Element{ .Weight = 0.0f, .Size = size, .Content = [size] { ImGui::Dummy(ImVec2(size, size)); } };
		}
        
        //======================================================================
        /// Factory functions for Layouts

        template<CElement...Elements>
        Layout<Elements...> Row(float spacing, Elements&&...elements)
        {
            return Layout<Elements...>(EAxis::Horizontal, spacing, std::forward<Elements>(elements)...);
        }

        template<CElement...Elements>
        Layout<Elements...> Row(Elements&&...elements)
        {
            return Row(ImGui::GetStyle().ItemSpacing.x, std::forward<Elements>(elements)...);
        }

        template<CElement...Elements>
        Layout<Elements...> Column(float spacing, Elements&&...elements)
        {
            return Layout<Elements...>(EAxis::Vertical, spacing, std::forward<Elements>(elements)...);
        }

        template<CElement...Elements>
        Layout<Elements...> Column(Elements&&...elements)
        {
            return Column(ImGui::GetStyle().ItemSpacing.y, std::forward<Elements>(elements)...);
        }

        //======================================================================
        /// Factory functions for nested Layouts. Return Element of some kind.

        template<CElement...Elements>
        auto ColumnGrow(float spacing, Elements&&...elements)
        {
            return Grow(spacing, Column(ImGui::GetStyle().ItemSpacing.y, std::forward<Elements>(elements)...));
        }

        template<CElement...Elements>
        auto ColumnGrow(Elements&&...elements)
        {
            return Grow(Column(ImGui::GetStyle().ItemSpacing.y, std::forward<Elements>(elements)...));
        }

        template<CElement...Elements>
        auto ColumnFixed(float size, Elements&&...elements)
        {
            return Fixed(size, Column(ImGui::GetStyle().ItemSpacing.y, std::forward<Elements>(elements)...));
        }

        template<CElement...Elements>
        auto ColumnFixed(Elements&&...elements)
        {
            return Fixed(Column(ImGui::GetStyle().ItemSpacing.y, std::forward<Elements>(elements)...));
        }

        template<CElement...Elements>
        auto RowGrow(float spacing, Elements&&...elements)
        {
            return Grow(spacing, Row(ImGui::GetStyle().ItemSpacing.x, std::forward<Elements>(elements)...));
        }

        template<CElement...Elements>
        auto RowGrow(Elements&&...elements)
        {
            return Grow(Row(ImGui::GetStyle().ItemSpacing.x, std::forward<Elements>(elements)...));
        }

        template<CElement...Elements>
        auto RowFixed(float size, Elements&&...elements)
        {
            return Fixed(size, Row(ImGui::GetStyle().ItemSpacing.x, std::forward<Elements>(elements)...));
        }

        template<CElement...Elements>
        auto RowFixed(Elements&&...elements)
        {
            return Fixed(Row(ImGui::GetStyle().ItemSpacing.x, std::forward<Elements>(elements)...));
        }

        //======================================================================
        template<CElement...Elements>
        struct Layout
        {
            using TupleType = std::tuple<Elements...>;
            static constexpr std::size_t cItemCount = std::tuple_size<TupleType>::value;

            EAxis Axis = EAxis::Vertical;
            float Spacing = 0.0f;
            TupleType Content;

            Layout(EAxis axis, float spacing, Elements&&...elements)
                : Axis(axis)
                , Spacing(spacing)
                , Content(std::forward<Elements>(elements)...)
            {
            }

            void ForEachItem(auto&& func)
            {
                std::apply([&func](auto&&... element)
                {
                    (std::invoke(func, element), ...);
                }, Content);
            }

            void ForEachItem(auto&& func) const
            {
                std::apply([&func](auto&&... element)
                {
                    (std::invoke(func, element), ...);
                }, Content);
            }

            void ComputeSizes(ImVec2 size)
            {
                ComputeSizes(size.x, size.y);
            }

            // Resolves flex-grow dimensions recursively across all branches
            void ComputeSizes(float targetWidth, float targetHeight)
            {
                float availableSpace = (Axis == EAxis::Horizontal) ? targetWidth : targetHeight;
                if (cItemCount > 1)
                    availableSpace -= Spacing * (cItemCount - 1);

                float totalWeight = 0.0f;
                float fixedSum = 0.0f;
                ForEachItem([&](auto&& element)
                {
                    if (element.Weight > 0.0f)
                        totalWeight += element.Weight;
                    else
                        fixedSum += element.Size;
                });

                float remainingSpace = ImMax(0.0f, availableSpace - fixedSum);
                float spacePerWeight = totalWeight > 0.0f ? (remainingSpace / totalWeight) : 0.0f;

                ForEachItem([&](auto&& element)
                {
                    if (element.Weight > 0.0f)
                        element.Size = element.Weight * spacePerWeight;

                    // If the element is a nested layout box, tell it to evaluate its children recursively
                    if constexpr (CLayout<decltype(element.Content)>)
                    {
                        if (Axis == EAxis::Horizontal)
                            element.Content.ComputeSizes(element.Size, targetHeight);
                        else
                            element.Content.ComputeSizes(targetWidth, element.Size);
                    }
                });
            }

            void Draw() const
            {
                ImGui::BeginGroup();

                ImVec2 cursor = ImGui::GetCursorScreenPos();

                ForEachItem([&](auto&& element) mutable
                {
                    ImGui::SetCursorScreenPos(cursor);

                    // Apply width constraints for the horizontal pass.
                    // Skip 0-size elements, we don't want ImGui to fallback to "default element size"
					Conditional<ScopedItemWidth> widthIf(Axis == EAxis::Horizontal and element.Size != 0.0f, element.Size);
                    // TODO: we might want to implement element skipping for 0-size, but currently it doesn't work with Flex::Spacing()

                    using ContentType = decltype(element.Content);

                    // Render content (individual callback or nested layout)
                    if constexpr (CLayout<ContentType>)
                    {
                        element.Content.Draw();
                    }
                    else
                    {
                        if constexpr (std::is_invocable_v<ContentType, ImVec2, float>)
                        {
                            // Item screen position and size
                            std::invoke(element.Content, cursor, element.Size);
                        }
                        else if constexpr (std::is_invocable_v<ContentType, float>)
                        {
                            // Item size
                            std::invoke(element.Content, element.Size);
                        }
                        else
                        {
                            // No parameters requested
                            std::invoke(element.Content);
                        }
                    }

                    if (Axis == EAxis::Horizontal)
                    {
                        // Advance horizontally
						cursor.x += element.Size + Spacing;
                    }
                    else // Axis == EAxis::Vertical
                    {
                        // Advance vertically by element size, but at least by ImGui widget height
						cursor.y += ImMax(ImGui::GetItemRectSize().y, element.Size) + Spacing;
                    }
                });

                ImGui::EndGroup();
            }
        };
    }

} // namespace JPL::ImGuiEx
