//
// Based on https://github.com/arnaud-jamin/imgui/blob/master/profiler.cpp
//

#include <cassert>
#include <mutils/time/profiler.h>

#include "imgui.h"
#include "imgui_internal.h"

namespace MUtils
{

    /*
Profiler MUtils::profilerGlobalInstance;

static inline unsigned int MultiplyColor(unsigned int c, float intensity)
{
    float s = intensity / 255.0f;
    float r = s * (c & 0xFF);
    float g = s * ((c >> 8) & 0xFF);
    float b = s * ((c >> 16) & 0xFF);
    float a = s * (c >> 24);

    unsigned int out;
    out = ((unsigned int)(((r < 0.0f) ? 0.0f : (r > 1.0f) ? 1.0f : r) * 255.0f + 0.5f));
    out |= ((unsigned int)(((g < 0.0f) ? 0.0f : (g > 1.0f) ? 1.0f : g) * 255.0f + 0.5f)) << 8;
    out |= ((unsigned int)(((b < 0.0f) ? 0.0f : (b > 1.0f) ? 1.0f : b) * 255.0f + 0.5f)) << 16;
    out |= ((unsigned int)(((a < 0.0f) ? 0.0f : (a > 1.0f) ? 1.0f : a) * 255.0f + 0.5f)) << 24;

    return out;
}

static inline int PositiveModulo(int x, int m)
{
    return (x % m) < 0 ? (x % m) + m : x % m;
}
//static inline int       ImSign(int value)                       { return (value < 0 ? -1 : 1); }
//static inline int       ImSign(float value)                     { return (value < 0 ? -1 : 1); }
//static inline int       ImSign(double value)                    { return (value < 0 ? -1 : 1); }
static inline double ImMin(double lhs, double rhs)
{
    return lhs < rhs ? lhs : rhs;
}
static inline double ImMax(double lhs, double rhs)
{
    return lhs >= rhs ? lhs : rhs;
}
static inline double ImClamp(double v, double mn, double mx)
{
    return (v < mn) ? mn : (v > mx) ? mx : v;
}

void Profiler::Initialize(bool* _isPaused, void (*_setPause)(bool))
{
    m_isPausedPtr = _isPaused;
    m_setPause = _setPause;
    m_timeDuration = 120.0f;
    m_frameAreaMaxDuration = 1000.0f / 30.0f;
    m_frameIndex = -1;
    m_threadIndex = 0;

    m_frameSelectionStart = 2;
    m_frameSelectionEnd = 0;

    memset(m_threads, 0, sizeof(m_threads));

    for (int i = 0; i < MaxThreads; ++i)
    {
        auto& thread = m_threads[i];
        thread.initialized = i == 0;
        thread.sectionIndex = 0;
    }

    timer_start(gTimer);
}

void Profiler::Shutdown()
{
}

void Profiler::NewFrame()
{
    if (IsPaused())
        return;

    float time = timer_to_ms(timer_get_elapsed(gTimer));

    if (m_frameCount > 0)
    {
        auto& previousFrame = m_frames[m_frameIndex];
        previousFrame.endTime = time;
    }

    m_frameIndex = (m_frameIndex + 1) % MaxFrames;
    m_frameCount = ImMin(m_frameCount + 1, MaxFrames);

    auto& frame = m_frames[m_frameIndex];
    frame.index = m_frameIndex;
    frame.startTime = time;
    frame.endTime = -1;
}

void Profiler::InitThreadInternal()
{
    LockCriticalSection2();

    IM_ASSERT(m_threadIndex == -1 && "Thread is already initialized!");

    for (int i = 0; i < MaxThreads; i++)
    {
        auto& thread = m_threads[m_threadIndex];
        if (thread.initialized == false)
        {
            m_threadIndex = i;
            thread.initialized = true;
            thread.activeSectionIndex = -1;
            UnLockCriticalSection2();
            return;
        }
    }

    IM_ASSERT(false && "Every thread slots are used!");
}

void Profiler::FinishThreadInternal()
{
    LockCriticalSection2();
    IM_ASSERT(m_threadIndex > -1 && "Trying to finish an uninitilized thread.");
    auto& thread = m_threads[m_threadIndex];
    thread.initialized = false;
    m_threadIndex = -1;
    UnLockCriticalSection2();
}

void Profiler::PushSectionInternal(const char* name, unsigned int color, const char* fileName, int line)
{
    if (IsPaused())
        return;

    auto& thread = m_threads[m_threadIndex];
    auto& section = thread.sections[thread.sectionIndex];

    if (color == 0x00000000)
    {
        if (thread.callStackDepth > 0 && thread.activeSectionIndex != -1)
        {
            section.color = MultiplyColor(thread.sections[thread.activeSectionIndex].color, 0.8f);
        }
        else
        {
            section.color = Blue;
        }
    }
    else
    {
        section.color = color;
    }

    section.name = name;
    section.fileName = fileName;
    section.line = line;

    section.startTime = timer_to_ms(timer_get_elapsed(gTimer));
    section.endTime = -1;
    section.callStackDepth = thread.callStackDepth;
    section.parentSectionIndex = thread.activeSectionIndex;

    thread.activeSectionIndex = thread.sectionIndex;
    thread.sectionIndex = (thread.sectionIndex + 1) % MaxSections;
    thread.sectionsCount = ImMin(thread.sectionsCount + 1, MaxSections);

    thread.callStackDepth++;
}

void Profiler::PopSectionInternal()
{
    if (IsPaused())
        return;

    auto& thread = m_threads[m_threadIndex];
    thread.callStackDepth--;

    auto& section = thread.sections[thread.activeSectionIndex];
    section.endTime = timer_to_ms(timer_get_elapsed(gTimer));
    thread.activeSectionIndex = section.parentSectionIndex;
}

int ScreenPositionToFrameOffset(float screenPosX, float framesAreaPosMax, float frameWidth, float frameSpacing, int maxFrames)
{
    return ImClamp((int)ceilf((framesAreaPosMax - screenPosX - frameWidth - frameSpacing * 0.5f) / (frameWidth + frameSpacing)), 0, maxFrames - 1);
}

int FrameOffsetToFrameIndex(int currentFrameIndex, int frameIndexOffset, int maxFrames)
{
    return PositiveModulo(currentFrameIndex - frameIndexOffset, maxFrames);
}

struct ThreadInfo
{
    double minTime;
    double maxTime;
    int maxCallStackDepth;
};

void Profiler::DrawUI()
{
    ProfileScopedSection(Profiler UI);

    bool showBorders = false;

    //int normalFontIndex = 0;
    int smallFontIndex = 0;

    // Controls
    float controlsHeight = ImGui::GetTextLineHeight();
    float controlsWidth = 80;
    unsigned int controlsBackColor = 0x88000000;

    // Frames
    float frameAreaHeight = 120;
    unsigned int frameAreaBackColor = 0x60000000;
    unsigned int frameRectColor = 0xAAFFFFFF;
    unsigned int frameNoInfoRectColor = 0x77FFFFFF;
    unsigned int frameSectionWindowColor = 0x55FF8866;
    float frameWidth = 6.0f;
    float frameSpacing = 2.0f;

    // Frame max duration text
    ImVec2 framesMaxDurationTextOffset(-4, 8);
    ImVec2 framesMaxDurationPadding(4, 2);

    // Vertical Slider
    float frameAreaSliderWidth = 0;
    float frameAreaSliderSpacing = 0;
    //unsigned int frameAreaSliderGrabColor = 0x99FFFFFF;
    //unsigned int frameAreaSliderBackColor = 0x22FFFFFF;

    // Timeline
    float timelineHeight = 60;
    unsigned int timlineFrameMarkerColor = 0x22FFFFFF;
    float timelineFrameDurationSpacing = 10;
    unsigned int separatorColor = 0xAA000000;

    // Threads
    float threadTitleWidth = 320;
    float threadTitleHeight = 40.0f;
    float threadSpacing = 20.0f;
    unsigned int threadTitleBackColor = 0x60000000;
    unsigned int threadBackColor = 0x40000000;

    // Sections
    float sectionHeight = 40;
    ImVec2 sectionSpacing = ImVec2(2, 2);
    ImVec2 sectionTextPadding = ImVec2(6, 6);
    float sectionMinWidthForText = 40;
    float sectionWheelZoomSpeed = 0.2f;
    float sectionDragZoomSpeed = 0.01f;
    unsigned int sectionTextColor = 0x90FFFFFF;

    // Interaction
    int selectionButton = 0;
    int panningButton = 0;
    int zoomingButton = 1;

    //-------------------------------------------------------------------------
    // Window
    //-------------------------------------------------------------------------
    char buffer[255];
    auto& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
    ImGui::Begin("Profiler", &m_isWindowOpen);

    //-------------------------------------------------------------------------
    // Controls
    //-------------------------------------------------------------------------
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 0));
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[smallFontIndex]);

        ImVec2 controlsAreaSize(ImGui::GetContentRegionAvailWidth(), controlsHeight);
        ImVec2 controlsAreaMin(ImGui::GetCursorScreenPos());
        ImVec2 controlsAreaMax(controlsAreaMin.x + controlsAreaSize.x, controlsAreaMin.y + controlsAreaSize.y);
        ImGui::BeginChild("Controls", controlsAreaSize, showBorders);
        ImGui::GetWindowDrawList()->AddRectFilled(controlsAreaMin, controlsAreaMax, controlsBackColor);

        ImGui::SameLine();
        if (ImGui::Button(IsPaused() ? "Resume" : "Pause", ImVec2(controlsWidth, controlsHeight)))
        {
            SetPause(!IsPaused());
        }

        ImGui::SameLine();
        ImFormatString(buffer, IM_ARRAYSIZE(buffer), "%.1f ms", m_frameAreaMaxDuration);
        if (ImGui::Button(buffer, ImVec2(controlsWidth, controlsHeight)))
        {
            ImGui::OpenPopup("FamesMaxDurationPopup");
        }

        if (ImGui::BeginPopup("FamesMaxDurationPopup"))
        {
            if (ImGui::Selectable("30 FPS"))
            {
                m_frameAreaMaxDuration = 1000.0f / 30.f;
            }
            ImGui::Separator();
            if (ImGui::Selectable("60 FPS"))
            {
                m_frameAreaMaxDuration = 1000.0f / 60.f;
            }
            ImGui::Separator();
            if (ImGui::Selectable("120 FPS"))
            {
                m_frameAreaMaxDuration = 1000.0f / 120.0f;
            }
            ImGui::EndPopup();
        }

        ImGui::EndChild();
        ImGui::PopFont();
        ImGui::PopStyleVar();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    //-------------------------------------------------------------------------
    // Compute min max timings
    //-------------------------------------------------------------------------
    double recordsMinTime = FLT_MAX;
    double recordsMaxTime = -FLT_MAX;
    double minSectionDuration = FLT_MAX;

    ThreadInfo threadInfos[MaxThreads];
    for (int i = 0; i < MaxThreads; i++)
    {
        auto& thread = m_threads[i];
        auto& threadInfo = threadInfos[i];

        threadInfo.minTime = FLT_MAX;
        threadInfo.maxTime = -FLT_MAX;
        threadInfo.maxCallStackDepth = 0;

        if (thread.initialized == false)
            continue;

        for (int j = 0; j < thread.sectionsCount; ++j)
        {
            auto& section = thread.sections[j];
            if (section.endTime < 0)
                continue;

            threadInfo.minTime = ImMin(threadInfo.minTime, section.startTime);
            threadInfo.maxTime = ImMax(threadInfo.maxTime, section.endTime);
            threadInfo.maxCallStackDepth = ImMax(threadInfo.maxCallStackDepth, section.callStackDepth);

            minSectionDuration = ImMin(minSectionDuration, section.endTime - section.startTime);
            recordsMinTime = ImMin(threadInfo.minTime, recordsMinTime);
            recordsMaxTime = ImMax(threadInfo.maxTime, recordsMaxTime);
        }
    }
    double recordsDuration = recordsMaxTime - recordsMinTime;

    //-------------------------------------------------------------------------
    // Move timings if unpaused
    //-------------------------------------------------------------------------
    if (IsPaused() == false)
    {
        auto& startFrame = m_frames[FrameOffsetToFrameIndex(m_frameIndex, m_frameSelectionStart, m_frameCount)];
        auto& endFrame = m_frames[FrameOffsetToFrameIndex(m_frameIndex, m_frameSelectionEnd, m_frameCount)];

        double endTime = endFrame.endTime == -1 ? recordsMaxTime : endFrame.endTime;
        m_timeOffset = recordsMaxTime - endTime;
        m_timeDuration = endTime - startFrame.startTime;
    }

    //-------------------------------------------------------------------------
    // Frames Area
    //-------------------------------------------------------------------------
    ImVec2 framesAreaSize(ImGui::GetContentRegionAvailWidth() - frameAreaSliderWidth - frameAreaSliderSpacing, frameAreaHeight);
    ImGui::BeginChild("Frames", framesAreaSize, showBorders);
    ImVec2 framesAreaMin(ImGui::GetCursorScreenPos());
    ImVec2 framesAreaMax(framesAreaMin.x + framesAreaSize.x, framesAreaMin.y + framesAreaSize.y);

    //-------------------------------------------------------------------------
    // Frames sections selection
    //-------------------------------------------------------------------------
    ImGui::InvisibleButton("##FramesSectionsWindowDummy", framesAreaSize);
    if (ImGui::IsItemActive())
    {
        if (ImGui::IsMouseClicked(selectionButton))
        {
            int frameIndexOffset = ScreenPositionToFrameOffset(ImGui::GetIO().MouseClickedPos[selectionButton].x, framesAreaMax.x, frameWidth, frameSpacing, MaxFrames);
            int frameIndex = FrameOffsetToFrameIndex(m_frameIndex, frameIndexOffset, MaxFrames);
            auto& frame = m_frames[frameIndex];
            if (frame.startTime > 0)
            {
                double endTime = frame.endTime == -1 ? recordsMaxTime : frame.endTime;
                m_timeOffset = recordsMaxTime - endTime;
                m_timeDuration = endTime - frame.startTime;
                m_frameSelectionStart = frameIndexOffset;
                m_frameSelectionEnd = frameIndexOffset;
            }
        }
        else if (ImGui::IsMouseDragging(selectionButton))
        {
            float mousePos = ImGui::GetIO().MouseClickedPos[selectionButton].x;
            int startDragFrameIndexOffset = ScreenPositionToFrameOffset(mousePos, framesAreaMax.x, frameWidth, frameSpacing, MaxFrames);
            int startDragFrameIndex = FrameOffsetToFrameIndex(m_frameIndex, startDragFrameIndexOffset, MaxFrames);
            auto& startDragFrame = m_frames[startDragFrameIndex];

            int endDragFrameIndexOffset = ScreenPositionToFrameOffset(mousePos + ImGui::GetMouseDragDelta(selectionButton).x, framesAreaMax.x, frameWidth, frameSpacing, MaxFrames);
            int endDragFrameIndex = FrameOffsetToFrameIndex(m_frameIndex, endDragFrameIndexOffset, MaxFrames);
            auto& endDragFrame = m_frames[endDragFrameIndex];

            if (startDragFrame.startTime > 0 && endDragFrame.startTime > 0)
            {
                Frame* startFrame;
                Frame* endFrame;

                if (startDragFrame.startTime < endDragFrame.startTime)
                {
                    m_frameSelectionStart = startDragFrameIndexOffset;
                    m_frameSelectionEnd = endDragFrameIndexOffset;
                    startFrame = &startDragFrame;
                    endFrame = &endDragFrame;
                }
                else
                {
                    m_frameSelectionStart = endDragFrameIndexOffset;
                    m_frameSelectionEnd = startDragFrameIndexOffset;
                    startFrame = &endDragFrame;
                    endFrame = &startDragFrame;
                }

                double endTime = endFrame->endTime == -1 ? recordsMaxTime : endFrame->endTime;
                m_timeOffset = recordsMaxTime - endTime;
                m_timeDuration = endTime - startFrame->startTime;

                if (IsPaused() == false)
                {
                    //SetPause(m_frameSelectionEnd != 0);
                }
            }
        }
    }
    ImGui::EndChild();

    //-------------------------------------------------------------------------
    // Vertical slider for fames max duration
    //-------------------------------------------------------------------------
    //{
    //    ImGui::SameLine();
    //    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + frameAreaSliderSpacing);
    //    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
    //    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    //    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(frameAreaSliderBackColor));
    //    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImColor(frameAreaSliderBackColor));
    //    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImColor(frameAreaSliderBackColor));
    //    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor(frameAreaSliderGrabColor));
    //    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImColor(frameAreaSliderGrabColor));
    //    ImGui::VSliderFloat("##frameMaxDuration", ImVec2(frameAreaSliderWidth, frameAreaHeight), &m_frameAreaMaxDuration, 1.0f, 1000.0f / 30.0f, "");
    //    ImGui::PopStyleColor(5);
    //    ImGui::PopStyleVar(2);
    //}

    //-------------------------------------------------------------------------
    // Store interaction button
    //-------------------------------------------------------------------------
    ImVec2 interactionAreaSize(ImGui::GetContentRegionAvailWidth() - threadTitleWidth, ImGui::GetWindowHeight() - ImGui::GetCursorPosY() - 20);
    ImVec2 interactionArea = ImGui::GetCursorScreenPos();
    ImVec2 interactionAreaMin(interactionArea.x + threadTitleWidth, interactionArea.y);
    ImVec2 interactionAreaMax(interactionAreaMin.x + interactionAreaSize.x, interactionAreaMin.y + interactionAreaSize.y);
    //ImGui::GetWindowDrawList()->AddRectFilled(interactionAreaMin, interactionAreaMax, Blue);

    //-------------------------------------------------------------------------
    // Timeline
    //-------------------------------------------------------------------------
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + threadTitleWidth, ImGui::GetCursorPosY()));
    ImVec2 timelineAreaSize(ImGui::GetContentRegionAvailWidth(), timelineHeight);
    ImGui::BeginChild("Timeline", timelineAreaSize, showBorders, ImGuiWindowFlags_NoMove);
    ImVec2 timelineAreaMin(ImGui::GetCursorScreenPos());
    ImVec2 timelineAreaMax(timelineAreaMin.x + timelineAreaSize.x, timelineAreaMin.y + timelineAreaSize.y);
    ImGui::EndChild();

    //-------------------------------------------------------------------------
    // Threads
    //-------------------------------------------------------------------------
    for (int threadIndex = 0; threadIndex < MaxThreads; ++threadIndex)
    {
        auto& thread = m_threads[threadIndex];
        auto& threadInfo = threadInfos[threadIndex];

        ImGui::PushID(threadIndex);

        ImFormatString(buffer, IM_ARRAYSIZE(buffer), "Thread %d", threadIndex);
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + (threadIndex == 0 ? 0 : threadSpacing)));
        ImVec2 threadStartScreenPos = ImGui::GetCursorScreenPos();

        //---------------------------------------------------------------------
        // Threads Infos
        //---------------------------------------------------------------------
        ImGui::SetCursorPosX(style.WindowPadding.x);
        ImGui::BeginChild(ImGui::GetID("Infos"), ImVec2(threadTitleWidth, threadTitleHeight), showBorders);

        int callStackDepth = 0;
        if (ImGui::TreeNode(buffer, buffer))
        {
            callStackDepth = threadInfo.maxCallStackDepth;
            ImGui::TreePop();
        }
        ImGui::EndChild();

        float threadHeight = (callStackDepth + 1) * (sectionHeight + sectionSpacing.y);

        //---------------------------------------------------------------------
        // Threads Title Background
        //---------------------------------------------------------------------
        ImVec2 backgroundMin = threadStartScreenPos;
        ImVec2 backgroundMax(backgroundMin.x + threadTitleWidth, backgroundMin.y + threadHeight);
        ImGui::GetWindowDrawList()->AddRectFilled(backgroundMin, backgroundMax, threadTitleBackColor);

        //---------------------------------------------------------------------
        // Threads Background
        //---------------------------------------------------------------------
        backgroundMin.x += threadTitleWidth;
        backgroundMin.y += threadTitleWidth;
        backgroundMax.x = backgroundMin.x + ImGui::GetContentRegionAvailWidth() - threadTitleWidth;

        //---------------------------------------------------------------------
        // Sections area
        //---------------------------------------------------------------------
        ImGui::SameLine();
        ImVec2 sectionsAreaSize(ImGui::GetContentRegionAvailWidth(), threadHeight);
        ImGui::BeginChild(ImGui::GetID("Sections"), sectionsAreaSize, showBorders, ImGuiWindowFlags_NoMove);
        ImVec2 sectionsAreaMin(ImGui::GetCursorScreenPos());
        ImVec2 sectionsAreaMax(sectionsAreaMin.x + sectionsAreaSize.x, sectionsAreaMin.y + sectionsAreaSize.y);
        ImGui::GetWindowDrawList()->AddRectFilled(sectionsAreaMin, sectionsAreaMax, threadBackColor);

        //---------------------------------------------------------------------
        // Sections
        //---------------------------------------------------------------------
        for (int sectionIndex = 0; sectionIndex < thread.sectionsCount; ++sectionIndex)
        {
            auto& section = thread.sections[sectionIndex];

            if (section.callStackDepth > callStackDepth)
                continue;

            double endTime = section.endTime;
            if (endTime < 0)
            {
                endTime = threadInfo.maxTime;
            }

            if (endTime < recordsMaxTime - m_timeOffset - m_timeDuration || (section.startTime > recordsMaxTime - m_timeOffset))
                continue;

            float sectionWidth = (float)ImMax(1.0, ((endTime - section.startTime) * sectionsAreaSize.x / m_timeDuration) - sectionSpacing.x);
            float sectionX = (float)(sectionsAreaMin.x + (section.startTime - (recordsMaxTime - m_timeOffset - m_timeDuration)) * (sectionsAreaSize.x / m_timeDuration));
            float sectionY = sectionsAreaMin.y + ((sectionHeight + sectionSpacing.y) * section.callStackDepth);

            ImVec2 rectMin(sectionX, sectionY);
            ImVec2 rectMax(rectMin.x + sectionWidth, rectMin.y + sectionHeight);
            ImVec4 rectClip(rectMin.x, rectMin.y, rectMax.x, rectMax.y);
            ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, section.color);

            if (sectionWidth > sectionMinWidthForText)
            {
                ImVec2 textPos(rectMin.x + sectionTextPadding.x, rectMin.y + sectionTextPadding.y);
                ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, sectionTextColor, section.name, NULL, 0.0f, &rectClip);
            }

            //-----------------------------------------------------------------
            // Tooltip
            //-----------------------------------------------------------------
            if (ImGui::IsMouseHoveringRect(rectMin, rectMax))
            {
                ImGui::SetTooltip("%s (%5.3f ms)\n%s(%d)\n",
                    section.name, endTime - section.startTime,
                    section.fileName, section.line);
            }
        }

        ImGui::EndChild();
        ImGui::PopID();
    }

    //-------------------------------------------------------------------------
    // Interactions on the section area
    //-------------------------------------------------------------------------
    {
        ImGui::SetCursorScreenPos(interactionAreaMin);
        ImGui::InvisibleButton("", interactionAreaSize);

        //---------------------------------------------------------------------
        // Panning
        //---------------------------------------------------------------------
        if ((ImGui::IsItemActive() || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsMouseHoveringRect(interactionAreaMin, interactionAreaMax))) && ImGui::IsMouseDragging(panningButton))
        {
            double offset = (ImGui::GetIO().MouseDelta.x * m_timeDuration / interactionAreaSize.x);
            m_timeOffset = ImClamp(m_timeOffset + offset, (double)0, (double)recordsDuration);
            RefreshFrameSelection(recordsMaxTime);
            SetPause(true);
        }

        //-------------------------------------------------------------------------
        // Zooming with mouse drag
        //-------------------------------------------------------------------------
        if (m_sectionAreaDurationWhenZoomStarted == 0 && ImGui::IsMouseHoveringRect(interactionAreaMin, interactionAreaMax) && ImGui::IsMouseDragging(zoomingButton))
        {
            m_sectionAreaDurationWhenZoomStarted = m_timeDuration;
            //SetPause(true);
        }

        if (m_sectionAreaDurationWhenZoomStarted > 0 && ImGui::IsMouseReleased(zoomingButton))
        {
            m_sectionAreaDurationWhenZoomStarted = 0;
        }

        if (m_sectionAreaDurationWhenZoomStarted > 0)
        {
            double oldTime = recordsMaxTime - m_timeOffset;
            double localMousePos = ImGui::GetIO().MouseClickedPos[zoomingButton].x - interactionAreaMin.x;
            double oldTimeOfMouse = (oldTime - m_timeDuration) + (localMousePos * m_timeDuration / interactionAreaSize.x);
            double amount = ImGui::GetMouseDragDelta(zoomingButton).x;
            double zoom = 1 - ImSign(amount) * sectionDragZoomSpeed;
            m_timeDuration = ImClamp(m_sectionAreaDurationWhenZoomStarted * pow(zoom, fabs(amount)), minSectionDuration, recordsDuration);
            double newTimeOfMouse = (oldTime - m_timeDuration) + (localMousePos * m_timeDuration / interactionAreaSize.x);
            double newTime = ImClamp(oldTime + (oldTimeOfMouse - newTimeOfMouse), recordsMinTime, recordsMaxTime);
            m_timeOffset = recordsMaxTime - newTime;
            RefreshFrameSelection(recordsMaxTime);
        }

        //-------------------------------------------------------------------------
        // Zooming with mouse wheel
        //-------------------------------------------------------------------------
        if (ImGui::IsMouseHoveringRect(interactionAreaMin, interactionAreaMax) && ImGui::GetIO().MouseWheel != 0)
        {
            double oldTime = recordsMaxTime - m_timeOffset;
            double localMousePos = ImGui::GetMousePos().x - interactionAreaMin.x;
            double oldTimeOfMouse = (oldTime - m_timeDuration) + (localMousePos * m_timeDuration / interactionAreaSize.x);
            double zoom = 1 - ImSign(ImGui::GetIO().MouseWheel) * sectionWheelZoomSpeed;
            m_timeDuration = ImClamp(m_timeDuration * zoom, minSectionDuration, recordsDuration);
            if (IsPaused())
            {
                double newTimeOfMouse = (oldTime - m_timeDuration) + (localMousePos * m_timeDuration / interactionAreaSize.x);
                double newTime = ImClamp(oldTime + (oldTimeOfMouse - newTimeOfMouse), recordsMinTime, recordsMaxTime);
                m_timeOffset = recordsMaxTime - newTime;
            }
            RefreshFrameSelection(recordsMaxTime);
        }
    }

    //-------------------------------------------------------------------------
    // Separator between thread title and sections
    //-------------------------------------------------------------------------
    ImGui::GetWindowDrawList()->AddLine(timelineAreaMin, ImVec2(timelineAreaMin.x, timelineAreaMin.y + ImGui::GetWindowHeight()), separatorColor, 1.0f);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(0, timelineAreaMin.y), ImVec2(timelineAreaMax.x + ImGui::GetContentRegionAvailWidth(), timelineAreaMin.y), separatorColor, 1.0f);

    //-------------------------------------------------------------------------
    // Frames and timeline rendering
    //-------------------------------------------------------------------------
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[smallFontIndex]);

        ImGui::GetWindowDrawList()->AddRectFilled(framesAreaMin, framesAreaMax, frameAreaBackColor);

        // Draw all the frame candles
        for (int i = 0; i < m_frameCount; ++i)
        {
            int frameIndex = PositiveModulo(m_frameIndex - i, m_frameCount);
            auto& frame = m_frames[frameIndex];

            if (frame.startTime <= 0)
                continue;

            double endTime = frame.endTime == -1 ? recordsMaxTime : frame.endTime;
            double frameDuration = endTime - frame.startTime;

            //-----------------------------------------------------------------
            // One frame inside the frame area
            //-----------------------------------------------------------------
            ImVec2 rectMin, rectMax;
            rectMin.x = framesAreaMax.x - i * (frameWidth + frameSpacing) - frameWidth;
            rectMax.x = rectMin.x + frameWidth;

            if (rectMax.x > framesAreaMin.x && rectMin.x < framesAreaMax.x)
            {
                float frameHeight = ImMin((float)(framesAreaSize.y * frameDuration / m_frameAreaMaxDuration), framesAreaSize.y);
                rectMin.y = framesAreaMax.y - frameHeight;
                rectMax.y = framesAreaMax.y;

                unsigned int color = (frame.startTime >= recordsMinTime) ? frameRectColor : frameNoInfoRectColor;
                ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, color);
            }

            //-----------------------------------------------------------------
            // Frame marker on the timeline and the sections area
            //-----------------------------------------------------------------
            if (frame.startTime >= recordsMaxTime - m_timeOffset - m_timeDuration && (frame.startTime <= recordsMaxTime - m_timeOffset))
            {
                float frameX = (float)(timelineAreaMin.x + (frame.startTime - (recordsMaxTime - m_timeOffset - m_timeDuration)) * (timelineAreaSize.x / m_timeDuration));
                ImGui::GetWindowDrawList()->AddLine(ImVec2(frameX, timelineAreaMin.y), ImVec2(frameX, timelineAreaMin.y + ImGui::GetWindowHeight()), timlineFrameMarkerColor, 1.0f);
            }

            //-----------------------------------------------------------------
            // Frame duration inside the timeline
            //-----------------------------------------------------------------
            if (endTime >= recordsMaxTime - m_timeOffset - m_timeDuration && (endTime <= recordsMaxTime - m_timeOffset))
            {
                ImFormatString(buffer, IM_ARRAYSIZE(buffer), "%5.2f ms", frameDuration);
                ImVec2 textSize = ImGui::CalcTextSize(buffer);

                //float timelineFrameWidth = (float)(timelineAreaSize.x * (endTime - frame.startTime) / m_timeDuration);

                if (textSize.x < frameWidth)
                {
                    ImVec2 textPos;
                    textPos.x = (float)(timelineAreaMin.x - timelineFrameDurationSpacing - textSize.x + (endTime - (recordsMaxTime - m_timeOffset - m_timeDuration)) * (timelineAreaSize.x / m_timeDuration));
                    textPos.y = timelineAreaMin.y + (timelineAreaSize.y - textSize.y) * 0.5f;
                    ImVec4 rectClip(timelineAreaMin.x, timelineAreaMin.y, timelineAreaMax.x, timelineAreaMax.y);
                    ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, sectionTextColor, buffer, 0, 0, &rectClip);
                }
            }
        }
        ImGui::PopFont();
    }

    //-------------------------------------------------------------------------
    // Frames Sections Window
    //-------------------------------------------------------------------------
    {
        ImVec2 rectMin, rectMax;
        rectMin.x = framesAreaMax.x - ((m_frameSelectionStart + 1) * (frameWidth + frameSpacing)) + frameSpacing;
        rectMin.y = framesAreaMin.y;
        rectMax.x = framesAreaMax.x - ((m_frameSelectionEnd) * (frameWidth + frameSpacing));
        rectMax.y = framesAreaMax.y;

        ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, frameSectionWindowColor);
    }

    ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing
    ImGui::End();
    ImGui::PopStyleVar(); // ImGuiStyleVar_FrameRounding
    ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
}

void Profiler::RefreshFrameSelection(double recordsMaxTime)
{
    int sectionAreaStartIndex = -1;
    int sectionAreaEndIndex = -1;

    for (int i = 0; i < m_frameCount; ++i)
    {
        int frameIndex = PositiveModulo(m_frameIndex - i, m_frameCount);
        auto& frame = m_frames[frameIndex];

        if (frame.startTime <= 0)
            continue;

        if (sectionAreaStartIndex == -1 && frame.startTime < recordsMaxTime - m_timeOffset - m_timeDuration)
        {
            sectionAreaStartIndex = i;
        }

        if (sectionAreaEndIndex == -1 && frame.startTime < recordsMaxTime - m_timeOffset)
        {
            sectionAreaEndIndex = i;
        }

        if (sectionAreaStartIndex != -1 && sectionAreaEndIndex != -1)
            break;
    }

    m_frameSelectionStart = sectionAreaStartIndex;
    m_frameSelectionEnd = sectionAreaEndIndex;
}
*/

namespace YAP
{

MUtils::timer gTimer;
int gPhaseCount = 0;
int gPhaseIndex = 0;
int gMaxThreads = 0;
int gMaxEntries = 0;
int gMaxPhases = 0;
bool gPaused = false;
bool gProfilerPaused = false;
thread_local int gTLSThreadIndex = -1;
ProfilerEntry* gProfilerEntries = NULL;
PhaseEntry* gPhaseEntries = NULL;
ThreadEntry* gThreadEntries = NULL;
void (*_freeMemFunction)(void* _memory);
std::mutex gMutex;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Profiler work
void Init(int maxPhases, int maxThreads, int maxEntries, int maxCallStack, void* (*allocMem)(size_t size), void (*freeMem)(void* _memory))
{
    assert(gTLSThreadIndex == -1 && "Main Thread is already initialized!");

    gPhaseEntries = (PhaseEntry*)allocMem(maxPhases * sizeof(PhaseEntry));
    gThreadEntries = (ThreadEntry*)allocMem(maxPhases * 2 * maxThreads * sizeof(ThreadEntry));

    int allocatedEntrySize = maxPhases * 2 * maxThreads * maxEntries * sizeof(ProfilerEntry);
    gProfilerEntries = (ProfilerEntry*)allocMem(allocatedEntrySize);

    for (int iZero = 0; iZero < maxPhases * 2 * maxThreads; iZero++)
    {
        ThreadEntry* threadEntry = &gThreadEntries[iZero];
        threadEntry->initialized = iZero == 0;
        threadEntry->entriesCount = 0;
        threadEntry->callStackDepth = 0;
        threadEntry->maxLevel = 0;
        threadEntry->minTime = std::numeric_limits<uint64_t>::max();
        threadEntry->maxTime = 0.0;

        threadEntry->entriesStack = (int*)allocMem(maxCallStack * sizeof(int));
        threadEntry->colorStack = (unsigned int*)allocMem(maxCallStack * sizeof(unsigned int));
        for (int iColor = 0; iColor < maxCallStack; iColor++)
            threadEntry->colorStack[iColor] = 0x80808080;
    }

    gMaxThreads = maxThreads;
    gMaxEntries = maxEntries;
    _freeMemFunction = freeMem;
    gMaxPhases = maxPhases;
    gPhaseCount = 0;
    gPhaseIndex = -1;
    gTLSThreadIndex = 0;
    gThreadEntries[0].initialized = true;
    timer_start(gTimer);
}

void InitThread()
{
    std::unique_lock<std::mutex> lk(gMutex);

    assert(gTLSThreadIndex == -1 && "Thread is already initialized!");

    for (int iThread = 0; iThread < gMaxThreads; iThread++)
    {
        ThreadEntry* threadEntry = &gThreadEntries[iThread];
        if (!threadEntry->initialized)
        {
            // use it
            gTLSThreadIndex = iThread;
            threadEntry->initialized = true;
            return;
        }
    }
    assert(false && "Every thread slots are used!");
}

void FinishThread()
{
    std::unique_lock<std::mutex> lk(gMutex);
    assert(gTLSThreadIndex > -1 && "Trying to finish an uninitilized thread.");
    gThreadEntries[gTLSThreadIndex].initialized = false;
    gTLSThreadIndex = -1;
}

void Finish()
{
    for (int iFree = 0; iFree < gMaxThreads * 2; iFree++)
    {
        _freeMemFunction(gThreadEntries[iFree].entriesStack);
        _freeMemFunction(gThreadEntries[iFree].colorStack);
    }

    _freeMemFunction(gProfilerEntries);
    _freeMemFunction(gPhaseEntries);
    _freeMemFunction(gThreadEntries);

    gProfilerEntries = NULL;
    gPhaseEntries = NULL;
    gThreadEntries = NULL;
}

void PushPhaseBase(const char* szPhaseName)
{
    if (gPaused)
        return;
    for (int iPhase = 0; iPhase < gPhaseCount; iPhase++)
    {
        if (!strcmp(gPhaseEntries[iPhase].szPhaseName, szPhaseName))
        {
            gPhaseIndex = iPhase;
            return;
        }
    }
    // not found
    gPhaseIndex = gPhaseCount;
    gPhaseCount++;
    assert(gPhaseCount <= gMaxPhases && "Maximum simulteanous phase count reached!");
    // todo: bound check!
    // add for both frames
    PhaseEntry* phase = &gPhaseEntries[gPhaseIndex];
    phase->szPhaseName = szPhaseName;
    phase->frameWrite = 0;
}

ThreadEntry* GetWrittingThread()
{
    if (gTLSThreadIndex == -1)
    {
        InitThread();
    }
    int frameWrite = gPhaseEntries[gPhaseIndex].frameWrite;
    int threadIndex = (gPhaseIndex * gMaxThreads * 2) + (frameWrite * gMaxThreads) + gTLSThreadIndex;
    ThreadEntry* threadEntry(&gThreadEntries[threadIndex]);
    return threadEntry;
}

ThreadEntry* GetReadingThread(int phaseIndex, int threadIndex)
{
    int frameReadIndex = (gPhaseEntries[phaseIndex].frameWrite + 1) & 1;
    int thread = (phaseIndex * gMaxThreads * 2) + (frameReadIndex * gMaxThreads) + threadIndex;
    ThreadEntry* threadEntry(&gThreadEntries[thread]);
    return threadEntry;
}

void PushSectionBase(const char* szSection, unsigned int color, const char* szFile, int line)
{
    if (gPaused)
        return;

    int frameWrite = gPhaseEntries[gPhaseIndex].frameWrite;
    ThreadEntry* threadEntry(GetWrittingThread());

    int entryIndex = gPhaseIndex * 2 * gMaxThreads * gMaxEntries;
    entryIndex += frameWrite * gMaxThreads * gMaxEntries;
    entryIndex += gTLSThreadIndex * gMaxEntries;
    entryIndex += threadEntry->entriesCount;

    assert(threadEntry->entriesCount < gMaxEntries && "Max profile entries reached. Did you call NewFrame? If so, increase the max entries count at init");

    ProfilerEntry* profilerEntry(&gProfilerEntries[entryIndex]);

    threadEntry->entriesStack[threadEntry->callStackDepth] = entryIndex;
    if (color == 0xFFFFFFFF)
        color = threadEntry->colorStack[threadEntry->callStackDepth];

    assert(szFile != NULL && "No file string specified");
    assert(szSection != NULL && "No section name specified");
    profilerEntry->color = color;
    profilerEntry->szFile = szFile;
    profilerEntry->szSection = szSection;
    profilerEntry->line = line;
    profilerEntry->endTime = profilerEntry->startTime = timer_get_time_now();

    if (++threadEntry->callStackDepth >= threadEntry->maxLevel)
        threadEntry->maxLevel = threadEntry->callStackDepth;

    threadEntry->colorStack[threadEntry->callStackDepth] = color;

    profilerEntry->level = threadEntry->callStackDepth;

    threadEntry->entriesCount++;

    if (profilerEntry->startTime < threadEntry->minTime)
        threadEntry->minTime = profilerEntry->startTime;
    if (profilerEntry->startTime > threadEntry->maxTime)
        threadEntry->maxTime = profilerEntry->startTime;
}

void PopSection()
{
    if (gPaused)
        return;

    ThreadEntry* threadEntry(GetWrittingThread());

    int entryIndex = threadEntry->entriesStack[--threadEntry->callStackDepth];
    if (entryIndex < 0)
        return; // not enough stack to pop

    ProfilerEntry* profilerEntry(&gProfilerEntries[entryIndex]);

    profilerEntry->endTime = timer_get_time_now();

    if (profilerEntry->endTime > threadEntry->maxTime)
        threadEntry->maxTime = profilerEntry->endTime;
}

void NewFrame()
{
    gPaused = gProfilerPaused;
    if (gPaused)
        return;
    ++gPhaseEntries[gPhaseIndex].frameWrite &= 1;
    ThreadEntry* threadEntry(GetWrittingThread());
    threadEntry->entriesCount = 0;
    threadEntry->maxLevel = 0;
    threadEntry->callStackDepth = 0;
    threadEntry->minTime = std::numeric_limits<uint64_t>::max();
    threadEntry->maxTime = 0.0;
    threadEntry->initialized = true;
}

void PopPhase()
{
    if (gPaused)
        return;

    int& rw = gPhaseEntries[gPhaseIndex].frameWrite;
    ++rw &= 1;
}


ThreadEntry* GetThreadEntry(int phaseIndex, int threadIndex)
{
    ThreadEntry* threadEntry(GetReadingThread(phaseIndex, threadIndex));
    return threadEntry;
}

int GetEntryCount(int phaseIndex, int threadIndex)
{
    return GetThreadEntry(phaseIndex, threadIndex)->entriesCount;
}

ProfilerEntry* GetEntries(int phaseIndex, int threadIndex)
{
    assert(phaseIndex < gPhaseCount && "No phase for this index");
    assert(threadIndex < gMaxThreads && "No thread for this index");

    int frameWrite = gPhaseEntries[phaseIndex].frameWrite;
    int frameReadIndex = (frameWrite + 1) & 1;
    int firstEntryIndex = phaseIndex * 2 * gMaxThreads * gMaxEntries;
    firstEntryIndex += frameReadIndex * gMaxThreads * gMaxEntries;
    firstEntryIndex += threadIndex * gMaxEntries;

    ProfilerEntry* profilerEntry(&gProfilerEntries[firstEntryIndex]);
    return profilerEntry;
}

ProfilerEntry* GetEntry(int phaseIndex, int threadIndex, int entryIndex)
{
    return &GetEntries(phaseIndex, threadIndex)[entryIndex];
}

void GetPhaseNames(char* destination, int maxLen)
{
    int av(0);

    for (int iPhase = 0; iPhase < gPhaseCount; iPhase++)
    {
        const char* szPhaseName = gPhaseEntries[iPhase].szPhaseName;
        int phaseNameLen = strlen(szPhaseName);
        for (int iLen = 0; iLen <= phaseNameLen; iLen++)
        {
            destination[av++] = szPhaseName[iLen];
            if (av == maxLen - 2)
            {
                destination[av] = 0;
                destination[av + 1] = 0;
                return;
            }
        }
    }
    destination[av++] = 0;
}

const char* GetPhaseName(int phaseIndex)
{
    if (phaseIndex < 0 || phaseIndex >= gPhaseCount)
        return NULL;
    return gPhaseEntries[phaseIndex].szPhaseName;
}

void LogDump(int (*Log)(const char* szFormat, ...))
{
    if (!Log)
        return;
    for (int iPhase = 0; iPhase < YAP::gPhaseCount; iPhase++)
    {
        Log("Phase %s\n", YAP::GetPhaseName(iPhase));
        for (int iThread = 0; iThread < YAP::gMaxThreads; iThread++)
        {
            Log("Thread %d :\n\n", iThread);

            int entriesCount = YAP::GetEntryCount(iPhase, iThread);
            for (int iEntry = 0; iEntry < entriesCount; iEntry++)
            {
                YAP::ProfilerEntry* entry = YAP::GetEntry(iPhase, iThread, iEntry);

                int iLevel = (entry->level >= 3) ? 3 : entry->level;
                static const char* indents[] = { "  ", "    ", "      ", "        " };
                Log("%s %s - %s(%d) %5.3f ms( %5.3f ms to %5.3f ms )\n",
                    indents[iLevel],
                    entry->szSection,
                    entry->szFile,
                    entry->line,
                    (float)timer_to_ms(entry->endTime - entry->startTime),
                    (float)timer_to_ms(entry->startTime),
                    (float)timer_to_ms(entry->endTime));
            }
            Log("---------------------------------------------------------------\n\n");
        }
    }
}

inline ImVec4 ImGuiColor(unsigned int v)
{
    ImVec4 color;
    color.w = (float)((v & 0xFF000000) >> 24) * (1.f / 255.f);
    color.z = (float)((v & 0xFF0000) >> 16) * (1.f / 255.f);
    color.y = (float)((v & 0xFF00) >> 8) * (1.f / 255.f);
    color.x = (float)((v & 0xFF)) * (1.f / 255.f);
    return color;
}

void ImGuiLogger(bool* opened)
{
    if (!ImGui::Begin("Profiler", opened))
    {
        ImGui::End();
        return;
    }

    static int currentPhaseIndex = 1;
    char tmps[1024];
    YAP::GetPhaseNames(tmps, 1024);
    ImGui::PushItemWidth(200);
    ImGui::Combo("Phase", &currentPhaseIndex, tmps);

    ImGui::SameLine();
    static int timeRange = 0;
    //ImGui::PushItemWidth(-100);
    ImGui::Combo("Time range", &timeRange, "Fit All\0 30Hz\0 60Hz\0 120Hz\0\0");
    ImGui::SameLine();

    if (ImGui::Button("Copy output to clipboard"))
    {
        ImGui::LogToClipboard();
        YAP::LogDump((int (*)(const char* szFormat, ...)) & ImGui::LogText);
        ImGui::LogFinish();
    }
    ImGui::SameLine();
    if (ImGui::Button(gProfilerPaused ? "Resume" : "Pause"))
        gProfilerPaused = !gProfilerPaused;

    ImGui::PopItemWidth();

    // get min/max
    uint64_t threadMinTime = std::numeric_limits<uint64_t>::max();
    uint64_t threadMaxTime = 0;
    for (int iThread = 0; iThread < YAP::gMaxThreads; iThread++)
    {
        YAP::ThreadEntry* te = YAP::GetThreadEntry(currentPhaseIndex, iThread);
        if (!te->initialized)
            continue;

        threadMinTime = (threadMinTime < te->minTime) ? threadMinTime : te->minTime;
        threadMaxTime = (threadMaxTime > te->maxTime) ? threadMaxTime : te->maxTime;
    }

    auto buttonHeight = ImGui::GetTextLineHeightWithSpacing();
    // GUI
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1)); // Tighten spacing
    for (int iThread = 0; iThread < YAP::gMaxThreads; iThread++)
    {
        ImGui::Text("Thread %d", iThread);
        ImGui::Separator();

        YAP::ThreadEntry* te = YAP::GetThreadEntry(currentPhaseIndex, iThread);
        if (!te->initialized)
            continue;

        float width = ImGui::GetWindowWidth() - 32;
        float scale(width / ((timeRange == 0) ? (float)(threadMaxTime - threadMinTime) : (33.333f / (float)timeRange)));

        for (int iLevel = 0; iLevel <= te->maxLevel; iLevel++)
        {
            int entriesCount = YAP::GetEntryCount(currentPhaseIndex, iThread);
            if (!entriesCount)
                continue;

            // count entries in level
            int entriesInLevelCount(0);
            for (int iEntry = 0; iEntry < entriesCount; iEntry++) // reversed order
            {
                YAP::ProfilerEntry* entry = YAP::GetEntry(currentPhaseIndex, iThread, iEntry);
                if (entry->level == iLevel)
                    entriesInLevelCount++;
            }
            if (!entriesInLevelCount)
                continue;

            uint64_t currentTime = threadMinTime;
            ImGui::BeginGroup();
            for (int iEntry = 0; iEntry < entriesCount; iEntry++) // reversed order
            {
                YAP::ProfilerEntry* entry = YAP::GetEntry(currentPhaseIndex, iThread, iEntry);

                if (entry->level != iLevel)
                    continue;

                float preTime = (float)timer_to_ms(entry->startTime - currentTime);
                float invisibleButWidth = preTime * scale;
                if (invisibleButWidth >= 1.f)
                {
                    ImGui::InvisibleButton("", ImVec2(invisibleButWidth, buttonHeight));
                    ImGui::SameLine();
                }

                float duration = (float)timer_to_ms(entry->endTime - entry->startTime);
                ImVec4 buttonColor(ImGuiColor(entry->color + 0xFF000000));
                ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonColor);
                float butWidth = duration * scale;
                if (butWidth >= 1.f)
                {
                    ImGui::Button(entry->szSection, ImVec2(butWidth, buttonHeight));
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s\n%s(%d)\nStart %5.3f ms\nEnd %5.3f ms\nDuration %5.3f ms",
                            entry->szSection,
                            entry->szFile,
                            entry->line,
                            (float)timer_to_ms(entry->startTime - threadMinTime),
                            (float)timer_to_ms(entry->endTime - threadMinTime),
                            (float)timer_to_ms(entry->endTime - entry->startTime));
                    }
                }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                currentTime = entry->endTime;
            }
            ImGui::EndGroup();
        }
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

} // namespace YAPL

} // namespace MUtils
