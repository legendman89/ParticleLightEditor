#include "scanner.hpp"

#include "animation.hpp"

#include "category.hpp"
#include "detector.hpp"
#include "logger.hpp"
#include "registry.hpp"
#include "settings.hpp"
#include "trace.hpp"
#include "utility.hpp"
#include "vertices.hpp"

#include <ClibUtil/EditorID.hpp>

namespace ParticleLightEditor
{

    void FindTorchBranches(RE::NiAVObject* a_object, std::vector<RE::NiAVObject*>& a_branches)
    {
        if (!a_object) {
            return;
        }

        const auto name = Category::Lowercase(a_object->name.empty() ? "" : a_object->name.c_str());
        if (name.contains("torch")) {
            a_branches.push_back(a_object);
            return;
        }

        if (auto* node = a_object->AsNode()) {
            for (auto& child : node->GetChildren()) {
                if (child && child->AsNode()) {
                    FindTorchBranches(child.get(), a_branches);
                }
            }
        }
    }

    RE::TESObjectLIGH* Scanner::GetEquippedLight(RE::PlayerCharacter* a_player) const
    {
        if (!a_player) {
            return nullptr;
        }
        for (const auto leftHand : { true, false }) {
            auto* equipped = a_player->GetEquippedObject(leftHand);
            auto* light = equipped ? equipped->As<RE::TESObjectLIGH>() : nullptr;
            if (light) {
                return light;
            }
        }
        return nullptr;
    }

    void Scanner::VisitEquippedLight(RE::PlayerCharacter* a_player, std::unordered_set<RE::NiAVObject*>& a_visited)
    {
        auto* equippedLight = GetEquippedLight(a_player);
        if (!a_player || !equippedLight) {
            return;
        }

        std::unordered_set<RE::NiAVObject*> roots;
        const auto candidateCountBefore = candidates.size();
        const auto countersBefore = counters;
        std::vector<std::string> branchNames;
        for (const auto firstPerson : { false, true }) {
            auto* root = a_player->Get3D(firstPerson);
            if (!root || !roots.insert(root).second) {
                continue;
            }

            std::vector<RE::NiAVObject*> torchBranches;
            std::unordered_set<RE::NiAVObject*> uniqueBranches;
            
            // Based on MLO2 lookup.
            for (const auto* branchName : { "AttachLight", "TorchFire", "Torch" }) {
                if (auto* branch = root->GetObjectByName(RE::BSFixedString(branchName))) {
                    if (uniqueBranches.insert(branch).second) {
                        torchBranches.push_back(branch);
                    }
                }
            }
            if (torchBranches.empty()) {
                FindTorchBranches(root, torchBranches);
            }
            for (auto* branch : torchBranches) {
                branchNames.emplace_back(branch->name.empty() ? "Unnamed" : branch->name.c_str());
                VisitNodeBranch(branch, a_player, a_visited, false, false, false, equippedLight, true);
            }
        }

        logger::info(
            "Equipped carried light {:08X}: branches=[{}], playerRoots={}, visitedBillboards={}, "
            "visitedTriShapes={}, collectedCandidates={}, rejectedGlow={}, rejectedShader={}, rejectedTopology={}",
            equippedLight->GetFormID(),
            fmt::join(branchNames, ", "),
            roots.size(),
            counters.billboardCount - countersBefore.billboardCount,
            counters.triShapeCount - countersBefore.triShapeCount,
            candidates.size() - candidateCountBefore,
            counters.rejectedGlowCount - countersBefore.rejectedGlowCount,
            counters.rejectedShaderCount - countersBefore.rejectedShaderCount,
            counters.rejectedTopologyCount - countersBefore.rejectedTopologyCount);
    }

