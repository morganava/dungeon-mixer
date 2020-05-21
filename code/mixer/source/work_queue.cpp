#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
    // WorkerManager

    void worker_thread(worker_id_t id)
    {
        const uint_fast8_t idx = static_cast<uint_fast8_t>(id);
        g_work_manager._thread_ids[idx] = std::this_thread::get_id();

        auto w = worker(id);
        work_item wi;

        while(g_work_manager.accepting_work())
        {
            // empty queue of work
            while(w.try_dequeue(wi))
            {
                wi.func();
                if(wi.finished != nullptr)
                {
                    *wi.finished = true;
                }
                wi = {};
            }
            // sleep for 1 ms before looking for work
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    work_manager::work_manager()
    {
        // set the main thread id
        static_assert(0 == static_cast<uint_fast8_t>(worker_id::ui));
        _thread_ids[0] = std::this_thread::get_id();

        // init threads, start on 1 because 0 is main thread
        for(uint_fast8_t k = 1; k < _worker_count; k++)
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

    void work_manager::enqueue(size_t workerIndex, run_once_task&& work)
    {
        enqueue(workerIndex, {std::move(work), nullptr});
    }

    void work_manager::enqueue(size_t workerIndex, run_once_task&& work, volatile bool* finished)
    {
        enqueue(workerIndex, {std::move(work), finished});
    }

    void work_manager::enqueue(size_t workerIndex, work_item&& workItem)
    {
        if (_accepting_work)
        {
            std::lock_guard<std::mutex> lock(_queue_mutex[workerIndex]);
            _queue[workerIndex].push(std::move(workItem));
        }
    }

    std::optional<work_item> work_manager::dequeue(size_t workerIndex)
    {
        MX_ASSERT(_thread_ids[workerIndex] == std::this_thread::get_id());

        std::lock_guard<std::mutex> lock(_queue_mutex[workerIndex]);
        if(_queue[workerIndex].size() > 0)
        {
            std::optional<work_item> retval = std::move(_queue[workerIndex].front());
            _queue[workerIndex].pop();
            return retval;
        }
        return std::nullopt;
    }

    // Worker

    void worker::enqueue(run_once_task&& work)
    {
        this->enqueue(std::move(work), nullptr);
    }

    void worker::enqueue(run_once_task&& work, volatile bool* finished)
    {
        g_work_manager.enqueue(_worker_index, std::move(work), finished);
    }

    bool worker::try_dequeue(work_item& out_work)
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