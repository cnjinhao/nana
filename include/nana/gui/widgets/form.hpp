/**
 *	A Form Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/form.hpp
 */

#ifndef NANA_GUI_WIDGET_FORM_HPP
#define NANA_GUI_WIDGET_FORM_HPP

#include "widget.hpp"
#include <nana/gui/place.hpp>

namespace nana
{
	class place;

	namespace drawerbase
	{
		namespace form
		{
			class trigger: public drawer_trigger
			{
			public:
				void attached(widget_reference, graph_reference)	override;
				void refresh(graph_reference)	override;
			private:
				widget*	wd_{nullptr};
			};

			class form_base
				: public widget_object<category::root_tag, drawerbase::form::trigger, detail::events_root_extension>
			{
			public:
				form_base(window owner, bool nested, const rectangle&, const appearance&);
				
				//place methods

				place & get_place();
				void div(const char* div_text);
				place::field_reference operator[](const char* field_name);
				void collocate() noexcept;
			private:
				std::unique_ptr<place> place_;
			};
		}//end namespace form
	}//end namespace drawerbase

	/// \brief Pop-up window. Is different from other window widgets: its default constructor creates the window.
	/// \see nana::appearance
	class form
		: public drawerbase::form::form_base
	{
	public:
		using appear = ::nana::appear;

		/// Creates a window at the point and size specified by rect, and with the specified appearance. Creates a form owned by the desktop.
		form(const rectangle& = API::make_center(300, 200), const appearance& = {});	//Default constructor
		form(const form&, const ::nana::size& = { 300, 200 }, const appearance& = {});	//Copy constructor
		form(window, const ::nana::size& = { 300, 200 }, const appearance& = {});
        /// Creates a window at the point and size specified by rect, with the specified appearance. This window is always floating above its owner.
		form(window, const rectangle&, const appearance& = {});

		void modality() const;
		void wait_for_this();

		void keyboard_accelerator(const accel_key&, const std::function<void()>& fn);
	};

	class nested_form
		: public drawerbase::form::form_base
	{
	public:
		using appear = ::nana::appear;

		nested_form(const form&, const rectangle& = {}, const appearance& = {});
		nested_form(const nested_form&, const rectangle& = {}, const appearance& = {});

		nested_form(window, const appearance&);
		nested_form(window, const rectangle& = {}, const appearance& = {});
	};
}//end namespace nana
#endif
