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

#include <JPLSpatial/Math/Math.h>
#include <JPLSpatial/Utilities/TypeUtilities.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <tuple>
#include <type_traits>

//==============================================================================
/// Flexbox-like layout.
/// 
/// Works by "placing elements" with "fixed" vs "grow" sizing policy.
/// The elements are not actually placed, instead ImGui cursor is shifted.
/// 
/// Element defines Size, MaxSize and Weight it takes in parent Layout bounds.
/// It can contain a user draw function, or a nested Layout.
/// 
/// Layout defines the direction (or Axis) of Element placement and Spacing.
/// The root layout is used to ComputeSizes() and actually Draw() the Elements.
namespace JPL::ImGuiEx::Flex
{
    enum class EAxis { Horizontal, Vertical };

    // Parameters to define Flex item behavior
    struct Params
    {
        uint16 Weight = 0;      // 0: fixed size, > 0: flex grow proportion
        float Size = 0.0f;      // Minimum/default fixed size in pixels
        float MaxSize = 0.0f;   // Maximum fixed size in pixels
    };

    // Holds data that user may use in item draw callback
    struct DrawCbInfo
    {
        EAxis Axis;
        float ItemSize;
        ImVec2 ScreenPos;
    };

    //==========================================================================
    template<class LayoutOrDrawCb, class ElementTag = void*>
    struct Element;

    template<class T>
    concept CElement = Type::CIsSpecializationOf<T, Element>;

    template<CElement...Elements>
    struct Layout;

    template<class T>
    concept CLayout = Type::CIsSpecializationOf<T, Layout>;

    // Item draw callback can take: no parameters, or DrawCbInfo
    template<class T>
    concept CDrawCb = std::is_invocable_v<T> or std::is_invocable_v<T, const DrawCbInfo&>;

    template<class T>
    concept CLayoutOrDrawCb = CLayout<T> or CDrawCb<T>;

    //==========================================================================
    /// Factory functions for Elements

    // Make a Flex element with specified parameters
    template<CLayoutOrDrawCb LayoutOrDrawCb>
    Element<LayoutOrDrawCb> Item(Params params, LayoutOrDrawCb&& content)
    {
        return Element<LayoutOrDrawCb>{
            .Weight = params.Weight,
            .Size = params.Size,
            .MaxSize = params.MaxSize,
            .Content = std::forward<LayoutOrDrawCb>(content)
        };
    }
        
    // Make a Flex element with fixed size and no grow factor
    template<CLayoutOrDrawCb LayoutOrDrawCb>
    Element<LayoutOrDrawCb> Fixed(float size, LayoutOrDrawCb&& content)
    {
        return Item({ .Size = size }, std::forward<LayoutOrDrawCb>(content));
    }

    // Make a Flex element with any size and specific grow factor
    template<CLayoutOrDrawCb LayoutOrDrawCb>
    Element<LayoutOrDrawCb> Grow(uint16 weight, LayoutOrDrawCb&& content)
    {
        return Item({ .Weight = weight }, std::forward<LayoutOrDrawCb>(content));
    }

    // Make a Flex element with any size and default grow factor 1.
    template<CLayoutOrDrawCb LayoutOrDrawCb>
    Element<LayoutOrDrawCb> Grow(LayoutOrDrawCb&& content)
    {
        return Item({ .Weight = 1 }, std::forward<LayoutOrDrawCb>(content));
    }

    //==========================================================================
    template<class LayoutOrDrawCb, class ElementTag>
    struct Element
    {
        // Cannot use concept in class template due to the order of declarations,
        // therefore using static assert.
        static_assert(CLayoutOrDrawCb<LayoutOrDrawCb>);

        // Flex Params
        uint16 Weight = 0;
        float Size = 0.0f;
        float MaxSize = 0.0f;

        [[no_unique_address]] LayoutOrDrawCb Content;

        // Used to distinguish some special kinds of Elements
        using Tag = ElementTag;
        [[no_unique_address]] Tag _Tag;

