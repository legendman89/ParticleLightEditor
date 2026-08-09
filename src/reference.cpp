#include "reference.hpp"

#include "detector.hpp"
#include "editor.hpp"
#include "logger.hpp"
#include "settings.hpp"
#include "utility.hpp"

namespace ParticleLightEditor
{
    void ReferenceManager::Capture(const Entry& a_entry, const Edit& a_edit)
    {
        if (a_entry.ownerFormID == 0 || a_entry.runtimeAttachment) {
            return;
        }

        const auto changed = a_edit.colorChanged || a_edit.intensityChanged || a_edit.radiusChanged || a_edit.positionChanged;
        std::scoped_lock lock(mutex);
        auto& referenceEdits = editsByReference[a_entry.ownerFormID];
        auto found = referenceEdits.end();
        for (auto iterator = referenceEdits.begin(); iterator != referenceEdits.end(); ++iterator) {
            if (iterator->editKey == a_entry.editKey) {
                found = iterator;
                break;
            }
        }
        if (!changed) {
            if (found != referenceEdits.end()) {
                referenceEdits.erase(found);
            }
            if (referenceEdits.empty()) {
                editsByReference.erase(a_entry.ownerFormID);
            }
            return;
        }

        ReferenceEdit referenceEdit;
        referenceEdit.ownerFormID = a_entry.ownerFormID;
        referenceEdit.originalLocal = a_edit.defaults.local;
        referenceEdit.originalRadius = a_edit.defaults.radius;
        referenceEdit.editKey = a_entry.editKey;
        referenceEdit.edit = a_edit;
        if (found != referenceEdits.end()) {
            *found = referenceEdit;
        }
        else {
            referenceEdits.push_back(referenceEdit);
        }
    }

    void ReferenceManager::Remove(const Entry& a_entry)
    {
        if (a_entry.runtimeAttachment) {
            return;
        }
        std::scoped_lock lock(mutex);
        const auto owner = editsByReference.find(a_entry.ownerFormID);
        if (owner == editsByReference.end()) {
            return;
        }

        for (auto iterator = owner->second.begin(); iterator != owner->second.end();) {
            if (iterator->editKey == a_entry.editKey) {
                iterator = owner->second.erase(iterator);
            }
            else {
                ++iterator;
            }
        }
        if (owner->second.empty()) {
            editsByReference.erase(owner);
        }
    }

    void ReferenceManager::RememberMaterial(RE::BSGeometry* a_geometry, RE::BSEffectShaderMaterial* a_material)
    {
        if (!a_geometry || !a_material) {
            return;
        }

        std::scoped_lock lock(mutex);
        materials[a_geometry] = a_material;
    }

    RE::BSEffectShaderMaterial* ReferenceManager::FindMaterial(RE::BSGeometry* a_geometry)
    {
        if (!a_geometry) {
            return nullptr;
        }

        std::scoped_lock lock(mutex);
        const auto found = materials.find(a_geometry);
        if (found == materials.end()) {
            return nullptr;
        }

        auto* shader = Utility::GetEffectShader(*a_geometry);
        if (!shader || shader->GetMaterial() != found->second) {
            materials.erase(found);
            return nullptr;
        }
        return found->second;
    }

    void ReferenceManager::ClearMaterials()
    {
        std::scoped_lock lock(mutex);
        materials.clear();
    }

    void ReferenceManager::Collect(RE::NiAVObject* a_object, bool a_nameValidated, bool a_glowBranch, std::vector<RE::BSGeometry*>& a_geometries) const
    {
        if (!a_object) {
            return;
        }

        const auto nameValidated = a_nameValidated || Utility::HasParticleLightName(a_object);
        const auto glowBranch = a_glowBranch || Utility::HasGlowName(a_object);
        if (auto* billboard = netimmerse_cast<RE::NiBillboardNode*>(a_object)) {
            if (glowBranch && !nameValidated && !Settings::GetSettings().includeGlowNodes) {
                return;
            }

            for (auto& child : billboard->GetChildren()) {
                auto* geometry = child ? child->AsTriShape() : nullptr;
                auto* shader = geometry ? Utility::GetEffectShader(*geometry) : nullptr;
                if (!geometry || !shader) {
                    continue;
                }

                const auto validatedByName = nameValidated || Utility::HasParticleLightName(geometry);
                if (!validatedByName && !Detector::HasStructure(*geometry, *shader)) {
                    continue;
                }

                if (!validatedByName && !Detector::HasParticleTopology(*geometry)) {
                    continue;
                }
                a_geometries.push_back(geometry);
            }
            return;
        }

        if (auto* node = a_object->AsNode()) {
            for (auto& child : node->GetChildren()) {
                if (child && child->AsNode()) {
                    Collect(child.get(), nameValidated, glowBranch, a_geometries);
                }
            }
        }
    }

    RE::BSGeometry* ReferenceManager::FindGeometry(const ReferenceEdit& a_edit, const std::vector<RE::BSGeometry*>& a_geometries,
        const std::unordered_set<RE::BSGeometry*>& a_claimed) const
    {
        RE::BSGeometry* closest = nullptr;
        auto closestScore = (std::numeric_limits<float>::max)();
        for (auto* geometry : a_geometries) {
            if (!geometry || a_claimed.contains(geometry)) {
                continue;
            }

            const auto positionScore = Utility::DistanceSquared(geometry->local.translate, a_edit.originalLocal.translate);
            const auto scaleDifference = geometry->local.scale - a_edit.originalLocal.scale;
            const auto score = positionScore + scaleDifference * scaleDifference * 100.0F;
            if (std::isfinite(score) && score < closestScore) {
                closest = geometry;
                closestScore = score;
            }
        }
        return closestScore <= 1.0F ? closest : nullptr;
    }

    void ReferenceManager::Apply(RE::TESObjectREFR* a_reference, RE::NiAVObject* a_root)
    {
        if (!a_reference || !a_root || !a_root->AsFadeNode()) {
            return;
        }

        std::vector<ReferenceEdit> referenceEdits;
        {
            std::scoped_lock lock(mutex);
            const auto found = editsByReference.find(a_reference->GetFormID());
            if (found == editsByReference.end()) {
                return;
            }
            referenceEdits = found->second;
        }

        std::vector<RE::BSGeometry*> geometries;
        Collect(a_root, false, false, geometries);
        std::unordered_set<RE::BSGeometry*> claimed;
        for (auto& referenceEdit : referenceEdits) {
            auto* geometry = FindGeometry(referenceEdit, geometries, claimed);
            if (!geometry) {
                logger::warn("Could not apply saved edit early for reference {:08X}", referenceEdit.ownerFormID);
                continue;
            }

            Entry entry;
            entry.geometry = RE::NiPointer<RE::BSGeometry>(geometry);
            entry.ownerFormID = referenceEdit.ownerFormID;
            entry.defaults.local = referenceEdit.originalLocal;
            entry.defaults.radius = referenceEdit.originalRadius;
            entry.editableMaterial = Editor::CreateEditableMaterial(*geometry, referenceEdit.ownerFormID);
            if (entry.editableMaterial && Editor::Apply(entry, referenceEdit.edit)) {
                claimed.insert(geometry);
                logger::info("Applied particle-light edit during Load3D for reference {:08X}", referenceEdit.ownerFormID);
            }
        }
    }
}
