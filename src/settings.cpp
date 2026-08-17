#include "settings.hpp"

#include "category.hpp"
#include "logger.hpp"
#include "scanner.hpp"
#include "utility.hpp"

#include <fstream>

namespace ParticleLightEditor::Settings
{
#define BOOL_SETTING_TO_JSON(NAME, DEFAULT_VALUE) { #NAME, a_settings.NAME },
#define FLOAT_SETTING_TO_JSON(NAME, DEFAULT_VALUE) { #NAME, a_settings.NAME },
#define INT_SETTING_TO_JSON(NAME, DEFAULT_VALUE) { #NAME, a_settings.NAME },
#define COLOR_SETTING_TO_JSON(NAME, RED, GREEN, BLUE, ALPHA) { #NAME, a_settings.NAME },

#define BOOL_SETTING_FROM_JSON(NAME, DEFAULT_VALUE) ReadValue(a_json, #NAME, a_settings.NAME);
#define FLOAT_SETTING_FROM_JSON(NAME, DEFAULT_VALUE) ReadValue(a_json, #NAME, a_settings.NAME);
#define INT_SETTING_FROM_JSON(NAME, DEFAULT_VALUE) ReadValue(a_json, #NAME, a_settings.NAME);
#define COLOR_SETTING_FROM_JSON(NAME, RED, GREEN, BLUE, ALPHA) ReadValue(a_json, #NAME, a_settings.NAME);

#define SETTING_DEFAULT(NAME, DEFAULT_VALUE) a_settings.NAME = DEFAULT_VALUE;
#define COLOR_SETTING_DEFAULT(NAME, RED, GREEN, BLUE, ALPHA) a_settings.NAME = { RED, GREEN, BLUE, ALPHA };

#define ANIMATION_BOOL_TO_JSON(NAME, DEFAULT_VALUE) { #NAME, a_animation.NAME },
#define ANIMATION_FLOAT_TO_JSON(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) { #NAME, a_animation.NAME },
#define ANIMATION_COLOR_TO_JSON(NAME, RED, GREEN, BLUE, ALPHA) { #NAME, { a_animation.NAME.red, a_animation.NAME.green, a_animation.NAME.blue, a_animation.NAME.alpha } },
#define ANIMATION_ENUM_TO_JSON(TYPE, NAME, DEFAULT_VALUE) { #NAME, static_cast<uint8_t>(a_animation.NAME) },

#define ANIMATION_REQUIRED_BOOL_FROM_JSON(NAME, DEFAULT_VALUE) a_animation.NAME = a_json.at(#NAME).get<bool>();
#define ANIMATION_OPTIONAL_BOOL_FROM_JSON(NAME, DEFAULT_VALUE) a_animation.NAME = a_json.value(#NAME, DEFAULT_VALUE);
#define ANIMATION_FLOAT_FROM_JSON(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) a_animation.NAME = a_json.at(#NAME).get<float>();
#define ANIMATION_COLOR_FROM_JSON(NAME, RED, GREEN, BLUE, ALPHA) \
    const auto& NAME##Json = a_json.at(#NAME); \
    if (!NAME##Json.is_array() || NAME##Json.size() != 4) { return false; } \
    a_animation.NAME = { NAME##Json.at(0).get<float>(), NAME##Json.at(1).get<float>(), NAME##Json.at(2).get<float>(), NAME##Json.at(3).get<float>() };
#define ANIMATION_ENUM_FROM_JSON(TYPE, NAME, DEFAULT_VALUE) \
    const auto NAME##Value = a_json.at(#NAME).get<uint8_t>(); \
    if (NAME##Value >= static_cast<uint8_t>(TYPE::kTotal)) { return false; } \
    a_animation.NAME = static_cast<TYPE>(NAME##Value);

    json AnimationToJson(const AnimationEdit& a_animation)
    {
        return {
            FOREACH_ANIMATION_REQUIRED_BOOL_PROPERTY(ANIMATION_BOOL_TO_JSON)
            FOREACH_ANIMATION_ENUM_PROPERTY(ANIMATION_ENUM_TO_JSON)
            FOREACH_ANIMATION_FLOAT_PROPERTY(ANIMATION_FLOAT_TO_JSON)
            FOREACH_ANIMATION_COLOR_PROPERTY(ANIMATION_COLOR_TO_JSON)
            FOREACH_ANIMATION_OPTIONAL_BOOL_PROPERTY(ANIMATION_BOOL_TO_JSON)
        };
    }

    bool AnimationFromJson(const json& a_json, AnimationEdit& a_animation)
    {
        if (!a_json.is_object()) {
            return false;
        }

        FOREACH_ANIMATION_REQUIRED_BOOL_PROPERTY(ANIMATION_REQUIRED_BOOL_FROM_JSON)
        FOREACH_ANIMATION_ENUM_PROPERTY(ANIMATION_ENUM_FROM_JSON)
        FOREACH_ANIMATION_FLOAT_PROPERTY(ANIMATION_FLOAT_FROM_JSON)
        FOREACH_ANIMATION_COLOR_PROPERTY(ANIMATION_COLOR_FROM_JSON)
        FOREACH_ANIMATION_OPTIONAL_BOOL_PROPERTY(ANIMATION_OPTIONAL_BOOL_FROM_JSON)
        return Animation::IsValid(a_animation);
    }

    json ToJson(const RuntimeSettings& a_settings)
    {
        return json{
            FOREACH_BOOL_SETTING(BOOL_SETTING_TO_JSON)
            FOREACH_FLOAT_SETTING(FLOAT_SETTING_TO_JSON)
            FOREACH_INT_SETTING(INT_SETTING_TO_JSON)
            FOREACH_COLOR_SETTING(COLOR_SETTING_TO_JSON)
        };
    }

    bool FromJson(const json& a_json, RuntimeSettings& a_settings)
    {
        if (!a_json.is_object()) {
            logger::error("Settings JSON root must be an object");
            return false;
        }

        try {
            FOREACH_BOOL_SETTING(BOOL_SETTING_FROM_JSON)
            FOREACH_FLOAT_SETTING(FLOAT_SETTING_FROM_JSON)
            FOREACH_INT_SETTING(INT_SETTING_FROM_JSON)
            FOREACH_COLOR_SETTING(COLOR_SETTING_FROM_JSON)
            a_settings.logLevel = NormalizeLogLevel(a_settings.logLevel);
        }
        catch (const json::exception& error) {
            logger::error("Failed to read settings JSON values: {}", error.what());
            return false;
        }

        return true;
    }

    json EditsToJson(const EditMap& a_edits)
    {
        json records = json::array();
        for (const auto& [key, edit] : a_edits) {
            if (!HasChanges(edit)) {
                continue;
            }

            json record{
                { "referenceFormID", key.referenceFormID },
                { "particleOrdinal", key.particleOrdinal }
            };
            if (edit.colorChanged) {
                record["color"] = { edit.color.red, edit.color.green, edit.color.blue, edit.color.alpha };
                record["colorRevision"] = edit.colorRevision;
            }
            if (edit.intensityChanged) {
                record["intensity"] = edit.intensity;
                record["intensityRevision"] = edit.intensityRevision;
            }
            if (edit.radiusChanged) {
                record["radius"] = edit.radius;
                record["radiusRevision"] = edit.radiusRevision;
            }
            if (edit.positionChanged) {
                record["position"] = { edit.localPosition.x, edit.localPosition.y, edit.localPosition.z };
                record["positionRevision"] = edit.localPositionRevision;
            }
            if (edit.enabledChanged) {
                record["enabled"] = edit.enabled;
                record["enabledRevision"] = edit.enabledRevision;
            }
            if (edit.animationChanged) {
                record["animation"] = AnimationToJson(edit.animation);
                record["animationRevision"] = edit.animationRevision;
            }
            records.push_back(std::move(record));
        }
        return records;
    }

    bool EditsFromJson(const json& a_json, EditMap& a_edits)
    {
        a_edits.clear();
        if (!a_json.contains("edits")) {
            return true;
        }

        const auto& records = a_json.at("edits");
        if (!records.is_array()) {
            logger::error("Settings JSON 'edits' value must be an array");
            return false;
        }

        try {
            for (const auto& record : records) {
                EditKey key;
                key.referenceFormID = record.at("referenceFormID").get<RE::FormID>();
                key.particleOrdinal = record.at("particleOrdinal").get<size_t>();
                if (key.referenceFormID == 0 || key.particleOrdinal == 0) {
                    logger::warn("Ignored saved particle-light edit with an invalid identity");
                    continue;
                }

                Edit edit;
                if (record.contains("color")) {
                    const auto& color = record.at("color");
                    if (!color.is_array() || color.size() != 4) {
                        logger::error("Saved particle-light color must contain four values");
                        return false;
                    }
                    edit.color = { color.at(0).get<float>(), color.at(1).get<float>(), color.at(2).get<float>(), color.at(3).get<float>() };
                    edit.colorChanged = true;
                    edit.colorRevision = record.value("colorRevision", uint64_t{ 0 });
                }
                if (record.contains("intensity")) {
                    edit.intensity = record.at("intensity").get<float>();
                    edit.intensityChanged = true;
                    edit.intensityRevision = record.value("intensityRevision", uint64_t{ 0 });
                }
                if (record.contains("radius")) {
                    edit.radius = record.at("radius").get<float>();
                    edit.radiusChanged = true;
                    edit.radiusRevision = record.value("radiusRevision", uint64_t{ 0 });
                }
                if (record.contains("position")) {
                    const auto& position = record.at("position");
                    if (!position.is_array() || position.size() != 3) {
                        logger::error("Saved particle-light position must contain three values");
                        return false;
                    }
                    edit.localPosition = { position.at(0).get<float>(), position.at(1).get<float>(), position.at(2).get<float>() };
                    edit.positionChanged = true;
                    edit.localPositionRevision = record.value("positionRevision", uint64_t{ 0 });
                }
                if (record.contains("enabled")) {
                    edit.enabled = record.at("enabled").get<bool>();
                    edit.enabledChanged = true;
                    edit.enabledRevision = record.value("enabledRevision", uint64_t{ 0 });
                }
                if (record.contains("animation")) {
                    if (!AnimationFromJson(record.at("animation"), edit.animation)) {
                        logger::error("Saved particle-light animation contains an invalid value");
                        return false;
                    }
                    edit.animationChanged = true;
                    edit.animationRevision = record.value("animationRevision", uint64_t{ 0 });
                }
                a_edits[key] = edit;
            }
        }
        catch (const json::exception& error) {
            logger::error("Failed to read saved particle-light edits: {}", error.what());
            return false;
        }

        return true;
    }

    json CategoryRulesToJson(const CategoryRuleMap& a_rules)
    {
        json records = json::array();
        for (const auto& [key, rule] : a_rules) {
            if (key.category == ParticleCategory::kUnclassified || !HasChanges(rule)) {
                continue;
            }

            json record{
                { "category", Category::Name(key.category) },
                { "cellFormID", key.cellFormID }
            };
            if (rule.colorChanged) {
                record["color"] = { rule.color.red, rule.color.green, rule.color.blue, rule.color.alpha };
                record["colorRevision"] = rule.colorRevision;
            }
            if (rule.intensityChanged) {
                record["intensity"] = rule.intensity;
                record["intensityRevision"] = rule.intensityRevision;
            }
            if (rule.radiusChanged) {
                record["radiusScale"] = rule.radiusScale;
                record["radiusRevision"] = rule.radiusScaleRevision;
            }
            if (rule.animationChanged) {
                record["animation"] = AnimationToJson(rule.animation);
                record["animationRevision"] = rule.animationRevision;
            }
            records.push_back(std::move(record));
        }
        return records;
    }

    bool CategoryRulesFromJson(const json& a_json, CategoryRuleMap& a_rules)
    {
        a_rules.clear();
        if (!a_json.contains("categoryRules")) {
            return true;
        }

        const auto& records = a_json.at("categoryRules");
        if (!records.is_array()) {
            logger::error("Settings JSON 'categoryRules' value must be an array");
            return false;
        }

        try {
            for (const auto& record : records) {
                const auto categoryName = record.at("category").get<std::string>();
                const auto category = Category::FromName(categoryName);
                if (category == ParticleCategory::kUnclassified) {
                    logger::warn("Ignored category rule with unsupported category '{}'", categoryName);
                    continue;
                }

                ScopeEdit rule;
                if (record.contains("color")) {
                    const auto& color = record.at("color");
                    if (!color.is_array() || color.size() != 4) {
                        logger::error("Category-rule color must contain four values");
                        return false;
                    }
                    rule.color = { color.at(0).get<float>(), color.at(1).get<float>(), color.at(2).get<float>(), color.at(3).get<float>() };
                    rule.colorChanged = true;
                    rule.colorRevision = record.value("colorRevision", uint64_t{ 0 });
                }
                if (record.contains("intensity")) {
                    rule.intensity = record.at("intensity").get<float>();
                    rule.intensityChanged = true;
                    rule.intensityRevision = record.value("intensityRevision", uint64_t{ 0 });
                }
                if (record.contains("radiusScale")) {
                    rule.radiusScale = record.at("radiusScale").get<float>();
                    rule.radiusChanged = true;
                    rule.radiusScaleRevision = record.value("radiusRevision", uint64_t{ 0 });
                }
                if (record.contains("animation")) {
                    if (!AnimationFromJson(record.at("animation"), rule.animation)) {
                        logger::error("Category-rule animation '{}' contains an invalid value", categoryName);
                        return false;
                    }
                    rule.animationChanged = true;
                    rule.animationRevision = record.value("animationRevision", uint64_t{ 0 });
                }
                if (!std::isfinite(rule.intensity) || !std::isfinite(rule.radiusScale) || rule.radiusScale <= 0.0F) {
                    logger::error("Category rule '{}' contains an invalid numeric value", categoryName);
                    return false;
                }

                const auto cellFormID = record.value("cellFormID", RE::FormID{ 0 });
                a_rules[{ category, cellFormID }] = rule;
            }
        }
        catch (const json::exception& error) {
            logger::error("Failed to read category rules: {}", error.what());
            return false;
        }
        return true;
    }

    json BaseRulesToJson(const BaseRuleMap& a_rules)
    {
        json records = json::array();
        for (const auto& [key, rule] : a_rules) {
            if (key.baseFormID == 0 || key.particleOrdinal == 0 || !HasChanges(rule)) {
                continue;
            }

            json record{
                { "baseFormID", key.baseFormID },
                { "particleOrdinal", key.particleOrdinal },
                { "cellFormID", key.cellFormID }
            };
            if (rule.colorChanged) {
                record["color"] = { rule.color.red, rule.color.green, rule.color.blue, rule.color.alpha };
                record["colorRevision"] = rule.colorRevision;
            }
            if (rule.intensityChanged) {
                record["intensity"] = rule.intensity;
                record["intensityRevision"] = rule.intensityRevision;
            }
            if (rule.radiusChanged) {
                record["radiusScale"] = rule.radiusScale;
                record["radiusRevision"] = rule.radiusScaleRevision;
            }
            if (rule.positionChanged) {
                record["position"] = { rule.localPosition.x, rule.localPosition.y, rule.localPosition.z };
                record["positionRevision"] = rule.localPositionRevision;
            }
            if (rule.enabledChanged) {
                record["enabled"] = rule.enabled;
                record["enabledRevision"] = rule.enabledRevision;
            }
            if (rule.animationChanged) {
                record["animation"] = AnimationToJson(rule.animation);
                record["animationRevision"] = rule.animationRevision;
            }
            records.push_back(std::move(record));
        }
        return records;
    }

    bool BaseRulesFromJson(const json& a_json, BaseRuleMap& a_rules)
    {
        a_rules.clear();
        if (!a_json.contains("baseRules")) {
            return true;
        }

        const auto& records = a_json.at("baseRules");
        if (!records.is_array()) {
            logger::error("Settings JSON 'baseRules' value must be an array");
            return false;
        }

        try {
            for (const auto& record : records) {
                BaseRuleKey key;
                key.baseFormID = record.at("baseFormID").get<RE::FormID>();
                key.particleOrdinal = record.at("particleOrdinal").get<size_t>();
                key.cellFormID = record.value("cellFormID", RE::FormID{ 0 });
                if (key.baseFormID == 0 || key.particleOrdinal == 0) {
                    logger::warn("Ignored base-object rule with an invalid identity");
                    continue;
                }

                ScopeEdit rule;
                if (record.contains("color")) {
                    const auto& color = record.at("color");
                    if (!color.is_array() || color.size() != 4) {
                        logger::error("Base-object rule color must contain four values");
                        return false;
                    }
                    rule.color = { color.at(0).get<float>(), color.at(1).get<float>(), color.at(2).get<float>(), color.at(3).get<float>() };
                    rule.colorChanged = true;
                    rule.colorRevision = record.value("colorRevision", uint64_t{ 0 });
                }
                if (record.contains("intensity")) {
                    rule.intensity = record.at("intensity").get<float>();
                    rule.intensityChanged = true;
                    rule.intensityRevision = record.value("intensityRevision", uint64_t{ 0 });
                }
                if (record.contains("radiusScale")) {
                    rule.radiusScale = record.at("radiusScale").get<float>();
                    rule.radiusChanged = true;
                    rule.radiusScaleRevision = record.value("radiusRevision", uint64_t{ 0 });
                }
                if (record.contains("position")) {
                    const auto& position = record.at("position");
                    if (!position.is_array() || position.size() != 3) {
                        logger::error("Base-object rule position must contain three values");
                        return false;
                    }
                    rule.localPosition = { position.at(0).get<float>(), position.at(1).get<float>(), position.at(2).get<float>() };
                    rule.positionChanged = true;
                    rule.localPositionRevision = record.value("positionRevision", uint64_t{ 0 });
                }
                if (record.contains("enabled")) {
                    rule.enabled = record.at("enabled").get<bool>();
                    rule.enabledChanged = true;
                    rule.enabledRevision = record.value("enabledRevision", uint64_t{ 0 });
                }
                if (record.contains("animation")) {
                    if (!AnimationFromJson(record.at("animation"), rule.animation)) {
                        logger::error("Base-object rule {:08X} contains an invalid animation", key.baseFormID);
                        return false;
                    }
                    rule.animationChanged = true;
                    rule.animationRevision = record.value("animationRevision", uint64_t{ 0 });
                }
                if (!Utility::IsFiniteColor(rule.color) || !std::isfinite(rule.intensity) || !std::isfinite(rule.radiusScale) || rule.radiusScale <= 0.0F ||
                    !std::isfinite(rule.localPosition.x) || !std::isfinite(rule.localPosition.y) || !std::isfinite(rule.localPosition.z)) {
                    logger::error("Base-object rule {:08X} contains an invalid value", key.baseFormID);
                    return false;
                }
                a_rules[key] = rule;
            }
        }
        catch (const json::exception& error) {
            logger::error("Failed to read base-object rules: {}", error.what());
            return false;
        }
        return true;
    }

    json CategoryOverridesToJson(const CategoryOverrideMap& a_overrides)
    {
        json records = json::array();
        for (const auto& [key, category] : a_overrides) {
            if (key.empty() || !Category::IsValid(category)) {
                continue;
            }
            records.push_back({ { "key", key }, { "category", Category::Name(category) } });
        }
        return records;
    }

    bool CategoryOverridesFromJson(const json& a_json, CategoryOverrideMap& a_overrides)
    {
        a_overrides.clear();
        if (!a_json.contains("categoryOverrides")) {
            return true;
        }

        const auto& records = a_json.at("categoryOverrides");
        if (!records.is_array()) {
            logger::error("Settings JSON 'categoryOverrides' value must be an array");
            return false;
        }

        try {
            for (const auto& record : records) {
                const auto key = record.at("key").get<std::string>();
                const auto categoryName = record.at("category").get<std::string>();
                const auto category = Category::FromName(categoryName);
                if (category == ParticleCategory::kUnclassified && categoryName != Category::Name(ParticleCategory::kUnclassified)) {
                    logger::warn("Ignored category override with unsupported category '{}'", categoryName);
                    continue;
                }
                if (key.empty() || !Category::IsValid(category)) {
                    logger::warn("Ignored category override with an invalid mesh key");
                    continue;
                }
                a_overrides[key] = category;
            }
        }
        catch (const json::exception& error) {
            logger::error("Failed to read category overrides: {}", error.what());
            return false;
        }
        return true;
    }

    bool EditsEqual(const EditMap& a_left, const EditMap& a_right)
    {
        size_t changedCount = 0;
        for (const auto& [key, edit] : a_left) {
            if (!HasChanges(edit)) {
                continue;
            }

            ++changedCount;
            const auto found = a_right.find(key);
            if (found == a_right.end() || !EditEqual(edit, found->second)) {
                return false;
            }
        }

        size_t rightChangedCount = 0;
        for (const auto& record : a_right) {
            const auto& edit = record.second;
            if (HasChanges(edit)) {
                ++rightChangedCount;
            }
        }
        return changedCount == rightChangedCount;
    }

    bool CategoryRulesEqual(const CategoryRuleMap& a_left, const CategoryRuleMap& a_right)
    {
        if (a_left.size() != a_right.size()) {
            return false;
        }
        for (const auto& [key, left] : a_left) {
            const auto found = a_right.find(key);
            if (found == a_right.end()) {
                return false;
            }
            const auto& right = found->second;
            if (!ScopeEditEqual(left, right)) {
                return false;
            }
        }
        return true;
    }

    bool BaseRulesEqual(const BaseRuleMap& a_left, const BaseRuleMap& a_right)
    {
        if (a_left.size() != a_right.size()) {
            return false;
        }
        for (const auto& [key, left] : a_left) {
            const auto found = a_right.find(key);
            if (found == a_right.end() || !ScopeEditEqual(left, found->second)) {
                return false;
            }
        }
        return true;
    }

    bool WriteJsonFile(const fs::path& a_path, const json& a_json)
    {
        std::error_code error;
        fs::create_directories(a_path.parent_path(), error);
        if (error) {
            logger::error("Failed to create settings directory '{}': {}", Utility::ToUTF8(a_path.parent_path()), error.message());
            return false;
        }

        std::ofstream file(a_path);
        if (!file.is_open()) {
            logger::error("Failed to open settings file for writing: {}", Utility::ToUTF8(a_path));
            return false;
        }

        file << a_json.dump(4);
        if (!file.good()) {
            logger::error("Failed while writing settings file: {}", Utility::ToUTF8(a_path));
            return false;
        }

        logger::info("Settings saved: {}", Utility::ToUTF8(a_path));
        return true;
    }

    bool ReadJsonFile(const fs::path& a_path, json& a_json)
    {
        std::ifstream file(a_path);
        if (!file.is_open()) {
            logger::error("Failed to open settings file: {}", Utility::ToUTF8(a_path));
            return false;
        }

        try {
            a_json = json::parse(file, nullptr, true, true);
        }
        catch (const json::exception& error) {
            logger::error("Failed to parse settings file '{}': {}", Utility::ToUTF8(a_path), error.what());
            return false;
        }

        return true;
    }

    void ResetScannerDefaults()
    {
        auto& a_settings = GetSettings();
        FOREACH_DETECTION_BOOL_SETTING(SETTING_DEFAULT)
        FOREACH_DETECTION_FLOAT_SETTING(SETTING_DEFAULT)
        FOREACH_DETECTION_INT_SETTING(SETTING_DEFAULT)
    }

    void ResetToolsDefaults()
    {
        auto& a_settings = GetSettings();
        FOREACH_DRAWING_BOOL_SETTING(SETTING_DEFAULT)
        FOREACH_DRAWING_FLOAT_SETTING(SETTING_DEFAULT)
        FOREACH_DRAWING_INT_SETTING(SETTING_DEFAULT)
        FOREACH_DRAWING_COLOR_SETTING(COLOR_SETTING_DEFAULT)
        ApplyLogLevel(a_settings.logLevel);
    }

    void ApplyLogLevel(int a_level)
    {
        const auto level = static_cast<spdlog::level::level_enum>(NormalizeLogLevel(a_level));
        spdlog::set_level(level);
        spdlog::flush_on(level);
    }

    bool Load()
    {
        RuntimeSettings settings;
        EditMap edits;
        CategoryRuleMap categoryRules;
        BaseRuleMap baseRules;
        CategoryOverrideMap categoryOverrides;
        const auto path = GetSettingsPath();
        std::error_code error;
        const auto exists = fs::exists(path, error);
        if (error) {
            logger::error("Could not inspect settings file '{}': {}", Utility::ToUTF8(path), error.message());
            return false;
        }

        if (exists) {
            json data;
            if (!ReadJsonFile(path, data) || !FromJson(data, settings) || !EditsFromJson(data, edits) || !CategoryRulesFromJson(data, categoryRules) ||
                !BaseRulesFromJson(data, baseRules) || !CategoryOverridesFromJson(data, categoryOverrides)) {
                return false;
            }
            logger::info("Settings loaded: {}", Utility::ToUTF8(path));
        }
        else {
            logger::info("Settings file does not exist; using defaults: {}", Utility::ToUTF8(path));
        }

        GetSettings() = settings;
        GetSavedSettings() = settings;
        GetSavedEdits() = edits;
        GetSavedCategoryRules() = categoryRules;
        GetSavedBaseRules() = baseRules;
        GetSavedCategoryOverrides() = categoryOverrides;
        auto& scanner = Scanner::GetSingleton();
        scanner.SetCategoryOverrides(categoryOverrides);
        scanner.SetCategoryRules(categoryRules);
        scanner.SetBaseRules(baseRules);
        scanner.SetEdits(edits);
        ApplyLogLevel(settings.logLevel);
        logger::info("Loaded {} saved particle-light edit(s)", edits.size());
        return true;
    }

    bool Save()
    {
        auto data = ToJson(GetSettings());
        auto& scanner = Scanner::GetSingleton();
        data["edits"] = EditsToJson(scanner.GetEdits());
        data["categoryRules"] = CategoryRulesToJson(scanner.GetCategoryRules());
        data["baseRules"] = BaseRulesToJson(scanner.GetBaseRules());
        data["categoryOverrides"] = CategoryOverridesToJson(scanner.GetCategoryOverrides());
        if (!WriteJsonFile(GetSettingsPath(), data)) {
            return false;
        }

        EditMap savedEdits;
        if (!EditsFromJson(data, savedEdits)) {
            logger::error("Could not normalize the particle-light edits after saving");
            return false;
        }
        GetSavedSettings() = GetSettings();
        GetSavedEdits() = scanner.GetEdits();
        GetSavedCategoryRules() = scanner.GetCategoryRules();
        GetSavedBaseRules() = scanner.GetBaseRules();
        GetSavedCategoryOverrides() = scanner.GetCategoryOverrides();
        logger::info("Saved {} particle-light edit(s)", GetSavedEdits().size());
        return true;
    }

    void RestoreEdits()
    {
        auto& scanner = Scanner::GetSingleton();
        scanner.SetCategoryOverrides(GetSavedCategoryOverrides());
        scanner.SetCategoryRules(GetSavedCategoryRules());
        scanner.SetBaseRules(GetSavedBaseRules());
        scanner.SetEdits(GetSavedEdits());
        logger::info("Restored {} saved particle-light edit(s) for runtime rebinding", GetSavedEdits().size());
    }

#undef BOOL_SETTING_TO_JSON
#undef FLOAT_SETTING_TO_JSON
#undef INT_SETTING_TO_JSON
#undef COLOR_SETTING_TO_JSON
#undef BOOL_SETTING_FROM_JSON
#undef FLOAT_SETTING_FROM_JSON
#undef INT_SETTING_FROM_JSON
#undef COLOR_SETTING_FROM_JSON
#undef SETTING_DEFAULT
#undef COLOR_SETTING_DEFAULT
}