        //! Note: if the factories belowe won't be utilized, they can be removed.

        // If this Element's Content is a Layout, this will return Layout with added Grow Element.
        template<CLayoutOrDrawCb T>
        auto AddGrow(uint16 weight, T&& content) && requires (CLayout<LayoutOrDrawCb>);

        // If this Element's Content is a Layout, this will return Layout with added Grow Element,
        // with default weight 1.0f.
        template<CLayoutOrDrawCb T>
        auto AddGrow(T&& content) && requires (CLayout<LayoutOrDrawCb>);

        // If this Element's Content is a Layout, this will return Layout with added Fixed Element.
        template<CLayoutOrDrawCb T>
        auto AddFixed(float size, T&& content) && requires (CLayout<LayoutOrDrawCb>);
    };

    namespace Impl
    {
        struct SpacerTag {};
        struct SpringTag {};

        template<class ElementType, class TagType>
        constexpr bool HasTag = requires
        {
            typename std::remove_cvref_t<ElementType>::Tag;
            requires std::same_as<typename std::remove_cvref_t<ElementType>::Tag, TagType>;
        };
    }

        /// Special kind of Element that simply adds empty space
	auto Spacing(float size = 0.0f)
    {
        // We need to make sure to pass non-zero size so that ImGui can ItemAdd the ItemSpacing.
        // ImGui uses it as default value to move cursor when size is 0.
        // But this will also ensure the LastItemData is updated as well and we can use it in our Layout::Draw().
        //const ImVec2 dummySize = size == 0.0f ? ImGui::GetStyle().ItemSpacing : ImVec2(size, size);
			
        return Element{ .Weight = 0, .Size = size, .Content = [] {}, ._Tag = Impl::SpacerTag{} };
	}

    /// Special kind of Element that takes up space with grow factor, but doesn't draw anything
    auto Spring(uint16 weight = 1, float minSize = 0.0f)
    {
        return Element{ .Weight = weight, .Size = minSize, .Content = [] {}, ._Tag = Impl::SpringTag{} };
    }

    //==========================================================================
    /// Factory functions for Layouts

    // Make a howizontal Flex layout with specific element spacing
    template<CElement...Elements>
    Layout<Elements...> Row(float spacing, Elements&&...elements)
    {
        return Layout<Elements...>(EAxis::Horizontal, spacing, std::forward<Elements>(elements)...);
    }

    // Make a howizontal Flex layout with default element spacing equal to ImGui's item spacing
    template<CElement...Elements>
    Layout<Elements...> Row(Elements&&...elements)
    {
        return Row(ImGui::GetStyle().ItemSpacing.x, std::forward<Elements>(elements)...);
    }

    // Make a vertical Flex layout with specific element spacing
    template<CElement...Elements>
    Layout<Elements...> Column(float spacing, Elements&&...elements)
    {
        return Layout<Elements...>(EAxis::Vertical, spacing, std::forward<Elements>(elements)...);
    }

    // Make a vertical Flex layout with default element spacing equal to ImGui's item spacing
    template<CElement...Elements>
    Layout<Elements...> Column(Elements&&...elements)
    {
        return Column(ImGui::GetStyle().ItemSpacing.y, std::forward<Elements>(elements)...);
    }

    // Make an empty vertical Flex layout with specific or default elements spacing.
    Layout<> Column(float spacing = ImGui::GetStyle().ItemSpacing.y);

    // Make an empty horizontal Flex layout with specific or default elements spacing.
    Layout<> Row(float spacing = ImGui::GetStyle().ItemSpacing.x);

    //==========================================================================
    /// Factory functions for nested Layouts. Return Element of some kind.

    // Make a vertical Flex layout with specific elements spacing,
    // wrapped in a Flex element with specific element params.
    template<CElement...Elements>
    auto Column(const Params& params, float spacing, Elements&&...elements)
    {
        return Item(params, Column(spacing, std::forward<Elements>(elements)...));
    }

