#pragma once

#include <app/BlockEncoder.h>
#include <app/Cfg.h>

#include <memory>

namespace buv {

// head up display
class Hud {
public:
    static auto create(Cfg const& cfg) -> std::unique_ptr<Hud>;

    Hud();
    virtual ~Hud();

    Hud(Hud const&) = delete;
    Hud(Hud&&) = delete;
    auto operator=(Hud const&) -> Hud& = delete;
    auto operator=(Hud&&) -> Hud& = delete;

    // Copies rgbSource, then draws dat based on the given info.
    virtual void draw(uint8_t const* rgbSource, ChangesInBlock const& cib) = 0;

    // Returns the drawn RGB data.
    [[nodiscard]] virtual auto data() const -> uint8_t const* = 0;

    // Number of bytes of the RGB data
    [[nodiscard]] virtual auto size() const -> size_t = 0;
};

} // namespace buv