    void Scanner::Refresh(RE::TESObjectCELL* a_cell, RE::PlayerCharacter* a_player)
    {
        const auto previousCount = entries.size();
        const auto previousCell = cell;

        for (auto& entry : entries) {
            const auto* edit = FindEdit(entry);
            if (edit && (edit->animationChanged || entry.animationApplied)) {
                Animation::RestoreDefault(entry, *edit);
            }
        }
        entries.clear();
        candidates.clear();
        editorIndices.clear();
        drawState.summaryPending = true;
        rescanRequested = false;
        scanElapsed = 0.0F;
        rescanDelay = 0.0F;
        counters.Reset();

        if (!a_cell || !a_cell->IsAttached()) {
            // Keep the cache unset so Update retries after Skyrim finishes
            // attaching the new cell's 3D. No scene-graph traversal occurs here.
            cell = nullptr;
            if (previousCount != 0) {
                logger::info("No attached player cell; cleared {} detected particle lights", previousCount);
            }
            return;
        }

        cell = a_cell;
        std::vector<RE::TESObjectREFR*> lightReferences;
        std::vector<RE::TESObjectREFR*> meshReferences;
        a_cell->ForEachReference(CellReferenceCollector{
            lightReferences, meshReferences, counters.referenceCount, counters.sourceCount });

        std::unordered_set<RE::FormID> currentLightReferenceIDs;
        currentLightReferenceIDs.reserve(lightReferences.size());
        for (const auto* reference : lightReferences) {
            currentLightReferenceIDs.insert(reference->GetFormID());
        }
        Registry::GetSingleton().RetainReferences(currentLightReferenceIDs);

        // My idea here is to collect light sources and their refs first
        // so that we can match particle lights to them next.
        std::unordered_set<RE::NiAVObject*> visited;
        for (auto* reference : lightReferences) {
            VisitReference(reference, visited, true);
        }

        // A held torch is nested in the player actor graph. Visit it with the
        // equipped LIGH form as its identity before any ordinary actor traversal.
        VisitEquippedLight(a_player, visited);

        // Traverse the remaining mesh references and collect particle-light candidates.
        for (auto* reference : meshReferences) {
            if (reference == a_player) {
                continue;
            }
            VisitReference(reference, visited, false);
        }

        MatchCandidates(lightReferences);

        if (previousCell != cell || previousCount != entries.size()) {
            logger::info(
                "Detected {} particle light candidate(s) in cell {:08X}; "
                "found {} placed light reference(s) among {} cell reference(s); "
                "visited {} billboard(s) and {} direct tri-shape child(ren), "
                "collected {} named and {} structural candidate(s), matched {} to light sources, "
                "left {} structural candidate(s) unmatched; rejected {} glow-named candidate(s), "
                "{} shader/alpha candidate(s) and {} topology candidate(s); "
                "scanned {} light-owned BSFadeNode root(s) plus {} mesh root(s), "
                "ignored {} non-BSFadeNode root(s)",
                entries.size(),
                a_cell->GetFormID(),
                counters.sourceCount,
                counters.referenceCount,
                counters.billboardCount,
                counters.triShapeCount,
                counters.nameValidatedCount,
                counters.structuralCandidateCount,
                counters.sourceMatchCount,
                counters.unmatchedStructuralCount,
                counters.rejectedGlowCount,
                counters.rejectedShaderCount,
                counters.rejectedTopologyCount,
                counters.sourceRootCount,
                counters.meshRootCount,
                counters.rejectedRootCount);
        }
    }

    void Scanner::VisitReference(RE::TESObjectREFR* a_reference, std::unordered_set<RE::NiAVObject*>& a_visited, bool a_directLightOwner)
    {
        auto* root = a_reference ? a_reference->Get3D() : nullptr;
        if (root && root->AsFadeNode()) {
            ++counters.rootCount;
            if (a_directLightOwner) {
                ++counters.sourceRootCount;
            }
            else {
                ++counters.meshRootCount;
            }
            VisitNodeBranch(root, a_reference, a_visited, false, false, a_directLightOwner);
        }
        else if (root) {
            ++counters.rejectedRootCount;
        }
    }

    void Scanner::VisitNodeBranch(
        RE::NiAVObject* a_object,
        RE::TESObjectREFR* a_owner,
        std::unordered_set<RE::NiAVObject*>& a_visited,
        bool a_nameValidated,
        bool a_glowBranch,
        bool a_directLightOwner,
        RE::TESForm* a_baseOverride,
        bool a_runtimeAttachment)
    {
        if (!a_object || !a_visited.insert(a_object).second) {
            return;
        }

        const auto nameValidated = a_nameValidated || Utility::HasParticleLightName(a_object);
        const auto glowBranch = a_glowBranch || Utility::HasGlowName(a_object);

        if (auto* billboard = netimmerse_cast<RE::NiBillboardNode*>(a_object)) {
            ++counters.billboardCount;
            VisitBillboard(billboard, a_owner, nameValidated, glowBranch, a_directLightOwner, a_baseOverride, a_runtimeAttachment);
            return;
        }

        if (auto* node = a_object->AsNode()) {
            for (auto& child : node->GetChildren()) {
                if (child && child->AsNode()) {
                    VisitNodeBranch(
                        child.get(),
                        a_owner,
                        a_visited,
                        nameValidated,
                        glowBranch,
                        a_directLightOwner,
                        a_baseOverride,
                        a_runtimeAttachment);
                }
            }
        }
    }

