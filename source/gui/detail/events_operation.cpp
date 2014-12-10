#include <nana/gui/detail/events_operation.hpp>

namespace nana
{
	namespace detail
	{
		//class events_operation
			typedef std::lock_guard<std::recursive_mutex> lock_guard;

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
				register_.insert(evt);
			}

			void events_operation::cancel(event_handle evt)
			{
				lock_guard lock(mutex_);
				register_.erase(evt);
			}

			void events_operation::erase(event_handle evt)
			{
				lock_guard lock(mutex_);

				auto i = register_.find(evt);
				if (i != register_.end())
				{
					reinterpret_cast<detail::docker_interface*>(evt)->get_event()->remove(evt);
				}
			}

			std::size_t events_operation::size() const
			{
				lock_guard lock(mutex_);
				return register_.size();
			}
		//end namespace events_operation
	}//end namespace detail
}//end namespace nana