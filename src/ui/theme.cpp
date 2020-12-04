#include <mutils/string/murmur_hash.h>
#include <mutils/ui/theme.h>

namespace MUtils
{

namespace Theme
{

ThemeManager& ThemeManager::Instance()
{
    static ThemeManager theme;
    return theme;
}

ThemeManager::ThemeManager()
{
    double golden_ratio_conjugate = 0.618033988749895;
    double h = .15f;
    for (int i = 0; i < (int)ThemeColor::UniqueColorLast; i++)
    {
        h += golden_ratio_conjugate;
        h = std::fmod(h, 1.0);
        m_uniqueColors.emplace_back(HSVToRGB(float(h) * 360.0f, 0.5f, 200.0f));
    }
    SetThemeType(ThemeType::Dark);
}

void ThemeManager::SetThemeType(ThemeType type)
{
    m_currentTheme = type;
    switch (type)
    {
    default:
    case ThemeType::Dark:
        SetDarkTheme();
        break;
    case ThemeType::Light:
        SetLightTheme();
        break;
    }
}

ThemeType ThemeManager::GetThemeType() const
{
    return m_currentTheme;
}

void ThemeManager::SetDarkTheme()
{
    m_colors[ThemeColor::Text] = NVec4f(1.0f);
    m_colors[ThemeColor::TextDim] = NVec4f(.45f, .45f, .45f, 1.0f);
    m_colors[ThemeColor::Background] = NVec4f(0.11f, 0.11f, 0.11f, 1.0f);
    m_colors[ThemeColor::HiddenText] = NVec4f(.9f, .1f, .1f, 1.0f);
    m_colors[ThemeColor::TabBorder] = NVec4f(.55f, .55f, .55f, 1.0f);
    m_colors[ThemeColor::TabInactive] = NVec4f(.4f, .4f, .4f, .55f);
    m_colors[ThemeColor::TabActive] = NVec4f(.65f, .65f, .65f, 1.0f);
    m_colors[ThemeColor::LineNumberBackground] = m_colors[ThemeColor::Background] + NVec4f(.02f, .02f, .02f, 0.0f);
    m_colors[ThemeColor::LineNumber] = NVec4f(.13f, 1.0f, .13f, 1.0f);
    m_colors[ThemeColor::LineNumberActive] = NVec4f(.13f, 1.0f, .13f, 1.0f);
    m_colors[ThemeColor::CursorNormal] = NVec4f(130.0f / 255.0f, 140.0f / 255.0f, 230.0f / 255.0f, 1.0f);
    m_colors[ThemeColor::CursorInsert] = NVec4f(1.0f, 1.0f, 1.0f, .9f);
    m_colors[ThemeColor::CursorLineBackground] = NVec4f(.25f, .25f, .25f, 1.0f);
    m_colors[ThemeColor::AirlineBackground] = NVec4f(.20f, .20f, .20f, 1.0f);
    m_colors[ThemeColor::Light] = NVec4f(1.0f);
    m_colors[ThemeColor::Dark] = NVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    m_colors[ThemeColor::VisualSelectBackground] = NVec4f(.47f, 0.30f, 0.25f, 1.0f);
    m_colors[ThemeColor::Mode] = NVec4f(.2f, 0.8f, 0.2f, 1.0f);

    m_colors[ThemeColor::Normal] = m_colors[ThemeColor::Text];
    m_colors[ThemeColor::Parenthesis] = m_colors[ThemeColor::Text];
    m_colors[ThemeColor::Comment] = NVec4f(0.0f, 1.0f, .1f, 1.0f);
    m_colors[ThemeColor::Keyword] = NVec4f(0.1f, 1.0f, 1.0f, 1.0f);
    m_colors[ThemeColor::Identifier] = NVec4f(1.0f, .75f, 0.5f, 1.0f);
    m_colors[ThemeColor::Number] = NVec4f(1.0f, 1.0f, 0.1f, 1.0f);
    m_colors[ThemeColor::String] = NVec4f(1.0f, 0.5f, 1.0f, 1.0f);
    m_colors[ThemeColor::Whitespace] = NVec4f(0.3f, .3f, .3f, 1.0f);

    m_colors[ThemeColor::Error] = NVec4f(0.65f, .2f, .15f, 1.0f);
    m_colors[ThemeColor::Warning] = NVec4f(0.15f, .2f, .65f, 1.0f);
    m_colors[ThemeColor::Info] = NVec4f(0.15f, .6f, .15f, 1.0f);

    m_colors[ThemeColor::WidgetBorder] = NVec4f(.5f, .5f, .5f, 1.0f);
    m_colors[ThemeColor::WidgetActive] = m_colors[ThemeColor::TabActive];
    m_colors[ThemeColor::WidgetInactive] = m_colors[ThemeColor::TabInactive];
    m_colors[ThemeColor::WidgetBackground] = NVec4f(.2f, .2f, .2f, 1.0f);
    m_colors[ThemeColor::FlashColor] = NVec4f(.80f, .40f, .05f, 1.0f);

    m_colors[ThemeColor::AccentColor1] = NVec4f(.90f, .58f, .03f, 1.0f);
    m_colors[ThemeColor::AccentColor2] = NVec4f(.90f, .45f, .02f, 1.0f);
    m_colors[ThemeColor::AccentColor3] = NVec4f(.90f, .30f, .02f, 1.0f);
    m_colors[ThemeColor::AccentColor4] = NVec4f(.80f, .35f, .02f, 1.0f);

    m_colors[ThemeColor::OppositeAccentColor1] = NVec4f(.03f, .58f, .90f, 1.0f);
    m_colors[ThemeColor::OppositeAccentColor2] = NVec4f(.02f, .45f, .90f, 1.0f);
    m_colors[ThemeColor::OppositeAccentColor3] = NVec4f(.02f, .30f, .90f, 1.0f);
    m_colors[ThemeColor::OppositeAccentColor4] = NVec4f(.02f, .35f, .80f, 1.0f);
}

void ThemeManager::SetLightTheme()
{
    m_colors[ThemeColor::Text] = NVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    m_colors[ThemeColor::TextDim] = NVec4f(0.55f, 0.55f, 0.55f, 1.0f);
    m_colors[ThemeColor::Background] = NVec4f(1.0f, 1.0f, 1.0f, 1.0f);
    m_colors[ThemeColor::HiddenText] = NVec4f(.9f, .1f, .1f, 1.0f);
    m_colors[ThemeColor::TabBorder] = NVec4f(.55f, .55f, .55f, 1.0f);
    m_colors[ThemeColor::TabInactive] = NVec4f(.4f, .4f, .4f, .55f);
    m_colors[ThemeColor::TabActive] = NVec4f(.55f, .55f, .55f, 1.0f);
    m_colors[ThemeColor::LineNumberBackground] = m_colors[ThemeColor::Background] - NVec4f(.02f, .02f, .02f, 0.0f);
    m_colors[ThemeColor::LineNumber] = NVec4f(.13f, .4f, .13f, 1.0f);
    m_colors[ThemeColor::LineNumberActive] = NVec4f(.13f, 0.6f, .13f, 1.0f);
    m_colors[ThemeColor::CursorNormal] = NVec4f(130.0f / 255.0f, 140.0f / 255.0f, 230.0f / 255.0f, 1.0f);
    m_colors[ThemeColor::CursorInsert] = NVec4f(1.0f, 1.0f, 1.0f, .9f);
    m_colors[ThemeColor::CursorLineBackground] = NVec4f(.85f, .85f, .85f, 1.0f);
    m_colors[ThemeColor::AirlineBackground] = NVec4f(.80f, .80f, .80f, 1.0f);
    m_colors[ThemeColor::Light] = NVec4f(1.0f);
    m_colors[ThemeColor::Dark] = NVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    m_colors[ThemeColor::VisualSelectBackground] = NVec4f(.49f, 0.60f, 0.45f, 1.0f);
    m_colors[ThemeColor::Mode] = NVec4f(.2f, 0.8f, 0.2f, 1.0f);

    m_colors[ThemeColor::Normal] = m_colors[ThemeColor::Text];
    m_colors[ThemeColor::Parenthesis] = m_colors[ThemeColor::Text];
    m_colors[ThemeColor::Comment] = NVec4f(0.1f, .4f, .1f, 1.0f);
    m_colors[ThemeColor::Keyword] = NVec4f(0.1f, .2f, .3f, 1.0f);
    m_colors[ThemeColor::Identifier] = NVec4f(0.2f, .2f, .1f, 1.0f);
    m_colors[ThemeColor::Number] = NVec4f(0.1f, .3f, .2f, 1.0f);
    m_colors[ThemeColor::String] = NVec4f(0.1f, .1f, .4f, 1.0f);
    m_colors[ThemeColor::Whitespace] = NVec4f(0.2f, .2f, .2f, 1.0f);

    m_colors[ThemeColor::Error] = NVec4f(0.89f, .2f, .15f, 1.0f);
    m_colors[ThemeColor::Warning] = NVec4f(0.15f, .2f, .89f, 1.0f);
    m_colors[ThemeColor::Info] = NVec4f(0.15f, .85f, .15f, 1.0f);

    m_colors[ThemeColor::WidgetActive] = m_colors[ThemeColor::TabActive];
    m_colors[ThemeColor::WidgetInactive] = m_colors[ThemeColor::TabInactive];

    m_colors[ThemeColor::WidgetBorder] = NVec4f(.5f, .5f, .5f, 1.0f);
    m_colors[ThemeColor::WidgetActive] = m_colors[ThemeColor::TabActive];
    m_colors[ThemeColor::WidgetInactive] = m_colors[ThemeColor::TabInactive];
    m_colors[ThemeColor::WidgetBackground] = NVec4f(.8f, .8f, .8f, 1.0f);

    m_colors[ThemeColor::FlashColor] = NVec4f(0.8f, .4f, .05f, 1.0f);

    m_colors[ThemeColor::AccentColor1] = NVec4f(.80f, .50f, .02f, 1.0f);
    m_colors[ThemeColor::AccentColor2] = NVec4f(.80f, .45f, .02f, 1.0f);
    m_colors[ThemeColor::AccentColor3] = NVec4f(.80f, .40f, .02f, 1.0f);
    m_colors[ThemeColor::AccentColor4] = NVec4f(.80f, .35f, .02f, 1.0f);
    
    m_colors[ThemeColor::OppositeAccentColor1] = NVec4f(.03f, .58f, .90f, 1.0f);
    m_colors[ThemeColor::OppositeAccentColor2] = NVec4f(.02f, .45f, .90f, 1.0f);
    m_colors[ThemeColor::OppositeAccentColor3] = NVec4f(.02f, .30f, .90f, 1.0f);
    m_colors[ThemeColor::OppositeAccentColor4] = NVec4f(.02f, .35f, .80f, 1.0f);
}

ThemeColor ThemeManager::GetUniqueColor(uint32_t index) const
{
    return ThemeColor((uint32_t)ThemeColor::UniqueColor0 + (uint32_t)(index % (uint32_t)ThemeColor::UniqueColorLast));
}

const NVec4f& ThemeManager::ColorFromName(const char* pszName, const uint32_t len)
{
    const auto col = murmur_hash(pszName, len, 0);
    return ThemeManager::Instance().GetColor(ThemeManager::Instance().GetUniqueColor(col));
}

const NVec4f& ThemeManager::GetColor(ThemeColor themeColor) const
{
    if (themeColor >= ThemeColor::UniqueColor0)
    {
        // Return the unique color
        return m_uniqueColors[((uint32_t)themeColor - (uint32_t)ThemeColor::UniqueColor0) % (uint32_t)ThemeColor::UniqueColorLast];
    }

    auto itr = m_colors.find(themeColor);
    if (itr == m_colors.end())
    {
        static const NVec4f one(1.0f);
        return one;
    }
    return itr->second;
}

NVec4f ThemeManager::GetComplement(const NVec4f& col, const NVec4f& adjust) const
{
    auto lum = Luminosity(col);
    if (lum > 0.5f)
        return GetColor(ThemeColor::Dark) + adjust;
    return GetColor(ThemeColor::Light) - adjust;
}

} // namespace Theme
} // namespace MUtils