    void Scanner::VisitBillboard(
        RE::NiBillboardNode* a_billboard,
        RE::TESObjectREFR* a_owner,
        bool a_nameValidated,
        bool a_glowBranch,
        bool a_directLightOwner,
        RE::TESForm* a_baseOverride,
        bool a_runtimeAttachment)
    {
        if (!a_billboard) {
            return;
        }

        const auto& settings = Settings::GetSettings();
        if (a_glowBranch && !a_nameValidated && !settings.includeGlowNodes) {
            ++counters.rejectedGlowCount;
            return;
        }

        for (auto& child : a_billboard->GetChildren()) {
            auto* triShape = child ? child->AsTriShape() : nullptr;
            if (!triShape) {
                continue;
            }

            ++counters.triShapeCount;

            const auto validatedByName = a_nameValidated || Utility::HasParticleLightName(triShape);

            // Required order: BSTriShape -> BSEffectShaderProperty -> topology.
            auto* shader = Utility::GetEffectShader(*triShape);
            if (!shader) {
                ++counters.rejectedShaderCount;
                continue;
            }

            if (!validatedByName && !Detector::HasStructure(*triShape, *shader)) {
                ++counters.rejectedShaderCount;
                continue;
            }

            if (!validatedByName && !Detector::HasParticleTopology(*triShape)) {
                ++counters.rejectedTopologyCount;
                continue;
            }

            const auto& topology = triShape->GetTrishapeRuntimeData();

            const auto ownerFormID = a_owner ? a_owner->GetFormID() : 0;
            auto* base = a_baseOverride ? a_baseOverride : (a_owner ? a_owner->GetBaseObject() : nullptr);
            const auto baseFormID = base ? base->GetFormID() : 0;
            auto baseEditorID = base ? clib_util::editorID::get_editorID(base) : std::string{};
            if (baseEditorID.empty()) {
                const auto* editorID = base ? base->GetFormEditorID() : nullptr;
                baseEditorID = editorID && editorID[0] != '\0' ? editorID : "Unavailable";
            }
            const auto* objectName = base ? base->GetName() : nullptr;
            const std::string baseName = objectName && objectName[0] != '\0' ? objectName : "Unnamed";
            const auto* model = base ? base->As<RE::TESModel>() : nullptr;
            const auto* modelName = model ? model->GetModel() : nullptr;
            const std::string modelPath = modelName && modelName[0] != '\0' ? modelName : "";
            const auto* nodeName = triShape->name.empty() ? "Unnamed" : triShape->name.c_str();

            const auto radius = triShape->worldBound.radius * 0.5F;
            if (!std::isfinite(radius) || radius <= 0.0F) {
                continue;
            }

            Entry entry;
            entry.geometry = RE::NiPointer<RE::BSGeometry>(triShape);
            entry.ownerFormID = ownerFormID;
            entry.baseFormID = baseFormID;
            entry.cellFormID = a_owner && a_owner->GetParentCell() ? a_owner->GetParentCell()->GetFormID() : 0;
            entry.nodeName = nodeName;
            entry.baseEditorID = baseEditorID;
            entry.baseName = baseName;
            entry.modelPath = modelPath;
            entry.categoryKey = Category::Lowercase(!modelPath.empty() ? modelPath : baseEditorID);
            entry.category = Category::Resolve(entry, categoryOverrides);
            entry.validatedByName = validatedByName;
            entry.runtimeAttachment = a_runtimeAttachment;
            entry.defaults.local = triShape->local;
            entry.defaults.radius = radius;
            entry.defaults.enabled = !triShape->GetAppCulled();
            if (auto* material = shader->GetMaterial()) {
                entry.defaults.color = material->baseColor;
                entry.defaults.materialColor = material->baseColor;
                entry.defaults.intensity = material->baseColorScale;
                entry.defaults.hasMaterial = true;
                const auto vertexColor = Vertices::ReadVertexColor(*triShape);
                if (vertexColor.valid && Vertices::IsBlackOrWhite(material->baseColor)) {
                    entry.defaults.color = vertexColor.color;
                    entry.defaults.vertexColor = vertexColor.color;
                    entry.defaults.usesVertexColors = true;
                }
                entry.currentColor = entry.defaults.color;
            }
            Animation::CaptureDefault(entry);

            candidates.push_back({
                std::move(entry),
                triShape->worldBound.center,
                radius,
                nodeName,
                topology.triangleCount,
                topology.vertexCount,
                0,
                a_directLightOwner
            });

            if (validatedByName) {
                ++counters.nameValidatedCount;
            }
            else {
                ++counters.structuralCandidateCount;
            }
        }
    }

