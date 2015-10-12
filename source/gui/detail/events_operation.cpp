#include <nana/gui/detail/events_operation.hpp>

namespace nana
{
	namespace detail
	{
		//class events_operation
			using lock_guard = std::lock_guard<std::recursive_mutex>;

			void events_operation::make(window wd, const std::shared_ptr<general_events>& sp)
			{
				lock_guard lock(mutex_);
				evt_table_[wd] = sp;
			}

			void events_operation::umake(window wd)
			{
				lock_guard lock(mutex_);
				evt_table_.erase(wd);
			}

			void events_operation::register_evt(event_handle evt)
			{
				lock_guard lock(mutex_);
				handles_.insert(evt);
			}

			void events_operation::cancel(event_handle evt)
			{
				lock_guard lock(mutex_);
				handles_.erase(evt);
			}

			void events_operation::erase(event_handle evt)
			{
				lock_guard lock(mutex_);

				auto i = handles_.find(evt);
				if (i != handles_.end())
				{
					reinterpret_cast<detail::docker_interface*>(evt)->get_event()->remove(evt);
				}
			}
		//end namespace events_operation
	}//end namespace detail
}//end namespace nana