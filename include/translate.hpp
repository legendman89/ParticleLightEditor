#pragma once

#include "logger.hpp"
#include "utility.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ParticleLightEditor::Trans
{
    namespace fs = std::filesystem;
    using json = nlohmann::json;

    inline fs::path GetTranslationPath()
    {
        const auto gamePath = fs::path(REL::Module::get().filename()).parent_path();
        return gamePath / "Data" / "SKSE" / "Plugins" / PRODUCT_NAME / "Translation" / "Translation.json";
    }

    class Translator
    {
    public:
        bool Load()
        {
            const auto path = GetTranslationPath();
            table.clear();
            missing.clear();

            std::ifstream input(path);
            if (!input) {
                logger::error("Translation: could not open file {}", Utility::ToUTF8(path));
                return false;
            }

            try {
                const auto data = json::parse(input, nullptr, true, true);
                if (!data.is_object()) {
                    logger::error("Translation: JSON root is not an object in file {}", Utility::ToUTF8(path));
                    return false;
                }
                for (const auto& [key, value] : data.items()) {
                    if (value.is_string()) {
                        table.emplace(key, value.get<std::string>());
                    }
                }
            }
            catch (const std::exception& error) {
                logger::error("Translation: JSON parse error in file {}: {}", Utility::ToUTF8(path), error.what());
                table.clear();
                return false;
            }

            logger::info("Loaded {} Particle Light Editor translations", table.size());
            return true;
        }

        const std::string& Get(std::string_view a_key)
        {
            const auto found = table.find(std::string(a_key));
            if (found != table.end()) {
                return found->second;
            }
            return missing.try_emplace(std::string(a_key), a_key).first->second;
        }

    private:
        std::unordered_map<std::string, std::string> table;
        std::unordered_map<std::string, std::string> missing;
    };

    inline Translator& GetTranslator()
    {
        static Translator translator;
        return translator;
    }

    inline const std::string& Tr(std::string_view a_key) { return GetTranslator().Get(a_key); }

    template <class... Args>
    std::string Format(std::string_view a_key, Args&&... a_args)
    {
        return std::vformat(Tr(a_key), std::make_format_args(a_args...));
    }
}
