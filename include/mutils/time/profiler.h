#pragma once

#include "mutils/time/timer.h"
#include <thread>
#include <tracy/Tracy.hpp>

#ifdef TRACY_ENABLE
#define MUtilsPlot(a, b) TracyPlot(a, b)
#define MUtilsLockable(a, b) TracyLockable(a, b)
#define MUtilsSharedLockable(a, b) TracySharedLockable(a, b)
#define MUtilsLockableBase(a) LockableBase(a)
#define MUtilsSharedLockableBase(a) SharedLockableBase(a)
#define MUtilsZoneScoped ZoneScoped
#define MUtilsZoneScopedN(a) ZoneScopedN(a)
#define MUtilsFrameMark FrameMark
#define MUtilsNameThread(a) tracy::SetThreadName(a)
#define MUtilsMessage(a, b) TracyMessage(a, b)
#else
#define MUtilsPlot(a, b)
#define MUtilsLockable(a, b) a b
#define MUtilsSharedLockable(a, b) a b
#define MUtilsLockableBase(a) a
#define MUtilsSharedLockableBase(a) a
#define MUtilsZoneScoped
#define MUtilsZoneScopedN(a)
#define MUtilsFrameMark
#define MUtilsNameThread(a)
#define MUtilsMessage(a, b)
#endif

/*
#define ProfileInitialize(isPaused, commandHandler) MUtils::profilerGlobalInstance.Initialize(isPaused, commandHandler)
#define ProfileShutdown() MUtils::profilerGlobalInstance.Shutdown()
#define ProfileNewFrame() MUtils::profilerGlobalInstance.NewFrame()
#define ProfileDrawUI() MUtils::profilerGlobalInstance.DrawUI()

#define ProfileInitThread() MUtils::profilerGlobalInstance.InitThreadInternal()
#define ProfileFinishThread() MUtils::profilerGlobalInstance.FinishThreadInternal()

#define ProfilePushSection_1(x) MUtils::profilerGlobalInstance.PushSectionInternal(#x, 0x00000000, __FILE__, __LINE__)
#define ProfilePushSection_2(x, y) MUtils::profilerGlobalInstance.PushSectionInternal(#x, y, __FILE__, __LINE__)
#define ProfilePushSection_X(x, y, z, ...) z

#define ProfilePushSection_FUNC_RECOMPOSER(argsWithParentheses) ProfilePushSection_X argsWithParentheses
#define ProfilePushSection_CHOOSE_FROM_ARG_COUNT(...) ProfilePushSection_FUNC_RECOMPOSER((__VA_ARGS__, ProfilePushSection_2, ProfilePushSection_1, ))
#define ProfilePushSection_MACRO_CHOOSER(...) ProfilePushSection_CHOOSE_FROM_ARG_COUNT(__VA_ARGS__())
#define ProfilePushSection(...) ProfilePushSection_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#define ProfilePopSection() gProf.PopSectionInternal()

#define ProfileScopedSection_1(x) MUtils::ProfileScope S##x__LINE__(#x, 0x00000000, __FILE__, __LINE__)
#define ProfileScopedSection_2(x, y) MUtils::ProfileScope S##x__LINE__(#x, y, __FILE__, __LINE__)
#define ProfileScopedSection_X(x, y, z, ...) z
#define ProfileScopedSection_FUNC_RECOMPOSER(argsWithParentheses) ProfileScopedSection_X argsWithParentheses
#define ProfileScopedSection_CHOOSE_FROM_ARG_COUNT(...) ProfileScopedSection_FUNC_RECOMPOSER((__VA_ARGS__, ProfileScopedSection_2, ProfileScopedSection_1, ))
#define ProfileScopedSection_MACRO_CHOOSER(...) ProfileScopedSection_CHOOSE_FROM_ARG_COUNT(__VA_ARGS__())
#define ProfileScopedSection(...) ProfileScopedSection_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#define ProfileFunction0() MUtils::ProfileScope S##__LINE__(__FUNCTION__, 0x00000000, __FILE__, __LINE__)
#define ProfileFunction1(x) MUtils::ProfileScope S##__LINE__(__FUNCTION__, x, __FILE__, __LINE__)
#define ProfileFunction2(x, y) MUtils::ProfileScope S##__LINE__(x, y, __FILE__, __LINE__)
*/
namespace MUtils
{

