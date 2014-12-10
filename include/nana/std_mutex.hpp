#ifndef NANA_STD_MUTEX_HPP
#define NANA_STD_MUTEX_HPP
#include <nana/config.hpp>

#if defined(STD_THREAD_NOT_SUPPORTED)
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/locks.hpp>

namespace std
{
    template<typename Mutex>
    using lock_guard = boost::lock_guard<Mutex>;

    template<typename Mutex>
    using unique_lock = boost::unique_lock<Mutex>;

    typedef boost::mutex mutex;
    typedef boost::recursive_mutex recursive_mutex;
}
#endif
#endif // NANA_STD_MUTEX_HPP