    // Make a vertical Flex layout with specific elements spacing,
    // wrapped in a Flex element with default grow factor 1.
    template<CElement...Elements>
    auto ColumnGrow(float spacing, Elements&&...elements)
    {
        return Grow(spacing, Column(ImGui::GetStyle().ItemSpacing.y, std::forward<Elements>(elements)...));
    }

    // Make a vertical Flex layout with default elements spacing,
    // wrapped in a Flex element with default grow factor 1.
    template<CElement...Elements>
    auto ColumnGrow(Elements&&...elements)
    {
        return Grow(Column(ImGui::GetStyle().ItemSpacing.y, std::forward<Elements>(elements)...));
    }

    // Make a vertical Flex layout with default elements spacing,
    // wrapped in a Flex element with specific fixed size.
    template<CElement...Elements>
    auto ColumnFixed(float size, Elements&&...elements)
    {
        return Fixed(size, Column(ImGui::GetStyle().ItemSpacing.y, std::forward<Elements>(elements)...));
    }

    // Make a horizontal Flex layout with specific elements spacing,
    // wrapped in a Flex element with default grow factor 1.
    template<CElement...Elements>
    auto RowGrow(float spacing, Elements&&...elements)
    {
        return Grow(spacing, Row(ImGui::GetStyle().ItemSpacing.x, std::forward<Elements>(elements)...));
    }

    // Make a horizontal Flex layout with default elements spacing,
    // wrapped in a Flex element with default grow factor 1.
    template<CElement...Elements>
    auto RowGrow(Elements&&...elements)
    {
        return Grow(Row(ImGui::GetStyle().ItemSpacing.x, std::forward<Elements>(elements)...));
    }

    // Make a horizontal Flex layout with default elements spacing,
    // wrapped in a Flex element with specific fixed size.
    template<CElement...Elements>
    auto RowFixed(float size, Elements&&...elements)
    {
        return Fixed(size, Row(ImGui::GetStyle().ItemSpacing.x, std::forward<Elements>(elements)...));
    }

    //==========================================================================
    template<CElement...Elements>
    struct Layout
    {
    public:
        using TupleType = std::tuple<Elements...>;

        // Number of child elements in this layout
        static constexpr std::size_t cItemCount = std::tuple_size<TupleType>::value;

    public:
        Layout(TupleType&& elementsTuple, EAxis axis, float spacing);
        Layout(EAxis axis, float spacing, Elements&&...elements);

        // Resolve flex-grow dimensions recursively across all branches
        void ComputeSizes(ImVec2 size) { ComputeSizes(size.x, size.y); }

        // Resolve flex-grow dimensions recursively across all branches
        void ComputeSizes(float targetWidth, float targetHeight);

        // Draw the layout
        void Draw() const;

        // Add element(s) to layout
        template <CElement ...T>
        Layout<Elements..., T...> Add(T&&...elements) &&;

        // Add spacing to layout
        auto AddSpacing(float size = 0.0f) &&;

        // Add spring to layout
        auto AddSpring(uint16 weight = 1, float size = 0.0f) &&;

        // Add default grow element(s) to layout (with weight 1)
        template<CLayoutOrDrawCb ...LayoutOrDrawCb>
        auto AddGrow(LayoutOrDrawCb&&...contents) &&;

        // Add grow element(s) to layout with specific proportion
        template<CLayoutOrDrawCb ...LayoutOrDrawCb>
        auto AddGrow(uint16 weight, LayoutOrDrawCb&&...contents) &&;

        // Add fixed size element(s) to layout with specific size
        template<CLayoutOrDrawCb ...LayoutOrDrawCb>
        auto AddFixed(float size, LayoutOrDrawCb&&...contents) &&;

    private:
        template<class Func>
        void ForEachItem(Func&& func) const { ForEachItem(*this, std::forward<Func>(func)); }
            
