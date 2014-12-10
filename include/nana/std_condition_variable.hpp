#ifndef NANA_STD_CONDITION_VARIABLE_HPP
#define NANA_STD_CONDITION_VARIABLE_HPP
#include <nana/config.hpp>

#if defined(STD_THREAD_NOT_SUPPORTED)
#include <boost/thread/condition_variable.hpp>
namespace std
{
    typedef boost::condition_variable condition_variable;
}
#endif
#endif // NANA_STD_CONDITION_VARIABLE_HPP
