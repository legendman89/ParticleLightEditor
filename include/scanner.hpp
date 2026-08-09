#pragma once

#include "types.hpp"

namespace ParticleLightEditor
{
    class Scanner
    {
    public:

        static Scanner& GetSingleton();

        void Reset();

        void RequestRescan(bool a_immediate = false);

        void Update(RE::PlayerCharacter* a_player, float a_delta);

        Stats GetStats() const;

        inline size_t GetLightCount() const { return editorIndices.size(); }

        size_t GetSelectedLightIndex() const;

        std::string GetLightLabel(size_t a_index) const;

        bool SelectLight(size_t a_index);

        size_t SelectReference(RE::TESObjectREFR* a_reference);

        bool GetSelectedEditorState(EditorState& a_state) const;

        bool SetSelectedColor(const RE::NiColorA& a_color);

        bool SetSelectedIntensity(float a_intensity);

        bool SetSelectedRadius(float a_radius);

        bool SetSelectedLocalPosition(const RE::NiPoint3& a_position);

        bool ResetSelectedLight();

        bool IsSelectedLightEdited() const;

        EditScope GetEditScope() const { return editScope; }

        bool SetEditScope(EditScope a_scope);

        ParticleCategory GetSelectedCategory() const;

        bool SetSelectedCategory(ParticleCategory a_category);

        ParticleCategory GetTargetCategory() const { return targetCategory; }

        bool SetTargetCategory(ParticleCategory a_category);

        size_t GetAffectedLightCount() const;

        bool ResetSelectedScope();

        bool IsSelectedScopeEdited() const;

        inline const EditMap& GetEdits() const { return edits; }

        void SetEdits(const EditMap& a_edits);

        const CategoryRuleMap& GetCategoryRules() const { return categoryRules; }

        void SetCategoryRules(const CategoryRuleMap& a_rules);

        const CategoryOverrideMap& GetCategoryOverrides() const { return categoryOverrides; }

        void SetCategoryOverrides(const CategoryOverrideMap& a_overrides);

    private:

        inline Entry* GetSelectedEntry() { return selectedIndex < entries.size() ? &entries[selectedIndex] : nullptr; }

        inline const Entry* GetSelectedEntry() const { return selectedIndex < entries.size() ? &entries[selectedIndex] : nullptr; }

        inline Edit* FindEdit(const Entry& a_entry)
        {
            const auto found = edits.find(a_entry.editKey);
            return found != edits.end() ? &found->second : nullptr;
        }
        inline const Edit* FindEdit(const Entry& a_entry) const
        {
            const auto found = edits.find(a_entry.editKey);
            return found != edits.end() ? &found->second : nullptr;
        }

        void Refresh(RE::TESObjectCELL* a_cell, RE::PlayerCharacter* a_player);

        RE::TESObjectLIGH* GetEquippedLight(RE::PlayerCharacter* a_player) const;

        void VisitEquippedLight(RE::PlayerCharacter* a_player, std::unordered_set<RE::NiAVObject*>& a_visited);

        void VisitReference(RE::TESObjectREFR* a_reference, std::unordered_set<RE::NiAVObject*>& a_visited, bool a_directSource);

        void VisitNodeBranch(RE::NiAVObject* a_object, RE::TESObjectREFR* a_owner, std::unordered_set<RE::NiAVObject*>& a_visited, bool a_nameValidated = false, bool a_glowBranch = false, bool a_directSource = false, RE::TESForm* a_baseOverride = nullptr, bool a_runtimeAttachment = false);

        void VisitBillboard(RE::NiBillboardNode* a_billboard, RE::TESObjectREFR* a_owner, bool a_nameValidated, bool a_glowBranch, bool a_directSource, RE::TESForm* a_baseOverride = nullptr, bool a_runtimeAttachment = false);

        void MatchCandidates(const std::vector<RE::TESObjectREFR*>& a_sources);

        std::vector<MatchEdge> BuildMatchEdges(const std::vector<Source>& a_sources, const std::vector<bool>& a_candidateClaimed, bool a_namedCandidates, float a_associationRangeSquared, float a_radiusWeight) const;

        void UpdateEditorList(RE::PlayerCharacter* a_player);

        void SetParticleLightEdit(Entry& a_entry);

        void ApplyParticleLightEdits();

        Edit GetEffectiveEdit(const Entry& a_entry) const;

        CategoryRuleKey GetSelectedCategoryRuleKey() const;

        bool MatchesSelectedScope(const Entry& a_entry) const;

        const Entry* GetScopeRepresentative() const;

        bool RestoreEntryRuntime(Entry& a_entry);

        std::vector<Entry> entries;
        std::vector<Candidate> candidates;
        std::vector<size_t> editorIndices;
        EditMap edits;
        CategoryRuleMap categoryRules;
        CategoryOverrideMap categoryOverrides;
        EditScope editScope{ EditScope::kSelectedLight };
        ParticleCategory targetCategory{ ParticleCategory::kUnclassified };
        size_t selectedIndex{ (std::numeric_limits<size_t>::max)() };
        RE::TESObjectCELL* cell{ nullptr };
        bool rescanRequested{ false };
        float scanElapsed{ 0.0F };
        float rescanDelay{ 0.0F };
        RE::FormID equippedLightFormID{ 0 };
        DrawState drawState;
        ScanCounters counters;
    };

    inline Scanner& Scanner::GetSingleton()
    {
        static Scanner singleton;
        return singleton;
    }
}
