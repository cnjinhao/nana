#ifndef NANA_DETAIL_EVENTS_HOLDER_HPP
#define NANA_DETAIL_EVENTS_HOLDER_HPP
#include <memory>

namespace nana
{
	struct general_events;

	namespace detail
	{
		class events_holder
		{
		public:
			virtual ~events_holder(){}
			virtual bool set_events(const std::shared_ptr<general_events>&) = 0;
			virtual general_events* get_events() const = 0;
		};
	}//end namespace detail
}//end namespace nana
#endif
