#include <nana/gui/detail/events_operation.hpp>
#include <nana/gui/detail/bedrock.hpp>

namespace nana
{
	namespace detail
	{
		//class events_operation
			using lock_guard = std::lock_guard<std::recursive_mutex>;

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
					reinterpret_cast<detail::event_docker_interface*>(evt)->get_event()->remove(evt);
			}
		//end namespace events_operation


		//class docker_base
			docker_base::docker_base(event_interface* evt, bool unignorable_flag):
				event_ptr(evt),
				flag_deleted(false),
				unignorable(unignorable_flag)
			{}

			detail::event_interface * docker_base::get_event() const
			{
				return event_ptr;
			}
		//end class docker_base

		//class event_base
			event_base::~event_base()
			{
				clear();
			}

			std::size_t event_base::length() const
			{
				internal_scope_guard lock;
				return (nullptr == dockers_ ? 0 : dockers_->size());
			}

			void event_base::clear() noexcept
			{
				internal_scope_guard lock;
				if (dockers_)
				{
					auto & evt_operation = bedrock::instance().evt_operation();

					for (auto p : *dockers_)
					{
						evt_operation.cancel(reinterpret_cast<event_handle>(p));
						delete p;
					}
					dockers_->clear();

					delete dockers_;
					dockers_ = nullptr;
				}
			}

			void event_base::remove(event_handle evt)
			{
				internal_scope_guard lock;

				for (auto i = dockers_->begin(), end = dockers_->end(); i != end; ++i)
				{
					if (reinterpret_cast<detail::event_docker_interface*>(evt) == *i)
					{
						//Checks whether this event is working now.
						if (emitting_count_)
						{
							static_cast<docker_base*>(*i)->flag_deleted = true;
							deleted_flags_ = true;
						}
						else
						{
							bedrock::instance().evt_operation().cancel(evt);
							dockers_->erase(i);
							delete reinterpret_cast<detail::event_docker_interface*>(evt);
						}
						return;
					}
				}
			}

			event_handle event_base::_m_emplace(detail::event_docker_interface* docker_ptr, bool in_front)
			{
				internal_scope_guard lock;
				if (nullptr == dockers_)
					dockers_ = new std::vector<detail::event_docker_interface*>;

				auto evt = reinterpret_cast<event_handle>(docker_ptr);

				if (in_front)
					dockers_->emplace(dockers_->begin(), docker_ptr);
				else
					dockers_->emplace_back(docker_ptr);

				detail::events_operation_register(evt);
				return evt;
			}
			
			//class emit_counter
				event_base::emit_counter::emit_counter(event_base* evt)
					: evt_{ evt }
				{
					++evt->emitting_count_;
				}

				event_base::emit_counter::~emit_counter()
				{
					if ((0 == --evt_->emitting_count_) && evt_->deleted_flags_)
					{
						evt_->deleted_flags_ = false;
						for (auto i = evt_->dockers_->begin(); i != evt_->dockers_->end();)
						{
							if (static_cast<docker_base*>(*i)->flag_deleted)
							{
								bedrock::instance().evt_operation().cancel(reinterpret_cast<event_handle>(*i));
								delete (*i);
								i = evt_->dockers_->erase(i);
							}
							else
								++i;
						}
					}
				}
			//end class emit_counter
		//end class event_base

	}//end namespace detail
}//end namespace nana