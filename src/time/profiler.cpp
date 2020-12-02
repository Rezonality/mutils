#include <algorithm>
#include <atomic>
#include <cassert>

#include <mutils/time/profiler.h>
#include <mutils/math/imgui_glm.h>
#include <mutils/math/math_utils.h>
#include <mutils/ui/dpi.h>

#include "imgui.h"
#include "imgui_internal.h"
#include <fmt/format.h>

using namespace std::chrono;

// Initial prototypes used:
// https://gist.github.com/CedricGuillemet/fc82e7a9b2f703ff1ebf
// https://github.com/arnaud-jamin/imgui/blob/master/profiler.cpp
// 
// This one is better at spans that cover frame boundaries, uses cross platform cpp libraries
// instead of OS specific ones.
// No claims made about performance, but in practice has been very useful in finding bugs
// Has a summary view and support for zoom/pan, CTRL+select range or click in the summary graphs
// There is an optional single 'Region' which I use for audio frame profiling; you can only have one
// The profile macros can pick a unique/nice color for a given name.
// There is a LOCK_GUARD wrapper around a mutex, for tracking lock times
// Requires some helper code from my mutils library (https://github.com/Rezonality/MUtils) for:
// - NVec/NRect
// - murmur hash (for text to color)
// - dpi 
// - fmt for text formatting
// I typically use this as a 'one shot, collect frames' debugger.  I hit the Pause/Resume button at interesting spots and collect a bunch of frames.
// You have to pause to navigate/inspect.
// All memory allocation is up-front; change the 'Max' values below to collect more frames
// The profiler just 'stops' when the memory is full.  It can be restarted/stopped.
// I pulled this together over the space of a weekend, it could be tidier here and there, but it works great ;)
namespace MUtils
{

using namespace Theme;

namespace Profiler
{

namespace
{

const unsigned int frameMarkerColor = 0x22FFFFFF;

const uint32_t MaxThreads = 50;
const uint32_t MaxCallStack = 20;
const uint32_t MaxEntriesPerThread = 100000;
const uint32_t MaxFrames = 10000;
const uint32_t MaxRegions = 10000;

const uint32_t MinLeadInFrames = 3;
const uint32_t MinFrame = MinLeadInFrames - 2;
const uint32_t MinSizeForTextDisplay = 5;

MUtils::timer gTimer;
bool gPaused = true;
bool gRequestPause = false;
bool gRestarting = true;

std::mutex gMutex;

std::atomic<uint64_t> gProfilerGeneration = 0;
thread_local int gThreadIndexTLS = -1;
thread_local uint64_t gGenerationTLS = -1;
float gMaxThreadNameSize = 0;

std::vector<ThreadData> gThreadData;
std::vector<Frame> gFrameData;
std::vector<Region> gRegionData;

int64_t gMaxFrameTime = duration_cast<nanoseconds>(milliseconds(20)).count();
uint32_t gCurrentFrame = 0;
int32_t gSelectedThread = -1;

// Region
uint32_t gCurrentRegion = 0;
int64_t gRegionTimeLimit = 0;
int64_t gRegionDisplayStart = 0;
int64_t gFrameDisplayStart = 0;
NRectf gCandleDragRect;

// Visible frame candle range
NVec2f gFrameCandleRange = NVec2f(0.0f, 0.0f);

// Visible time range
NVec2ll gTimeRange = NVec2ll(0, 0);

// Frames visible inside the current time range
NVec2i gVisibleFrames = NVec2i(0, 0);

} // namespace

// Run Init every time a profile is started
void Init()
{
    gThreadData.resize(MaxThreads);

    gProfilerGeneration++;

    for (uint32_t iZero = 0; iZero < MaxThreads; iZero++)
    {
        ThreadData* threadData = &gThreadData[iZero];
        threadData->initialized = iZero == 0;
        threadData->maxLevel = 0;
        threadData->minTime = std::numeric_limits<int64_t>::max();
        threadData->maxTime = 0;
        threadData->currentEntry = 0;
        threadData->name = std::string("Thread ") + std::to_string(iZero);
        threadData->entries.resize(MaxEntriesPerThread);
        memset(threadData->entries.data(), 0, sizeof(ProfilerEntry) * threadData->entries.size());
        threadData->entryStack.resize(50);
        threadData->callStackDepth = 0;
    }

    gFrameData.resize(MaxFrames);
    gRegionData.resize(MaxRegions);
    for (auto& frame : gFrameData)
    {
        frame.frameThreads.resize(MaxThreads);
        frame.frameThreadCount = 0;
        for (auto& threadInfo : frame.frameThreads)
        {
            threadInfo.activeEntry = 0;
            threadInfo.threadIndex = 0;
        }
    }

    gThreadIndexTLS = 0;
    gGenerationTLS = 0;
    gThreadData[0].initialized = true;
    gRestarting = true;
    gCurrentFrame = 0;
    gCurrentRegion = 0;
    gMaxFrameTime = duration_cast<nanoseconds>(milliseconds(30)).count();
    gVisibleFrames = NVec2<uint32_t>(0, 0);
    gFrameCandleRange = NVec2f(0, 0);
    gFrameDisplayStart = 0;
    gRegionDisplayStart = 0;
    gMaxThreadNameSize = 0.0f;
    timer_start(gTimer);

    gPaused = false;
}

void InitThread()
{
    std::unique_lock<std::mutex> lk(gMutex);

    for (uint32_t iThread = 0; iThread < MaxThreads; iThread++)
    {
        ThreadData* threadData = &gThreadData[iThread];
        if (!threadData->initialized)
        {
            // use it
            gThreadIndexTLS = iThread;
            gGenerationTLS = gProfilerGeneration;
            threadData->currentEntry = 0;
            threadData->initialized = true;
            return;
        }
    }
    assert(false && "Every thread slots are used!");
}

void FinishThread()
{
    std::unique_lock<std::mutex> lk(gMutex);
    assert(gThreadIndexTLS > -1 && "Trying to finish an uninitilized thread.");
    gThreadData[gThreadIndexTLS].initialized = false;
    gThreadIndexTLS = -1;
}

void Finish()
{
    gThreadData.clear();
}

void SetPaused(bool pause)
{
    if (gPaused != pause)
    {
        gRequestPause = pause;
    }
}

ThreadData* GetThreadData()
{
    if (gGenerationTLS != gProfilerGeneration.load())
    {
        gThreadIndexTLS = -1;
    }

    if (gThreadIndexTLS == -1)
    {
        InitThread();
    }

    return &gThreadData[gThreadIndexTLS];
}

void HideThread()
{
    if (gPaused)
    {
        return;
    }
    ThreadData* threadData = GetThreadData();
    threadData->hidden = true;
}

void Reset()
{
    std::unique_lock<std::mutex> lk(gMutex);
    gThreadIndexTLS = -1;
    Init();
}

bool CheckEndState()
{
    if (gThreadData[gThreadIndexTLS].currentEntry >= MaxEntriesPerThread)
    {
        gPaused = true;
    }

    if (gCurrentFrame >= MaxFrames)
    {
        gPaused = true;
    }

    if (gCurrentRegion >= MaxRegions)
    {
        gPaused = true;
    }
    return gPaused;
}

void PushSectionBase(const char* szSection, unsigned int color, const char* szFile, int line)
{
    if (gPaused)
    {
        return;
    }

    ThreadData* threadData = GetThreadData();
    if (CheckEndState())
    {
        return;
    }

    assert(threadData->callStackDepth < MaxCallStack && "Might need to make call stack bigger!");

    // check again
    if (gPaused)
    {
        return;
    }

    // Write entry 0
    ProfilerEntry* profilerEntry = &threadData->entries[threadData->currentEntry];
    threadData->entryStack[threadData->callStackDepth] = threadData->currentEntry;

    if (threadData->callStackDepth > 0)
    {
        profilerEntry->parent = threadData->entryStack[threadData->callStackDepth - 1];
        assert(profilerEntry->parent < threadData->currentEntry);
    }
    else
    {
        profilerEntry->parent = 0xFFFFFFFF;
    }

    assert(szFile != NULL && "No file string specified");
    assert(szSection != NULL && "No section name specified");

    profilerEntry->color = color;
    profilerEntry->szFile = szFile;
    profilerEntry->szSection = szSection;
    profilerEntry->line = line;
    profilerEntry->startTime = timer_get_elapsed(gTimer);
    profilerEntry->endTime = std::numeric_limits<int64_t>::max();
    profilerEntry->level = threadData->callStackDepth;
    threadData->callStackDepth++;
    threadData->currentEntry++;

    threadData->maxLevel = std::max(threadData->maxLevel, threadData->callStackDepth);

    threadData->minTime = std::min(profilerEntry->startTime, threadData->minTime);
    threadData->maxTime = std::max(profilerEntry->startTime, threadData->maxTime);

    if (gRestarting)
    {
        gRestarting = false;
    }
}

void PopSection()
{
    if (gPaused)
    {
        return;
    }

    ThreadData* threadData = GetThreadData();
    if (CheckEndState())
    {
        return;
    }


    if (gRestarting || gPaused)
    {
        return;
    }

    if (threadData->callStackDepth <= 0)
    {
        return;
    }

    // Back to the last entry we wrote
    threadData->callStackDepth--;

    int entryIndex = threadData->entryStack[threadData->callStackDepth];
    ProfilerEntry* profilerEntry = &threadData->entries[entryIndex];

    assert(profilerEntry->szSection != nullptr);

    // store end time
    profilerEntry->endTime = timer_get_elapsed(gTimer);

    threadData->maxTime = std::max(profilerEntry->endTime, threadData->maxTime);
}

void SetRegionLimit(uint64_t maxTimeNs)
{
    gRegionTimeLimit = maxTimeNs;
}

void NameThread(const char* pszName)
{
    if (gPaused)
    {
        return;
    }

    // Must get thread data to init the thread
    ThreadData* threadData = GetThreadData();
    threadData->name = pszName;
}

// You are allowed one secondary region - I use it for audio thread monitoring
void BeginRegion()
{
    if (gPaused)
    {
        return; 
    }

    // Must get thread data to init the thread
    ThreadData* threadData = GetThreadData();
    if (CheckEndState())
    {
        return;
    }

    auto& region = gRegionData[gCurrentRegion];
    region.startTime = timer_get_elapsed(gTimer);
}

void EndRegion()
{
    if (gPaused)
    {
        return;
    }

    ThreadData* threadData = GetThreadData();
    if (CheckEndState())
    {
        return;
    }

    auto& region = gRegionData[gCurrentRegion];
    region.endTime = timer_get_elapsed(gTimer);
    region.name = fmt::format("{:.2f}ms", float(timer_to_ms(region.endTime - region.startTime)));

    gCurrentRegion++;
}

void NewFrame()
{
    if (gPaused)
    {
        return;
    }

    if (CheckEndState())
    {
        return;
    }

    auto& frame = gFrameData[gCurrentFrame];
    for (uint32_t threadIndex = 0; threadIndex < MaxThreads; threadIndex++)
    {
        auto& thread = gThreadData[threadIndex];
        if (!thread.initialized)
        {
            continue;
        }
        if (thread.currentEntry > 0)
        {
            // Remember which entry was active for this thread
            auto& threadInfo = frame.frameThreads[frame.frameThreadCount];
            threadInfo.activeEntry = thread.currentEntry - 1;
            threadInfo.threadIndex = threadIndex;

            // Next thread
            frame.frameThreadCount++;
        }
    }

    frame.startTime = timer_get_elapsed(gTimer);
    if (gCurrentFrame > 0)
    {
        gFrameData[gCurrentFrame - 1].endTime = frame.startTime;
        gFrameData[gCurrentFrame - 1].name = fmt::format("{:.2f}ms", float(timer_to_ms(frame.startTime - gFrameData[gCurrentFrame - 1].startTime)));
    }
    gCurrentFrame++;
}

// Which frames we can see in the main viewport for the current zoom
void UpdateVisibleFrameRange()
{
    if (gCurrentFrame < 2)
    {
        gVisibleFrames.x = gVisibleFrames.y = 0;
        gFrameCandleRange.x = gFrameCandleRange.y = 0.0f;
    }

    NVec2<int32_t> frameRangeLimits = NVec2<int32_t>(0, gCurrentFrame - 2);

    gVisibleFrames.x = std::clamp(gVisibleFrames.x, 0, frameRangeLimits.x);
    gVisibleFrames.y = std::clamp(gVisibleFrames.y, 0, frameRangeLimits.y);

    while ((gVisibleFrames.y < frameRangeLimits.y) && (gFrameData[gVisibleFrames.y].endTime < gTimeRange.y))
    {
        gVisibleFrames.y++;
    };

    while ((gVisibleFrames.y > frameRangeLimits.x) && (gFrameData[gVisibleFrames.y].startTime > gTimeRange.y))
    {
        gVisibleFrames.y--;
    };

    while ((gVisibleFrames.x < frameRangeLimits.y) && (gFrameData[gVisibleFrames.x].endTime < gTimeRange.x))
    {
        gVisibleFrames.x++;
    };

    while ((gVisibleFrames.x > frameRangeLimits.x) && (gFrameData[gVisibleFrames.x].startTime > gTimeRange.x))
    {
        gVisibleFrames.x--;
    };
    gVisibleFrames.y++;
}

// Show the frame and region candles at the top.
// Returns possible selected time range from the mouse
NVec2ll ShowCandles(NVec2f& regionMin, NVec2f& regionMax)
{
    // Show the frame candles
    auto handleMouse = [&](const char* buttonName, const NRectf& region, auto& range, const auto& currentFrame) {
        const auto minCandlesPerView = int(4);
        const NVec2f regionSize = region.Size();
        bool changed = false;

        if (!gPaused)
        {
            gCandleDragRect.Clear();
            return false;
        }

        // This code could be abstracted into a generalized zoom and perhaps shared with the similar code below
        ImGui::InvisibleButton(buttonName, regionSize);
        if (ImGui::IsItemActive())
        {
            if (ImGui::IsMouseDragging(0))
            {
                auto delta = ImGui::GetMouseDragDelta(0).x;
                if (ImGui::GetIO().KeyCtrl)
                {
                    gCandleDragRect = NRectf(NVec2f(ImGui::GetIO().MouseClickedPos[0]) - region.topLeftPx, NVec2f(ImGui::GetIO().MouseClickedPos[0]) - region.topLeftPx + NVec2f(ImGui::GetMouseDragDelta(0)));
                    gCandleDragRect.Normalize();
                }
                else
                {
                    gCandleDragRect = NRectf();
                    auto dragCandleDelta = float((delta / regionSize.x) * (range.y - range.x));
                    ImGui::ResetMouseDragDelta(0);

                    auto newVisible = range - NVec2f(dragCandleDelta, dragCandleDelta);
                    if ((newVisible.y < (currentFrame - 1) && newVisible.x >= MinFrame))
                    {
                        range = newVisible;
                        changed = true;
                    }
                }
            }
            else if (ImGui::IsMouseClicked(0))
            {
                gCandleDragRect.Clear();
            }
        }

        // Zoom and mouse click over area
        if (ImGui::IsMouseHoveringRect(region.topLeftPx, region.bottomRightPx))
        {
            if (ImGui::IsMouseReleased(0))
            {
                if (gCandleDragRect.Empty())
                {
                    gCandleDragRect = NRectf(NVec2f(ImGui::GetIO().MouseClickedPos[0]) - region.topLeftPx, NVec2f(ImGui::GetIO().MouseClickedPos[0]) - region.topLeftPx + NVec2f(1.0f, 0.0f));
                    gCandleDragRect.Normalize();
                }
            }

            if (ImGui::GetIO().MouseWheel != 0)
            {
                gCandleDragRect.Clear();

                const auto sectionWheelZoomSpeed = 1.0f;
                auto mouseToCandle = [&](NVec2f& range) {
                    return (((ImGui::GetMousePos().x - region.Left()) / regionSize.x) * (range.y - range.x)) + range.x;
                };

                auto zoom = ImSign(ImGui::GetIO().MouseWheel) * sectionWheelZoomSpeed;
                if (zoom != 0.0f)
                {
                    auto candleRange = range.y - range.x;
                    auto tenPercent = ((range.y - range.x) * .1f) * zoom;
                    auto newVisible = range + NVec2f(tenPercent, -tenPercent);

                    auto oldMouseCandle = mouseToCandle(range);
                    auto newMouseCandle = mouseToCandle(newVisible);

                    newVisible.x -= (newMouseCandle - oldMouseCandle);
                    newVisible.y -= (newMouseCandle - oldMouseCandle);

                    if ((newVisible.y - newVisible.x) >= minCandlesPerView)
                    {
                        range = newVisible;
                        if (range.x < MinFrame)
                        {
                            auto diff = MinFrame - range.x;
                            range.x += diff;
                            range.y += diff;
                        }

                        if (range.y > int32_t(currentFrame - 1))
                        {
                            auto diff = range.y - (currentFrame - 1);
                            range.x -= diff;
                            range.y -= diff;
                        }
                        range.x = std::clamp(range.x, float(MinFrame), float(currentFrame - 1));
                        range.y = std::clamp(range.y, float(MinFrame), float(currentFrame - 1));
                        changed = true;
                    }
                }
            }
        }
        return changed;
    };

    const float CandleHeight = 30 * dpi.scaleFactorXY.y;
    NRectf regionFrames = NRectf(regionMin.x, regionMin.y, regionMax.x - regionMin.x, CandleHeight);
    NRectf regionRegion = NRectf(regionMin.x, regionMin.y + CandleHeight, regionMax.x - regionMin.x, CandleHeight);
    NRectf regionBoth = NRectf(regionFrames.topLeftPx, regionRegion.bottomRightPx);

    handleMouse("##frameButton", regionBoth, gFrameCandleRange, gCurrentFrame);

    if (!gPaused)
    {
        if (gCurrentFrame >= MinLeadInFrames)
        {
            gFrameCandleRange = NVec2f(MinFrame, float(gCurrentFrame - 1));
            gFrameCandleRange.x = std::max(gFrameCandleRange.x, 0.0f);
        }
    }

    NVec2ll dragTimeRange = NVec2ll(0);
    auto drawRegions = [&dragTimeRange](const auto& region, const auto& framesStartTime, const auto& framesDuration, auto& regionData, auto& regionDisplayStart, const auto& maxTime, const auto& limitTime, const auto& color1, const auto& color2) {
        const NVec2f candleRegionSize = region.Size();
        const auto pDrawList = ImGui::GetWindowDrawList();
        const auto LimitColor = ThemeManager::Instance().GetColor(ThemeColor::Error);

        auto timePerPixel = framesDuration / int64_t(region.Width());
        auto MaxRegion = int64_t(gCurrentRegion);

        // Keep global counters to simplify finding the regions
        while (regionDisplayStart > 0 && regionData[regionDisplayStart].startTime > framesStartTime)
        {
            regionDisplayStart--;
        }
        while ((regionDisplayStart < MaxRegion) && regionData[regionDisplayStart].endTime < framesStartTime)
        {
            regionDisplayStart++;
        }

        auto dragLimits = NVec2f(gCandleDragRect.topLeftPx.x + region.Left(), gCandleDragRect.Right() + region.Left());
        auto pixelTime = framesStartTime - timePerPixel;
        auto currentRegion = regionDisplayStart;
        auto lastX = -1.0f;
        auto lastHeight = 0.0f;
        auto lastLimit = 0.0f;
        auto colOn = false;
        auto regionFind = region.Contains(ImGui::GetIO().MouseClickedPos[0]) && (dragLimits.x != dragLimits.y);

        // Walk the pixels
        for (float pixel = region.Left(); pixel < region.Right(); pixel++)
        {
            pixelTime += timePerPixel;

            // Catch up to the pixel
            while ((currentRegion < MaxRegion) && (regionData[currentRegion].endTime < pixelTime))
            {
                currentRegion++;
            }

            // Don't fall off the end
            if (currentRegion >= MaxRegion)
            {
                break;
            }

            // We are ahead, move to next pixel
            if (regionData[currentRegion].startTime > (pixelTime + timePerPixel))
            {
                continue;
            }

            NVec2ll regionTimeRange;
            if (currentRegion < MaxRegion)
            {
                regionTimeRange.x = regionData[int64_t(currentRegion)].startTime;

                // Collect durations of all candles within this pixel
                uint32_t count = 0;
                int64_t totalDuration = 0;
                while (currentRegion < MaxRegion)
                {
                    auto& data = regionData[int64_t(currentRegion)];
                    regionTimeRange.y = data.endTime;
                    totalDuration += data.endTime - data.startTime;
                    
                    // Find the time region which the mouse has selected
                    if (regionFind)
                    {
                        if (dragTimeRange.x == 0 && (dragLimits.x < pixel))
                        {
                            dragTimeRange.x = regionTimeRange.x;
                            dragTimeRange.y = regionTimeRange.y;
                        }
                        if (dragLimits.y >= pixel)
                        {
                            dragTimeRange.y = regionTimeRange.y;
                        }
                    }

                    count++;
                    if (regionData[currentRegion].endTime > (pixelTime + timePerPixel))
                    {
                        break;
                    }
                    currentRegion++;
                }

                // Found something, draw average
                if (count > 0)
                {
                    totalDuration /= count;
                    float candleHeight = totalDuration / float(maxTime);
                    candleHeight = std::min(candleHeight, 1.0f);

                    float candleLimit = totalDuration / float(limitTime);
                    candleLimit = std::clamp(candleLimit, 0.0f, 1.0f);

                    if (lastX == -1)
                    {
                        lastX = pixel;
                    }
                    else
                    {
                        if (lastHeight != candleHeight)
                        {
                            // Should probably blend here
                            auto col = colOn ? color1 : color2;
                            colOn = !colOn;

                            col = MUtils::Mix(col, LimitColor, lastLimit);

                            pDrawList->AddRectFilled(ImVec2(lastX, region.Bottom() - 1.0f - std::max(1.0f, (lastHeight * region.Height() - 2.0f))), ImVec2(pixel, region.Bottom() - 1.0f), ToPackedABGR(col));
                            lastX = pixel;
                        }
                    }
                    lastHeight = candleHeight;
                    lastLimit = candleLimit;
                }
            }
        }

        if (lastX != -1)
        {
            // Should probably blend here
            auto col = colOn ? color1 : color2;
            col = MUtils::Mix(col, LimitColor, lastLimit);

            pDrawList->AddRectFilled(ImVec2(lastX, region.Bottom() - 1.0f - std::max(1.0f, (lastHeight * region.Height() - 2.0f))), ImVec2(region.Right(), region.Bottom() - 1.0f), ToPackedABGR(col));
        }

        if (!gCandleDragRect.Empty())
        {
            pDrawList->AddRectFilled(ImVec2(gCandleDragRect.Left() + region.topLeftPx.x, region.topLeftPx.y), ImVec2(gCandleDragRect.Right() + region.topLeftPx.x, region.Bottom()), 0x77777777);
        }

        assert(dragTimeRange.x <= dragTimeRange.y);
    };

    const auto FrameCandleColor = ThemeManager::Instance().GetColor(ThemeColor::AccentColor1);
    const auto FrameCandleAltColor = ThemeManager::Instance().GetColor(ThemeColor::AccentColor2) * .85f;
    const auto RegionCandleColor = ThemeManager::Instance().GetColor(ThemeColor::Warning);
    const auto RegionCandleAltColor = ThemeManager::Instance().GetColor(ThemeColor::Warning) * .85f;
    const auto framesStartTime = gFrameData[int64_t(gFrameCandleRange.x)].startTime;
    const auto framesDuration = gFrameData[int64_t(gFrameCandleRange.y)].startTime - framesStartTime;

    drawRegions(regionFrames, framesStartTime, framesDuration, gFrameData, gFrameDisplayStart, gMaxFrameTime, gMaxFrameTime, FrameCandleColor, FrameCandleAltColor);
    regionMin.y += CandleHeight + 2.0f * dpi.scaleFactorXY.y;

    drawRegions(regionRegion, framesStartTime, framesDuration, gRegionData, gRegionDisplayStart, gRegionTimeLimit, gRegionTimeLimit, RegionCandleColor, RegionCandleAltColor);
    regionMin.y += CandleHeight;

    if (dragTimeRange.x > dragTimeRange.y)
    {
        dragTimeRange = NVec2ll(0); 
    }
    return dragTimeRange;
}

// Show the profiler window
void ShowProfile(bool* opened)
{
    if (!ImGui::Begin("Profiler", opened))
    {
        ImGui::End();
        return;
    }

    PROFILE_SCOPE(Profile_UI);

    bool pause = gPaused;
    bool changed = false;

    if (pause != gRequestPause)
    {
        changed = true;
        pause = gRequestPause;
    }

    if (ImGui::Button(pause ? "Resume" : "Pause"))
    {
        changed = true;
        pause = !pause;
    }

    if (changed)
    {
        // Call reset before setting paused
        if (!pause)
        {
            Reset();
        }
        gPaused = pause;
        gRequestPause = pause;
    }

    ImGui::SameLine();

    static float scale = 1.0f;

    ImGui::PushItemWidth(100 * dpi.scaleFactorXY.x);
    ImGui::SliderFloat("Scale", &scale, .5f, 1.0f, "%.2f");

    // Ignore the first frame, which is likely a long delay due to
    // the time that expires after this profiler is created and the first
    // frame is drawn
    const uint32_t MaxFrame = gCurrentFrame - 2;
    if (gCurrentFrame < MinLeadInFrames)
    {
        ImGui::End();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto regionSize = ImGui::GetContentRegionAvail();
    NVec2f regionMin(ImGui::GetCursorScreenPos());
    NVec2f regionMax(regionMin.x + regionSize.x, regionMin.y + regionSize.y);
    NVec2f topLeft = regionMin;

    auto selectedTimeRange = ShowCandles(regionMin, regionMax);

    // Reset region size
    regionSize = regionMax - regionMin;

    // Setup
    const NVec2f textPadding = NVec2f(3, 3) * dpi.scaleFactorXY;
    auto pDrawList = ImGui::GetWindowDrawList();
    const auto pFont = ImGui::GetFont();
    const auto fontSize = ImGui::GetFontSize() * scale;
    const auto smallFontSize = fontSize * .66f;
    const auto heightPerLevel = fontSize + 2.0f;

    const auto maxTime = gFrameData[gCurrentFrame - 1].startTime;
    const auto minTime = gFrameData[MinFrame].startTime;
    int64_t visibleDuration;
    double pixelsPerTime;
    double timePerPixels;

    auto setTimeRange = [&](NVec2<int64_t> range) {
        assert(range.y > range.x);
        if ((range.y < range.x) || ((range.y - range.x) < 1000))
        {
            range.x = std::clamp(range.x, minTime, maxTime - 1000);
            range.y = range.x + 1000;
        }

        if (range.x < minTime)
        {
            auto diff = minTime - range.x;
            range.x += diff;
            range.y += diff;
        }
        else if (range.y > maxTime)
        {
            auto diff = range.y - maxTime;
            range.y -= diff;
            range.x -= diff;
        }
        range.x = std::clamp(range.x, minTime, maxTime);
        range.y = std::clamp(range.y, minTime, maxTime);
        assert((range.y - range.x) >= 1000);

        gTimeRange = range;
        visibleDuration = range.y - range.x;
        pixelsPerTime = (double)regionSize.x / double(visibleDuration);
        timePerPixels = 1.0 / pixelsPerTime;
    };

    const auto now = (int64_t)timer_get_elapsed(gTimer);
    if (!gPaused)
    {
        auto duration = duration_cast<nanoseconds>(milliseconds(50)).count();
        setTimeRange(NVec2<int64_t>(now - duration, now));
    }
    else
    {
        if (selectedTimeRange.x != selectedTimeRange.y)
        {
            // Add 10 percent to the selection
            auto tenPercent = int64_t((selectedTimeRange.y - selectedTimeRange.x) * .05f);
            selectedTimeRange.x -= tenPercent;
            selectedTimeRange.y += tenPercent;

            gTimeRange = selectedTimeRange;
        }
        setTimeRange(gTimeRange);
    }

    // Time normalized to gTimeRange.x
    auto xFromTime = [&](int64_t time) {
        return ((time - gTimeRange.x) * regionSize.x) / visibleDuration;
    };

    auto timeFromX = [&](uint32_t x) {
        return gTimeRange.x + double(x / regionSize.x) * visibleDuration;
    };

    regionSize.y -= 2;
    NVec2f mouseClick;
    ImGui::InvisibleButton("##FramesSectionsWindowDummy", regionSize);
    if (ImGui::IsItemActive())
    {
        if (ImGui::IsMouseDragging(0))
        {
            gCandleDragRect.Clear();

            auto delta = ImGui::GetMouseDragDelta(0).x;
            auto dragTimeDelta = int64_t((delta / regionSize.x) * visibleDuration);
            ImGui::ResetMouseDragDelta(0);
            auto newTime = NVec2<int64_t>(gTimeRange.x - dragTimeDelta, gTimeRange.y - dragTimeDelta);
            if ((newTime.y < maxTime && newTime.x >= minTime))
            {
                setTimeRange(newTime);
            }
        }
        else if (ImGui::IsMouseClicked(0))
        {
            mouseClick = ImGui::GetMousePos();
            gCandleDragRect.Clear();
        }
    }

    // Zoom
    if (ImGui::IsMouseHoveringRect(regionMin, regionMax) && ImGui::GetIO().MouseWheel != 0)
    {
        gCandleDragRect.Clear();

        const auto sectionWheelZoomSpeed = 1.0f;
        auto zoom = ImSign(ImGui::GetIO().MouseWheel) * sectionWheelZoomSpeed;
        auto localMousePos = uint32_t(ImGui::GetMousePos().x - regionMin.x);
        auto tenPercent = (int64_t)((gTimeRange.y - gTimeRange.x) * .1 * zoom);

        auto mouseTime = timeFromX(localMousePos);
        auto newTime = NVec2<int64_t>((gTimeRange.x + tenPercent), gTimeRange.y - tenPercent);

        setTimeRange(newTime);

        auto newMouseTime = timeFromX(localMousePos);
        auto timeDelta = int64_t(newMouseTime - mouseTime);
        newTime = NVec2<int64_t>(gTimeRange.x - timeDelta, gTimeRange.y - timeDelta);
        setTimeRange(newTime);
    }

    UpdateVisibleFrameRange();

    // Walk the visible frames, drawing each thread
    double lastFrameX = -regionSize.x;
    bool firstFrame = true;

    for (int32_t frameIndex = gVisibleFrames.x; frameIndex < gVisibleFrames.y; frameIndex++)
    {
        auto& frameInfo = gFrameData[frameIndex];

        // Left hand side of the frame
        auto xFrameMarker = xFromTime(frameInfo.startTime);
        if (xFrameMarker >= 0.0 && (xFrameMarker - lastFrameX) > 20.0)
        {
            // Vertical frame line
            pDrawList->AddLine(ImVec2(regionMin.x + float(xFrameMarker), regionMin.y), ImVec2(regionMin.x + float(xFrameMarker), regionMax.y), frameMarkerColor, 1.0f);

            // Frame text
            if ((xFrameMarker - lastFrameX) > ImGui::CalcTextSize(frameInfo.name.c_str()).x)
            {
                pDrawList->AddText(pFont, smallFontSize, ImVec2(regionMin.x + float(xFrameMarker) + textPadding.x, regionMin.y + textPadding.y), 0xFFAAAAAA, frameInfo.name.c_str(), NULL, 0.0f, nullptr);
            }
            lastFrameX = xFrameMarker;
        }

        float y = regionMin.y + smallFontSize + textPadding.y;

        for (uint32_t threadIndex = 0; threadIndex < gFrameData[frameIndex].frameThreadCount; threadIndex++)
        {
            auto& frameThreadInfo = frameInfo.frameThreads[threadIndex];
            auto& threadData = gThreadData[frameThreadInfo.threadIndex];

            if (threadData.hidden)
            {
                continue;
            }

            float threadHeight = (heightPerLevel * threadData.maxLevel) + textPadding.y * 2.0f;

            assert(threadData.initialized);

            if (firstFrame)
            {
                pDrawList->AddRectFilled(ImVec2(regionMin.x, y), ImVec2(regionMax.x, y + threadHeight), gSelectedThread == threadIndex ? 0xFF333333 : 0xFF111111);
                pDrawList->AddLine(ImVec2(regionMin.x, y), ImVec2(regionMax.x, y), 0xFF333333);
            }

            y += textPadding.y;

            auto showEntry = [&](uint32_t index) {
                auto& entry = threadData.entries[index];

                // Ignore wholly outside our visible range
                if (entry.startTime > gTimeRange.y || entry.endTime < gTimeRange.x)
                {
                    return;
                }

                float yEntry = y + entry.level * heightPerLevel;
                float xEntry = float(xFromTime(entry.startTime));
                float xEnd = float(xFromTime(entry.endTime));

                // Avoid alliasing/make it easy to see small entries
                if (xEnd < (xEntry + 1))
                {
                    xEnd = xEntry + 1;
                }

                ImVec2 rectMin(std::max(xEntry + regionMin.x, regionMin.x), yEntry);
                ImVec2 rectMax(std::min(xEnd + regionMin.x, regionMax.x), yEntry + heightPerLevel);
                pDrawList->AddRectFilled(rectMin, rectMax, entry.color | 0xFF000000);

                if (ImGui::IsMouseHoveringRect(rectMin, rectMax))
                {
                    auto tip = fmt::format("{}: {:.4f}ms ({:.2f}us)\nRange: {:.4f}ms - {:.4f}ms\n\n{} (Ln {})",
                        entry.szSection,
                        timer_to_ms(std::min(entry.endTime, threadData.maxTime) - entry.startTime),
                        (std::min(entry.endTime, threadData.maxTime) - entry.startTime) / 1000.0f,
                        timer_to_ms(entry.startTime),
                        timer_to_ms(entry.endTime),
                        entry.szFile, entry.line);
                    ImGui::SetTooltip("%s", tip.c_str());
                }

                float width = rectMax.x - rectMin.x;
                auto clip = ImVec4(rectMin.x, rectMin.y, rectMax.x, rectMax.y);
                auto textSize = ImGui::CalcTextSize(entry.szSection);

                // Center the text if possible
                float textPos = textPadding.x + rectMin.x;
                if (textSize.x < width)
                {
                    textPos += (width - textSize.x) * .5f;
                }

                if (width > MinSizeForTextDisplay)
                {
                    pDrawList->AddText(pFont, fontSize, ImVec2(textPos, yEntry + textPadding.y), LuminanceARGB(entry.color) > .5f ? 0xFF000000 : 0xFFFFFFFF, entry.szSection, NULL, 0.0f, &clip);
                }
            };

            // Get the most recent thread entry for this frame
            auto currentEntry = frameThreadInfo.activeEntry;

            // Walk up the stack to find the outer parent; since it might have started before this frame
            while (threadData.entries[currentEntry].parent != 0xFFFFFFFF)
            {
                currentEntry = threadData.entries[currentEntry].parent;
            }

            // Step back to find entries that began before the frame
            while (currentEntry > 0 && threadData.entries[currentEntry].endTime > frameInfo.startTime)
            {
                currentEntry--;
            }

            while ((currentEntry < (threadData.currentEntry - 1)) && threadData.entries[currentEntry].startTime < frameInfo.endTime)
            {
                showEntry(currentEntry);
                currentEntry++;
            }

            if (firstFrame && mouseClick.y >= y && mouseClick.y <= (y + threadHeight))
            {
                if (gSelectedThread != threadIndex)
                {
                    gSelectedThread = threadIndex;
                }
                else
                {
                    gSelectedThread = -1;
                }
            }

            // Draw the text over the top
            gMaxThreadNameSize = std::max(ImGui::CalcTextSize(threadData.name.c_str()).x, gMaxThreadNameSize);
            pDrawList->AddRectFilled(ImVec2(regionMin.x, y + threadHeight - textPadding.y - smallFontSize), ImVec2(regionMin.x + gMaxThreadNameSize, y + threadHeight - textPadding.y), gSelectedThread == threadIndex ? 0xFF333333 : 0xFF111111);
            pDrawList->AddText(pFont, smallFontSize, ImVec2(regionMin.x + textPadding.x, y + threadHeight - smallFontSize - textPadding.y), 0xFFAAAAAA, threadData.name.c_str(), NULL, 0.0f, nullptr);

            // Next thread
            y += heightPerLevel * threadData.maxLevel + textPadding.y;
        }
        firstFrame = false;
    }

    ImGui::PopStyleVar(1);
    ImGui::End();
}

} // namespace Profiler
} // namespace MUtils
