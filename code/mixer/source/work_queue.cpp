#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
    // WorkerManager

    void worker_thread(worker_id_t id)
    {
        g_work_manager._thread_ids[static_cast<size_t>(id)] = std::this_thread::get_id();

        while(g_work_manager.accepting_work())
        {
            // empty queue of work
            g_work_manager.do_tasks(id);
            // sleep for 1 ms before looking for work
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    work_manager::work_manager()
    {
        // set the main thread id
        static_assert(0 == static_cast<size_t>(worker_id::ui));
        _thread_ids[0] = std::this_thread::get_id();

        // init threads, start on 1 because 0 is main thread
        for(size_t k = 1; k < _worker_count; k++)
        {
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

    void work_manager::do_tasks(worker_id_t id)
    {
        const size_t workerIndex = static_cast<size_t>(id);
        MX_ASSERT(g_work_manager.current_index() == workerIndex);
        for(auto opttask = g_work_manager.dequeue(workerIndex);
            opttask != std::nullopt;
            opttask = g_work_manager.dequeue(workerIndex))
        {
            auto& task = opttask.value();
            task.function.run();
            task.promise.set_value();
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
            _queue[workerIndex].push(std::move(task));
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

    std::optional<task_item> work_manager::dequeue(size_t workerIndex)
    {
        MX_ASSERT(_thread_ids[workerIndex] == std::this_thread::get_id());

        std::lock_guard<std::mutex> lock(_queue_mutex[workerIndex]);
        if(_queue[workerIndex].size() > 0)
        {
            std::optional<task_item> retval = std::move(_queue[workerIndex].front());
            _queue[workerIndex].pop();
            return retval;
        }
        return std::nullopt;
    }

    // Worker

    std::future<void> worker::enqueue(run_once_task&& task)
    {
        return g_work_manager.enqueue(_worker_index, std::move(task));
    }

    void worker::enqueue_blocking(run_once_task&& task)
    {
        g_work_manager.enqueue_blocking(_worker_index, std::move(task));
    }


    bool worker::try_dequeue(task_item& out_work)
    {
        auto retval = g_work_manager.dequeue(_worker_index);
        if (retval == std::nullopt) {
            return false;
        }
        out_work = std::move(retval.value());
        return true;
    }

    float worker::nextFloat(float min, float max)
    {
        MX_ASSERT(min <= max);
        MX_ASSERT(g_work_manager._thread_ids[_worker_index] == std::this_thread::get_id());

        std::uniform_real_distribution<float> uniform(min, max);
        return uniform(g_work_manager._rand[_worker_index]);
    }

    size_t worker::nextIndex(size_t count)
    {
        MX_ASSERT(count > 0);
        MX_ASSERT(g_work_manager._thread_ids[_worker_index] == std::this_thread::get_id());

        std::uniform_int_distribution<size_t> uniform(size_t(0), count - 1);
        return  uniform(g_work_manager._rand[_worker_index]);
    }

    uuid worker::nextUUID()
    {
        MX_ASSERT(g_work_manager._thread_ids[_worker_index] == std::this_thread::get_id());

        uuid result;
        std::uniform_int_distribution<uint64_t> uniform(uint64_t(0), ~uint64_t(0));
        auto& rand = g_work_manager._rand[_worker_index];
        result.qwords[0] = uniform(rand);
        result.qwords[1] = uniform(rand);

        // set version to 4
        result.bytes[6] = (result.bytes[6] & 0x0F) | 0x40;
        // set variant to 1
        result.bytes[8] = (result.bytes[8] & 0x3F) | 0x80;

        return result;
    }
}