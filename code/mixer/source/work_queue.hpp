#pragma once

#include "uuid.hpp"

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

    enum worker_id
    {
        Worker_UI = 0,
        Worker_FileIO,
        Worker_Background,
        Worker_Logging,

        Worker_Count,
    };

    // https://godbolt.org/z/9L1Rda

    class worker_manager
    {
    public:
        void initialize();
        bool accepting_work() const { return _accepting_work; }
        void finish();

    private:
        std::atomic_bool _accepting_work = true;

        std::mutex _mutex[Worker_Count];
        std::queue<work_item> _queue[Worker_Count];
        std::thread _threads[Worker_Count];
        std::thread::id _thread_ids[Worker_Count];

        std::minstd_rand _rand[Worker_Count];

        friend class worker;
        friend void worker_thread(size_t);
    };

    class worker
    {
    private:
        const size_t _index;
    public:
        worker(size_t index) : _index(index) {}
        std::thread::id getThreadId() const;
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

    void init_work_manager();
    void free_work_manager();
}