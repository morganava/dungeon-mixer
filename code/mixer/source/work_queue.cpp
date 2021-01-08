#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"

namespace dm
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

            // don't sleep immediately
            if(diff < 1ms)
            {
                continue;
            }
            // sleep only for a bit until two frames
            else if(diff < 32ms)
            {
                std::this_thread::sleep_for(1ms);
            }
            // sleep for a frame until 1 second has elapsed (60 frames)
            else if (diff < 1s)
            {
                std::this_thread::sleep_for(16ms);
            }
            // finally suspend
            else
            {
                g_work_manager.suspend_worker(workerIndex);
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
            // init the last queued task with current time
            _last_task[k] = now;
            _threads[k] = std::thread(&worker_thread, static_cast<worker_id_t>(k));
        }
    }

    work_manager::~work_manager()
    {
        _accepting_work = false;

        for(size_t k = 1; k < _worker_count; k++)
        {
            notify_worker(k);
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
                try
                {
                    task.function.run();
                }
                catch(const std::runtime_error& ex)
                {
                    MX_LOG_ERROR(ex.what());
                }
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

    void work_manager::suspend_worker(size_t workerIndex)
    {
        auto& m = _queue_mutex[workerIndex];
        std::unique_lock<std::mutex> ul(m);
        auto& queue = _producer_queue[workerIndex];

        // only suspend if our queue is empty
        if (queue.empty())
        {
            if (workerIndex != worker_id::logging)
            {
                MX_LOG_INFO("Suspending {}", static_cast<worker_id_t>(workerIndex));
            }

            auto& cv = _suspend_handle[workerIndex];
            cv.wait(ul);

            if (workerIndex != worker_id::logging)
            {
                MX_LOG_INFO("Resuming {}", static_cast<worker_id_t>(workerIndex));
            }
        }
    }

    void work_manager::notify_worker(size_t workerIndex)
    {
        auto& cv = _suspend_handle[workerIndex];
        cv.notify_one();
    }

    std::future<void> work_manager::enqueue_impl(size_t workerIndex, run_once_task&& work)
    {
        std::lock_guard<std::mutex> lock(_queue_mutex[workerIndex]);

        task_item task(std::move(work));
        auto retval = task.promise.get_future();

        _producer_queue[workerIndex].push_back(std::move(task));
        _last_task[workerIndex] = std::chrono::system_clock::now();

        return retval;
    }

    std::future<void> work_manager::enqueue(size_t workerIndex, run_once_task&& work)
    {
        std::future<void> retval;
        if (_accepting_work)
        {
            retval = enqueue_impl(workerIndex, std::move(work));
            notify_worker(workerIndex);
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