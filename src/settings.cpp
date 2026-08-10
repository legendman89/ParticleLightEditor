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
            }
            if (edit.intensityChanged) {
                record["intensity"] = edit.intensity;
            }
            if (edit.radiusChanged) {
                record["radius"] = edit.radius;
            }
            if (edit.positionChanged) {
                record["position"] = { edit.localPosition.x, edit.localPosition.y, edit.localPosition.z };
            }
            if (edit.enabledChanged) {
                record["enabled"] = edit.enabled;
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
                }
                if (record.contains("intensity")) {
                    edit.intensity = record.at("intensity").get<float>();
                    edit.intensityChanged = true;
                }
                if (record.contains("radius")) {
                    edit.radius = record.at("radius").get<float>();
                    edit.radiusChanged = true;
                }
                if (record.contains("position")) {
                    const auto& position = record.at("position");
                    if (!position.is_array() || position.size() != 3) {
                        logger::error("Saved particle-light position must contain three values");
                        return false;
                    }
                    edit.localPosition = { position.at(0).get<float>(), position.at(1).get<float>(), position.at(2).get<float>() };
                    edit.positionChanged = true;
                }
                if (record.contains("enabled")) {
                    edit.enabled = record.at("enabled").get<bool>();
                    edit.enabledChanged = true;
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
            }
            if (rule.intensityChanged) {
                record["intensity"] = rule.intensity;
            }
            if (rule.radiusChanged) {
                record["radiusScale"] = rule.radiusScale;
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

                CategoryRule rule;
                if (record.contains("color")) {
                    const auto& color = record.at("color");
                    if (!color.is_array() || color.size() != 4) {
                        logger::error("Category-rule color must contain four values");
                        return false;
                    }
                    rule.color = { color.at(0).get<float>(), color.at(1).get<float>(), color.at(2).get<float>(), color.at(3).get<float>() };
                    rule.colorChanged = true;
                }
                if (record.contains("intensity")) {
                    rule.intensity = record.at("intensity").get<float>();
                    rule.intensityChanged = true;
                }
                if (record.contains("radiusScale")) {
                    rule.radiusScale = record.at("radiusScale").get<float>();
                    rule.radiusChanged = true;
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
            if (!CategoryRuleEqual(left, right)) {
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
            logger::error("Failed to create settings directory '{}': {}", a_path.parent_path().string(), error.message());
            return false;
        }

        std::ofstream file(a_path);
        if (!file.is_open()) {
            logger::error("Failed to open settings file for writing: {}", a_path.string());
            return false;
        }

        file << a_json.dump(4);
        if (!file.good()) {
            logger::error("Failed while writing settings file: {}", a_path.string());
            return false;
        }

        logger::info("Settings saved: {}", a_path.string());
        return true;
    }

    bool ReadJsonFile(const fs::path& a_path, json& a_json)
    {
        std::ifstream file(a_path);
        if (!file.is_open()) {
            logger::error("Failed to open settings file: {}", a_path.string());
            return false;
        }

        try {
            a_json = json::parse(file, nullptr, true, true);
        }
        catch (const json::exception& error) {
            logger::error("Failed to parse settings file '{}': {}", a_path.string(), error.what());
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
        CategoryOverrideMap categoryOverrides;
        const auto path = GetSettingsPath();
        std::error_code error;
        const auto exists = fs::exists(path, error);
        if (error) {
            logger::error("Could not inspect settings file '{}': {}", path.string(), error.message());
            return false;
        }

        if (exists) {
            json data;
            if (!ReadJsonFile(path, data) || !FromJson(data, settings) || !EditsFromJson(data, edits) ||
                !CategoryRulesFromJson(data, categoryRules) || !CategoryOverridesFromJson(data, categoryOverrides)) {
                return false;
            }
            logger::info("Settings loaded: {}", path.string());
        }
        else {
            logger::info("Settings file does not exist; using defaults: {}", path.string());
        }

        GetSettings() = settings;
        GetSavedSettings() = settings;
        GetSavedEdits() = edits;
        GetSavedCategoryRules() = categoryRules;
        GetSavedCategoryOverrides() = categoryOverrides;
        auto& scanner = Scanner::GetSingleton();
        scanner.SetCategoryOverrides(categoryOverrides);
        scanner.SetCategoryRules(categoryRules);
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
        GetSavedCategoryOverrides() = scanner.GetCategoryOverrides();
        logger::info("Saved {} particle-light edit(s)", GetSavedEdits().size());
        return true;
    }

    void RestoreEdits()
    {
        auto& scanner = Scanner::GetSingleton();
        scanner.SetCategoryOverrides(GetSavedCategoryOverrides());
        scanner.SetCategoryRules(GetSavedCategoryRules());
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
