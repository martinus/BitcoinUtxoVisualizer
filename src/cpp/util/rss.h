#pragma once

#include <cstddef>

namespace util {

// max RSS of this process (or 0 if not supported)
[[nodiscard]] auto maxRss() -> size_t;

} // namespace util
