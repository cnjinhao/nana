#ifndef NANA_STD_THREAD_HPP
#define NANA_STD_THREAD_HPP
#include <nana/config.hpp>

#if defined(STD_THREAD_NOT_SUPPORTED)

#if defined(USE_github_com_meganz_mingw_std_threads)
#include <mingw.thread.h>
#else
#include <boost/thread.hpp>
namespace std
{
    typedef boost::thread thread;
}
#endif  // (USE_github_com_meganz_mingw_std_threads)
#endif // (STD_THREAD_NOT_SUPPORTED)
#endif // NANA_STD_THREAD_HPP
