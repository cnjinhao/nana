/**
 *	A date chooser Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/date_chooser.hpp
 */

#ifndef NANA_GUI_WIDGETS_DATE_CHOOSER_HPP
#define NANA_GUI_WIDGETS_DATE_CHOOSER_HPP

#include "widget.hpp"
#include <nana/datetime.hpp>

namespace nana
{
	namespace drawerbase
	{
		namespace date_chooser
		{
			class trigger : public drawer_trigger
			{
			public:
				static const int topbar_height = 34;
				static const int border_size = 3;

				enum class transform_action{none, to_left, to_right, to_enter, to_leave};
				enum class where{none, left_button, right_button, topbar, textarea};
				enum class page{date, month};

				struct drawing_basis
				{
					nana::point refpos;
					double line_s;
					double row_s;
				};

				trigger();
				bool chose() const;
				nana::date read() const;
				void week_name(unsigned index, const std::string&);
			private:
				where _m_pos_where(graph_reference, const ::nana::point& pos);
				void _m_draw_topbar(graph_reference);
				void _m_make_drawing_basis(drawing_basis&, graph_reference, const nana::point& refpos);
				void _m_draw_pos(drawing_basis &, graph_reference, int x, int y, const ::std::string&, bool primary, bool sel);
				void _m_draw_pos(drawing_basis &, graph_reference, int x, int y, int number, bool primary, bool sel);
				void _m_draw_ex_days(drawing_basis &, graph_reference, int begx, int begy, bool before);
				void _m_draw_days(const nana::point& refpos, graph_reference);
				void _m_draw_months(const nana::point& refpos, graph_reference);
				bool _m_get_trace(point, int & res);
				void _m_perf_transform(transform_action tfid, graph_reference,  graph_reference dirtybuf, graph_reference newbuf, const nana::point& refpos);
			private:
				void refresh(graph_reference) override;
				void attached(widget_reference, graph_reference)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
			private:
				::std::string weekstr_[7];

				widget * widget_;
				
				bool chose_;
				page	page_;
				where	pos_;
				nana::point trace_pos_;

				drawing_basis	dbasis_;

				struct
				{
					int year;
					int month;
					int day;
				}chdate_;

				struct
				{
					int year;
					int month;				
				}chmonth_;

				struct color_tag
				{
					::nana::color highlight;
					::nana::color selected;
					::nana::color normal;
					::nana::color bgcolor;
				}color_;
			};
		
		}//end namespace date_chooser
	
	}//end namespace drawerbase

	/// \see nana::date
	class date_chooser
		: public widget_object<category::widget_tag, drawerbase::date_chooser::trigger>
	{
	public:
		date_chooser();
		date_chooser(window, bool visible);
		date_chooser(window, const ::std::string& text, bool visible = true);
		date_chooser(window, const char* text, bool visible = true);
		date_chooser(window, const nana::rectangle& r = rectangle(), bool visible = true);

		bool chose() const;
		nana::date read() const;
		void weekstr(unsigned index, const ::std::string&);///<Set the week strings which will be displayed for day, index is in range of [0, 6]
	};
}//end namespace nana

#endif
