#pragma once

namespace tabletopify
{
    struct work_item
    {
        // work to do
        std::function<void(void)> func;
        // consumer sets to true when finished
        volatile bool* finished;
    };

    enum worker_id
    {
        Worker_UI = 0,
        Worker_FileIO,
        Worker_Background,

        Worker_Count,
    };

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
    extern worker_manager* g_worker_manager;

    class worker
    {
    private:
        const size_t _index;
    public:
        worker(size_t index) : _index(index) {}
        void enqueue(std::function<void(void)>&& work);
        void enqueue(std::function<void(void)>&& work, volatile bool* finished);
        bool try_dequeue(work_item& out_work);
        float nextFloat(float min, float max);
        size_t nextIndex(size_t count);
    };
}