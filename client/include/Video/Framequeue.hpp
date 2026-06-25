#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class FrameQueue
{
public: 
    void push(T&& value)
    {
        {
            std::lock_guard lock(mMutex);
            if (mQueue.size() >= mMaxSize)
                mQueue.pop();
            mQueue.push(std::move(value));
        }
        mCondition.notify_one();
    }

    bool pop(T& value)
    {
        std::unique_lock lock(mMutex);

        mCondition.wait(lock, [&]
        {
            return !mQueue.empty() || !mRunning;
        });

        if (!mRunning)
            return false;

        value = std::move(mQueue.front());
        mQueue.pop();

        return true;
    }

    void stop()
    {
        {
            std::lock_guard lock(mMutex);
            mRunning = false;
        }

        mCondition.notify_all();
    }

    bool tryPop(T& value, std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(mMutex);
        if (!mCondition.wait_for(lock, timeout, [&]{
            return !mQueue.empty() || !mRunning;
        })) return false;
        if (!mRunning) return false;
        value = std::move(mQueue.front());
        mQueue.pop();
        return true;
    }

private:
    size_t mMaxSize = 4;
    
    bool mRunning = true;

    std::queue<T> mQueue;

    std::mutex mMutex;
    std::condition_variable mCondition;
};