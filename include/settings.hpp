#pragma once

#include "animation.hpp"
#include "scanner.hpp"
#include "settings_defs.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <nlohmann/json.hpp>

#include <array>

namespace fs = std::filesystem;
using json = nlohmann::json;

#define BOOL_SETTING_DEFINE(NAME, DEFAULT_VALUE) bool NAME{ DEFAULT_VALUE };
#define FLOAT_SETTING_DEFINE(NAME, DEFAULT_VALUE) float NAME{ DEFAULT_VALUE };
#define INT_SETTING_DEFINE(NAME, DEFAULT_VALUE) int NAME{ DEFAULT_VALUE };
#define COLOR_SETTING_DEFINE(NAME, RED, GREEN, BLUE, ALPHA) std::array<float, 4> NAME{ RED, GREEN, BLUE, ALPHA };
#define SETTING_EQUAL(NAME, DEFAULT_VALUE) a_left.NAME == a_right.NAME &&
#define COLOR_SETTING_EQUAL(NAME, RED, GREEN, BLUE, ALPHA) a_left.NAME == a_right.NAME &&
#define EDIT_PROPERTY_FLAG_EQUAL(PROPERTY, NAME, CHANGED, COMPARISON, LABEL) a_left.CHANGED == a_right.CHANGED &&
#define EDIT_PROPERTY_VALUE_EQUAL(PROPERTY, NAME, CHANGED, COMPARISON, LABEL) (!a_left.CHANGED || EDIT_PROPERTY_COMPARE_##COMPARISON(NAME)) &&
#define EDIT_PROPERTY_COMPARE_COLOR(NAME) Utility::ColorsEqual(a_left.NAME, a_right.NAME)
#define EDIT_PROPERTY_COMPARE_POINT(NAME) Utility::PointsEqual(a_left.NAME, a_right.NAME)
#define EDIT_PROPERTY_COMPARE_VALUE(NAME) a_left.NAME == a_right.NAME

namespace ParticleLightEditor::Settings
{
    struct RuntimeSettings
    {
        FOREACH_BOOL_SETTING(BOOL_SETTING_DEFINE)
        FOREACH_FLOAT_SETTING(FLOAT_SETTING_DEFINE)
        FOREACH_INT_SETTING(INT_SETTING_DEFINE)
        FOREACH_COLOR_SETTING(COLOR_SETTING_DEFINE)

        static RuntimeSettings& GetSingleton();

        inline void ResetDefaults() { *this = RuntimeSettings{}; }
    };

    json ToJson(const RuntimeSettings& a_settings);

    bool FromJson(const json& a_json, RuntimeSettings& a_settings);

    json EditsToJson(const EditMap& a_edits);

    bool EditsFromJson(const json& a_json, EditMap& a_edits);

    json CategoryRulesToJson(const CategoryRuleMap& a_rules);

    bool CategoryRulesFromJson(const json& a_json, CategoryRuleMap& a_rules);

    json CategoryOverridesToJson(const CategoryOverrideMap& a_overrides);

    bool CategoryOverridesFromJson(const json& a_json, CategoryOverrideMap& a_overrides);

    bool EditsEqual(const EditMap& a_left, const EditMap& a_right);

    bool CategoryRulesEqual(const CategoryRuleMap& a_left, const CategoryRuleMap& a_right);

    bool WriteJsonFile(const fs::path& a_path, const json& a_json);

    bool ReadJsonFile(const fs::path& a_path, json& a_json);

    void ResetScannerDefaults();

    void ResetToolsDefaults();

    void ApplyLogLevel(int a_level);

    bool Load();

    bool Save();
    
    void RestoreEdits();

    inline RuntimeSettings& RuntimeSettings::GetSingleton()
    {
        static RuntimeSettings singleton;
        return singleton;
    }

    inline RuntimeSettings& GetSettings() { return RuntimeSettings::GetSingleton(); }

    inline RuntimeSettings& GetSavedSettings()
    {
        static RuntimeSettings settings;
        return settings;
    }

    inline EditMap& GetSavedEdits()
    {
        static EditMap edits;
        return edits;
    }

    inline CategoryRuleMap& GetSavedCategoryRules()
    {
        static CategoryRuleMap rules;
        return rules;
    }

    inline CategoryOverrideMap& GetSavedCategoryOverrides()
    {
        static CategoryOverrideMap overrides;
        return overrides;
    }

