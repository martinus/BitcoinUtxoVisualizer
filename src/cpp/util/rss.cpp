#include "rss.h"

#include <sys/resource.h>
#include <sys/time.h>

namespace util {

[[nodiscard]] auto maxRss() -> size_t {
    auto usage = rusage();
    if (0 != ::getrusage(RUSAGE_SELF, &usage)) {
        return 0U;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    return static_cast<size_t>(usage.ru_maxrss) * 1024U;
}

} // namespace util
