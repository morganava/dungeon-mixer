#include "precomp.hpp"

#include "utilities.hpp"
#include "work_queue.hpp"

namespace tabletopify
{
	work_queue* g_work_queue = nullptr;

	void work_queue::enqueue(size_t idx, std::function<void(void)>&& work)
	{
		enqueue(idx, std::forward<std::function<void(void)>>(work), nullptr);
	}

	void work_queue::enqueue(size_t idx, std::function<void(void)>&& work, bool* finished)
	{
		if(_accepting_work)
		{
			std::lock_guard<std::mutex> lock(_mutex[idx]);
			work_item wi = {work, finished};
			_queue[idx].emplace(std::move(wi));
		}
	}

	bool work_queue::try_dequeue(size_t idx, work_item& out_work)
	{
		std::lock_guard<std::mutex> lock(_mutex[idx]);
		if(_queue[idx].size() > 0)
		{
			out_work = std::move(_queue[idx].front());
			_queue[idx].pop();
			return true;
		}
		return false;
	}
}