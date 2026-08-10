#include "trace.hpp"

#include "category.hpp"
#include "logger.hpp"
#include "utility.hpp"

namespace ParticleLightEditor::Trace
{
    void Candidate(const Entry& a_entry, const ParticleLightEditor::Candidate& a_candidate, const Source* a_source)
    {
        const auto hasAssociation = a_source != nullptr;
        const auto associationDistance = a_source ? std::sqrt(Utility::DistanceSquared(a_candidate.center, a_source->position)) : -1.0F;
        const auto validation = a_entry.runtimeAttachment ? "Equipped Light" : a_candidate.entry.validatedByName ?
            (hasAssociation ? "Node Name + Nearby Light" : "Node Name") :
            (hasAssociation ? (a_candidate.directLightOwner ? "Owned by Light Reference" : "Matched to Nearby Light") : "Matching Base Particle");
        auto* geometry = a_entry.geometry.get();
        auto* shader = geometry ? Utility::GetEffectShader(*geometry) : nullptr;
        auto* material = shader ? shader->GetMaterial() : nullptr;
        auto* renderer = geometry ? geometry->GetGeometryRuntimeData().rendererData : nullptr;
        logger::trace(
            "Particle light candidate:\n"
            "  Field                       | Value\n"
            "  --------------------------- | ----------------------------------------\n"
            "  Node                        | {}\n"
            "  Validation                  | {}\n"
            "  Object                      | {}\n"
            "  Category                    | {}\n"
            "  Model path                  | {}\n"
            "  Runtime attachment          | {}\n"
            "  Triangles                   | {}\n"
            "  Vertices                    | {}\n"
            "  Owner reference             | {:08X}\n"
            "  Base form                   | {:08X}\n"
            "  Associated light reference  | {:08X}\n"
            "  Associated light base       | {:08X}\n"
            "  Association distance        | {:.2f}\n"
            "  Particle radius             | {:.2f}\n"
            "  Light radius                | {:.2f}\n"
            "  Base Editor ID              | {}\n"
            "  Associated Editor ID        | {}\n"
            "  Associated light            | {}\n"
            "  Color source                | {}\n"
            "  Material color              | ({:.3f}, {:.3f}, {:.3f}, {:.3f})\n"
            "  Effective color             | ({:.3f}, {:.3f}, {:.3f}, {:.3f})\n"
            "  Geometry                    | {:p}\n"
            "  Shader                      | {:p}\n"
            "  Material                    | {:p}\n"
            "  Renderer                    | {:p}\n"
            "  Raw vertex data             | {:p}\n"
            "  Renderer references         | {}\n"
            "  Runtime NiLight             | {:p}\n"
            "  Runtime node                | {}",
            a_entry.nodeName,
            validation,
            a_entry.baseName,
            Category::Name(a_entry.category),
            a_entry.modelPath.empty() ? "Unavailable" : a_entry.modelPath,
            a_entry.runtimeAttachment,
            a_candidate.triangleCount,
            a_candidate.vertexCount,
            a_entry.ownerFormID,
            a_entry.baseFormID,
            a_entry.associatedLightRefID,
            a_entry.associatedLightBaseID,
            associationDistance,
            a_candidate.radius,
            a_source ? a_source->radius : 0.0F,
            a_entry.baseEditorID,
            a_entry.associatedLightEditorID,
            a_entry.associatedLightName,
            a_entry.defaults.usesVertexColors ? "vertex" : "material",
            a_entry.defaults.materialColor.red,
            a_entry.defaults.materialColor.green,
            a_entry.defaults.materialColor.blue,
            a_entry.defaults.materialColor.alpha,
            a_entry.defaults.color.red,
            a_entry.defaults.color.green,
            a_entry.defaults.color.blue,
            a_entry.defaults.color.alpha,
            static_cast<void*>(geometry),
            static_cast<void*>(shader),
            static_cast<void*>(material),
            static_cast<void*>(renderer),
            renderer ? static_cast<void*>(renderer->rawVertexData) : nullptr,
            renderer ? renderer->refCount : 0,
            static_cast<void*>(a_entry.runtimeLight.get()),
            a_entry.runtimeLightNodeName);
    }
}
