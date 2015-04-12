#ifndef NANA_STD_THREAD_HPP
#define NANA_STD_THREAD_HPP
#include <nana/config.hpp>

#if STD_THREAD_NOT_SUPPORTED
#include <boost/thread.hpp>
namespace std
{
    typedef boost::thread thread;
}
#else

#include <thread>    

#endif

#endif // NANA_STD_THREAD_HPP
