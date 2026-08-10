#pragma once

#include "edit_defs.hpp"

#include <cstring>
#include <type_traits>

#define EDIT_CHANGED_DEFINE(NAME, CHANGED, COMPARISON) bool CHANGED{ false };
#define EDIT_CHANGED_CHECK(NAME, CHANGED, COMPARISON) a_edit.CHANGED ||

namespace ParticleLightEditor
{
    enum class ParticleCategory : uint8_t
    {
        kUnclassified,
        kCandle,
        kChandelier,
        kLantern,
        kFireEmber,
        kTorchBrazier,
        kTotal
    };

    enum class EditScope : uint8_t
    {
        kSelectedLight,
        kCategoryCell,
        kCategoryGlobal,
        kTotal
    };

    struct CategoryRuleKey
    {
        ParticleCategory category{ ParticleCategory::kUnclassified };
        RE::FormID cellFormID{ 0 };  // Zero means every cell.

        bool operator==(const CategoryRuleKey&) const = default;
    };

    struct CategoryRuleKeyHash
    {
        size_t operator()(const CategoryRuleKey& a_key) const
        {
            const auto categoryHash = std::hash<uint8_t>{}(static_cast<uint8_t>(a_key.category));
            const auto cellHash = std::hash<RE::FormID>{}(a_key.cellFormID);
            return categoryHash ^ (cellHash + 0x9E3779B9 + (categoryHash << 6) + (categoryHash >> 2));
        }
    };

    struct CategoryRule
    {
        RE::NiColorA color{ 1.0F, 1.0F, 1.0F, 1.0F };
        float intensity{ 1.0F };
        float radiusScale{ 1.0F };
        FOREACH_CATEGORY_RULE_PROPERTY(EDIT_CHANGED_DEFINE)
    };

    inline bool HasChanges(const CategoryRule& a_edit) { return FOREACH_CATEGORY_RULE_PROPERTY(EDIT_CHANGED_CHECK) false; }

    using CategoryRuleMap = std::unordered_map<CategoryRuleKey, CategoryRule, CategoryRuleKeyHash>;
    using CategoryOverrideMap = std::unordered_map<std::string, ParticleCategory>;

    struct ScanCounters
    {
        size_t referenceCount{ 0 };
        size_t sourceCount{ 0 };
        size_t sourceRootCount{ 0 };
        size_t meshRootCount{ 0 };
        size_t rootCount{ 0 };
        size_t rejectedRootCount{ 0 };
        size_t billboardCount{ 0 };
        size_t triShapeCount{ 0 };
        size_t nameValidatedCount{ 0 };
        size_t rejectedGlowCount{ 0 };
        size_t rejectedShaderCount{ 0 };
        size_t rejectedTopologyCount{ 0 };
        size_t structuralCandidateCount{ 0 };
        size_t sourceMatchCount{ 0 };
        size_t unmatchedStructuralCount{ 0 };

        void Reset() noexcept
        {
            // This should be fine with standard types without custom ctor/dtors.
            std::memset(static_cast<void*>(this), 0, sizeof(*this));
        }
    };

    static_assert(std::is_trivially_copyable_v<ScanCounters>);

    struct DrawCounters
    {
        size_t drawnCount{ 0 };
        size_t successfulClearCount{ 0 };
        size_t failedClearCount{ 0 };

        void Reset() noexcept
        {
            std::memset(static_cast<void*>(this), 0, sizeof(*this));
        }
    };

    static_assert(std::is_trivially_copyable_v<DrawCounters>);

    struct Stats
    {
        ScanCounters scan;
        DrawCounters draw;
        size_t cachedLights{ 0 };
        size_t registeredRuntimeLights{ 0 };
    };

    struct DrawState
    {
        DrawCounters counters;
        bool summaryPending{ true };
        bool reportedClearFailure{ false };
    };

    struct EditorState
    {
        std::string nodeName;
        std::string objectName;
        std::string baseEditorID;
        std::string associatedLightEditorID;
        std::string associatedLightName;
        std::string runtimeLightNodeName;
        RE::FormID ownerFormID{ 0 };
        RE::FormID associatedLightRefID{ 0 };
        RE::NiColorA color{ 1.0F, 1.0F, 1.0F, 1.0F };
        RE::NiPoint3 localPosition;
        float intensity{ 1.0F };
        float radius{ 0.0F };
        bool enabled{ true };
        ParticleCategory category{ ParticleCategory::kUnclassified };
    };

    struct EditKey
    {
        RE::FormID referenceFormID{ 0 };
        size_t particleOrdinal{ 0 };

        bool operator==(const EditKey&) const = default;
    };

