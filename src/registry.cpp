#include "registry.hpp"

#include "logger.hpp"

#include <ClibUtil/EditorID.hpp>

namespace ParticleLightEditor
{
    std::string Registry::ResolveEditorID(const RE::TESObjectLIGH* a_source)
    {
        if (!a_source) {
            return {};
        }

        auto editorID = clib_util::editorID::get_editorID(a_source);
        if (!editorID.empty()) {
            return editorID;
        }

        const auto* fallback = a_source->GetFormEditorID();
        return fallback && fallback[0] != '\0' ? fallback : std::string{};
    }

    std::string Registry::ResolveDisplayName(const RE::TESObjectLIGH* a_source)
    {
        if (!a_source) {
            return "Unknown Light";
        }

        if (auto editorID = ResolveEditorID(a_source); !editorID.empty()) {
            return editorID;
        }

        const auto* name = a_source->GetName();
        if (name && name[0] != '\0') {
            return name;
        }

        return std::format("LIGH {:08X}", a_source->GetFormID());
    }

    void Registry::Capture(RE::NiPointLight* a_runtimeLight, RE::TESObjectLIGH* a_baseLight, RE::TESObjectREFR* a_reference)
    {
        if (!a_runtimeLight || !a_baseLight || !a_reference) {
            return;
        }

        auto* runtimeReference = a_runtimeLight->GetUserData();
        auto* identityReference = runtimeReference ? runtimeReference : a_reference;
        if (runtimeReference && runtimeReference != a_reference) {
            logger::warn(
                "Generated NiPointLight {:p} reference mismatch: hook ref={:08X}, userData ref={:08X}",
                static_cast<void*>(a_runtimeLight),
                a_reference->GetFormID(),
                runtimeReference->GetFormID());
        }

        Record record;
        record.light = RE::NiPointer<RE::NiPointLight>(a_runtimeLight);
        record.referenceFormID = identityReference->GetFormID();
        record.baseFormID = a_baseLight->GetFormID();
        record.editorID = ResolveEditorID(a_baseLight);
        record.displayName = ResolveDisplayName(a_baseLight);
        record.runtimeNodeName = ResolveNodeName(a_runtimeLight);

        std::scoped_lock guard(lock);
        if (const auto existing = lightByReference.find(record.referenceFormID);
            existing != lightByReference.end() && existing->second != a_runtimeLight) {
            recordsByLight.erase(existing->second);
        }

        lightByReference[record.referenceFormID] = a_runtimeLight;
        recordsByLight[a_runtimeLight] = record;

        logger::debug(
            "Captured runtime NiPointLight {:p}: ref={:08X}, base={:08X}, editorID='{}', "
            "displayName='{}', runtimeNode='{}'",
            static_cast<void*>(a_runtimeLight),
            record.referenceFormID,
            record.baseFormID,
            record.editorID.empty() ? "Unavailable" : record.editorID,
            record.displayName,
            record.runtimeNodeName);
    }

    void Registry::Reset()
    {
        std::scoped_lock guard(lock);
        recordsByLight.clear();
        lightByReference.clear();
    }

    void Registry::RetainReferences(const std::unordered_set<RE::FormID>& a_referenceFormIDs)
    {
        std::scoped_lock guard(lock);
        for (auto iterator = lightByReference.begin(); iterator != lightByReference.end();) {
            if (a_referenceFormIDs.contains(iterator->first)) {
                ++iterator;
                continue;
            }

            recordsByLight.erase(iterator->second);
            iterator = lightByReference.erase(iterator);
        }
    }

    std::optional<Registry::Record> Registry::FindByReference(RE::FormID a_referenceFormID) const
    {
        std::scoped_lock guard(lock);
        const auto reference = lightByReference.find(a_referenceFormID);
        if (reference == lightByReference.end()) {
            return std::nullopt;
        }

        const auto record = recordsByLight.find(reference->second);
        if (record == recordsByLight.end()) {
            return std::nullopt;
        }

        auto result = record->second;
        if (result.light) {
            result.runtimeNodeName = ResolveNodeName(result.light.get());
        }
        return result;
    }

    std::optional<Registry::Record> Registry::FindByRuntimeLight(RE::NiPointLight* a_runtimeLight) const
    {
        std::scoped_lock guard(lock);
        const auto record = recordsByLight.find(a_runtimeLight);
        if (record == recordsByLight.end()) {
            return std::nullopt;
        }

        auto result = record->second;
        if (result.light) {
            result.runtimeNodeName = ResolveNodeName(result.light.get());
        }
        return result;
    }

    size_t Registry::Size() const
    {
        std::scoped_lock guard(lock);
        return recordsByLight.size();
    }
}
