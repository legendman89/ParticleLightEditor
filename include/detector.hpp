#pragma once

#include "types.hpp"

namespace ParticleLightEditor::Detector
{
    inline bool HasStructure(RE::BSGeometry& a_geometry, RE::BSEffectShaderProperty& a_shader)
    {
        if (a_shader.lightData) {
            return false;
        }

        if (!a_geometry.parent || !netimmerse_cast<RE::NiBillboardNode*>(a_geometry.parent)) {
            return false;
        }

        using Flag = RE::BSShaderProperty::EShaderPropertyFlag;
        if (!a_shader.flags.all(Flag::kSoftEffect, Flag::kZBufferTest)) {
            return false;
        }

        const auto* alpha = a_geometry.GetGeometryRuntimeData().alphaProperty.get();
        return alpha && alpha->alphaFlags == kParticleLightAlphaFlags;
    }

    inline bool HasParticleTopology(RE::BSTriShape& a_geometry)
    {
        const auto& topology = a_geometry.GetTrishapeRuntimeData();
        return (topology.triangleCount == kParticleLightTriangleCount && topology.vertexCount == kParticleLightVertexCount) ||
            (topology.triangleCount == kComplexParticleLightTriangleCount && topology.vertexCount == kComplexParticleLightVertexCount);
    }

    inline bool IsSameBaseParticle(const Candidate& a_left, const Candidate& a_right)
    {
        if (a_left.entry.baseFormID == 0 || a_left.entry.baseFormID != a_right.entry.baseFormID || a_left.baseParticleOrdinal != a_right.baseParticleOrdinal ||
            a_left.triangleCount != a_right.triangleCount || a_left.vertexCount != a_right.vertexCount) {
            return false;
        }
        return true;
    }
}
