/*
 *	A Form Implementation
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/form.cpp
 */

#include <nana/gui/widgets/form.hpp>
#include <nana/gui/detail/bedrock.hpp>

namespace nana
{
	namespace drawerbase
	{
		namespace form
		{
		//class trigger
			void trigger::attached(widget_reference wdg, graph_reference)
			{
				wd_ = &wdg;
				API::ignore_mouse_focus(wdg, true);
			}

			void trigger::refresh(graph_reference graph)
			{
				graph.rectangle(true, API::bgcolor(*wd_));
			}
			//end class trigger

			//class form_base
				form_base::form_base(window owner, bool nested, const rectangle& r, const appearance& app)
					: widget_object<category::root_tag, drawerbase::form::trigger, detail::events_root_extension>(owner, nested, r, app)
				{}

				place & form_base::get_place()
				{
					if (this->empty())
						throw std::runtime_error("form::get_plac: the form has destroyed.");

					if (!place_)
						place_.reset(new place{ *this });

					return *place_;
				}

				void form_base::div(const char* div_text)
				{
					get_place().div(div_text);
				}

				place::field_reference form_base::operator[](const char* field_name)
				{
					return get_place()[field_name];
				}

				void form_base::collocate() noexcept
				{
					if (place_)
						place_->collocate();
				}
			//end class form_base
		}//end namespace form
	}//end namespace drawerbase

	//class form
		using form_base = drawerbase::form::form_base;

		form::form(const form& fm, const ::nana::size& sz, const appearance& apr)
			: form_base(fm.handle(), false, API::make_center(fm.handle(), sz.width, sz.height), apr)
		{
		}

		form::form(const rectangle& r, const appearance& apr)
			: form_base(nullptr, false, r, apr)
		{}

		form::form(window owner, const ::nana::size& sz, const appearance& apr)
			: form_base(owner, false, API::make_center(owner, sz.width, sz.height), apr)
		{}

		form::form(window owner, const rectangle& r, const appearance& apr)
			: form_base(owner, false, r, apr)
		{}

		void form::modality() const
		{
			API::modal_window(handle());
		}

		void form::wait_for_this()
		{
			API::wait_for(handle());
		}

		void form::keyboard_accelerator(const accel_key& key, const std::function<void()>& fn)
		{
			nana::detail::bedrock::instance().keyboard_accelerator(this->native_handle(), key, fn);
		}
	//end class form

	//class nested_form
		nested_form::nested_form(const form& fm, const rectangle& r, const appearance& apr)
			: form_base(fm.handle(), true, r, apr)
		{
		}

		nested_form::nested_form(const nested_form& fm, const rectangle& r, const appearance& apr)
			: form_base(fm.handle(), true, r, apr)
		{
		}

		nested_form::nested_form(window owner, const appearance& apr)
			: form_base(owner, true, rectangle(), apr)
		{}

		nested_form::nested_form(window owner, const rectangle& r, const appearance& apr)
			: form_base(owner, true, r, apr)
		{}
	//end nested_form
}//end namespace nana
