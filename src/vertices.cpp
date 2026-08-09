#include "vertices.hpp"

#include "logger.hpp"
#include "utility.hpp"

#include "RE/R/Renderer.h"
#include "REX/W32/D3D11.h"
#include "REX/W32/KERNEL32.h"

#include <cstring>

#ifdef InterlockedDecrement
#undef InterlockedDecrement
#endif

namespace ParticleLightEditor::Vertices
{

    // Creates an instance-local GPU vertex buffer from Skyrim's host vertex data.
    RE::ID3D11Buffer* Manager::CreateVertexBuffer(RE::BSGraphics::TriShape& a_renderer, size_t a_vertexBytes)
    {
        auto* oldBuffer = reinterpret_cast<REX::W32::ID3D11Buffer*>(a_renderer.vertexBuffer);
        auto* device = RE::BSGraphics::Renderer::GetDevice(); 
        if (!oldBuffer || !device || !a_renderer.rawVertexData || a_vertexBytes == 0) {
            return nullptr;
        }

        REX::W32::D3D11_BUFFER_DESC description{};
        oldBuffer->GetDesc(&description);
        if (description.byteWidth < a_vertexBytes) {
            return nullptr;
        }

        std::vector<uint8_t> initialBytes;
        const void* source = a_renderer.rawVertexData;
        if (description.byteWidth > a_vertexBytes) {
            initialBytes.resize(description.byteWidth);
            std::memcpy(initialBytes.data(), a_renderer.rawVertexData, a_vertexBytes);
            source = initialBytes.data();
        }

        REX::W32::D3D11_SUBRESOURCE_DATA initialData{};
        initialData.sysMem = source;
        REX::W32::ID3D11Buffer* buffer = nullptr;
        return device->CreateBuffer(&description, &initialData, &buffer) >= 0 ? reinterpret_cast<RE::ID3D11Buffer*>(buffer) : nullptr;
    }

    // Prepares instance-local renderer data before editing vertex colors. Inspired by Open Shaders.
    bool Manager::PrepareLocalRenderer(RE::BSTriShape& a_geometry, RE::FormID a_ownerFormID)
    {
        auto& runtime = a_geometry.GetGeometryRuntimeData();
        auto* renderer = runtime.rendererData;
        if (!renderer || !renderer->rawVertexData || !renderer->vertexDesc.HasFlag(RE::BSGraphics::Vertex::Flags::VF_COLORS)) {
            return false;
        }

        const auto vertexSize = renderer->vertexDesc.GetSize();
        const auto vertexCount = a_geometry.GetTrishapeRuntimeData().vertexCount;
        const auto vertexBytes = static_cast<size_t>(vertexSize) * vertexCount;
        if (vertexSize == 0 || vertexCount == 0 || vertexBytes == 0) {
            return false;
        }

        if (const auto found = localRenderers.find(&a_geometry); found != localRenderers.end()) {
            if (found->second.renderer == renderer && found->second.ownerFormID == a_ownerFormID) {
                return true;
            }
            localRenderers.erase(found);
        }

        if (renderer->refCount > 1) {
            const auto previousRefCount = renderer->refCount;
            auto* clone = RE::malloc<RE::BSGraphics::TriShape>();
            auto* vertexData = RE::malloc<uint8_t>(vertexBytes);
            const auto indexCount = static_cast<size_t>(a_geometry.GetTrishapeRuntimeData().triangleCount) * 3;
            auto* indexData = renderer->rawIndexData && indexCount > 0 ? RE::malloc<uint16_t>(indexCount * sizeof(uint16_t)) : nullptr;
            if (!clone || !vertexData || (renderer->rawIndexData && indexCount > 0 && !indexData)) {
                if (indexData) {
                    RE::free(indexData);
                }
                if (vertexData) {
                    RE::free(vertexData);
                }
                if (clone) {
                    RE::free(clone);
                }
                logger::warn("Could not isolate vertex data for particle light {:08X}", a_ownerFormID);
                return false;
            }

            std::memcpy(vertexData, renderer->rawVertexData, vertexBytes);
            if (indexData) {
                std::memcpy(indexData, renderer->rawIndexData, indexCount * sizeof(uint16_t));
            }
            std::memcpy(clone, renderer, sizeof(RE::BSGraphics::TriShape));
            clone->refCount = 1;
            clone->rawVertexData = vertexData;
            clone->rawIndexData = indexData;

            auto* oldVertexBuffer = renderer->vertexBuffer;
            auto* vertexBuffer = oldVertexBuffer ? CreateVertexBuffer(*clone, vertexBytes) : nullptr;
            if (oldVertexBuffer && !vertexBuffer) {
                if (indexData) {
                    RE::free(indexData);
                }
                RE::free(vertexData);
                RE::free(clone);
                logger::warn("Could not isolate the vertex buffer for particle light {:08X}", a_ownerFormID);
                return false;
            }

            clone->vertexBuffer = vertexBuffer;
            if (auto* indexBuffer = reinterpret_cast<REX::W32::ID3D11Buffer*>(clone->indexBuffer)) {
                indexBuffer->AddRef();
            }

            REX::W32::InterlockedDecrement(&renderer->refCount);
            runtime.rendererData = clone;
            renderer = clone;
            logger::info("Isolated shared vertex data for particle light {:08X}; previous renderer had {} references", a_ownerFormID, previousRefCount);
        }

        LocalRenderer localRenderer;
        localRenderer.renderer = renderer;
        localRenderer.ownerFormID = a_ownerFormID;
        localRenderer.data.assign(renderer->rawVertexData, renderer->rawVertexData + vertexBytes);
        localRenderers.emplace(&a_geometry, std::move(localRenderer));
        return true;
    }

