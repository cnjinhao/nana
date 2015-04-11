#ifndef NANA_STD_CONDITION_VARIABLE_HPP
#define NANA_STD_CONDITION_VARIABLE_HPP
#include <nana/config.hpp>

#if defined(STD_THREAD_NOT_SUPPORTED) && !defined(NANA_MINGW) || __GNUC_MINOR__ < 8

#include <boost/thread/condition_variable.hpp>
namespace std
{
    typedef boost::condition_variable condition_variable;
}

#else

#include <condition_variable>    

#endif
#endif // NANA_STD_CONDITION_VARIABLE_HPP
