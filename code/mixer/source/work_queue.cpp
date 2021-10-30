#include "logger.hpp"
#include "work_queue.hpp"

namespace dm
{
    thread_local worker_flag_t work_manager::_worker_flag = worker_flag::none;

    bool can_call_from_current_thread(worker_flag_t allowedThreads)
    {
        return ((allowedThreads & g_work_manager->current_worker_flag()) != worker_flag::none);
    }

    // WorkerManager

    void worker_thread(worker_id_t id)
    {
        if (id != worker_id::ui)
        {
            smol::set_current_thread_name(std::string(to_string_view(id)));
        }

        const auto workerIndex = static_cast<size_t>(id);

        // cache off our flag to thread local var
        work_manager::_worker_flag = static_cast<worker_flag_t>(1 << workerIndex);

        // references
        auto& pending_tasks = g_work_manager_unsafe._pending_tasks;
        bool const& terminating = g_work_manager_unsafe._terminating[workerIndex];
        auto& mut = g_work_manager_unsafe._queue_mutex[workerIndex];
        auto& consumer = g_work_manager_unsafe._consumer_queue[workerIndex];
        auto& producer = g_work_manager_unsafe._producer_queue[workerIndex];
        auto& suspend_handle = g_work_manager_unsafe._suspend_handle[workerIndex];

        const auto working = [&]() -> bool
        {
            // throughout most of the programs execution, we will not be terminating
            if (!terminating) [[likely]]
            {
                return true;
            }
            // if this hits 0 there are no tasks running so we can return
            // this can probably be std::memory_order::relaxed, but I'm not sure
            // it doesn't really matter from a perf persective, since we should
            // only be executing the load when we are in the terminating state
            else if (pending_tasks.load() > 0) [[unlikely]]
            {
                return true;
            }
            else [[unlikely]]
            {
                return false;
            }
        };

        // keep spinning while there is work to do
        while(working())
        {
            // get our tasks from producer queue if any tasks available
            if(std::unique_lock<std::mutex> lock(mut); true) {

                // migrate producer tasks to our consumer queue for consuption
                if (!producer.empty())
                {
                    // todo: look into using std::list and splicing
                    for(auto& task : consumer)
                    {
                        producer.push_back(std::move(task));
                    }
                    consumer.clear();
                    std::swap(consumer, producer);
                }
                // potentially suspend worker threads
                else if(!terminating)
                {
                    if (!consumer.empty())
                    {
                        const auto startTime = consumer.front().start_time;
                        const auto now = std::chrono::steady_clock::now();
                        if (startTime > now)
                        {
                            // wait until we are ready to consume this task
                            suspend_handle.wait_until(lock, startTime);
                        }
                    }
                    else
                    {
                        // wait until a new task is enqueued
                        suspend_handle.wait(lock);
                    }
                }
            }

            // execute our tasks
            if (!consumer.empty())
            {
                // sort tasks by requsted start time
                std::sort(
                    consumer.begin(),
                    consumer.end(),
                    [](const auto& l, const auto& r) -> bool
                    {
                        return l.start_time < r.start_time;
                    });

                auto it = consumer.begin();
                for(; it < consumer.end(); ++it)
                {
                    auto& task = *it;
                    const auto now = std::chrono::steady_clock::now();
                    if (now < task.start_time)
                    {
                        break;
                    }

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

                // calculate numbr of tasks completed
                auto count = std::distance(consumer.begin(), it);
                // erase completed tasks
                consumer.erase(consumer.begin(), it);

                // decrement global task count
                pending_tasks.fetch_sub(count, std::memory_order::relaxed);
            }
        }
    }

    work_manager::work_manager()
    {
        // set the main thread id
        _terminating[0] = false;

        // init threads, start on 1 because 0 is main thread
        for(size_t k = 1; k < _worker_count; k++)
        {
            _terminating[k] = false;
            // start thread
            _threads[k] = std::thread(&worker_thread, static_cast<worker_id_t>(k));
        }
    }

    work_manager::~work_manager()
    {
        // wait for non-ui (main) workers to complete
        for(size_t k = 1; k < _worker_count; k++)
        {
            _threads[k].join();
        }
    }

    void work_manager::signal_termination()
    {
        for(size_t k = 0; k < _worker_count; k++)
        {
            enqueue_impl(k, [workerIndex=k]() -> void
            {
                g_work_manager_unsafe._terminating[workerIndex] = true;
            }, std::chrono::steady_clock::now());
        }
    }

    worker_flag_t work_manager::current_worker_flag() const
    {
        return work_manager::_worker_flag;
    }

    std::future<void> work_manager::enqueue_impl(size_t workerIndex, run_once_task&& work, timestamp_t startTime)
    {
        _pending_tasks.fetch_add(1, std::memory_order::relaxed);

        task_item task{std::move(work), startTime};
        auto retval = task.promise.get_future();

        if(std::lock_guard<std::mutex> lock(_queue_mutex[workerIndex]); true)
        {
            _producer_queue[workerIndex].push_back(std::move(task));
        }

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
        return this->enqueue(std::move(task), std::chrono::steady_clock::now());
    }

    task_future worker::enqueue(run_once_task&& task, timestamp_t startTime) const
    {
        return g_work_manager_unsafe.enqueue(_worker_index, std::move(task), startTime);
    }

    bool worker::terminating() const {
        return g_work_manager_unsafe.terminating(_worker_index);
    }

    // task future
    task_future task_future::fulfilled()
    {
        std::promise<void> p;
        auto retval = p.get_future();
        p.set_value();

        return task_future{std::move(retval)};
    }

    void task_future::try_wait()
    {
        if (this->valid())
        {
            this->wait();
        }
    }
}