    void Scanner::MatchCandidates(const std::vector<RE::TESObjectREFR*>& a_lightReferences)
    {
        std::unordered_map<RE::FormID, size_t> baseParticleOrdinals;
        for (auto& candidate : candidates) {
            candidate.baseParticleOrdinal = ++baseParticleOrdinals[candidate.entry.ownerFormID];
        }

        std::vector<Source> sources;
        sources.reserve(a_lightReferences.size());
        for (auto* reference : a_lightReferences) {
            auto* baseObject = reference ? reference->GetBaseObject() : nullptr;
            auto* light = baseObject ? baseObject->As<RE::TESObjectLIGH>() : nullptr;
            if (!reference || !light) {
                continue;
            }

            sources.push_back({
                reference,
                light,
                reference->GetPosition(),
                static_cast<float>(light->data.radius),
                false
            });
        }

        const auto& settings = Settings::GetSettings();
        const auto associationRange = std::max(1.0F, settings.associationRange);
        const auto associationRangeSquared = associationRange * associationRange;
        const auto radiusWeight = std::max(0.0F, settings.radiusMatchWeight);
        const auto noMatch = std::numeric_limits<size_t>::max();
        std::vector<size_t> associatedSources(candidates.size(), noMatch);
        std::vector<bool> candidateClaimed(candidates.size(), false);

        // Explicitly named particle lights are conclusive. Pair them first.
        for (const auto& edge : BuildMatchEdges(sources, candidateClaimed, true, associationRangeSquared, radiusWeight)) {
            if (candidateClaimed[edge.candidateIndex] || sources[edge.sourceIndex].claimed) {
                continue;
            }
            candidateClaimed[edge.candidateIndex] = true;
            sources[edge.sourceIndex].claimed = true;
            associatedSources[edge.candidateIndex] = edge.sourceIndex;
        }

        // If a particle candidate is directly inside a LIGH reference, reserve its
        // own source before considering proximity matches from separate meshes.
        std::unordered_map<RE::FormID, size_t> particleOrdinals;
        for (size_t candidateIndex = 0; candidateIndex < candidates.size(); ++candidateIndex) {
            const auto& candidate = candidates[candidateIndex];
            if (candidate.entry.validatedByName || !candidate.directLightOwner || candidateClaimed[candidateIndex]) {
                continue;
            }

            for (size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex) {
                auto& source = sources[sourceIndex];
                if (!source.claimed && source.reference->GetFormID() == candidate.entry.ownerFormID) {
                    candidateClaimed[candidateIndex] = true;
                    source.claimed = true;
                    associatedSources[candidateIndex] = sourceIndex;
                    ++counters.sourceMatchCount;
                    break;
                }
            }
        }

        // Remaining unnamed candidates compete for remaining lights.
        // We assume one particle candidate per light and one light per candidate.
        for (const auto& edge : BuildMatchEdges(sources, candidateClaimed, false, associationRangeSquared, radiusWeight)) {
            if (candidateClaimed[edge.candidateIndex] || sources[edge.sourceIndex].claimed) {
                continue;
            }
            candidateClaimed[edge.candidateIndex] = true;
            sources[edge.sourceIndex].claimed = true;
            associatedSources[edge.candidateIndex] = edge.sourceIndex;
            ++counters.sourceMatchCount;
        }

        std::vector<size_t> validatedParticles;
        validatedParticles.reserve(candidates.size());
        for (size_t candidateIndex = 0; candidateIndex < candidates.size(); ++candidateIndex) {
            if (candidates[candidateIndex].entry.validatedByName || associatedSources[candidateIndex] != noMatch) {
                validatedParticles.push_back(candidateIndex);
            }
        }

        // I noticed this case on candles often where named candidates remain conclusive 
        // without a nearby placed light. Solution is to find a structural one confirmed
        // on another instance of the same base NIF.
        for (size_t candidateIndex = 0; candidateIndex < candidates.size(); ++candidateIndex) {
            const auto& candidate = candidates[candidateIndex];
            const auto sourceIndex = associatedSources[candidateIndex];
            const auto hasAssociation = sourceIndex != noMatch;
            const auto* source = hasAssociation ? &sources[sourceIndex] : nullptr;
            auto baseParticleValidated = false;
            if (!candidate.entry.validatedByName && !hasAssociation) {
                for (const auto validatedIndex : validatedParticles) {
                    if (Detector::IsSameBaseParticle(candidate, candidates[validatedIndex])) {
                        baseParticleValidated = true;
                        break;
                    }
                }
            }

           
            if (!candidate.entry.validatedByName && !hasAssociation && !baseParticleValidated && !candidate.entry.runtimeAttachment) {
                ++counters.unmatchedStructuralCount;
                continue;
            }

            const auto* associatedLightName = source ? source->base->GetName() : nullptr;
            std::optional<Registry::Record> runtimeRecord;
            if (source) {
                runtimeRecord = Registry::GetSingleton().FindByReference(source->reference->GetFormID());
            }
            const auto resolvedEditorID = runtimeRecord && !runtimeRecord->editorID.empty() ? runtimeRecord->editorID : (source ? Registry::ResolveEditorID(source->base) : std::string{});
            const auto resolvedDisplayName = runtimeRecord ? runtimeRecord->displayName : (source ? Registry::ResolveDisplayName(source->base) : std::string{});
            auto entry = candidate.entry;
            if (runtimeRecord) {
                entry.runtimeLight = runtimeRecord->light;
            }
            entry.associatedLightRefID = source ? source->reference->GetFormID() : 0;
            entry.associatedLightBaseID = source ? source->base->GetFormID() : 0;
            entry.associatedLightEditorID = !resolvedEditorID.empty() ? resolvedEditorID : "Unavailable";
            entry.associatedLightName = associatedLightName && associatedLightName[0] != '\0' ? associatedLightName : (!resolvedDisplayName.empty() ? resolvedDisplayName : "Unnamed");
            entry.runtimeLightNodeName = runtimeRecord ? runtimeRecord->runtimeNodeName : "Not Captured";

            const auto sourceIdentity = entry.runtimeAttachment ? entry.baseFormID :
                (entry.associatedLightRefID != 0 ? entry.associatedLightRefID : (entry.ownerFormID != 0 ? entry.ownerFormID : entry.baseFormID));

            entry.particleOrdinal = ++particleOrdinals[sourceIdentity];

            SetParticleLightEdit(entry);
            entries.push_back(std::move(entry));
            Trace::Candidate(entries.back(), candidate, source);
        }

    }

