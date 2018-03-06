#include "precomp.hpp"

#include "utilities.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
    // WorkerManager

    worker_manager* g_worker_manager = nullptr;

    void worker_thread(size_t idx)
    {
        g_worker_manager->_thread_ids[idx] = std::this_thread::get_id();

        auto w = worker(idx);
        work_item wi;

        while(g_worker_manager->accepting_work())
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
            sleep_for(1);
        }
    }

    void worker_manager::initialize()
    {
        // set the main thread id
        _thread_ids[Worker_UI] = std::this_thread::get_id();

        // init threads, start on 1 because 0 is main thread
        for(size_t k = 1; k < Worker_Count; k++)
        {
            _threads[k] = std::thread(&worker_thread, k);
        }
    }

    void worker_manager::finish()
    {
        std::cout << "Finishing Work Queues" << std::endl;

        _accepting_work = false;

        std::cout << "Waiting for workers" << std::endl;
        for(size_t k = 1; k < Worker_Count; k++)
        {
            _threads[k].join();
        }
    }

    // Worker

    void worker::enqueue(std::function<void(void)>&& work)
    {
        this->enqueue(std::forward<std::function<void(void)>>(work), nullptr);
    }

    void worker::enqueue(std::function<void(void)>&& work, volatile bool* finished)
    {
        if(g_worker_manager->_accepting_work)
        {
            std::lock_guard<std::mutex> lock(g_worker_manager->_mutex[_index]);
            work_item wi = {work, finished};
            g_worker_manager->_queue[_index].emplace(std::move(wi));
        }
    }

    bool worker::try_dequeue(work_item& out_work)
    {
        assert(g_worker_manager->_thread_ids[_index] == std::this_thread::get_id());

        std::lock_guard<std::mutex> lock(g_worker_manager->_mutex[_index]);
        if(g_worker_manager->_queue[_index].size() > 0)
        {
            out_work = std::move(g_worker_manager->_queue[_index].front());
            g_worker_manager->_queue[_index].pop();
            return true;
        }
        return false;
    }

    float worker::nextFloat(float min, float max)
    {
        assert(min <= max);
        assert(g_worker_manager->_thread_ids[_index] == std::this_thread::get_id());

        std::uniform_real_distribution<float> uniform(min, max);
        return uniform(g_worker_manager->_rand[_index]);
    }

    size_t worker::nextIndex(size_t count)
    {
        assert(count > 0);
        assert(g_worker_manager->_thread_ids[_index] == std::this_thread::get_id());

        std::uniform_int_distribution<size_t> uniform(size_t(0), count - 1);
        return  uniform(g_worker_manager->_rand[_index]);
    }

    uuid worker::nextUUID()
    {
        assert(g_worker_manager->_thread_ids[_index] == std::this_thread::get_id());

        uuid result;
        std::uniform_int_distribution<uint8_t> uniform(uint8_t(0), ~uint8_t(0));
        auto& rand = g_worker_manager->_rand[_index];
        for(auto& val : result.raw)
        {
            val = uniform(rand);
        }

        // set version to 4
        result.raw[6] = (result.raw[6] & 0x0F) | 0x40;
        // set variant to 1
        result.raw[8] = (result.raw[8] & 0x3F) | 0x80;

        return result;
    }
}