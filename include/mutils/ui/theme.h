#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include <mutils/math/math.h>
#include <mutils/string/string_utils.h>

namespace MUtils
{

namespace Theme
{

#ifdef DECLARE_THEME_COLOR
#define DECLARE_THEME_COLOR(name) MUtils::StringId color_##name(#name);
#else
#define DECLARE_THEME_COLOR(name) extern MUtils::StringId color_##name;
#endif

DECLARE_THEME_COLOR(None)
DECLARE_THEME_COLOR(TabBorder)
DECLARE_THEME_COLOR(HiddenText)
DECLARE_THEME_COLOR(Text)
DECLARE_THEME_COLOR(TextDim)
DECLARE_THEME_COLOR(Background)
DECLARE_THEME_COLOR(TabInactive)
DECLARE_THEME_COLOR(TabActive)
DECLARE_THEME_COLOR(LineNumberBackground)
DECLARE_THEME_COLOR(LineNumber)
DECLARE_THEME_COLOR(LineNumberActive)
DECLARE_THEME_COLOR(CursorNormal)
DECLARE_THEME_COLOR(CursorInsert)
DECLARE_THEME_COLOR(Light)
DECLARE_THEME_COLOR(Dark)
DECLARE_THEME_COLOR(VisualSelectBackground)
DECLARE_THEME_COLOR(CursorLineBackground)
DECLARE_THEME_COLOR(AirlineBackground)
DECLARE_THEME_COLOR(Mode)
DECLARE_THEME_COLOR(Normal)
DECLARE_THEME_COLOR(Keyword)
DECLARE_THEME_COLOR(Identifier)
DECLARE_THEME_COLOR(Number)
DECLARE_THEME_COLOR(String)
DECLARE_THEME_COLOR(Comment)
DECLARE_THEME_COLOR(Whitespace)
DECLARE_THEME_COLOR(HiddenChar)
DECLARE_THEME_COLOR(Parenthesis)
DECLARE_THEME_COLOR(Error)
DECLARE_THEME_COLOR(Warning)
DECLARE_THEME_COLOR(Info)
DECLARE_THEME_COLOR(WidgetBorder)
DECLARE_THEME_COLOR(WidgetBackground)
DECLARE_THEME_COLOR(WidgetActive)
DECLARE_THEME_COLOR(WidgetInactive)
DECLARE_THEME_COLOR(FlashColor)
DECLARE_THEME_COLOR(AccentColor1)
DECLARE_THEME_COLOR(AccentColor2)
DECLARE_THEME_COLOR(AccentColor3)
DECLARE_THEME_COLOR(AccentColor4)
DECLARE_THEME_COLOR(OppositeAccentColor1)
DECLARE_THEME_COLOR(OppositeAccentColor2)
DECLARE_THEME_COLOR(OppositeAccentColor3)
DECLARE_THEME_COLOR(OppositeAccentColor4)
DECLARE_THEME_COLOR(Custom)

DECLARE_THEME_COLOR(UniqueColor0)
DECLARE_THEME_COLOR(UniqueColor1)
DECLARE_THEME_COLOR(UniqueColor2)
DECLARE_THEME_COLOR(UniqueColor3)
DECLARE_THEME_COLOR(UniqueColor4)
DECLARE_THEME_COLOR(UniqueColor5)
DECLARE_THEME_COLOR(UniqueColor6)
DECLARE_THEME_COLOR(UniqueColor7)
DECLARE_THEME_COLOR(UniqueColor8)
DECLARE_THEME_COLOR(UniqueColor9)
DECLARE_THEME_COLOR(UniqueColor10)
DECLARE_THEME_COLOR(UniqueColor11)
DECLARE_THEME_COLOR(UniqueColor12)
DECLARE_THEME_COLOR(UniqueColor13)
DECLARE_THEME_COLOR(UniqueColor14)
DECLARE_THEME_COLOR(UniqueColor15)

#define DARK_THEME "dark"
#define LIGHT_THEME "light"

class ThemeManager
{
public:
    ThemeManager();
    virtual ~ThemeManager() {}

    virtual NVec4f GetComplement(const NVec4f& col, const NVec4f& adjust = NVec4f(0.0f));

    void Set(const StringId& id, const NVec4f& color)
    {
        m_themeColors[m_currentTheme][id.id] = color;
    }

    const NVec4f& Get(const StringId& id)
    {
        return m_themeColors[m_currentTheme][id.id];
    }

    const StringId& GetUniqueColor(uint32_t index) const;

    void SetCurrentTheme(std::string themeName)
    {
        m_currentTheme = themeName;
        if (themeName == DARK_THEME)
        {
            SetDarkTheme();
        }
        else if (themeName == LIGHT_THEME)
        {
            SetLightTheme();
        }
    }

    std::string GetCurrentTheme() const
    {
        return m_currentTheme;
    }

    static ThemeManager& Instance();
    static const NVec4f& ColorFromName(const char* pszName, const uint32_t len);

private:
    void AddDefaultColors();
    void SetDarkTheme();
    void SetLightTheme();

private:
    std::vector<NVec4f> m_uniqueColors;
    std::string m_currentTheme;

    using ThemeColors = std::map<StringId, NVec4f>;
    std::map<std::string, ThemeColors> m_themeColors;
};

} // namespace Theme
} // namespace MUtils
