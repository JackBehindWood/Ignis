#pragma once

#include "../Panels/IPanel.h"

namespace Ignis
{

enum class SplitDir : uint8_t
{
    Left,
    Right,
    Up,
    Down,
};

struct LayoutNode
{
    bool is_leaf() const
    {
        return !child_a && !child_b;
    }

    // Split fields — only valid when !is_leaf()
    SplitDir              split_dir = SplitDir::Left;
    float                 ratio     = 0.5f;
    UniquePtr<LayoutNode> child_a; // the split-off piece (at split_dir)
    UniquePtr<LayoutNode> child_b; // the remainder

    // Leaf fields — only valid when is_leaf()
    Vector<PanelId> panel_ids;
    bool            hide_tab_bar = false;
};

} // namespace Ignis
