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

		/// The destruction
		~group();

		group& enable_format_caption(bool format);


		place& get_place();

		void collocate();
		void div(const char* div_str);
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

		void _m_complete_creation() override;
		::nana::string _m_caption() const override;
		void _m_caption(::nana::string&&) override;
	private:
		std::unique_ptr<implement> impl_;
    };

}//end namespace nana
#endif
