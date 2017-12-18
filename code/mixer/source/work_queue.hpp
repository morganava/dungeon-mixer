#pragma once

namespace tabletopify
{
    enum queue_id
    {
        Queue_UI = 0,     // main thread
        Queue_FileIO,     // file read/write thread
        Queue_Background, // generic work thread

        Queue_Count,
    };

    struct work_item
    {
        // work to do
        std::function<void(void)> func;
        // consumer sets to true when finished
        bool* finished;
    };

    class work_queue
    {
    public:
        void enqueue(size_t idx, std::function<void(void)>&& work);
        void enqueue(size_t idx, std::function<void(void)>&& work, bool *finished);
        bool try_dequeue(size_t idx, work_item& out_work);

        bool accepting_work() {return _accepting_work;}
        void finish() {_accepting_work = false;}
    private:
        std::mutex _mutex[Queue_Count];
        std::queue<work_item> _queue[Queue_Count];

        std::atomic_bool _accepting_work = true;
    };

    extern work_queue* g_work_queue;
}