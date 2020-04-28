#pragma once

#include <SDL.h>

#include <mutils/file/file.h>

#include <glm/glm.hpp>
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
    glm::ivec2 startSize = glm::ivec2(1024, 768);
    uint32_t flags = AppStarterFlags::None;
    glm::vec4 clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);
    std::string appName = "My App";
};

struct IAppStarterClient
{
    virtual fs::path GetRootPath() const = 0;
    virtual void Init() = 0;
    virtual void Update(float seconds, const glm::ivec2& displaySize) = 0;
    virtual void Destroy() = 0;
    virtual void Draw(const glm::ivec2& displaySize) = 0;
    virtual void DrawGUI(const glm::ivec2& displaySize) = 0;
    virtual void KeyEvent(const SDL_KeyboardEvent&) {};
    virtual AppStarterSettings& GetSettings() = 0;
};

int sdl_imgui_start(int argCount, char** ppArgs, gsl::not_null<IAppStarterClient*> pClient);

} // MUtils