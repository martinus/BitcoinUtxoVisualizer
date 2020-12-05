#include "kbhit.h"

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace util {

// see https://stackoverflow.com/a/22166185/48181
auto kbhit() -> bool {
    auto term = termios();
    tcgetattr(0, &term);

    auto term2 = term;

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    auto byteswaiting = int();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term);
    return byteswaiting > 0;
}

} // namespace util
