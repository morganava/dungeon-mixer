#include "logger.hpp"
#include "uuid.hpp"
#include "utilities.hpp"
#include "work_queue.hpp"

namespace dm
{
    bool can_call_from_current_thread(worker_flag_t allowedThreads)
    {
        auto currentThread = g_work_manager.unsafe_get().current_thread();
        auto currentThreadFlag = static_cast<worker_flag_t>(1 << static_cast<size_t>(currentThread));

        auto retval = (allowedThreads & currentThreadFlag) != worker_flag::none;
        if (!retval) {
            MX_LOG_ERROR("Called from thread: {}", currentThread);
        }

        return retval;
    }

    // WorkerManager

    void worker_thread(worker_id_t id)
    {
        const auto workerIndex = static_cast<size_t>(id);

        // store off our threadId
        g_work_manager_unsafe._thread_ids[workerIndex] = std::this_thread::get_id();
        bool const& terminating = g_work_manager_unsafe._terminating[workerIndex];

        // keep spinning until there are no more pending tasks
        while(!terminating || g_work_manager_unsafe._pending_tasks > 0)
        {
            // executes all queued tasks and returns timestamp of last time
            // this thread did work (so if there are no pending tasks, a cached timestamp
            // is returned)
            const auto completionTime = g_work_manager_unsafe.do_tasks(id);

            const auto now = std::chrono::system_clock::now();
            const auto diff = now - completionTime;

            // don't sleep immediately
            if(diff < 1ms)
            {
                continue;
            }
            // sleep only for a bit until two frames have elapsed
            else if(diff < 32ms)
            {
                std::this_thread::sleep_for(1ms);
            }
            // sleep for a frame until 1 second has elapsed (60 frames)
            else if (diff < 1s)
            {
                std::this_thread::sleep_for(16ms);
            }
            // finally suspend if no new tasks have been added to the producer queue for over a second
            else
            {
                // protect the suspend with the queue lock and ensure no notify sneaks
                // through right before we suspend
                auto& m = g_work_manager_unsafe._queue_mutex[workerIndex];
                std::unique_lock<std::mutex> ul(m);
                auto& queue = g_work_manager_unsafe._producer_queue[workerIndex];

                // only suspend if we have no pending tasks
                if (queue.empty())
                {
                    auto& cv = g_work_manager_unsafe._suspend_handle[workerIndex];
                    cv.wait(ul);
                }
            }
        }
    }

    work_manager::work_manager()
    {
        // set the main thread id
        static_assert(0 == static_cast<size_t>(worker_id::ui));
        _thread_ids[0] = std::this_thread::get_id();
        _terminating[0] = false;

        const auto now = std::chrono::system_clock::now();
        // init threads, start on 1 because 0 is main thread
        for(size_t k = 1; k < _worker_count; k++)
        {
            // init the last queued task with current time
            _completion_time[k] = now;
            // init termination flags
            _terminating[k] = false;
            // start thread
            _threads[k] = std::thread(&worker_thread, static_cast<worker_id_t>(k));
        }
    }

    work_manager::~work_manager()
    {
        _terminating[0] = true;

        // notify each worker that we are terminating
        for(size_t k = 1; k < _worker_count; k++)
        {
            // assuming our enqueue method properly wakes sleeping threads
            // this will never deadlock
            enqueue_impl(k, [workerIndex=k]() -> void
            {
                g_work_manager_unsafe._terminating[workerIndex] = true;
            }, std::chrono::system_clock::now());
        }

        // wait for threads to complete their work
        for(size_t k = 1; k < _worker_count; k++)
        {
            _threads[k].join();
        }
    }

    timestamp_t work_manager::do_tasks(worker_id_t id)
    {
        const size_t index = static_cast<size_t>(id);
        MX_ASSERT(this->current_index() == index);

        auto& completionTime = _completion_time[index];

        while(true)
        {
            auto& consumer = _consumer_queue[index];
            auto& producer = _producer_queue[index];
            // acquire our task list
            {
                std::lock_guard<std::mutex> lock(_queue_mutex[index]);
                // when we have no tasks left in our consumer queue, just swap
                if (consumer.empty())
                {
                    std::swap(consumer, producer);
                }
                // otherwise, we need to move new tasks into our consumer queue
                else
                {
                    // todo: shorten this later
                    for(auto& task : producer)
                    {
                        consumer.push_back(std::move(task));
                    }
                    producer.clear();
                }
            }
            // early out with previous time stamp if no new work
            if (consumer.empty())
            {
                return completionTime;
            }

            // sort our tasks by start time
            std::sort(consumer.begin(), consumer.end(), [](const auto& left, const auto& right) -> bool
            {
                return left.start_time < right.start_time;
            });

            for(auto it = consumer.begin(); it < consumer.end(); ++it)
            {
                auto& task = *it;
                const auto now = std::chrono::system_clock::now();
                // check to see if we need to do the next item
                if (now < task.start_time)
                {
                    // erase the completed items
                    consumer.erase(consumer.begin(), it);
                    completionTime = now;
                    return now;
                }

                try
                {
                    task.function.run();
                    _pending_tasks--;
                }
                catch(const std::runtime_error& ex)
                {
                    MX_LOG_ERROR(ex.what());
                }
                task.promise.set_value();
            }

            // if we get this far then we've executed every task in our queue, so empty it
            consumer.clear();
            _completion_time[index] = std::chrono::system_clock::now();
        }
    }

    worker_id work_manager::current_thread() const
    {
        return static_cast<worker_id>(this->current_index());
    }

    size_t work_manager::current_index() const
    {
        const thread_local size_t index = [this]()
        {
            const auto id = std::this_thread::get_id();
            for(size_t k = 0; k < this->_worker_count; k++)
            {
                if (id == this->_thread_ids[k]) {
                    return k;
                }
            }
            MX_FAIL_FAST("current_index called from invalid thread");
            return static_cast<size_t>(worker_id::invalid);
        }();

        return index;
    }

    std::future<void> work_manager::enqueue_impl(size_t workerIndex, run_once_task&& work, timestamp_t startTime)
    {
        _pending_tasks++;

        std::lock_guard<std::mutex> lock(_queue_mutex[workerIndex]);

        task_item task{std::move(work), startTime};
        auto retval = task.promise.get_future();

        _producer_queue[workerIndex].push_back(std::move(task));

        auto& cv = _suspend_handle[workerIndex];
        cv.notify_one();

        return retval;
    }

    task_future work_manager::enqueue(size_t workerIndex, run_once_task&& work, timestamp_t startTime)
    {
        auto retval = enqueue_impl(workerIndex, std::move(work), startTime);
        return task_future{std::move(retval)};
    }

    // Worker

    task_future worker::enqueue(run_once_task&& task) const
    {
        return this->enqueue(std::move(task), std::chrono::system_clock::now());
    }

    task_future worker::enqueue(run_once_task&& task, timestamp_t startTime) const
    {
        return g_work_manager->enqueue(_worker_index, std::move(task), startTime);
    }

    // task future

    void task_future::try_wait()
    {
        if (this->valid())
        {
            this->wait();
        }
    }
}
