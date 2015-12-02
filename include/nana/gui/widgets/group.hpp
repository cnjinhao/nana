/**
 *	A group widget implementation
 *	Nana C++ Library(http://www.nanaro.org)
 *	Copyright(C) 2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/group.hpp
 *
 *	@Author: Stefan Pfeifer(st-321), Ariel Vina-Rodriguez (qPCR4vir)
 *
 *	@brief group is a widget used to visually group and layout other widgets.
 */

#ifndef NANA_GUI_WIDGETS_GROUP_HPP
#define NANA_GUI_WIDGETS_GROUP_HPP

#include <nana/gui/place.hpp>
#include <nana/gui/widgets/panel.hpp>

namespace nana{
	class group
		: public panel<true>
	{
		struct implement;
	public:
		using field_reference = place::field_reference;

		/// The default construction
		group();

		/// The construction that creates the widget
		group(window parent, const rectangle& = {}, bool visible = true);

		///  The construction that creates the widget and set the titel or caption

		group(window			parent,		///< a handle to the parent
			  ::std::string	titel,		///< caption of the group
			  bool				formatted = false,  ///< Enable/disable the formatted text for the title
			  unsigned			gap = 2,			///< betwen the content  and the external limit
			  const rectangle&	r = {} ,
			  bool				visible = true
		     );


		/// The destruction
		~group();

		/// Adds an option for user selection
		group& add_option(::std::string);

		/// Enables/disables the radio mode which is single selection
		group& radio_mode(bool);

		/// Returns the index of option in radio_mode, it throws a logic_error if radio_mode is false.
		std::size_t option() const;

		/// Determines whether a specified option is checked, it throws an out_of_range if !(pos < number of options)
		bool option_checked(std::size_t pos) const;

		group& enable_format_caption(bool format);

		group& collocate() throw();
		group& div(const char* div_str) throw();
		field_reference operator[](const char* field);
		
		template<typename Widget, typename ...Args>
		Widget* create_child(const char* field, Args && ... args)
		{
			auto wdg = new Widget(handle(), std::forward<Args>(args)...);
			_m_add_child(field, wdg);
			return wdg;
		}
	private:
		void _m_add_child(const char* field, widget*);
		void _m_init();
		void _m_complete_creation() override;
		native_string_type _m_caption() const throw() override;
		void _m_caption(native_string_type&&) override;
	private:
		std::unique_ptr<implement> impl_;
    };

}//end namespace nana
#endif