    /*
class Profiler
{
public:
    static const int MaxThreads = 4;
    static const int MaxFrames = 600;
    static const int MaxSections = 2000;

    enum Color : unsigned int
    {
        Yellow = 0xFF3594B7,
        Red = 0xFF3945D5,
        Blue = 0xFFC55344,
        Green = 0xFF4C8F00,
        Purple = 0xFF8B377A,
        Dark = 0xFF222222,
    };

    void Initialize(bool* isPaused, void (*setPause)(bool));
    void Shutdown();
    void NewFrame();
    void InitThreadInternal();
    void FinishThreadInternal();
    void PushSectionInternal(const char* sectionName, unsigned int color, const char* fileName, int line);
    void PopSectionInternal();

    void DrawUI();
    inline void LockCriticalSection2() {}
    inline void UnLockCriticalSection2() {}

    inline bool IsPaused()
    {
        return m_isPausedPtr != nullptr && *m_isPausedPtr;
    }
    inline void SetPause(bool value)
    {
        if (m_setPause != nullptr)
        {
            m_setPause(value);
        }
    }

    struct Section
    {
        const char* name;
        const char* fileName;
        int line;
        unsigned int color;
        int callStackDepth;
        double startTime;
        double endTime;
        int parentSectionIndex;
    };

    struct Frame
    {
        int index;
        double startTime;
        double endTime;
    };

    struct Thread
    {
        bool initialized;
        int callStackDepth;
        int sectionsCount;
        int sectionIndex;
        Section sections[MaxSections];
        int activeSectionIndex;
    };

    MUtils::timer gTimer;
    Frame m_frames[MaxFrames];
    int m_frameCount;
    int m_frameIndex;
    Thread m_threads[MaxThreads];
    int m_threadIndex;
    bool* m_isPausedPtr;
    void (*m_setPause)(bool);

    // Ui
    double m_timeOffset;
    double m_timeDuration;
    bool m_isWindowOpen;
    float m_frameAreaMaxDuration;
    int m_frameSelectionStart;
    int m_frameSelectionEnd;
    double m_sectionAreaDurationWhenZoomStarted;

private:
    void RefreshFrameSelection(double recordsMaxTime);
};

extern Profiler profilerGlobalInstance;

struct ProfileScope
{
    ProfileScope(const char* name, unsigned int color, const char* fileName, int line)
    {
        profilerGlobalInstance.PushSectionInternal(name, color, fileName, line);
    }

    ~ProfileScope()
    {
        profilerGlobalInstance.PopSectionInternal();
    }
};
*/

namespace YAP
{

enum Colors
{
    InitBase = 0x000655,
    InitMesh = 0x151d80,
    InitTexture = 0x3941aa,
    InitGame = 0x6a72d4,
    InitWindow = 0xaab0ff,
    InitShaders = 0x151d80,
    InitAudio = 0x3941aa,
    InitGUI = 0xaab0ff,
    InitRender = 0x000655,

    LoadMap = 0x002755,
    Load = 0x154580,
    BuildComponents = 0x396caa,

    Nav = 0x6a9ad4,
    NavmeshCompute = 0x9e5d62,
    NavpathCompute = 0x8dc382,

    Render = 0x333300,
    RenderBuildQueue = 0x4d4d0d,
    RenderShadows = 0x666622,
    RenderPostprocess = 0x7f7f40,
    RenderGBuffer = 0x999966,
    RenderSwapBuffer = 0x333300,

    GUI = 0x0b4100,
    GUIQueue = 0x1e6210,
    GUIComponent = 0x3a822b,
    GUIRender = 0x5fa351,

    Game = 0x460633,
    GameAI = 0x63194d,
    GameProcedural = 0x7b3065,
    GameComponents = 0x975282,
    GameSound = 0xb680a6,
    GamePhysics = 0x460633,

