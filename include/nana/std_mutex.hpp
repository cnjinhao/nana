#ifndef NANA_STD_MUTEX_HPP
#define NANA_STD_MUTEX_HPP
#include <nana/config.hpp>

#if defined(STD_THREAD_NOT_SUPPORTED)

#if defined(USE_github_com_meganz_mingw_std_threads)
#include <windows.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <pthread.h>
#include <errno.h>
#include <cstdio>
// http://lxr.free-electrons.com/source/include/uapi/asm-generic/errno.h#L53
#define EPROTO          71      /* Protocol error */
#include <mingw.thread.h>
#include <mingw.mutex.h>
#else
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
#endif  // (USE_github_com_meganz_mingw_std_threads)
#endif // (STD_THREAD_NOT_SUPPORTED)
#endif // NANA_STD_MUTEX_HPP
