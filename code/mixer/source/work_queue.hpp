#pragma once

#include "uuid.hpp"
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

        void operator()()
        {
            _run(this);
            std::memset(this, 0x00, sizeof(*this));
        };
    private:
        void(*_run)(run_once_task*) = [](run_once_task* self)
        {
            auto lambda = reinterpret_cast<void(*)(void)>(self->_callable);
            lambda();
        };
        void* _callable = nullptr;
    };

    struct work_item
    {
        // work to do
        run_once_task func;
        // consumer sets to true when finished
        volatile bool* finished;    // TODO: get rid of volatile, replace wth std::mutex* or atomic_flag
    };

    MX_MAKE_ENUM(worker_id, size_t,
        ui,
        fileio,
        background,
        logging
    );


    // https://godbolt.org/z/9L1Rda

    class work_manager
    {
    public:
        work_manager();
        ~work_manager();

        void initialize();
        bool accepting_work() const { return _accepting_work; }
        void finish();

        void enqueue(size_t workerIndex, run_once_task&& work);
        void enqueue(size_t workerIndex, run_once_task&& work, volatile bool* finished);
        void enqueue(size_t workerIndex, work_item&& workItem);

        std::optional<work_item> dequeue(size_t workerIndex);

    private:
        std::atomic_bool _accepting_work = true;

        constexpr static size_t _worker_count = static_cast<size_t>(worker_id::count);

        std::mutex _queue_mutex[_worker_count];
        std::queue<work_item> _queue[_worker_count];
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
        void enqueue(run_once_task&& work);
        void enqueue(run_once_task&& work, volatile bool* finished);

        worker& operator<<(run_once_task&& work) {
            this->enqueue(std::move(work));
            return *this;
        }

        bool try_dequeue(work_item& out_work);
        float nextFloat(float min, float max);
        size_t nextIndex(size_t count);
        uuid nextUUID();
    };
}