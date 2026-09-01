#pragma once

#include "utility.hpp"

#include <fstream>
#include <spdlog/sinks/ostream_sink.h>

namespace logger = SKSE::log;

inline void SetupLog(const spdlog::level::level_enum a_level = spdlog::level::info)
{
    const auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) {
        SKSE::stl::report_and_fail("SKSE log directory not provided");
    }

    const auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    const auto logPath = *logsFolder / std::format("{}.log", pluginName);
    static auto stream = std::make_shared<std::ofstream>(logPath, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!stream->is_open()) {
        SKSE::stl::report_and_fail(std::format("Failed to open SKSE log file {}", ParticleLightEditor::Utility::ToUTF8(logPath)));
    }

    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(*stream);
    auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

    spdlog::set_default_logger(std::move(log));
    spdlog::set_level(a_level);
    spdlog::flush_on(a_level);
}
