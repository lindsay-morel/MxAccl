#ifndef MX_UTILS
#define MX_UTILS

#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>
#include <unistd.h>
#include <sys/syscall.h>
#include <iostream>

using namespace std;

namespace MX
{
    namespace Utils
    {
        template <typename T>
        class fifo_queue
        {
        private:
            std::queue<T> m_queue;

        public:
            std::mutex m_mutex;

            size_t size()
            {
                lock_guard<mutex> lock(m_mutex);
                return m_queue.size();
            }
            void push(T item)
            {
                lock_guard<mutex> lock(m_mutex);
                m_queue.push(item);
            }
            T pop()
            {
                lock_guard<mutex> lock(m_mutex);
                T item = m_queue.front();
                m_queue.pop();
                return item;
            }
            fifo_queue &operator=(const fifo_queue &rhs) // copy assignment
            {
                if (this == &rhs)
                {
                    return *this;
                }
            }
        };

        template <typename T1, typename T2>
        class fifo_deque
        {
        private:
            // std::deque<T> m_queue;

        public:
            std::mutex m_mutex;
            std::deque<std::pair<T1,T2>> m_queue;

            size_t size()
            {
                lock_guard<mutex> lock(m_mutex);
                return m_queue.size();
            }
            void push(std::pair<T1,T2> item)
            {
                lock_guard<mutex> lock(m_mutex);
                m_queue.push_back(item);
            }
            std::pair<T1,T2> pop()
            {
                lock_guard<mutex> lock(m_mutex);
                std::pair<T1,T2> item = m_queue.pop_front();
                return item;
            }
            std::pair<T1,T2> get()
            {
                lock_guard<mutex> lock(m_mutex);
                std::pair<T1,T2> item = m_queue[0];
                return item;
            }
            std::optional<std::pair<T1,T2>> ifPophold(std::pair<T1,T2> item){
                m_mutex.lock();
                if(m_queue.size()==0) return {};
                std::pair<T1,T2> front = m_queue[0];
                if(item.first==front.first){
                    m_queue.pop_front();
                    return front;
                }
                return {};                
            }
        };

        typedef struct retval{
            bool error_flag;
            std::string error_msg;
            retval(bool flag){
                error_flag = flag;
            }
            retval(){};
        } mx_retval;

        void mx_checkandthrow(mx_retval ret);
        void mx_checkandprint(mx_retval ret);
    } // namespace Utils
} // namespace MX

#endif