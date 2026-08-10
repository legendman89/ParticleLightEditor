#pragma once

#include "types.hpp"

namespace ParticleLightEditor::Category
{
    inline constexpr const char* kNames[]{
        "Unclassified",
        "Candle",
        "Chandelier",
        "Lantern",
        "Fire / Ember",
        "Torch / Brazier"
    };

    inline bool IsValid(ParticleCategory a_category)
    {
        return a_category >= ParticleCategory::kUnclassified && a_category < ParticleCategory::kTotal;
    }

    inline const char* Name(ParticleCategory a_category)
    {
        const auto index = static_cast<size_t>(a_category);
        return index < std::size(kNames) ? kNames[index] : kNames[0];
    }

    inline ParticleCategory FromName(std::string_view a_name)
    {
        for (size_t index = 0; index < std::size(kNames); ++index) {
            if (a_name == kNames[index]) {
                return static_cast<ParticleCategory>(index);
            }
        }
        return ParticleCategory::kUnclassified;
    }

    inline std::string Lowercase(std::string_view a_value)
    {
        std::string result(a_value);
        for (auto& character : result) {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }
        return result;
    }

    inline ParticleCategory ClassifyIdentity(std::string_view a_identity)
    {
        const auto identity = Lowercase(a_identity);
        if (identity.contains("chandelier") || identity.contains("chandlier")) {
            return ParticleCategory::kChandelier;
        }
        if (identity.contains("lantern")) {
            return ParticleCategory::kLantern;
        }
        if (identity.contains("torch") || identity.contains("brazier")) {
            return ParticleCategory::kTorchBrazier;
        }
        if (identity.contains("candle") || identity.contains("candlestick") || identity.contains("candelabra")) {
            return ParticleCategory::kCandle;
        }
        if (identity.contains("fire") || identity.contains("ember") || identity.contains("flame") || identity.contains("hearth") || identity.contains("coal")) {
            return ParticleCategory::kFireEmber;
        }
        return ParticleCategory::kUnclassified;
    }

    inline ParticleCategory Classify(const Entry& a_entry)
    {
        // Never combine fields: a candle record might use a model whose path contains 
        // "chandelier" (and vice versa). Prefer the record EditorID, then fall back 
        // one field at a time only when unavailable.
        if (!a_entry.baseEditorID.empty() && a_entry.baseEditorID != "Unavailable") {
            return ClassifyIdentity(a_entry.baseEditorID);
        }
        if (!a_entry.modelPath.empty()) {
            return ClassifyIdentity(a_entry.modelPath);
        }
        if (!a_entry.baseName.empty() && a_entry.baseName != "Unnamed") {
            return ClassifyIdentity(a_entry.baseName);
        }
        return ClassifyIdentity(a_entry.nodeName);
    }

    inline ParticleCategory Resolve(const Entry& a_entry, const CategoryOverrideMap& a_overrides)
    {
        if (!a_entry.categoryKey.empty()) {
            if (const auto found = a_overrides.find(a_entry.categoryKey); found != a_overrides.end() && IsValid(found->second)) {
                return found->second;
            }
        }
        return Classify(a_entry);
    }
}
