#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace util::args {

// Provides static access to program arguments. Parses all arguments into key=value, e.g.
// -cfg=../../asdf.json becomes "-cfg" => "../../asdf.json". no = means value is "".
auto get() -> std::unordered_map<std::string, std::string> const&;

// Returns value if found, or std::none if argument is not present
auto get(std::string const& argName) -> std::optional<std::string>;

// sets program arguments
void set(int argc, char** argv);

} // namespace util::args
