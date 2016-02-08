#ifndef NANA_STD_THREAD_HPP
#define NANA_STD_THREAD_HPP
#include <nana/config.hpp>

#if defined(STD_THREAD_NOT_SUPPORTED)

#if defined(NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
#include <mingw.thread.h>
#else
#include <boost/thread.hpp>
namespace std
{
    typedef boost::thread thread;
}
#endif  // (NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
#endif // (STD_THREAD_NOT_SUPPORTED)
#endif // NANA_STD_THREAD_HPP