    World = 0xd90d12,
    WorldPhysics = 0x682227,
    WorldCharacter = 0x813a3f, // remaining : 9e5d62,bd8a8e

    //remaining:
    // 1d055f, 3a1e87,5a3da8,866bcd,b5a2eb
    Misc = 0x606060,
};
extern int gPhaseCount;
extern int gPhaseIndex;
extern int gMaxThreads;
extern int gMaxEntries;
extern int gMaxPhases;
extern bool gPaused;
extern thread_local int gTLSThreadIndex;
extern bool gProfilerPaused;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Structures

struct ProfilerEntry
{
    // static infos
    const char* szSection;
    const char* szFile;
    int line;
    unsigned int color;
    // infos used for rendering infos
    int level;
    uint64_t startTime;
    uint64_t endTime;
};

struct PhaseEntry
{
    const char* szPhaseName;
    int frameWrite;
};

struct ThreadEntry
{
    bool initialized;
    int entriesCount;
    int callStackDepth;
    int maxLevel;
    uint64_t minTime;
    uint64_t maxTime;
    unsigned int* colorStack;
    int* entriesStack;
};
/*
extern ProfilerEntry* gProfilerEntries;
extern PhaseEntry* gPhaseEntries;
extern ThreadEntry* gThreadEntries;
extern void (*_freeMemFunction)(void* _memory);
*/

void Init(int maxPhases, int maxThreads, int maxEntries, int maxCallStack, void* (*allocMem)(size_t size) = malloc, void (*freeMem)(void* _memory) = free);
void NewFrame();
void PushPhaseBase(const char* szPhaseName = "unknownPhase");
void PushSectionBase(const char*, unsigned int, const char*, int);
void PopSection();
void PopPhase();
void LogDump(int (* /*Log*/)(const char* szFormat, ...));
void ImGuiLogger(bool* opened);
void Finish();

struct ScopedSectionBase
{
    ScopedSectionBase(const char* szSection, unsigned int color, const char* szFile, int line)
    {
        PushSectionBase(szSection, color, szFile, line);
    }
    ~ScopedSectionBase()
    {
        PopSection();
    }
};



} // namespace YAP
} // namespace MUtils

// Phase
#define PushPhase(x) MUtils::YAP::PushPhaseBase(#x)
#define PopPhase() MUtils::YAP::PopPhase()

// Section
#define PushSection_1(x) MUtils::YAP::PushSectionBase(#x, 0xFFFFFFFF, __FILE__, __LINE__)
#define PushSection_2(x, y) MUtils::YAP::PushSectionBase(#x, y, __FILE__, __LINE__)
#define PushSection_X(x, y, z, ...) z

#define PushSection_FUNC_RECOMPOSER(argsWithParentheses) PushSection_X argsWithParentheses
#define PushSection_CHOOSE_FROM_ARG_COUNT(...) PushSection_FUNC_RECOMPOSER((__VA_ARGS__, PushSection_2, PushSection_1, ))
#define PushSection_MACRO_CHOOSER(...) PushSection_CHOOSE_FROM_ARG_COUNT(__VA_ARGS__())
#define PushSection(...) PushSection_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

// Scoped Section
#define ScopedSection_1(x) MUtils::YAP::ScopedSectionBase S##x__LINE__(#x, 0xFFFFFFFF, __FILE__, __LINE__)
#define ScopedSection_2(x, y) MUtils::YAP::ScopedSectionBase S##x__LINE__(#x, y, __FILE__, __LINE__)
#define ScopedSection_X(x, y, z, ...) z

#define ScopedSection_FUNC_RECOMPOSER(argsWithParentheses) ScopedSection_X argsWithParentheses
#define ScopedSection_CHOOSE_FROM_ARG_COUNT(...) ScopedSection_FUNC_RECOMPOSER((__VA_ARGS__, ScopedSection_2, ScopedSection_1, ))
#define ScopedSection_MACRO_CHOOSER(...) ScopedSection_CHOOSE_FROM_ARG_COUNT(__VA_ARGS__())
#define ScopedSection(...) ScopedSection_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
