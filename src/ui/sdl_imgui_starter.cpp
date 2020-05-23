// dear imgui: standalone example application for SDL2 + OpenGL
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

#include "mutils/ui/sdl_imgui_starter.h"

#include "mutils/file/runtree.h"
#include "mutils/logger/logger.h"
#include "mutils/profile/profile.h"
#include "mutils/time/timer.h"
#include "mutils/ui/dpi.h"

#include <imgui/examples/imgui_impl_opengl3.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/misc/freetype/imgui_freetype.h>

#include <GL/gl3w.h>
using namespace gsl;

namespace MUtils
{

#undef ERROR
#ifdef _DEBUG
Logger logger = { true, DEBUG };
#else
Logger logger = { true, INFO };
#endif
bool LOG::disabled = false;

IAppStarterClient* pClient = nullptr;
int sdl_imgui_start(int argCount, char** ppArgs, not_null<IAppStarterClient*> pClient)
{
    check_dpi();

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup app runtree
    runtree_init(pClient->GetRootPath());

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    auto& settings = pClient->GetSettings();

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow(settings.appName.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, settings.startSize.x, settings.startSize.y, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    bool err = gl3wInit() != 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetStyle().ScaleAllSizes(dpi.scaleFactor);

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    if (settings.flags & AppStarterFlags::DockingEnable)
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    }
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Font for editor
    static const ImWchar ranges[] = {
        0x0020,
        0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.DstFont = ImGui::GetFont();
    io.Fonts->AddFontFromFileTTF(runtree_find_asset("fonts/ProggyClean.ttf").string().c_str(), 13 * dpi.scaleFactor, &config, ranges);

    unsigned int flags = 0; // ImGuiFreeType::NoHinting;
    ImGuiFreeType::BuildFontAtlas(io.Fonts, flags);

    // Our state
    bool show_demo_window = false;
    bool firstInit = true;

    timer theTimer;
    timer_start(theTimer);

    // Main loop
    bool done = false;

    while (!done)
    {

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            {
                pClient->KeyEvent(event.key);
            }
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        auto displaySize = NVec2i(w, h);

        glViewport(0, 0, (int)displaySize.x, (int)displaySize.y);
        glClearColor(settings.clearColor.x, settings.clearColor.y, settings.clearColor.z, settings.clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (displaySize.x > 0 && displaySize.y > 0)
        {
            pClient->Update(timer_get_elapsed_seconds(theTimer), displaySize);
            pClient->Draw(displaySize);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        if (firstInit)
        {
            pClient->Init();
            firstInit = false;
        }

        bool shouldHide = ((settings.flags & AppStarterFlags::HideCursor) ? true : false);
        if (shouldHide)
        {
            SDL_ShowCursor(0);
        }

        pClient->DrawGUI(displaySize);

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (settings.flags & AppStarterFlags::ShowDemoWindow)
            ImGui::ShowDemoWindow(&show_demo_window);

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }

        SDL_GL_SwapWindow(window);

        MUtilsFrameMark;
    }

    pClient->Destroy();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void fbo_bind(const AppFBO& fbo)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
    glViewport(0, 0, (int)fbo.fboSize.x, (int)fbo.fboSize.y);
}

void fbo_unbind(const AppFBO& fbo, const NVec2i& displaySize)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, (int)displaySize.x, (int)displaySize.y);
}

AppFBO fbo_create()
{
    AppFBO fbo;
    glGenFramebuffers(1, &fbo.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

    // The texture we're going to render to
    glGenTextures(1, &fbo.fboTexture);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, fbo.fboTexture);

    glGenRenderbuffers(1, &fbo.fboDepth);
    return fbo;
}

void sdl_imgui_clear(const NVec4f& color)
{
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void fbo_resize(AppFBO& fbo, const NVec2i& newFboSize)
{
    if (fbo.fboSize == newFboSize)
    {
        return;
    }

    fbo.fboSize = newFboSize;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
    glBindTexture(GL_TEXTURE_2D, fbo.fboTexture);

    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbo.fboSize.x, fbo.fboSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbo.fboTexture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, fbo.fboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fbo.fboSize.x, fbo.fboSize.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo.fboDepth);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, DrawBuffers);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG(ERROR) << "FBO Error: " << status;
    }
}

void fbo_destroy(const AppFBO& fbo)
{
    glDeleteFramebuffers(1, &fbo.fbo);
    glDeleteRenderbuffers(1, &fbo.fboDepth);
    glDeleteTextures(1, &fbo.fboTexture);
}

} // namespace MUtils
