#ifndef __MPMC_QUEUE_H__
#define __MPMC_QUEUE_H__

#include <mutex>
#include <condition_variable>
#include <deque>
#include <string>
#include <optional>
#include <chrono>

enum class MpmcFullBehavior
{
    BLOCK,
    DROP,
    DISCARD_OLDEST
};

enum class MpmcResult
{
    SUCCESS,
    TIMEOUT,
    DROPPED,
    DISABLED
};

template <typename T>
class MpmcQueue
{
public:
    MpmcQueue(std::string name, size_t capacity) : _name(name), _capacity(capacity), _enabled(true)
    {
    }

    void enable()
    {
        std::lock_guard<std::mutex> lck(_mutex);
        _enabled = true;
    }

    bool enabled()
    {
        return _enabled;
    }

    void disable()
    {
        {
            std::lock_guard<std::mutex> lck(_mutex);
            _enabled = false;
        }
        _item_pushed_cv.notify_all();
        _item_popped_cv.notify_all();
    }

    MpmcResult push(T item, MpmcFullBehavior behavior, std::optional<std::chrono::duration<double>> timeout = std::nullopt)
    {
        {
            std::unique_lock<std::mutex> lck(_mutex);
            if (!_enabled)
            {
                return MpmcResult::DISABLED;
            }
            if (_queue.size() >= _capacity)
            {
                if (behavior == MpmcFullBehavior::BLOCK)
                {
                    bool success = true;
                    if (timeout.has_value())
                    {
                        success = _item_popped_cv.wait_for(lck, *timeout, [&]
                                                           { return _queue.size() < _capacity || !_enabled; });
                    }
                    else
                    {
                        _item_popped_cv.wait(lck, [&]
                                             { return _queue.size() < _capacity || !_enabled; });
                    }
                    if (!_enabled)
                    {
                        return MpmcResult::DISABLED;
                    }
                    if (!success)
                    {
                        return MpmcResult::TIMEOUT;
                    }
                }
                else if (behavior == MpmcFullBehavior::DROP)
                {
                    return MpmcResult::DROPPED;
                }
                else if (behavior == MpmcFullBehavior::DISCARD_OLDEST)
                {
                    _queue.pop_front();
                }
            }
            _queue.push_back(item);
        }
        _item_pushed_cv.notify_one();
        return MpmcResult::SUCCESS;
    }

    MpmcResult pop(T &item, std::optional<std::chrono::duration<double>> timeout = std::nullopt)
    {
        std::unique_lock<std::mutex> lck(_mutex);
        if (_queue.empty())
        {
            if (!_enabled)
            {
                return MpmcResult::DISABLED;
            }
            bool success = true;
            if (timeout.has_value())
            {
                success = _item_pushed_cv.wait_for(lck, *timeout, [&]
                                                   { return !_queue.empty() || !_enabled; });
            }
            else
            {
                _item_pushed_cv.wait(lck, [&]
                                     { return !_queue.empty() || !_enabled; });
            }
            if (!_enabled)
            {
                return MpmcResult::DISABLED;
            }
            if (!success)
            {
                return MpmcResult::TIMEOUT;
            }
        }
        item = _queue.front();
        _queue.pop_front();
        _item_popped_cv.notify_one();
        return MpmcResult::SUCCESS;
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lck(_mutex);
        return _queue.size();
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lck(_mutex);
        return _queue.empty();
    }

    bool full()
    {
        std::lock_guard<std::mutex> lck(_mutex);
        return _queue.size() >= _capacity;
    }

    size_t capacity()
    {
        return _capacity;
    }

private:
    std::mutex _mutex;
    std::condition_variable _item_pushed_cv;
    std::condition_variable _item_popped_cv;
    std::deque<T> _queue;
    std::string _name;
    const size_t _capacity;
    bool _enabled;
};

#endif