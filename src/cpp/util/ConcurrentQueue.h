#include "ConcurrentPushPopContainer.h"

#include <queue>

namespace util {

namespace detail {

template <typename T>
struct PushPop<T, std::queue<T>> {
    template <typename... Args>
    void push(std::queue<T>& q, Args&&... args) {
        q.emplace(std::forward<Args>(args)...);
    }

    auto pop(std::queue<T>& q) -> T {
        auto item = std::move(q.front());
        q.pop();
        return item;
    }
};

} // namespace detail

// push/emplace inserts at top, pop removes at top.
template <typename T>
using ConcurrentQueue = ConcurrentPushPopContainer<T, std::queue<T>>;

} // namespace util
