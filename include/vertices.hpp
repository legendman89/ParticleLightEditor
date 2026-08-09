#pragma once

#include "types.hpp"

namespace ParticleLightEditor::Vertices
{
    // Vertex-colored particle lights, such as fires, may share renderer data
    // between mesh instances. This gives each edited geometry its own renderer 
    // and vertex buffer so per-reference edits do not affect the other instances.
    struct LocalRenderer
    {
        RE::BSGraphics::TriShape* renderer{ nullptr };
        RE::FormID ownerFormID{ 0 };
        std::vector<uint8_t> data;
    };

    struct VertexColor
    {
        RE::NiColorA color{ 1.0F, 1.0F, 1.0F, 1.0F };
        bool valid{ false };
    };

    struct PackedColor
    {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;
    };

    class Manager
    {
    public:
        static Manager& GetSingleton();

        bool Apply(RE::BSTriShape& a_geometry, const RE::NiColorA& a_color, RE::FormID a_ownerFormID);

        bool Restore(RE::BSTriShape& a_geometry, RE::FormID a_ownerFormID);

        void Clear();

    private:

        bool PrepareLocalRenderer(RE::BSTriShape& a_geometry, RE::FormID a_ownerFormID);

        bool RefreshBuffer(RE::BSTriShape& a_geometry);

        void RefreshGeometry(RE::BSTriShape& a_geometry);

        RE::ID3D11Buffer* CreateVertexBuffer(RE::BSGraphics::TriShape& a_renderer, size_t a_vertexBytes);

        std::mutex mutex;
        std::unordered_map<RE::BSTriShape*, LocalRenderer> localRenderers;
    };

    inline Manager& Manager::GetSingleton()
    {
        static Manager singleton;
        return singleton;
    }

    // Some PLs color the vertices but keep the base color white or black.
    // We need to identify those and color their vertices instead.
    inline bool IsBlackOrWhite(const RE::NiColorA& a_color)
    {
        const auto neutral = std::abs(a_color.red - a_color.green) <= 1.0F / 255.0F && std::abs(a_color.red - a_color.blue) <= 1.0F / 255.0F;
        return neutral && (a_color.red <= 1.0F / 255.0F || a_color.red >= 0.9F);
    }

    VertexColor ReadVertexColor(RE::BSTriShape& a_geometry);

}
