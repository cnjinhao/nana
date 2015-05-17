/*
 *	A Form Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/form.cpp
 */

#include <nana/gui/widgets/form.hpp>

namespace nana
{
	namespace drawerbase
	{
		namespace form
		{
		//class trigger
			void trigger::attached(widget_reference wdg, graph_reference graph)
			{
				wd_ = &wdg;
				API::ignore_mouse_focus(wdg, true);
			}

			void trigger::refresh(graph_reference graph)
			{
				graph.rectangle(true, API::bgcolor(*wd_));
			}
		}//end namespace form
	}//end namespace drawerbase

	//class form
	typedef widget_object<category::root_tag, drawerbase::form::trigger, ::nana::detail::events_root_extension> form_base_t;

		form::form(const form& fm, const ::nana::size& sz, const appearance& apr)
			: form_base_t(fm.handle(), false, API::make_center(fm.handle(), sz.width, sz.height), apr)
		{
		}

		form::form(const rectangle& r, const appearance& apr)
			: form_base_t(nullptr, false, r, apr)
		{}

		form::form(window owner, const ::nana::size& sz, const appearance& apr)
			: form_base_t(owner, false, API::make_center(owner, sz.width, sz.height), apr)
		{}

		form::form(window owner, const rectangle& r, const appearance& apr)
			: form_base_t(owner, false, r, apr)
		{}

		void form::modality() const
		{
			API::modal_window(handle());
		}

		void form::wait_for_this()
		{
			API::wait_for(handle());
		}
	//end class form

	//class nested_form
		nested_form::nested_form(const form& fm, const rectangle& r, const appearance& apr)
			: form_base_t(fm.handle(), true, r, apr)
		{
		}

		nested_form::nested_form(const nested_form& fm, const rectangle& r, const appearance& apr)
			: form_base_t(fm.handle(), true, r, apr)
		{
		}

		nested_form::nested_form(window owner, const appearance& apr)
			: form_base_t(owner, true, rectangle(), apr)
		{}

		nested_form::nested_form(window owner, const rectangle& r, const appearance& apr)
			: form_base_t(owner, true, r, apr)
		{}
	//end nested_form
}//end namespace nana
