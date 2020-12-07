#include "ConcurrentPushPopContainer.h"

#include <vector>

namespace util {

namespace detail {

template <typename T>
struct PushPop<T, std::vector<T>> {
    template <typename... Args>
    void push(std::vector<T>& q, Args&&... args) {
        q.emplace_back(std::forward<Args>(args)...);
    }

    auto pop(std::vector<T>& q) -> T {
        auto item = std::move(q.back());
        q.pop_back();
        return item;
    }
};

} // namespace detail

// push/emplace inserts at top, pop removes at top.
template <typename T>
using ConcurrentStack = ConcurrentPushPopContainer<T, std::vector<T>>;

} // namespace util
