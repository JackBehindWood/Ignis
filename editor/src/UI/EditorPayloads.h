#pragma once

namespace Ignis
{

struct AssetDragPayload
{
    static constexpr const char* k_type      = "IGNIS_ASSET_PAYLOAD";
    static constexpr uint32_t    k_max_items = 8;

    struct Item
    {
        char rel_path[512]; // relative to project assets root
        char type_label[8];
        char extension[16];
    };

    uint32_t count = 0;
    Item     items[k_max_items];
};

} // namespace Ignis
