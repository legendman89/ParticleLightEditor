#pragma once

#include "types.hpp"

namespace ParticleLightEditor::Editor
{
    RE::BSEffectShaderMaterial* CreateEditableMaterial(RE::BSGeometry& a_geometry, RE::FormID a_ownerFormID);

    RE::BSEffectShaderMaterial* GetEditableMaterial(Entry& a_entry);
    
    bool Apply(Entry& a_entry, Edit& a_edit);

    bool Restore(Entry& a_entry, const Edit& a_edit);

    inline EditKey GetKey(const Entry& a_entry)
    {
        if (a_entry.runtimeAttachment && a_entry.baseFormID != 0) {
            return { a_entry.baseFormID, a_entry.particleOrdinal };
        }
        return { a_entry.associatedLightRefID != 0 ? a_entry.associatedLightRefID : a_entry.ownerFormID, a_entry.particleOrdinal };
    }

    inline void UpdateTransform(RE::BSGeometry& a_geometry)
    {
        RE::NiUpdateData updateData{};
        updateData.time = 0.0F;
        updateData.flags = RE::NiUpdateData::Flag::kDirty;
        if (a_geometry.parent) {
            a_geometry.parent->UpdateTransformAndBounds(updateData);
        }
        else {
            a_geometry.UpdateTransformAndBounds(updateData);
        }
    }
}
