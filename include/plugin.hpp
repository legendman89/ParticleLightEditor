#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::literals;

namespace ParticleLightEditor
{
    void MessageHandler(SKSE::MessagingInterface::Message* a_message);
}
