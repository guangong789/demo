#include "utility/threadpool.h"

Threadpool::Threadpool(int min, int max) : m_stop(false), m_idleThread(min), m_curThread(min) {
    // 创建工作线程
    for (int i = 0; i < min; ++i) {
        m_workers.emplace_back(&Threadpool::worker, this);
    }
}

Threadpool::~Threadpool() {
    {
        std::lock_guard<std::mutex> locker(m_queueMutex);
        m_stop = true;
    }
    m_condition.notify_all();
    for (auto& it : m_workers) {  // 线程对象不允许拷贝
        if (it.joinable()) {
            it.join();
        }
    }
}

void Threadpool::worker() {
    while (!m_stop) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> locker(m_queueMutex);
            m_condition.wait(locker, [this]() {
                return m_stop || !m_tasks.empty();
            });
            // 如果线程池将关闭或者任务队列不为空，就唤醒线程，否则线程继续休眠
            if (m_stop && m_tasks.empty()) {
                return;
            }
            task = move(m_tasks.front());
            m_tasks.pop();
        }
        m_idleThread--;
        task();
        m_idleThread++;
    }
}