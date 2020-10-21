#include <algorithm>

#include <imgui.h>
#include <imgui_internal.h>

#include "mutils/ui/imgui_extras.h"

namespace ImGui
{
static auto vector_getter = [](void* vec, int idx, const char** out_text) {
    auto& vector = *static_cast<std::vector<std::string>*>(vec);
    if (idx < 0 || idx >= static_cast<int>(vector.size()))
    {
        return false;
    }
    *out_text = vector.at(idx).c_str();
    return true;
};

bool Combo(const char* label, int* currIndex, std::vector<std::string>& values)
{
    if (values.empty())
    {
        return false;
    }
    return Combo(label, currIndex, vector_getter,
        static_cast<void*>(&values), (int)values.size());
}

bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values)
{
    if (values.empty())
    {
        return false;
    }
    return ListBox(label, currIndex, vector_getter,
        static_cast<void*>(&values), (int)values.size());
}

bool DragIntRange4(const char* label, MUtils::NVec4<int>& v, float v_speed, int v_min, int v_max, const char* format, const char* format_max)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    PushID(label);
    BeginGroup();
    PushMultiItemsWidths(4, CalcItemWidth());

    bool value_changed = DragInt("##amin", &v.x, v_speed, v_min, v_max, format);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);
    v.y = std::max(v.x, v.y);
    value_changed |= DragInt("##bmin", &v.y, v_speed, ImMin(v.x, v_max), v_max, format_max ? format_max : format);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);
    v.z = std::max(v.y, v.z);
    value_changed |= DragInt("##cmin", &v.z, v_speed, ImMin(v.y, v_max), v_max, format_max ? format_max : format);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);
    v.w = std::max(v.z, v.w);
    value_changed |= DragInt("##dmin", &v.w, v_speed, ImMin(v.z, v_max), v_max, format_max ? format_max : format);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);

    TextEx(label, FindRenderedTextEnd(label));
    EndGroup();
    PopID();

    return value_changed;
}
} // namespace ImGui