    bool Manager::RefreshBuffer(RE::BSTriShape& a_geometry)
    {
        auto* renderer = a_geometry.GetGeometryRuntimeData().rendererData;
        auto* oldBuffer = renderer ? reinterpret_cast<REX::W32::ID3D11Buffer*>(renderer->vertexBuffer) : nullptr;
        if (!renderer || !oldBuffer) {
            return renderer && !oldBuffer;
        }

        const auto vertexBytes = static_cast<size_t>(renderer->vertexDesc.GetSize()) * a_geometry.GetTrishapeRuntimeData().vertexCount;
        auto* buffer = CreateVertexBuffer(*renderer, vertexBytes);
        if (!buffer) {
            return false;
        }

        renderer->vertexBuffer = buffer;
        oldBuffer->Release();
        return true;
    }

    void Manager::RefreshGeometry(RE::BSTriShape& a_geometry)
    {
        a_geometry.SetMaterialNeedsUpdate(true);
        if (auto* shader = Utility::GetEffectShader(a_geometry)) {
            shader->DoClearRenderPasses();
            shader->SetupGeometry(&a_geometry);
        }
    }

    bool Manager::Apply(RE::BSTriShape& a_geometry, const RE::NiColorA& a_color, RE::FormID a_ownerFormID)
    {
        std::unique_lock lock(mutex);
        if (!PrepareLocalRenderer(a_geometry, a_ownerFormID)) {
            return false;
        }

        auto* renderer = a_geometry.GetGeometryRuntimeData().rendererData;
        const auto vertexSize = renderer->vertexDesc.GetSize();
        const auto colorOffset = renderer->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::Attribute::VA_COLOR);
        const auto vertexCount = a_geometry.GetTrishapeRuntimeData().vertexCount;
        const auto localRenderer = localRenderers.find(&a_geometry);
        if (vertexSize < sizeof(PackedColor) || colorOffset > vertexSize - sizeof(PackedColor) || localRenderer == localRenderers.end()) {
            return false;
        }

