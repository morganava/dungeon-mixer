#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
    // WorkerManager

    void worker_thread(worker_id_t id)
    {
        const auto workerIndex = static_cast<size_t>(id);
        g_work_manager._thread_ids[workerIndex] = std::this_thread::get_id();

        while(g_work_manager.accepting_work())
        {
            // empty queue of work
            const auto lastTask = g_work_manager.do_tasks(id);
            const auto now = std::chrono::system_clock::now();
            const auto diff = now - lastTask;

            if(diff < 1ms)
            {
                continue;
            }
            else if(diff < 16ms)
            {
                std::this_thread::sleep_for(1ms);
            }
            else
            {
                std::this_thread::sleep_for(16ms);
            }
        }
    }

    work_manager::work_manager()
    {
        // set the main thread id
        static_assert(0 == static_cast<size_t>(worker_id::ui));
        _thread_ids[0] = std::this_thread::get_id();

        const auto now = std::chrono::system_clock::now();
        // init threads, start on 1 because 0 is main thread
        for(size_t k = 1; k < _worker_count; k++)
        {
            _last_task[k] = now;
            _threads[k] = std::thread(&worker_thread, static_cast<worker_id_t>(k));
        }
    }

    work_manager::~work_manager()
    {
        _accepting_work = false;

        for(uint_fast8_t k = 1; k < _worker_count; k++)
        {
            _threads[k].join();
        }
    }

    work_manager::timestamp work_manager::do_tasks(worker_id_t id)
    {
        const size_t index = static_cast<size_t>(id);
        MX_ASSERT(g_work_manager.current_index() == index);

        timestamp retval;

        while(true)
        {
            {
                std::lock_guard<std::mutex> lock(_queue_mutex[index]);
                std::swap(_producer_queue[index], _consumer_queue[index]);
                retval = _last_task[index];
            }
            auto& queue = _consumer_queue[index];
            if (queue.empty())
            {
                return retval;
            }

            for(auto& task : queue)
            {
                task.function.run();
                task.promise.set_value();
            }
            queue.clear();
        }
    }

    size_t work_manager::current_index() const
    {
        const thread_local size_t index = []()
        {
            const auto id = std::this_thread::get_id();
            for(size_t k = 0; k < g_work_manager._worker_count; k++)
            {
                if (id == g_work_manager._thread_ids[k]) {
                    return k;
                }
            }
            MX_FAIL_FAST("current_index called from invalid thread");
            return static_cast<size_t>(worker_id::invalid);
        }();

        return index;
    }

    std::future<void> work_manager::enqueue(size_t workerIndex, run_once_task&& work)
    {
        std::future<void> retval;
        if (_accepting_work)
        {
            std::lock_guard<std::mutex> lock(_queue_mutex[workerIndex]);
            task_item task = {std::move(work)};
            retval = task.promise.get_future();
            _producer_queue[workerIndex].push_back(std::move(task));
            _last_task[workerIndex] = std::chrono::system_clock::now();
        }
        return retval;
    }

     void work_manager::enqueue_blocking(size_t workerIndex, run_once_task&& work)
    {
        if (_accepting_work)
        {
            auto fut = enqueue(workerIndex, std::move(work));
            fut.wait();
        }
    }

    // Worker

    std::future<void> worker::enqueue(run_once_task&& task) const
    {
        return g_work_manager.enqueue(_worker_index, std::move(task));
    }

    void worker::enqueue_blocking(run_once_task&& task) const
    {
        g_work_manager.enqueue_blocking(_worker_index, std::move(task));
    }
}