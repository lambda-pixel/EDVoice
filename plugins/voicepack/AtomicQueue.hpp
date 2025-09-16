#pragma once

#include <mutex>
#include <queue>


template<typename T>
class AtomicQueue
{
public:
    void push(const T& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queque.push(value);
    }

    void pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queque.pop();
    }

    T front() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queque.front();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queque.size();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queque.empty();
    }

    bool has(const T& value) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<T> copy = m_queque;
        while (!copy.empty()) {
            if (copy.front() == value) return true;
            copy.pop();
        }
        return false;
    }

private:
    std::queue<T> m_queque;
    mutable std::mutex m_mutex;
};
