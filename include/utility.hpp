#pragma once

namespace ParticleLightEditor::Utility
{
    inline float DistanceSquared(const RE::NiPoint3& a_left, const RE::NiPoint3& a_right)
    {
        const auto offset = a_left - a_right;
        return offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;
    }

    inline RE::BSEffectShaderProperty* GetEffectShader(RE::BSGeometry& a_geometry)
    {
        auto* shader = a_geometry.GetGeometryRuntimeData().shaderProperty.get();
        return shader ? netimmerse_cast<RE::BSEffectShaderProperty*>(shader) : nullptr;
    }

    inline bool IsOnScreen(const RE::GRectF& a_frame, const RE::NiPoint2& a_point)
    {
        return a_point.x >= a_frame.left && a_point.x <= a_frame.right && a_point.y >= a_frame.top && a_point.y <= a_frame.bottom;
    }

    inline std::string GetLowercaseName(const RE::NiAVObject* a_object)
    {
        if (!a_object || a_object->name.empty()) {
            return {};
        }

        std::string name = a_object->name.c_str();
        for (auto& character : name) {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }
        return name;
    }

    inline bool HasParticleLightName(const RE::NiAVObject* a_object)
    {
        const auto name = GetLowercaseName(a_object);
        return name.contains("particle") && name.contains("light");
    }

    inline bool HasGlowName(const RE::NiAVObject* a_object) { return GetLowercaseName(a_object).contains("glow"); }

    inline bool ColorsEqual(const RE::NiColorA& a_left, const RE::NiColorA& a_right)
    {
        return a_left.red == a_right.red && a_left.green == a_right.green && a_left.blue == a_right.blue && a_left.alpha == a_right.alpha;
    }

    inline bool PointsEqual(const RE::NiPoint3& a_left, const RE::NiPoint3& a_right)
    {
        return a_left.x == a_right.x && a_left.y == a_right.y && a_left.z == a_right.z;
    }
}
