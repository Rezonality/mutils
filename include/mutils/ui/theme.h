#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include <mutils/math/math.h>

namespace MUtils
{

namespace Theme
{

enum class ThemeColor
{
    None,
    TabBorder,
    HiddenText,
    Text,
    TextDim,
    Background,
    TabInactive,
    TabActive,
    LineNumberBackground,
    LineNumber,
    LineNumberActive,
    CursorNormal,
    CursorInsert,
    Light,
    Dark,
    VisualSelectBackground,
    CursorLineBackground,
    AirlineBackground,
    Mode,
    Normal,
    Keyword,
    Identifier,
    Number,
    String,
    Comment,
    Whitespace,
    HiddenChar,
    Parenthesis,
    Error,
    Warning,
    Info,
    WidgetBorder,
    WidgetBackground,
    WidgetActive,
    WidgetInactive,
    FlashColor,
    AccentColor1,
    AccentColor2,
    AccentColor3,
    AccentColor4,
    OppositeAccentColor1,
    OppositeAccentColor2,
    OppositeAccentColor3,
    OppositeAccentColor4,
    Custom,

    // Last in the list!
    // A set of pregenerated, easy to differentiate unique colors
    UniqueColor0,
    UniqueColor1,
    UniqueColor2,
    UniqueColor3,
    UniqueColor4,
    UniqueColor5,
    UniqueColor6,
    UniqueColor7,
    UniqueColor8,
    UniqueColor9,
    UniqueColor10,
    UniqueColor11,
    UniqueColor12,
    UniqueColor13,
    UniqueColor14,
    UniqueColor15,
    UniqueColorLast,
};

enum class ThemeType
{
    Dark,
    Light
};

class ThemeManager
{
public:
    ThemeManager();
    virtual ~ThemeManager() {}

    virtual const NVec4f& GetColor(ThemeColor themeColor) const;
    virtual NVec4f GetComplement(const NVec4f& col, const NVec4f& adjust = NVec4f(0.0f)) const;
    virtual ThemeColor GetUniqueColor(uint32_t id) const;

    void SetThemeType(ThemeType type);
    ThemeType GetThemeType() const;

    static ThemeManager& Instance();
    static const NVec4f& ColorFromName(const char* pszName, const uint32_t len);

private:
    void SetDarkTheme();
    void SetLightTheme();

private:
    std::vector<NVec4f> m_uniqueColors;
    std::map<ThemeColor, NVec4f> m_colors;
    ThemeType m_currentTheme = ThemeType::Dark;
};

} // namespace Theme

} // namespace MUtils
