#include "scanner.hpp"

#include "category.hpp"
#include "draw.hpp"
#include "logger.hpp"
#include "registry.hpp"
#include "settings.hpp"
#include "vertices.hpp"

namespace ParticleLightEditor
{
    void Scanner::Reset()
    {
        Vertices::Manager::GetSingleton().Clear();
        entries.clear();
        candidates.clear();
        editorIndices.clear();
        edits.clear();
        categoryRules.clear();
        categoryOverrides.clear();
        editScope = EditScope::kSelectedLight;
        targetCategory = ParticleCategory::kUnclassified;
        selectedIndex = (std::numeric_limits<size_t>::max)();
        cell = nullptr;
        rescanRequested = false;
        scanElapsed = 0.0F;
        rescanDelay = 0.0F;
        equippedLightFormID = 0;
        drawState = {};
        counters.Reset();
    }

    void Scanner::RequestRescan(bool a_immediate)
    {
        rescanRequested = true;
        rescanDelay = a_immediate ? 0.0F : 0.2F;
    }

    void Scanner::SetEdits(const EditMap& a_edits)
    {
        edits = a_edits;
        for (auto& entry : entries) {
            SetParticleLightEdit(entry);
        }
    }

    void Scanner::SetCategoryRules(const CategoryRuleMap& a_rules)
    {
        // Applying an Edit only writes fields whose changed flag is set. Restore
        // first so values belonging to a removed rule do not remain on the mesh.
        for (auto& entry : entries) {
            RestoreEntryRuntime(entry);
        }
        categoryRules = a_rules;
        ApplyParticleLightEdits();
    }

    void Scanner::SetCategoryOverrides(const CategoryOverrideMap& a_overrides)
    {
        for (auto& entry : entries) {
            RestoreEntryRuntime(entry);
        }
        categoryOverrides = a_overrides;
        for (auto& entry : entries) {
            entry.category = Category::Resolve(entry, categoryOverrides);
        }
        ApplyParticleLightEdits();
    }

    Stats Scanner::GetStats() const
    {
        Stats stats;
        stats.scan = counters;
        stats.draw = drawState.counters;
        stats.cachedLights = entries.size();
        stats.registeredRuntimeLights = Registry::GetSingleton().Size();
        return stats;
    }

    void Scanner::Update(RE::PlayerCharacter* a_player, float a_delta)
    {
        if (!a_player) {
            return;
        }

        auto* currentCell = a_player->GetParentCell();
        const auto& settings = Settings::GetSettings();

        const auto* equippedLight = GetEquippedLight(a_player);
        const auto currentEquippedLightFormID = equippedLight ? equippedLight->GetFormID() : 0;
        if (currentEquippedLightFormID != equippedLightFormID) {
            equippedLightFormID = currentEquippedLightFormID;
            rescanRequested = true;
            rescanDelay = 0.2F;
            logger::info("Player carried-light attachment changed to {:08X}; scheduling a particle-light rescan", equippedLightFormID);
        }

        if (rescanRequested && rescanDelay > 0.0F && std::isfinite(a_delta) && a_delta > 0.0F) {
            rescanDelay = (std::max)(0.0F, rescanDelay - a_delta);
        }

        if (settings.scanInterval > 0.0F && std::isfinite(a_delta) && a_delta > 0.0F) {
            scanElapsed += a_delta;
            if (scanElapsed >= settings.scanInterval) {
                rescanRequested = true;
                rescanDelay = 0.0F;
            }
        }
        else if (settings.scanInterval <= 0.0F) {
            scanElapsed = 0.0F;
        }

        // A cell's scene graph is cached once by default. Walking around inside the
        // same cell only performs cheap distance checks unless the user requests a
        // rescan or explicitly enables the periodic interval.
        if (currentCell != cell || (rescanRequested && rescanDelay <= 0.0F)) {
            Refresh(currentCell, a_player);
        }

        ApplyParticleLightEdits();
        UpdateEditorList(a_player);
        Draw::Lights(entries, selectedIndex, a_player, settings, drawState);
    }

}
