#pragma once

#include "registry.hpp"

namespace Hooks
{
    using GeneratedFunction = RE::NiPointLight*(RE::TESObjectLIGH*, RE::TESObjectREFR*, RE::NiNode*, bool, bool, bool);

    template <size_t Index>
    struct GeneratedLightHook
    {
        // PO3's hook.
        static RE::NiPointLight* Thunk(RE::TESObjectLIGH* a_light, RE::TESObjectREFR* a_reference, RE::NiNode* a_node, bool a_forceDynamic, bool a_useLightRadius, bool a_affectRequesterOnly)
        {
            auto* runtimeLight = generatedFunction<Index>(a_light, a_reference, a_node, a_forceDynamic, a_useLightRadius, a_affectRequesterOnly);
            ParticleLightEditor::Registry::GetSingleton().Capture(runtimeLight, a_light, a_reference);
            return runtimeLight;
        }
    };

    struct ReferenceLoad
    {
        // QTR's hook.
        static RE::NiAVObject* Thunk(RE::TESObjectREFR* a_reference, bool a_backgroundLoading);
        static void Install();
    };

    struct PlayerUpdate
    {
        static void Thunk(RE::PlayerCharacter* a_player, float a_delta);
        static void Install();

    };

    struct RuntimeLightCapture
    {
        static void Install();
    };

    template <size_t Index>
    inline REL::Relocation<GeneratedFunction> generatedFunction;

    inline REL::Relocation<decltype(PlayerUpdate::Thunk)> playerUpdateFunction;

    inline REL::Relocation<decltype(ReferenceLoad::Thunk)> referenceLoadFunction;

    void Install();
}
