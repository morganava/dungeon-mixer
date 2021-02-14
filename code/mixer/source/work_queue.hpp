#pragma once

#include "enums.hpp"
#include "singleton.hpp"

namespace dm
{
    // this type of task is run once and then deletes itself
    class run_once_task
    {
    public:
        run_once_task() = default;
        run_once_task(const run_once_task&) = delete;
        run_once_task& operator=(const run_once_task&) = delete;
        run_once_task(run_once_task&& that)
        {
            *this = std::move(that);
        }
        run_once_task& operator=(run_once_task&& that)
        {
            this->_run = that._run;
            this->_callable = that._callable;
            that._run = nullptr;
            that._callable = nullptr;

            return *this;
        }

        template<typename func_t>
        run_once_task(func_t&& lambda)
        {
            // convertible to raw function pointer
            if constexpr (std::is_convertible<func_t, void(*)(void)>::value)
            {
                _run = [](run_once_task* self)
                {
                    auto lambda = reinterpret_cast<void(*)(void)>(self->_callable);
                    lambda();
                };
                _callable = reinterpret_cast<void*>(static_cast<void(*)(void)>(lambda));
            }
            // small lambda optimization
            else if constexpr (sizeof(func_t) <= sizeof(_callable))
            {
                new(&_callable) func_t(std::move(lambda));
                _run = [](run_once_task* self)
                {
                    auto* plambda = reinterpret_cast<func_t*>(&self->_callable);
                    (*plambda)();
                    plambda->~func_t();
                };
            }
            // new heap copy
            else
            {
                _callable = new func_t(std::move(lambda));
                _run = [](run_once_task* self)
                {
                    auto* plambda = reinterpret_cast<func_t*>(self->_callable);
                    (*plambda)();
                    delete plambda;
                };
            }
        }

        void run()
        {
            _run(this);
            _run = nullptr;
            _callable = nullptr;
        };
    private:
        void(*_run)(run_once_task*) = nullptr;
        void* _callable = nullptr;
    };

    // same as future but try_wait first checks to see if the future is valid
    class task_future : public std::future<void>
    {
    public:
        task_future() = default;
        task_future(std::future<void>&& f) : std::future<void>(std::move(f)) {}
        void try_wait();
    };

    MX_MAKE_ENUM(worker_id, size_t,
        ui,
        fileio,
        background,
        audio,
        logging
    );

    typedef std::chrono::time_point<std::chrono::system_clock> timestamp;

    class work_manager
    {
        struct task_item
        {
            // work to do
            run_once_task function;
            // task runs no earlier than this
            timestamp start_time;
            // promise used to notify the caller's future
            std::promise<void> promise;
        };

        typedef struct
        {
            inline bool operator()(const task_item& left, const task_item& right)
            {
                return left.start_time < right.start_time;
            }
        } task_item_comparator_t;

    public:
        work_manager();
        ~work_manager();

        bool accepting_work() const { return _accepting_work; }

        // run all of the tasks in the given worker's queue
        // returns the timestamp of the last enqueued task
        timestamp do_tasks(worker_id_t id);

        task_future enqueue(size_t workerIndex, run_once_task&& work, timestamp startTime);

        worker_id current_thread() const;
    private:
        size_t current_index() const;

        std::future<void> enqueue_impl(size_t workerIndex, run_once_task&& work, timestamp startTime);
        void suspend_worker(size_t workerIndex);
        void notify_worker(size_t workerIndex);

        std::atomic_bool _accepting_work = true;

        constexpr static size_t _worker_count = static_cast<size_t>(worker_id::count);

        // mutexes for thread data synchronization
        std::mutex _queue_mutex[_worker_count];

        // other threads add tasks to producer thread
        std::vector<task_item> _producer_queue[_worker_count];
        // worker swaps queues and reads from consumer thread
        std::vector<task_item> _consumer_queue[_worker_count];
        // the last time the thread was working
        timestamp _completion_time[_worker_count];
        // condition variables for suspending/resuming threads
        std::condition_variable _suspend_handle[_worker_count];

        std::thread _threads[_worker_count];
        // only used for verification purposes
        std::thread::id _thread_ids[_worker_count];

        friend class worker;
        friend void worker_thread(worker_id_t);
    };
    MX_MAKE_SINGLETON(work_manager);

    class worker
    {
    private:
        const size_t _worker_index;
    public:
        constexpr worker(worker_id_t id) : _worker_index(static_cast<size_t>(id)) {}
        task_future enqueue(run_once_task&& task) const;
        task_future enqueue(run_once_task&& task, timestamp startTime) const;
        template<typename Rep, typename Period>
        task_future enqueue(run_once_task&& task, std::chrono::duration<Rep, Period> delay) const
        {
            return enqueue(std::move(task), std::chrono::system_clock::now() + delay);
        }
    };

    constexpr worker worker_ui(worker_id::ui);
    constexpr worker worker_fileio(worker_id::fileio);
    constexpr worker worker_background(worker_id::background);
    constexpr worker worker_audio(worker_id::audio);
    constexpr worker worker_logging(worker_id::logging);
}