#pragma once

#include "utility.hpp"

#include <spdlog/sinks/basic_file_sink.h>

namespace logger = SKSE::log;

inline void SetupLog(const spdlog::level::level_enum a_level = spdlog::level::info)
{
    const auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) {
        SKSE::stl::report_and_fail("SKSE log directory not provided");
    }

    const auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    const auto logPath = *logsFolder / std::format("{}.log", pluginName);
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(ParticleLightEditor::Utility::ToUTF8(logPath), true);
    auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

    spdlog::set_default_logger(std::move(log));
    spdlog::set_level(a_level);
    spdlog::flush_on(a_level);
}
