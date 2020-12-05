#pragma once

namespace util {

// returns char if kbhit, otherwise 0.
[[nodiscard]] auto kbhit() -> bool;

} // namespace util
