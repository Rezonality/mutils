#pragma once

#include <string>
#include <mutils/file/file.h>

namespace MUtils
{

using LayoutShowFlags = std::map<std::string, bool>;
struct LayoutInfo
{
    std::string windowLayout;
    LayoutShowFlags showFlags;
};

using fnLoadCB = std::function<void(const LayoutInfo&)>;
struct WindowState
{
    std::string name;
    bool* pVisible;
};
struct LayoutManagerData
{
    fnLoadCB loadCB;
    fs::path layoutSettingsPath;
    std::map<std::string, LayoutInfo> layouts;
    std::string lastLayout;
    std::map<std::string, WindowState> mapWindowState;
};

extern LayoutManagerData LayoutData;

void layout_manager_load_layouts_file(const std::string& appName, const fnLoadCB& fnLoad, bool forceReset = false);
void layout_manager_save_layouts_file();
void layout_manager_load_layout(const std::string& layoutName);
void layout_manager_save_layout(const std::string& layoutName, const std::string& layoutText);

void layout_manager_register_window(const std::string& key, const std::string& name, bool* showState);

}; // namespace MUtils