#include "args.h"

#include <cstring>

namespace {

auto staticArgs() -> std::unordered_map<std::string, std::string>& {
    static auto sArgs = std::unordered_map<std::string, std::string>();
    return sArgs;
}

} // namespace

namespace util::args {

// Provides static access to program arguments
auto get() -> std::unordered_map<std::string, std::string> const& {
    return staticArgs();
}

auto get(std::string const& argName) -> std::optional<std::string> {
    auto const& s = get();
    if (auto it = s.find(argName); it != s.end()) {
        return it->second;
    }
    return {};
}

// sets program arguments
void set(int argc, char** argv) {
    auto& s = staticArgs();
    s.clear();

    // start from 1 so we skip the actual executable name
    for (int i = 1; i < argc; ++i) {
        auto const* arg = argv[i];
        auto const* foundPos = std::strchr(arg, '=');
        if (nullptr == foundPos) {
            s.emplace(arg, "");
        } else {
            s.emplace(std::string(arg, foundPos), foundPos + 1);
        }
    }
}

} // namespace util::args
