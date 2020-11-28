#pragma once

namespace buv {

class LinearFunction {
public:
    LinearFunction(double x1, double y1, double x2, double y2)
        : mK((y2 - y1) / (x2 - x1))
        , mD(y1 - mK * x1) {}

    [[nodiscard]] auto k() const -> double {
        return mK;
    }

    [[nodiscard]] auto d() const -> double {
        return mD;
    }

    auto operator()(double x) const -> double {
        return mK * x + mD;
    }

private:
    double const mK;
    double const mD;
};

} // namespace buv
