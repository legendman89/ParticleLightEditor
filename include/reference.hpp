#pragma once

#include "types.hpp"

namespace ParticleLightEditor
{
    struct ReferenceEdit
    {
        RE::FormID ownerFormID{ 0 };
        RE::NiTransform originalLocal;
        EditKey editKey;
        Edit edit;
        float originalRadius{ 0.0F };
    };

    class ReferenceManager
    {
    public:
        static ReferenceManager& GetSingleton();

        void Capture(const Entry& a_entry, const Edit& a_edit);

        void Remove(const Entry& a_entry);

        void Apply(RE::TESObjectREFR* a_reference, RE::NiAVObject* a_root);

        void RememberMaterial(RE::BSGeometry* a_geometry, RE::BSEffectShaderMaterial* a_material);

        RE::BSEffectShaderMaterial* FindMaterial(RE::BSGeometry* a_geometry);

        void ClearMaterials();

    private:

        void Collect(RE::NiAVObject* a_object, bool a_nameValidated, bool a_glowBranch, std::vector<RE::BSGeometry*>& a_geometries) const;

        RE::BSGeometry* FindGeometry(const ReferenceEdit& a_edit, const std::vector<RE::BSGeometry*>& a_geometries, const std::unordered_set<RE::BSGeometry*>& a_claimed) const;

        std::mutex mutex;
        std::unordered_map<RE::FormID, std::vector<ReferenceEdit>> editsByReference;
        std::unordered_map<RE::BSGeometry*, RE::BSEffectShaderMaterial*> materials;
    };

    inline ReferenceManager& ReferenceManager::GetSingleton()
    {
        static ReferenceManager singleton;
        return singleton;
    }
}
