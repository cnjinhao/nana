#ifndef NANA_DETAIL_EVENTS_OPERATION_HPP
#define NANA_DETAIL_EVENTS_OPERATION_HPP

#include <nana/gui/detail/general_events.hpp>
#include <unordered_set>
#include <unordered_map>

#if defined(STD_THREAD_NOT_SUPPORTED)
#include <nana/std_mutex.hpp>
#else
#include <mutex>
#endif

namespace nana
{
	namespace detail
	{
		class events_operation
		{
		public:
			void make(window, const std::shared_ptr<general_events> &);
			void umake(window);
			void register_evt(event_handle);
			void cancel(event_handle);
			void erase(event_handle);
		private:
			std::recursive_mutex mutex_;
			std::unordered_set<event_handle>	handles_;
			std::unordered_map<window, std::shared_ptr<general_events>> evt_table_;
		};
	}//end namespace detail
}//end namespace nana

#endif
