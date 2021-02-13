#define DECLARE_THEME_COLOR

#include <mutils/string/murmur_hash.h>
#include <mutils/ui/theme.h>

namespace MUtils
{

namespace Theme
{

namespace
{
std::vector<StringId> UniqueColors{
    color_UniqueColor0,
    color_UniqueColor1,
    color_UniqueColor2,
    color_UniqueColor3,
    color_UniqueColor4,
    color_UniqueColor5,
    color_UniqueColor6,
    color_UniqueColor7,
    color_UniqueColor8,
    color_UniqueColor9,
    color_UniqueColor10,
    color_UniqueColor11,
    color_UniqueColor12,
    color_UniqueColor13,
    color_UniqueColor14,
    color_UniqueColor15,
};
}

ThemeManager& ThemeManager::Instance()
{
    static ThemeManager theme;
    return theme;
}

ThemeManager::ThemeManager()
{
    SetCurrentTheme(DARK_THEME);
}

void ThemeManager::AddDefaultColors()
{
    double golden_ratio_conjugate = 0.618033988749895;
    double h = .15f;
    for (int i = 0; i < int(UniqueColors.size()); i++)
    {
        h += golden_ratio_conjugate;
        h = std::fmod(h, 1.0);
        Set(UniqueColors[i], HSVToRGB(float(h) * 360.0f, 0.5f, 200.0f));
    }
}

void ThemeManager::SetDarkTheme()
{
    AddDefaultColors();
    Set(color_Text, NVec4f(1.0f));
    Set(color_TextDim, NVec4f(.45f, .45f, .45f, 1.0f));
    Set(color_Background, NVec4f(0.11f, 0.11f, 0.11f, 1.0f));
    Set(color_HiddenText, NVec4f(.9f, .1f, .1f, 1.0f));
    Set(color_TabBorder, NVec4f(.55f, .55f, .55f, 1.0f));
    Set(color_TabInactive, NVec4f(.4f, .4f, .4f, .55f));
    Set(color_TabActive, NVec4f(.65f, .65f, .65f, 1.0f));
    Set(color_LineNumberBackground, Get(color_Background) + NVec4f(.02f, .02f, .02f, 0.0f));
    Set(color_LineNumber, NVec4f(.13f, 1.0f, .13f, 1.0f));
    Set(color_LineNumberActive, NVec4f(.13f, 1.0f, .13f, 1.0f));
    Set(color_CursorNormal, NVec4f(130.0f / 255.0f, 140.0f / 255.0f, 230.0f / 255.0f, 1.0f));
    Set(color_CursorInsert, NVec4f(1.0f, 1.0f, 1.0f, .9f));
    Set(color_CursorLineBackground, NVec4f(.25f, .25f, .25f, 1.0f));
    Set(color_AirlineBackground, NVec4f(.20f, .20f, .20f, 1.0f));
    Set(color_Light, NVec4f(1.0f));
    Set(color_Dark, NVec4f(0.0f, 0.0f, 0.0f, 1.0f));
    Set(color_VisualSelectBackground, NVec4f(.47f, 0.30f, 0.25f, 1.0f));
    Set(color_Mode, NVec4f(.2f, 0.8f, 0.2f, 1.0f));

    Set(color_Normal, Get(color_Text));
    Set(color_Parenthesis, Get(color_Text));
    Set(color_Comment, NVec4f(0.0f, 1.0f, .1f, 1.0f));
    Set(color_Keyword, NVec4f(0.1f, 1.0f, 1.0f, 1.0f));
    Set(color_Identifier, NVec4f(1.0f, .75f, 0.5f, 1.0f));
    Set(color_Number, NVec4f(1.0f, 1.0f, 0.1f, 1.0f));
    Set(color_String, NVec4f(1.0f, 0.5f, 1.0f, 1.0f));
    Set(color_Whitespace, NVec4f(0.3f, .3f, .3f, 1.0f));

    Set(color_Error, NVec4f(0.65f, .2f, .15f, 1.0f));
    Set(color_Warning, NVec4f(0.15f, .2f, .65f, 1.0f));
    Set(color_Info, NVec4f(0.15f, .6f, .15f, 1.0f));

    Set(color_WidgetBorder, NVec4f(.5f, .5f, .5f, 1.0f));
    Set(color_WidgetActive, Get(color_TabActive));
    Set(color_WidgetInactive, Get(color_TabInactive));
    Set(color_WidgetBackground, NVec4f(.2f, .2f, .2f, 1.0f));
    Set(color_FlashColor, NVec4f(.80f, .40f, .05f, 1.0f));

    Set(color_AccentColor1, NVec4f(.90f, .58f, .03f, 1.0f));
    Set(color_AccentColor2, NVec4f(.90f, .45f, .02f, 1.0f));
    Set(color_AccentColor3, NVec4f(.90f, .30f, .02f, 1.0f));
    Set(color_AccentColor4, NVec4f(.80f, .35f, .02f, 1.0f));

    Set(color_OppositeAccentColor1, NVec4f(.03f, .58f, .90f, 1.0f));
    Set(color_OppositeAccentColor2, NVec4f(.02f, .45f, .90f, 1.0f));
    Set(color_OppositeAccentColor3, NVec4f(.02f, .30f, .90f, 1.0f));
    Set(color_OppositeAccentColor4, NVec4f(.02f, .35f, .80f, 1.0f));
}

void ThemeManager::SetLightTheme()
{
    AddDefaultColors();
    Set(color_Text, NVec4f(0.0f, 0.0f, 0.0f, 1.0f));
    Set(color_TextDim, NVec4f(0.55f, 0.55f, 0.55f, 1.0f));
    Set(color_Background, NVec4f(1.0f, 1.0f, 1.0f, 1.0f));
    Set(color_HiddenText, NVec4f(.9f, .1f, .1f, 1.0f));
    Set(color_TabBorder, NVec4f(.55f, .55f, .55f, 1.0f));
    Set(color_TabInactive, NVec4f(.4f, .4f, .4f, .55f));
    Set(color_TabActive, NVec4f(.55f, .55f, .55f, 1.0f));
    Set(color_LineNumberBackground, Get(color_Background) - NVec4f(.02f, .02f, .02f, 0.0f));
    Set(color_LineNumber, NVec4f(.13f, .4f, .13f, 1.0f));
    Set(color_LineNumberActive, NVec4f(.13f, 0.6f, .13f, 1.0f));
    Set(color_CursorNormal, NVec4f(130.0f / 255.0f, 140.0f / 255.0f, 230.0f / 255.0f, 1.0f));
    Set(color_CursorInsert, NVec4f(1.0f, 1.0f, 1.0f, .9f));
    Set(color_CursorLineBackground, NVec4f(.85f, .85f, .85f, 1.0f));
    Set(color_AirlineBackground, NVec4f(.80f, .80f, .80f, 1.0f));
    Set(color_Light, NVec4f(1.0f));
    Set(color_Dark, NVec4f(0.0f, 0.0f, 0.0f, 1.0f));
    Set(color_VisualSelectBackground, NVec4f(.49f, 0.60f, 0.45f, 1.0f));
    Set(color_Mode, NVec4f(.2f, 0.8f, 0.2f, 1.0f));

    Set(color_Normal, Get(color_Text));
    Set(color_Parenthesis, Get(color_Text));
    Set(color_Comment, NVec4f(0.1f, .4f, .1f, 1.0f));
    Set(color_Keyword, NVec4f(0.1f, .2f, .3f, 1.0f));
    Set(color_Identifier, NVec4f(0.2f, .2f, .1f, 1.0f));
    Set(color_Number, NVec4f(0.1f, .3f, .2f, 1.0f));
    Set(color_String, NVec4f(0.1f, .1f, .4f, 1.0f));
    Set(color_Whitespace, NVec4f(0.2f, .2f, .2f, 1.0f));

    Set(color_Error, NVec4f(0.89f, .2f, .15f, 1.0f));
    Set(color_Warning, NVec4f(0.15f, .2f, .89f, 1.0f));
    Set(color_Info, NVec4f(0.15f, .85f, .15f, 1.0f));

    Set(color_WidgetActive, Get(color_TabActive));
    Set(color_WidgetInactive, Get(color_TabInactive));

    Set(color_WidgetBorder, NVec4f(.5f, .5f, .5f, 1.0f));
    Set(color_WidgetActive, Get(color_TabActive));
    Set(color_WidgetInactive, Get(color_TabInactive));
    Set(color_WidgetBackground, NVec4f(.8f, .8f, .8f, 1.0f));

    Set(color_FlashColor, NVec4f(0.8f, .4f, .05f, 1.0f));

    Set(color_AccentColor1, NVec4f(.80f, .50f, .02f, 1.0f));
    Set(color_AccentColor2, NVec4f(.80f, .45f, .02f, 1.0f));
    Set(color_AccentColor3, NVec4f(.80f, .40f, .02f, 1.0f));
    Set(color_AccentColor4, NVec4f(.80f, .35f, .02f, 1.0f));

    Set(color_OppositeAccentColor1, NVec4f(.03f, .58f, .90f, 1.0f));
    Set(color_OppositeAccentColor2, NVec4f(.02f, .45f, .90f, 1.0f));
    Set(color_OppositeAccentColor3, NVec4f(.02f, .30f, .90f, 1.0f));
    Set(color_OppositeAccentColor4, NVec4f(.02f, .35f, .80f, 1.0f));
}

const StringId& ThemeManager::GetUniqueColor(uint32_t index) const
{
    return UniqueColors[(uint32_t)(index % UniqueColors.size())];
}

const NVec4f& ThemeManager::ColorFromName(const char* pszName, const uint32_t len)
{
    const auto col = murmur_hash(pszName, len, 0);
    return ThemeManager::Instance().Get(ThemeManager::Instance().GetUniqueColor(col));
}

NVec4f ThemeManager::GetComplement(const NVec4f& col, const NVec4f& adjust)
{
    auto lum = Luminosity(col);
    if (lum > 0.5f)
        return Get(color_Dark) + adjust;
    return Get(color_Light) - adjust;
}

} // namespace Theme
} // namespace MUtils