        template<class Func>
        void ForEachItem(Func&& func) { ForEachItem(*this, std::forward<Func>(func)); }

        template<class ThisType>
        static void ForEachItem(ThisType& self, auto&& func);

    private:
        EAxis mAxis = EAxis::Vertical;
        float mSpacing = 0.0f;
        TupleType mContent;
        float mDesiredSize[std::max(1llu, cItemCount)] = {};
        float mTargetLayoutSize = 0.0f;
        float mCrossAxisSize = 0.0f;
    };

    //==========================================================================
    // Has to be defined after Layout class
    inline Layout<> Column(float spacing)
    {
        return Layout<>(std::tuple<>(), EAxis::Vertical, spacing);
    }

    inline Layout<> Row(float spacing)
    {
        return Layout<>(std::tuple<>(), EAxis::Horizontal, spacing);
    }

} // namespace JPL::ImGuiEx::Flex

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

namespace JPL::ImGuiEx::Flex
{
    //==========================================================================
    /// Element

    template<class LayoutOrDrawCb, class ElementTag>
    template<CLayoutOrDrawCb T>
    auto Element<LayoutOrDrawCb, ElementTag>::AddGrow(uint16 weight, T&& content) && requires (CLayout<LayoutOrDrawCb>)
    {
        return Item({
            .Weight = Weight,
            .Size = Size,
            .MaxSize = MaxSize,
            .Content = std::move(Content).AddGrow(weight, std::forward<T>(content))
        });
    }

    template<class LayoutOrDrawCb, class ElementTag>
    template<CLayoutOrDrawCb T>
    auto Element<LayoutOrDrawCb, ElementTag>::AddGrow(T&& content) && requires (CLayout<LayoutOrDrawCb>)
    {
        return AddGrow(1.0f, std::forward<T>(content));
    }

    template<class LayoutOrDrawCb, class ElementTag>
    template<CLayoutOrDrawCb T>
    auto Element<LayoutOrDrawCb, ElementTag>::AddFixed(float size, T&& content) && requires (CLayout<LayoutOrDrawCb>)
    {
        return Item({
            .Weight = Weight,
            .Size = Size,
            .MaxSize = MaxSize,
            .Content = std::move(Content).AddFixed(size, std::forward<T>(content))
        });
    }

    //==========================================================================
    /// Layout

    template<CElement...Elements>
    Layout<Elements...>::Layout(TupleType&& tuple, EAxis axis, float spacing)
        : mAxis(axis)
        , mSpacing(spacing)
        , mContent(std::move(tuple))
    {
    }

    template<CElement...Elements>
    Layout<Elements...>::Layout(EAxis axis, float spacing, Elements&&...elements)
        : Layout(std::make_tuple(std::forward<Elements>(elements)...), axis, spacing)
    {
    }

    template<CElement...Elements>
    template<class ThisType>
    void Layout<Elements...>::ForEachItem(ThisType& self, auto&& func)
    {
        std::apply([&func](auto&&... elements)
        {
            [[maybe_unused]] std::size_t i = 0;

            auto wrapper = [&func, &i](auto&& element)
            {
                if constexpr (std::is_invocable_v<decltype(func), decltype(element), std::size_t>)
                {
                    std::invoke(func, element, i++);
                }
                else
                {
                    std::invoke(func, element);
                }
            };

            (wrapper(elements), ...);

        }, self.mContent);
    }

