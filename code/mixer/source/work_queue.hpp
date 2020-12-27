#pragma once

#include "enums.hpp"
#include "singleton.hpp"

namespace tabletopify
{
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

    struct task_item
    {
        // work to do
        run_once_task function;
        // promise used to notify the caller's future
        std::promise<void> promise;
    };

    MX_MAKE_ENUM(worker_id, size_t,
        ui,
        fileio,
        background,
		audio,
        logging
    );

    class work_manager
    {
    public:
        work_manager();
        ~work_manager();

        bool accepting_work() const { return _accepting_work; }

        // run all of the tasks in the given worker's queue
        void do_tasks(worker_id_t id);

        std::future<void> enqueue(size_t workerIndex, run_once_task&& work);
        void enqueue_blocking(size_t workerIndex, run_once_task&& work);
    private:
        size_t current_index() const;

        std::atomic_bool _accepting_work = true;

        constexpr static size_t _worker_count = static_cast<size_t>(worker_id::count);

        std::mutex _queue_mutex[_worker_count];
        std::vector<task_item> _producer_queue[_worker_count];
        std::vector<task_item> _consumer_queue[_worker_count];
        std::thread _threads[_worker_count];
        std::thread::id _thread_ids[_worker_count];

        std::minstd_rand _rand[_worker_count];

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
        std::future<void> enqueue(run_once_task&& work) const;
        void enqueue_blocking(run_once_task&& task) const;

        worker& operator<<(run_once_task&& work) {
            this->enqueue(std::move(work));
            return *this;
        }
    };

    constexpr worker worker_ui(worker_id::ui);
    constexpr worker worker_fileio(worker_id::fileio);
    constexpr worker worker_background(worker_id::background);
    constexpr worker worker_audio(worker_id::audio);
    constexpr worker worker_logging(worker_id::logging);
}