
#include <mutils/common.h>
#include <mutils/file/runtree.h>
#include <mutils/file/toml_utils.h>
#include <mutils/logger/logger.h>
#include <mutils/ui/layout_manager.h>

#include <fmt/format.h>

#include <imgui.h>

#include <cppcodec/base64_default_rfc4648.hpp>

#undef ERROR
namespace MUtils
{

LayoutManagerData LayoutData;

void layout_manager_load_layouts_file(const std::string& appName, const fnLoadCB& fnLoad, bool forceReset)
{
    LayoutData.layouts.clear();
    LayoutData.loadCB = fnLoad;
    try
    {
        LayoutData.layoutSettingsPath = file_appdata_path();
        if (LayoutData.layoutSettingsPath.empty())
        {
            LayoutData.layoutSettingsPath = file_documents_path();
            if (LayoutData.layoutSettingsPath.empty())
            {
                LayoutData.layoutSettingsPath = fs::temp_directory_path();
            }
        }

        // TOOD: Error message on no path
        LayoutData.layoutSettingsPath = LayoutData.layoutSettingsPath / appName / "settings" / "layouts.toml";
        if (forceReset || !fs::exists(LayoutData.layoutSettingsPath))
        {
            // Make layouts directory
            fs::create_directories(LayoutData.layoutSettingsPath.parent_path());

            auto flags = fs::copy_options::recursive;
            flags |= fs::copy_options::overwrite_existing;

            // Copy everything from settings/layouts
            auto sourceSettings = runtree_path() / "settings" / "layouts.toml";
            if (fs::exists(sourceSettings))
            {
                fs::copy(sourceSettings, LayoutData.layoutSettingsPath.parent_path(), flags);
                LOG(INFO, "Copied layouts");
            }
            else
            {
                LOG(ERROR, "Default setting file not found: " << sourceSettings.string());
                return;
            }
        }

        // Read the layouts.toml
        auto settingsData = toml_read(LayoutData.layoutSettingsPath.string());

        auto root = toml::expect<toml::array>(settingsData, "layout");
        if (root.is_ok())
        {
            // Read back all the layouts, remember the enable keys
            auto arr = root.unwrap();
            for (auto& layout : arr)
            {
                LayoutInfo info;
                std::string base64 = toml_get<std::string>(layout.as_table(), "windows", "");

                // Decode the base64 back to a layout that the client can understand
                auto vecData = cppcodec::base64_rfc4648::decode(base64);
                info.windowLayout = std::string((const char*)vecData.data(), vecData.size());

                // Get the name
                auto name = toml_get<std::string>(layout.as_table(), "name", "Default");

                // Get the enables for this layout
                auto enables = toml::expect<toml::table>(layout, "enables");
                if (enables.is_ok())
                {
                    auto enableTable = enables.unwrap();
                    for (auto& state : enableTable)
                    {
                        info.showFlags[state.first] = state.second.as_boolean();
                    }
                }

                if (!name.empty() && !info.windowLayout.empty())
                {
                    LayoutData.layouts[name] = info;

                    if (LayoutData.lastLayout.empty() && name == "Default")
                    {
                        LayoutData.loadCB(info);
                        LayoutData.lastLayout = name;
                    }
                }
            }
        }
    }
    catch (std::exception& ex)
    {
        M_UNUSED(ex);
        LOG(DBG, "Failed to read settings: ");
        LOG(DBG, ex.what());
    }
}

void layout_manager_save_layouts_file()
{
    toml::table values;
    
    toml::array entries;

    // Store all the layouts with enable keys
    for (auto& [layoutName, layout] : LayoutData.layouts)
    {
        toml::table entry;
        entry["name"] = layoutName;

        auto enc = cppcodec::base64_rfc4648::encode(layout.windowLayout);
        entry["windows"] = enc;
       
        toml::table enables;
        for (auto& [key, visible] : layout.showFlags)
        {
            enables[key] = visible;
        }
        entry["enables"] = enables;

        entries.push_back(entry);
    }

    // Store the last layout, in case one hasn't been loaded yet
    values["last_layout"] = LayoutData.lastLayout; 

    // [layout.entries]
    values["layout"] = entries;

    std::ofstream fileOut(LayoutData.layoutSettingsPath, std::ios_base::out | std::ios_base::trunc);
    fileOut << (toml::value)values;
}

void layout_manager_save_layout(const std::string& layoutName, const std::string& layoutString)
{
    if (layoutName.empty())
    {
        return;
    }

    // Copy existing show state
    for (auto& [key, value] : LayoutData.mapWindowState)
    {
        LayoutData.layouts[layoutName].showFlags[key] = *value.pVisible;
    }

    // Copy layout state
    LayoutData.layouts[layoutName].windowLayout = layoutString;

    layout_manager_save_layouts_file();
}

void layout_manager_load_layout(const std::string& layoutName)
{
    // Find the layout, if not return
    auto itr = LayoutData.layouts.find(layoutName);
    if (itr == LayoutData.layouts.end())
    {
        return;
    }

    // Remember the last thing we loaded
    LayoutData.lastLayout = layoutName;

    // Set the window show states for things we found; everything else stays the same
    for (auto& [key, value] : itr->second.showFlags)
    {
        auto itrFound = LayoutData.mapWindowState.find(key);
        if (itrFound != LayoutData.mapWindowState.end())
        {
            *itrFound->second.pVisible = value;
        }
    }
    LayoutData.loadCB(itr->second);
}

void layout_manager_register_window(const std::string& key, const std::string& name, bool* showState)
{
    // Remember a show pointer for managing window show state
    LayoutData.mapWindowState[key] = WindowState{ name, showState };
}

} // namespace MUtils
