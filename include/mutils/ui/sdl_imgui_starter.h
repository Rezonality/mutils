#pragma once

#include <SDL.h>

#include <imgui/imgui.h>

#include <mutils/file/file.h>
#include <mutils/math/math.h>

#include <gsl/gsl.hpp>

namespace MUtils
{

namespace AppStarterFlags
{
enum
{
    None = (0),
    ShowDemoWindow = (1 << 0),
    DockingEnable = (1 << 1),
    HideCursor = (1 << 2)
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
    virtual void Init() = 0;
    virtual void Update(float seconds, const NVec2i& displaySize) = 0;
    virtual void Destroy() = 0;
    virtual void Draw(const NVec2i& displaySize) = 0;
    virtual void DrawGUI(const NVec2i& displaySize) = 0;
    virtual void KeyEvent(const SDL_KeyboardEvent&) {};
    virtual AppStarterSettings& GetSettings() = 0;
};

struct AppFBO
{
    uint32_t fbo = 0;
    uint32_t fboTexture = 0;
    uint32_t fboDepth = 0;
    NVec2i fboSize;
    NVec2i displaySize;
};


int sdl_imgui_start(int argCount, char** ppArgs, gsl::not_null<IAppStarterClient*> pClient);
void sdl_imgui_clear(const NVec4f& color);

void fbo_bind(const AppFBO& fbo);
void fbo_unbind(const AppFBO& fbo, const NVec2i& displaySize);
AppFBO fbo_create();
void fbo_resize(AppFBO& fbo, const NVec2i& size);
void fbo_destroy(const AppFBO& fbo);


} // MUtils