#ifndef NANA_STD_CONDITION_VARIABLE_HPP
#define NANA_STD_CONDITION_VARIABLE_HPP
#include <nana/config.hpp>

#if defined(STD_THREAD_NOT_SUPPORTED)
#if defined(NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
    #include <mingw.condition_variable.h>
#else
#include <boost/thread/condition_variable.hpp>
namespace std
{
    typedef boost::condition_variable condition_variable;
}
#endif  // (NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)
#endif // (STD_THREAD_NOT_SUPPORTED)
#endif // NANA_STD_CONDITION_VARIABLE_HPP
