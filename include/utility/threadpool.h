#pragma once
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>  // 条件变量
#include <future>
#include <type_traits>
#include <memory>
#include <vector>
#include <queue>

class Threadpool {
   public:
    Threadpool(int min = 4, int max = std::thread::hardware_concurrency());
    ~Threadpool();

    template <typename F, typename... Args>
    auto add_task(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;
        // 1. 打包任务函数（带参数）
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        // 2. 获取 future，用于异步取结果
        std::future<return_type> res = task->get_future();
        // 3. 加入任务队列（加锁保护）
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_stop.load())
                throw std::runtime_error("ThreadPool is stopped, cannot add task.");
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_condition.notify_one();
        return res;
    }

   private:
    void worker();

   private:
    std::vector<std::thread> m_workers;
    std::atomic<int> m_curThread;
    std::atomic<int> m_idleThread;              // 空闲线程数量
    std::atomic<bool> m_stop;                   // 开关
    std::queue<std::function<void()>> m_tasks;  // 可调用对象
    std::mutex m_queueMutex;                    // 互斥锁
    std::condition_variable m_condition;        // 条件变量
};