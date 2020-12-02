#pragma once

#include <SDL.h>

#include <imgui.h>

#include <mutils/file/file.h>
#include <mutils/math/math.h>

#include <gsl-lite/gsl-lite.hpp>

namespace MUtils
{

namespace AppStarterFlags
{
enum
{
    None = (0),
    ShowDemoWindow = (1 << 0),
    DockingEnable = (1 << 1),
    ViewportsEnable = (1 << 2),
    HideCursor = (1 << 3)
};
}

struct AppStarterSettings
{
    NVec2i startSize = NVec2i(1024, 768);
    uint32_t flags = AppStarterFlags::None;
    NVec4f clearColor = NVec4f(0.45f, 0.55f, 0.60f, 1.00f);
    std::string appName = "My App";
};

struct IAppStarterClient
{
    virtual fs::path GetRootPath() const = 0;
    virtual void AddFonts(float size_pixels, const ImFontConfig*, const ImWchar*){};
    virtual void InitDuringDraw() = 0;
    virtual void InitBeforeDraw() = 0;
    virtual void Update(float seconds, const NVec2i& displaySize) = 0;
    virtual void Destroy() = 0;
    virtual void Draw(const NVec2i& displaySize) = 0;
    virtual void DrawGUI(const NVec2i& displaySize) = 0;
    virtual void KeyEvent(const SDL_KeyboardEvent&) {};
    virtual AppStarterSettings& GetSettings() = 0;
};
int sdl_imgui_start(int argCount, char** ppArgs, gsl::not_null<IAppStarterClient*> pClient);

} // MUtils