#ifndef __TURN_QUEUE_H__
#define __TURN_QUEUE_H__

#include "mpmc_queue.h"

class TurnQueue
{
public:
    TurnQueue() : _next_turn(0), _enabled(true)
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
        _cv.notify_all();
    }

    MpmcResult WaitForTurn(int turn, std::optional<std::chrono::duration<double>> timeout = std::nullopt)
    {
        {
            std::unique_lock<std::mutex> lck(_mutex);
            if (!_enabled)
            {
                return MpmcResult::DISABLED;
            }
            if (timeout.has_value())
            {
                _cv.wait_for(lck, *timeout, [&]
                             { return turn == _next_turn || !_enabled; });
            }
            else
            {
                _cv.wait(lck, [&]
                         { return turn == _next_turn || !_enabled; });
            }
            if (!_enabled)
            {
                return MpmcResult::DISABLED;
            }
            if (turn != _next_turn)
            {
                return MpmcResult::TIMEOUT;
            }
        }
        return MpmcResult::SUCCESS;
    }

    MpmcResult MarkTurnComplete(int turn)
    {
        {
            std::lock_guard<std::mutex> lck(_mutex);
            if (!_enabled)
            {
                return MpmcResult::DISABLED;
            }
            if (turn != _next_turn)
            {
                return MpmcResult::DROPPED;
            }
            _next_turn++;
        }
        _cv.notify_all();
        return MpmcResult::SUCCESS;
    }

private:
    std::mutex _mutex;
    std::condition_variable _cv;
    int _next_turn;
    bool _enabled;
};

#endif