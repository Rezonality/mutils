#pragma once

#include <mutils/string/murmur_hash.h>
#include <mutils/string/string_utils.h>

namespace MUtils
{

namespace Style
{
enum class StyleType
{
    Unknown,
    Float,
    Vec2f,
    Vec3f,
    Vec4f
};

struct StyleValue
{
    StyleValue()
        : type(StyleType::Unknown)
        , f4(NVec4f(0.0f))
    {
    }
    StyleValue(const NVec4f& val)
        : type(StyleType::Vec4f)
        , f4(val)
    {
    }
    StyleValue(const NVec3f& val)
        : type(StyleType::Vec3f)
        , f3(val)
    {
    }
    StyleValue(const NVec2f& val)
        : type(StyleType::Vec2f)
        , f2(val)
    {
    }
    StyleValue(const float& val)
        : type(StyleType::Float)
        , f(val)
    {
    }

    void Check() const
    {
        if (type == StyleType::Unknown)
        {
            assert(!"Type undeclared");
        }
    }

    NVec4f ToVec4f() const
    {
        Check();
        if (type == StyleType::Vec4f)
        {
            return f4;
        }
        return NVec4(f);
    }

    float ToFloat() const
    {
        Check();
        if (type == StyleType::Float)
        {
            return f;
        }
        return f4.x;
    }
    union
    {
        NVec4f f4;
        NVec3f f3;
        NVec2f f2;
        float f;
    };
    StyleType type;
};

using StyleMap = std::map<uint32_t, StyleValue>;
class StyleManager
{
public:
    static StyleManager& Instance()
    {
        static StyleManager style;
        return style;
    }

    void Set(const StringId& id, const StyleValue& value)
    {
        m_styles[m_currentStyle][id.id] = value;
    }

    const StyleValue& Get(const StringId& id)
    {
        auto& style = m_styles[m_currentStyle];
        return style[id.id];
    }

    float GetFloat(const StringId& id)
    {
        auto& style = m_styles[m_currentStyle];
        return style[id.id].ToFloat();
    }

    NVec4f GetVec4f(const StringId& id)
    {
        auto& style = m_styles[m_currentStyle];
        return style[id.id].ToVec4f();
    }

    std::map<std::string, StyleMap> m_styles;
    std::string m_currentStyle;
};

} // namespace Style
} // namespace MUtils