        const auto red = static_cast<uint8_t>(std::lround(std::clamp(a_color.red, 0.0F, 1.0F) * 255.0F));
        const auto green = static_cast<uint8_t>(std::lround(std::clamp(a_color.green, 0.0F, 1.0F) * 255.0F));
        const auto blue = static_cast<uint8_t>(std::lround(std::clamp(a_color.blue, 0.0F, 1.0F) * 255.0F));
        auto changed = false;
        for (uint16_t index = 0; index < vertexCount; ++index) {
            const auto* original = reinterpret_cast<const PackedColor*>(localRenderer->second.data.data() + static_cast<size_t>(vertexSize) * index + colorOffset);
            auto* color = reinterpret_cast<PackedColor*>(renderer->rawVertexData + static_cast<size_t>(vertexSize) * index + colorOffset);
            const auto white = original->red >= 254 && original->green >= 254 && original->blue >= 254;
            const auto black = original->red <= 1 && original->green <= 1 && original->blue <= 1;
            if (white || black) {
                continue;
            }
            if (color->red != red || color->green != green || color->blue != blue) {
                color->red = red;
                color->green = green;
                color->blue = blue;
                changed = true;
            }
        }
        if (!changed) {
            return true;
        }
        if (!RefreshBuffer(a_geometry)) {
            return false;
        }

        lock.unlock();
        RefreshGeometry(a_geometry);
        return true;
    }

    bool Manager::Restore(RE::BSTriShape& a_geometry, RE::FormID a_ownerFormID)
    {
        std::unique_lock lock(mutex);
        const auto found = localRenderers.find(&a_geometry);
        auto* renderer = a_geometry.GetGeometryRuntimeData().rendererData;
        if (found == localRenderers.end() || found->second.renderer != renderer || found->second.ownerFormID != a_ownerFormID || !renderer || !renderer->rawVertexData) {
            return false;
        }

        std::memcpy(renderer->rawVertexData, found->second.data.data(), found->second.data.size());
        const auto restored = RefreshBuffer(a_geometry);
        localRenderers.erase(found);
        lock.unlock();
        if (restored) {
            RefreshGeometry(a_geometry);
        }
        return restored;
    }

    void Manager::Clear()
    {
        std::scoped_lock lock(mutex);
        localRenderers.clear();
    }

    VertexColor ReadVertexColor(RE::BSTriShape& a_geometry)
    {
        const auto& runtime = a_geometry.GetGeometryRuntimeData();
        auto* renderer = runtime.rendererData;
        if (!renderer || !renderer->rawVertexData || !renderer->vertexDesc.HasFlag(RE::BSGraphics::Vertex::Flags::VF_COLORS)) {
            return {};
        }

        const auto vertexSize = renderer->vertexDesc.GetSize();
        const auto colorOffset = renderer->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::Attribute::VA_COLOR);
        const auto vertexCount = a_geometry.GetTrishapeRuntimeData().vertexCount;
        if (vertexSize < sizeof(PackedColor) || colorOffset > vertexSize - sizeof(PackedColor) || vertexCount == 0) {
            return {};
        }

        PackedColor selected{};
        auto selectedAlpha = uint8_t{ 0 };
        auto selectedBrightness = uint16_t{ 0 };
        auto found = false;
        for (uint16_t index = 0; index < vertexCount; ++index) {
            const auto* color = reinterpret_cast<const PackedColor*>(renderer->rawVertexData + static_cast<size_t>(vertexSize) * index + colorOffset);
            const auto white = color->red >= 254 && color->green >= 254 && color->blue >= 254;
            const auto zero = color->red <= 1 && color->green <= 1 && color->blue <= 1;
            if (white || zero) {
                continue;
            }

            const auto brightness = static_cast<uint16_t>(color->red) + color->green + color->blue;
            if (!found || color->alpha > selectedAlpha || (color->alpha == selectedAlpha && brightness > selectedBrightness)) {
                selected = *color;
                selectedAlpha = color->alpha;
                selectedBrightness = brightness;
                found = true;
            }
        }

        if (!found) {
            return {};
        }
        return { { selected.red / 255.0F, selected.green / 255.0F, selected.blue / 255.0F, selected.alpha / 255.0F }, true };
    }
}