    inline fs::path GetSettingsDirectory()
    {
        return fs::path(REL::Module::get().filename()).parent_path() / "Data" / "SKSE" / "Plugins" / PRODUCT_NAME / "Settings";
    }

    inline fs::path GetSettingsPath() { return GetSettingsDirectory() / "Settings.json"; }

    inline int NormalizeLogLevel(int a_level) { return std::clamp(a_level, 0, 6); }

    inline bool SettingsEqual(const RuntimeSettings& a_left, const RuntimeSettings& a_right) {
         return FOREACH_BOOL_SETTING(SETTING_EQUAL) FOREACH_FLOAT_SETTING(SETTING_EQUAL) FOREACH_INT_SETTING(SETTING_EQUAL)
            FOREACH_COLOR_SETTING(COLOR_SETTING_EQUAL) true;
    }

    inline bool DrawingEqual(const RuntimeSettings& a_left, const RuntimeSettings& a_right) { 
        return FOREACH_DRAWING_BOOL_SETTING(SETTING_EQUAL) FOREACH_DRAWING_FLOAT_SETTING(SETTING_EQUAL) FOREACH_DRAWING_INT_SETTING(SETTING_EQUAL)
            FOREACH_DRAWING_COLOR_SETTING(COLOR_SETTING_EQUAL) true;
    }

    inline bool DetectionEqual(const RuntimeSettings& a_left, const RuntimeSettings& a_right) { 
        return FOREACH_DETECTION_BOOL_SETTING(SETTING_EQUAL) FOREACH_DETECTION_FLOAT_SETTING(SETTING_EQUAL) FOREACH_DETECTION_INT_SETTING(SETTING_EQUAL) true; 
    }

    inline bool EditEqual(const Edit& a_left, const Edit& a_right) { return FOREACH_EDIT_PROPERTY(EDIT_PROPERTY_FLAG_EQUAL) FOREACH_EDIT_PROPERTY(EDIT_PROPERTY_VALUE_EQUAL)
        a_left.animationChanged == a_right.animationChanged && (!a_left.animationChanged || Animation::Equal(a_left.animation, a_right.animation)); }

    inline bool CategoryRuleEqual(const CategoryRule& a_left, const CategoryRule& a_right) { return FOREACH_CATEGORY_RULE_PROPERTY(EDIT_PROPERTY_FLAG_EQUAL) FOREACH_CATEGORY_RULE_PROPERTY(EDIT_PROPERTY_VALUE_EQUAL)
        a_left.animationChanged == a_right.animationChanged && (!a_left.animationChanged || Animation::Equal(a_left.animation, a_right.animation)); }

    inline bool AreEditsDirty()
    {
        const auto& scanner = Scanner::GetSingleton();
        return !EditsEqual(scanner.GetEdits(), GetSavedEdits()) ||
            !CategoryRulesEqual(scanner.GetCategoryRules(), GetSavedCategoryRules()) ||
            scanner.GetCategoryOverrides() != GetSavedCategoryOverrides();
    }

    inline bool IsDirty() { return !SettingsEqual(GetSettings(), GetSavedSettings()) || AreEditsDirty(); }

    inline bool IsScannerDirty() { return !DetectionEqual(GetSettings(), GetSavedSettings()) || AreEditsDirty(); }

    inline bool IsToolsDirty() { return !DrawingEqual(GetSettings(), GetSavedSettings()); }

    inline bool IsScannerDefault() { return DetectionEqual(GetSettings(), RuntimeSettings{}); }

    inline bool IsToolsDefault() { return DrawingEqual(GetSettings(), RuntimeSettings{}); }

    template <class Value>
    inline void ReadValue(const json& a_json, const char* a_key, Value& a_value)
    {
        if (a_json.contains(a_key)) {
            a_value = a_json.at(a_key).get<Value>();
        }
    }
}

#undef BOOL_SETTING_DEFINE
#undef FLOAT_SETTING_DEFINE
#undef INT_SETTING_DEFINE
#undef COLOR_SETTING_DEFINE
#undef SETTING_EQUAL
#undef COLOR_SETTING_EQUAL
#undef EDIT_PROPERTY_FLAG_EQUAL
#undef EDIT_PROPERTY_VALUE_EQUAL
#undef EDIT_PROPERTY_COMPARE_COLOR
#undef EDIT_PROPERTY_COMPARE_POINT
#undef EDIT_PROPERTY_COMPARE_VALUE
