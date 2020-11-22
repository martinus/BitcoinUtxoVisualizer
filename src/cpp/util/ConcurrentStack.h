#pragma once

#include <condition_variable>
#include <mutex>
#include <stack>
#include <vector>

namespace util {

// a simple queue where pop() blocks if the queue is empty.
template <typename T>
class ConcurrentStack {
    std::stack<T, std::vector<T>> mStack{};
    mutable std::mutex mMutex{};
    std::condition_variable mCond{};

public:
    // Returns an item. blocks if the stack is empty.
    auto pop() -> T {
        auto lock = std::unique_lock(mMutex);
        while (mStack.empty()) {
            mCond.wait(lock);
        }
        auto item = std::move(mStack.top());
        mStack.pop();
        return item;
    }

    template <typename... Args>
    void push(Args&&... args) {
        {
            auto lock = std::unique_lock(mMutex);
            mStack.emplace(std::forward<Args>(args)...);
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
