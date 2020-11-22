#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace util {

// Custom type safe numberwrappers similar to std::chrono::durtion, custom type safe types!
template <typename Tag, typename Rep>
class TypedNumber {
public:
    using value_type = Rep;
    using tag_type = Tag;

    // Acceptance logic is same as for std::chrono::durations
    // * if value_type is floating point, accept any type that's convertible (int, float, ...).
    // * if value_type is not floating point, only accept non-floating point types that are convertible.
    template <typename Rep2,
              typename = std::enable_if_t<std::is_convertible_v<Rep2 const&, Rep> &&
                                          (std::is_floating_point_v<Rep> || !std::is_floating_point_v<Rep2>)>>
    constexpr explicit TypedNumber(Rep2 const& value) noexcept
        : mValue{static_cast<value_type>(value)} {}

    constexpr TypedNumber() = default;

    // comparisons

    constexpr auto operator<(TypedNumber const& other) const noexcept -> bool {
        return mValue < other.mValue;
    }
    constexpr auto operator>(TypedNumber const& other) const noexcept -> bool {
        return mValue > other.mValue;
    }
    constexpr auto operator<=(TypedNumber other) const noexcept -> bool {
        return mValue <= other.mValue;
    }
    constexpr auto operator>=(TypedNumber const& other) const noexcept -> bool {
        return mValue >= other.mValue;
    }
    constexpr auto operator==(TypedNumber const& other) const noexcept -> bool {
        return mValue == other.mValue;
    }
    constexpr auto operator!=(TypedNumber const& other) const noexcept -> bool {
        return mValue != other.mValue;
    }

    // arithmetics

    constexpr auto operator+=(TypedNumber const& other) noexcept -> TypedNumber& {
        mValue += other.mValue;
        return *this;
    }
    constexpr auto operator-=(TypedNumber const& other) noexcept -> TypedNumber& {
        mValue -= other.mValue;
        return *this;
    }
    constexpr auto operator*=(value_type const& factor) noexcept -> TypedNumber& {
        mValue *= factor;
        return *this;
    }
    constexpr auto operator/=(value_type const& factor) noexcept -> TypedNumber& {
        mValue /= factor;
        return *this;
    }

    // unary + and -

    constexpr auto operator+() const noexcept -> TypedNumber {
        return *this;
    }

    constexpr auto operator-() const noexcept -> TypedNumber {
        return TypedNumber{-mValue};
    }

    // increment & decrement

    constexpr auto operator++() noexcept -> TypedNumber& {
        ++mValue;
        return *this;
    }
    constexpr auto operator++(int) noexcept -> TypedNumber {
        return TypedNumber{mValue++};
    }
    constexpr auto operator--() noexcept -> TypedNumber& {
        --mValue;
        return *this;
    }
    constexpr auto operator--(int) noexcept -> TypedNumber {
        return TypedNumber{mValue--};
    }

    // access value

    constexpr auto count() const noexcept -> value_type {
        return mValue;
    }

private:
    value_type mValue{};
};

template <typename Tag, typename Rep>
constexpr auto operator+(TypedNumber<Tag, Rep> const& a, TypedNumber<Tag, Rep> const& b) noexcept
    -> TypedNumber<Tag, Rep> {
    return TypedNumber<Tag, Rep>{a.count() + b.count()};
}

template <typename Tag, typename Rep>
constexpr auto operator-(TypedNumber<Tag, Rep> const& a, TypedNumber<Tag, Rep> const& b) noexcept
    -> TypedNumber<Tag, Rep> {
    return TypedNumber<Tag, Rep>{a.count() - b.count()};
}

template <typename Tag, typename Rep>
constexpr auto operator*(Rep const& factor, TypedNumber<Tag, Rep> const& quantity) noexcept -> TypedNumber<Tag, Rep> {
    return TypedNumber<Tag, Rep>{factor * quantity.count()};
}

template <typename Tag, typename Rep>
constexpr auto operator*(TypedNumber<Tag, Rep> const& quantity, Rep const& factor) noexcept -> TypedNumber<Tag, Rep> {
    return TypedNumber<Tag, Rep>{quantity.count() * factor};
}

template <typename Tag, typename Rep>
constexpr auto operator/(TypedNumber<Tag, Rep> const& dividend, Rep const& divisor) noexcept -> TypedNumber<Tag, Rep> {
    return TypedNumber<Tag, Rep>{dividend.count() / divisor};
}

template <typename Tag, typename Rep>
constexpr auto operator/(TypedNumber<Tag, Rep> const& dividend, TypedNumber<Tag, Rep> const& divisor) noexcept -> Rep {
    return dividend.count() / divisor.count();
}

} // namespace util

namespace std {

template <typename Tag, typename Rep>
struct numeric_limits<util::TypedNumber<Tag, Rep>> {
    static constexpr auto max() noexcept -> util::TypedNumber<Tag, Rep> {
        return util::TypedNumber<Tag, Rep>{numeric_limits<Rep>::max()};
    }

    static constexpr auto min() noexcept -> util::TypedNumber<Tag, Rep> {
        return util::TypedNumber<Tag, Rep>{numeric_limits<Rep>::min()};
    }
};

} // namespace std