    std::vector<MatchEdge> Scanner::BuildMatchEdges(const std::vector<Source>& a_sources, const std::vector<bool>& a_candidateClaimed,
        bool a_namedCandidates, float a_associationRangeSquared, float a_radiusWeight) const
    {
        std::vector<MatchEdge> edges;
        for (size_t candidateIndex = 0; candidateIndex < candidates.size(); ++candidateIndex) {
            const auto& candidate = candidates[candidateIndex];
            if (candidate.entry.validatedByName != a_namedCandidates || a_candidateClaimed[candidateIndex]) {
                continue;
            }
            if (candidate.entry.runtimeAttachment) {
                continue;
            }

            // A candidate found beneath a placed LIGH reference may only claim that reference.
            // Do not let a failed direct association consume an unrelated nearby light.
            if (!a_namedCandidates && candidate.directLightOwner) {
                continue;
            }

            for (size_t sourceIndex = 0; sourceIndex < a_sources.size(); ++sourceIndex) {
                const auto& source = a_sources[sourceIndex];
                if (source.claimed) {
                    continue;
                }

                const auto squaredDistance = Utility::DistanceSquared(candidate.center, source.position);
                if (!std::isfinite(squaredDistance) || squaredDistance > a_associationRangeSquared) {
                    continue;
                }

                const auto distance = std::sqrt(squaredDistance);
                const auto relativeRadiusDifference = std::abs(candidate.radius - source.radius) / std::max(1.0F, source.radius);
                edges.push_back({ candidateIndex, sourceIndex, distance, distance + relativeRadiusDifference * a_radiusWeight });
            }
        }

        std::ranges::sort(edges, {}, &MatchEdge::score);
        return edges;
    }

}