    template<CElement...Elements>
    void Layout<Elements...>::ComputeSizes(float targetWidth, float targetHeight)
    {
        auto computeNestedLayoutSize = [&](auto&& element, std::size_t i)
        {
            // If the element is a nested layout box, tell it to evaluate its children recursively
            if constexpr (CLayout<decltype(element.Content)>)
            {
                if (mAxis == EAxis::Horizontal)
                    element.Content.ComputeSizes(mDesiredSize[i], targetHeight);
                else
                    element.Content.ComputeSizes(targetWidth, mDesiredSize[i]);
            }
        };

        float availableSpace = (mAxis == EAxis::Horizontal) ? targetWidth : targetHeight;
        const float crossAxisSpace = (mAxis == EAxis::Horizontal) ? targetHeight : targetWidth;

        // We don't want to recalculated the entire layour if content region hasn't changed.
        if (Math::IsNearlyEqual(availableSpace, mTargetLayoutSize))
        {
            if (not Math::IsNearlyEqual(crossAxisSpace, mCrossAxisSize))
            {
                ForEachItem(computeNestedLayoutSize);
                mCrossAxisSize = crossAxisSpace;
            }

            return;
        }

        mCrossAxisSize = crossAxisSpace;
        mTargetLayoutSize = availableSpace;

        if (cItemCount > 1)
            availableSpace -= mSpacing * (cItemCount - 1);

        uint32 totalWeight = 0;
        float minFixedSize = 0.0f;

        struct ElementData
        {
            float Size;
            float AllowedGrowth;
            uint16 Weight;
        };
        std::array<ElementData, cItemCount> elementData;

        // Collect element data for easy access
        ForEachItem([&](auto&& element, std::size_t i)
        {
            if constexpr (Impl::HasTag<decltype(element), Impl::SpacerTag>)
            {
                if (element.Size == 0.0f) // User requested default spacing, get value for the right axis
                {
                    const ImVec2 defaulSpacing = ImGui::GetStyle().ItemSpacing;
                    element.Size = mAxis == EAxis::Horizontal ? defaulSpacing.x : defaulSpacing.y;
                }
            }

            elementData[i].Size = element.Size;
            elementData[i].AllowedGrowth = element.MaxSize - element.Size;
            elementData[i].Weight = element.Weight;

            totalWeight += element.Weight;
            minFixedSize += element.Size; // treating as minimal size
        });

        float remainingSpace = ImMax(0.0f, availableSpace - minFixedSize);

        // If remainingSpace is already 0, just keep the items' current min size;
        if (remainingSpace <= 0.0f)
        {
            // Assing base size as desired
            ForEachItem([&](auto&& element, std::size_t i)
            {
                mDesiredSize[i] = element.Size;
            });

            ForEachItem(computeNestedLayoutSize);
            return;
        }

        bool frozen[cItemCount]{};

        bool bSizeViolationOccured = false;
        do
        {
            const float spacePerWeight = totalWeight > 0 ? (remainingSpace / totalWeight) : 0.0f;

            for (uint32 i = 0; i < cItemCount; ++i)
            {
                if (frozen[i])
                    continue;

                const ElementData& element = elementData[i];

                const float desiredGrowth = element.Weight * spacePerWeight;

                // Check if violating item's max size
                if (element.AllowedGrowth > 0.0f and desiredGrowth > element.AllowedGrowth)
                {
                    bSizeViolationOccured = true;
                    frozen[i] = true;

                    mDesiredSize[i] = element.Size + element.AllowedGrowth;

                    remainingSpace -= element.AllowedGrowth;
                    totalWeight -= element.Weight;

                    // Repeat the calculation with this element frozen
                    break;
                }
                else
                {
                    mDesiredSize[i] = element.Size + desiredGrowth;
                    bSizeViolationOccured = false;
                }
            }

            // If all elements are frozen, stop recalculating space
            if (std::ranges::all_of(frozen, std::identity{}))
                break;

        } while (bSizeViolationOccured);

        // Compute nested layouts after we have our desired sizes
        ForEachItem(computeNestedLayoutSize);
    }


