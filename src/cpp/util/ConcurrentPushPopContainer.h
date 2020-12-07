#pragma once

#include <condition_variable>
#include <mutex>

namespace util {

namespace detail {

// generic push & pop for the container
template <typename T, typename Container>
struct PushPop {
    template <typename... Args>
    void push(Container& c, Args&&... args);
    auto pop(Container& c) -> T;
};

} // namespace detail

// a simple queue where pop() blocks if the queue is empty.
template <typename T, typename Container>
class ConcurrentPushPopContainer {
    Container mStack{};
    mutable std::mutex mMutex{};
    std::condition_variable mCond{};

public:
    // Returns an item. blocks if the stack is empty.
    auto pop() -> T {
        auto lock = std::unique_lock(mMutex);
        while (mStack.empty()) {
            mCond.wait(lock);
        }

        // generic template to get the item that pop() would remove
        return detail::PushPop<T, Container>{}.pop(mStack);
    }

    void push(T const& obj) {
        emplace(obj);
    }

    void push(T&& obj) {
        emplace(std::move(obj));
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        {
            auto lock = std::unique_lock(mMutex);
            detail::PushPop<T, Container>{}.push(mStack, std::forward<Args>(args)...);
        }
        // notify outside of the lock, so another waiting thread can immediately acquire the lock
        mCond.notify_one();
    }

    auto size() const -> size_t {
        auto lock = std::unique_lock(mMutex);
        return mStack.size();
    }
};

} // namespace util
