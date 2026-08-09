#include "hooks.hpp"

#include "reference.hpp"
#include "scanner.hpp"
#include "logger.hpp"

namespace Hooks
{
    void PlayerUpdate::Thunk(RE::PlayerCharacter* a_player, float a_delta)
    {
        playerUpdateFunction(a_player, a_delta);

        if (a_player) {
            ParticleLightEditor::Scanner::GetSingleton().Update(a_player, a_delta);
        }
    }

    void PlayerUpdate::Install()
    {
        playerUpdateFunction = REL::Relocation<uintptr_t>(RE::PlayerCharacter::VTABLE[0]).write_vfunc(0xAD, Thunk);
        logger::info("Player update hook installed");
    }

    void RuntimeLightCapture::Install()
    {
        if (REL::Module::IsVR()) {
            logger::warn("Runtime NiLight capture is currently unavailable on Skyrim VR");
            return;
        }

        auto& trampoline = SKSE::GetTrampoline();

        REL::Relocation<uintptr_t> clone3DTarget{ RELOCATION_ID(17206, 17603), 0x1D3 };
        generatedFunction<0> = trampoline.write_call<5>(clone3DTarget.address(), GeneratedLightHook<0>::Thunk);

        REL::Relocation<uintptr_t> addLightTarget{ RELOCATION_ID(19252, 19678), 0xB8 };
        generatedFunction<1> = trampoline.write_call<5>(addLightTarget.address(), GeneratedLightHook<1>::Thunk);

        logger::info("Installed runtime NiPointLight capture hooks");
    }

    RE::NiAVObject* ReferenceLoad::Thunk(RE::TESObjectREFR* a_reference, bool a_backgroundLoading)
    {
        auto* root = referenceLoadFunction(a_reference, a_backgroundLoading);
        ParticleLightEditor::ReferenceManager::GetSingleton().Apply(a_reference, root);
        return root;
    }

    void ReferenceLoad::Install()
    {
        referenceLoadFunction = REL::Relocation<uintptr_t>(RE::TESObjectREFR::VTABLE[0]).write_vfunc(0x6A, Thunk);
        logger::info("Reference Load3D particle-light edit hook installed");
    }

    void Install()
    {
        SKSE::AllocTrampoline(1 << 6);
        RuntimeLightCapture::Install();
        ReferenceLoad::Install();
        PlayerUpdate::Install();
    }
}
