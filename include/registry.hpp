#pragma once

namespace ParticleLightEditor
{
    class Registry
    {
    public:

        struct Record
        {
            RE::NiPointer<RE::NiPointLight> light;
            RE::FormID referenceFormID{ 0 };
            RE::FormID baseFormID{ 0 };
            std::string editorID;
            std::string displayName;
            std::string runtimeNodeName;
        };

        static Registry& GetSingleton();

        static std::string ResolveEditorID(const RE::TESObjectLIGH* a_source);

        static std::string ResolveDisplayName(const RE::TESObjectLIGH* a_source);

        void Capture(RE::NiPointLight* a_runtimeLight, RE::TESObjectLIGH* a_baseLight, RE::TESObjectREFR* a_reference);

        void Reset();

        void RetainReferences(const std::unordered_set<RE::FormID>& a_referenceFormIDs);

        std::optional<Record> FindByReference(RE::FormID a_referenceFormID) const;

        std::optional<Record> FindByRuntimeLight(RE::NiPointLight* a_runtimeLight) const;

        size_t Size() const;

    private:
    
        static std::string ResolveNodeName(const RE::NiPointLight* a_source)
        {
            return !a_source || a_source->name.empty() ? "Unnamed NiLight" : a_source->name.c_str();
        }

        mutable std::mutex lock;
        std::unordered_map<RE::NiPointLight*, Record> recordsByLight;
        std::unordered_map<RE::FormID, RE::NiPointLight*> lightByReference;
    };

    inline Registry& Registry::GetSingleton()
    {
        static Registry singleton;
        return singleton;
    }
}
