#ifndef __PTS_QUEUE_H__
#define __PTS_QUEUE_H__

#include "mpmc_queue.h"
#include <set>
#include <thread>

class PtsQueue
{
public:
    PtsQueue() : _last_pts(0xF000000000000000), _capacity(3), _enabled(true)
    {
    }

    void enable()
    {
        std::lock_guard<std::mutex> lck(_mutex);
        _enabled = true;
    }

    bool enabled()
    {
        std::lock_guard<std::mutex> lck(_mutex);
        return _enabled;
    }

    void disable()
    {
        std::lock_guard<std::mutex> lck(_mutex);
        _enabled = false;
    }

    MpmcResult WaitForTurn(uint64_t pts)
    {
        using namespace std::chrono_literals;
        {
            std::unique_lock<std::mutex> lck(_mutex);
            _ptss.insert(pts);
        }
        while (true)
        {
            {
                std::unique_lock<std::mutex> lck(_mutex);
                if (!_enabled)
                {
                    return MpmcResult::DISABLED;
                }
                auto p = std::min_element(_ptss.begin(), _ptss.end());
                if (*p == pts)
                {
                    if (pts < _last_pts + 50000000)
                    {
                        _ptss.erase(p);
                        return MpmcResult::SUCCESS;
                    }
                    if (_ptss.size() > _capacity)
                    {
                        _ptss.erase(p);
                        return MpmcResult::TIMEOUT;
                    }
                }
            }
            std::this_thread::sleep_for(0.001s);
        }
    }

    void MarkTurnComplete(uint64_t pts)
    {
        {
            std::lock_guard<std::mutex> lck(_mutex);
            _last_pts = pts;
        }
        _cv.notify_all();
    }

private:
    std::mutex _mutex;
    std::condition_variable _cv;
    uint64_t _last_pts;
    std::set<uint64_t> _ptss;
    size_t _capacity;
    bool _enabled;
};

#endif