    template<CElement...Elements>
    void Layout<Elements...>::Draw() const
    {
        ImGui::BeginGroup();

        ImVec2 cursor = ImGui::GetCursorScreenPos();

        ForEachItem([&](auto&& element, std::size_t i)
        {
            ImGui::SetCursorScreenPos(cursor);

            const float elementSize = mDesiredSize[i];

            // This might never happen
            if (elementSize <= 0.0f)
                return;

            // Apply width constraints for the horizontal pass.
            Conditional<ScopedItemWidth> widthIf(mAxis == EAxis::Horizontal, elementSize);

            using ElementType = std::remove_cvref_t<decltype(element)>;
            using ContentType = decltype(element.Content);

            const ImVec2 itemRectMax = ImGui::GetItemRectMax();

            static constexpr bool bIsUtilItem =
                Impl::HasTag<ElementType, Impl::SpacerTag> or
                Impl::HasTag<ElementType, Impl::SpringTag>;

            // Render content (individual callback or nested layout)
            if constexpr (CLayout<ContentType>)
            {
                element.Content.Draw();
            }
            else
            {
                if constexpr (std::is_invocable_v<ContentType, const DrawCbInfo&>)
                {
                    // User requested item info
                    const DrawCbInfo info{
                        .Axis = mAxis,
                        .ItemSize = elementSize,
                        .ScreenPos = cursor
                    };

                    std::invoke(element.Content, info);
                }
                else
                {
                    // No parameters requested
                    std::invoke(element.Content);
                }
            }

            const ImVec2 newItemRectMax = ImGui::GetItemRectMax();

            if (mAxis == EAxis::Horizontal)
            {
                // Advance horizontally, but at least by the item's width to avoid overlap
                if constexpr (bIsUtilItem)
                {
                    cursor.x += elementSize + mSpacing;
                }
                else
                {
                    const float nonOverlapWidth = ImGui::GetItemRectSize().x; // this should be `= elementSize` anyway
                    cursor.x += ImMax(nonOverlapWidth, elementSize) + mSpacing;
                }
            }
            else // Axis == EAxis::Vertical
            {
                // Advance vertically by element size, but at least by item's height to avoid overlap
                if constexpr (bIsUtilItem)
                {
                    cursor.y += elementSize + mSpacing;
                }
                else
                {
                    const float nonOverlapHeight = ImGui::GetItemRectSize().y;
                    cursor.y += ImMax(nonOverlapHeight, elementSize) + mSpacing;
                }
            }
        });

        ImGui::EndGroup();
    }

    //==========================================================================
    template<CElement...Elements>
    template <CElement ...T>
    auto Layout<Elements...>::Add(T&&...elements) && -> Layout<Elements..., T...>
    {
        return Layout<Elements..., T...>(
            std::tuple_cat(std::move(mContent), std::make_tuple<T...>(std::forward<T>(elements)...)),
            mAxis,
            mSpacing
        );
    }

    template<CElement...Elements>
    auto Layout<Elements...>::AddSpacing(float size) &&
    {
        return std::move(*this).Add(Flex::Spacing(size));
    }

    template<CElement ...Elements>
    auto Layout<Elements...>::AddSpring(uint16 weight, float size) &&
    {
        return std::move(*this).Add(Flex::Spring(weight, size));
    }
    
    template<CElement...Elements>
    template<CLayoutOrDrawCb ...LayoutOrDrawCb>
    auto Layout<Elements...>::AddGrow(LayoutOrDrawCb&&...contents) &&
    {
        return std::move(*this).Add(Grow(1.0f, std::forward<LayoutOrDrawCb>(contents))...);
    }

    template<CElement...Elements>
    template<CLayoutOrDrawCb ...LayoutOrDrawCb>
    auto Layout<Elements...>::AddGrow(uint16 weight, LayoutOrDrawCb&&...contents) &&
    {
        return std::move(*this).Add(Grow(weight, std::forward<LayoutOrDrawCb>(contents))...);
    }

    template<CElement...Elements>
    template<CLayoutOrDrawCb ...LayoutOrDrawCb>
    auto Layout<Elements...>::AddFixed(float size, LayoutOrDrawCb&&...contents) &&
    {
        return std::move(*this).Add(Fixed(size, std::forward<LayoutOrDrawCb>(contents))...);
    }
} // namespace JPL::ImGuiEx::Flex