    struct EditKeyHash
    {
        inline size_t operator()(const EditKey& a_key) const
        {
            const auto referenceHash = std::hash<RE::FormID>{}(a_key.referenceFormID);
            const auto ordinalHash = std::hash<size_t>{}(a_key.particleOrdinal);
            return referenceHash ^ (ordinalHash + 0x9E3779B9 + (referenceHash << 6) + (referenceHash >> 2));
        }
    };

    struct ParticleDefault
    {
        RE::NiTransform local;
        RE::NiColorA color{ 1.0F, 1.0F, 1.0F, 1.0F };
        RE::NiColorA materialColor{ 1.0F, 1.0F, 1.0F, 1.0F };
        float intensity{ 1.0F };
        float radius{ 0.0F };
        bool hasMaterial{ false };
        RE::NiColorA vertexColor{ 1.0F, 1.0F, 1.0F, 1.0F };
        bool usesVertexColors{ false };
        bool enabled{ true };
    };

    struct Edit
    {
        ParticleDefault defaults;

        RE::NiColorA color{ 1.0F, 1.0F, 1.0F, 1.0F };
        RE::NiPoint3 localPosition;
        float intensity{ 1.0F };
        float radius{ 0.0F };
        bool enabled{ true };
        FOREACH_EDIT_PROPERTY(EDIT_CHANGED_DEFINE)
        bool initialized{ false };
    };

    inline bool HasChanges(const Edit& a_edit) { return FOREACH_EDIT_PROPERTY(EDIT_CHANGED_CHECK) false; }

    using EditMap = std::unordered_map<EditKey, Edit, EditKeyHash>;

    struct Entry
    {
        RE::NiPointer<RE::BSGeometry> geometry;
        RE::NiPointer<RE::NiPointLight> runtimeLight;
        RE::FormID ownerFormID{ 0 };
        RE::FormID baseFormID{ 0 };
        RE::FormID associatedLightRefID{ 0 };
        RE::FormID associatedLightBaseID{ 0 };
        RE::FormID cellFormID{ 0 };
        std::string nodeName;
        std::string baseEditorID;
        std::string baseName;
        std::string modelPath;
        std::string categoryKey;
        std::string associatedLightEditorID;
        std::string associatedLightName;
        std::string runtimeLightNodeName;
        size_t particleOrdinal{ 1 };
        bool validatedByName{ false };
        bool runtimeAttachment{ false };
        ParticleDefault defaults;
        RE::NiColorA currentColor{ 1.0F, 1.0F, 1.0F, 1.0F };
        ParticleCategory category{ ParticleCategory::kUnclassified };
        RE::BSEffectShaderMaterial* editableMaterial{ nullptr };
        EditKey editKey;
    };

    struct Candidate
    {
        Entry entry;
        RE::NiPoint3 center;
        float radius{ 0.0F };
        std::string nodeName;
        uint16_t triangleCount{ 0 };
        uint16_t vertexCount{ 0 };
        size_t baseParticleOrdinal{ 0 };
        bool directLightOwner{ false };
    };

    struct Source
    {
        RE::TESObjectREFR* reference{ nullptr };
        RE::TESObjectLIGH* base{ nullptr };
        RE::NiPoint3 position;
        float radius{ 0.0F };
        bool claimed{ false };
    };

    struct MatchEdge
    {
        size_t candidateIndex{ 0 };
        size_t sourceIndex{ 0 };
        float distance{ 0.0F };
        float score{ 0.0F };
    };

    struct CellReferenceCollector
    {
        std::vector<RE::TESObjectREFR*>& sources;
        std::vector<RE::TESObjectREFR*>& meshes;
        size_t& referenceCount;
        size_t& sourceCount;

        inline RE::BSContainer::ForEachResult operator()(RE::TESObjectREFR* a_reference) const
        {
            if (!a_reference) {
                return RE::BSContainer::ForEachResult::kContinue;
            }

            ++referenceCount;
            const auto* base = a_reference->GetBaseObject();
            if (base && base->Is(RE::FormType::Light)) {
                ++sourceCount;
                sources.push_back(a_reference);
            }
            else {
                meshes.push_back(a_reference);
            }
            return RE::BSContainer::ForEachResult::kContinue;
        }
    };

    inline constexpr uint16_t kParticleLightAlphaFlags = 4109;
    inline constexpr uint16_t kParticleLightTriangleCount = 8;
    inline constexpr uint16_t kParticleLightVertexCount = 9;
    inline constexpr uint16_t kComplexParticleLightTriangleCount = 128;
    inline constexpr uint16_t kComplexParticleLightVertexCount = 81;
    inline constexpr auto kCanvasName = "ParticleLightEditorCanvas";
    inline constexpr RE::NiColorA kDefaultDrawColor{ 1.0F, 0.65F, 0.1F, 0.9F };
}

#undef EDIT_CHANGED_DEFINE
#undef EDIT_CHANGED_CHECK
