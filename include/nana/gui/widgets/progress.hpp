/**
 *	A Progress Indicator Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/progress.hpp
 */

#ifndef NANA_GUI_WIDGET_PROGRESS_HPP
#define NANA_GUI_WIDGET_PROGRESS_HPP
#include "widget.hpp"

namespace nana
{
	namespace drawerbase
	{
		namespace progress
		{
			class trigger: public drawer_trigger
			{
			public:
				unsigned value() const;
				unsigned value(unsigned);
				unsigned inc();
				unsigned Max() const;
				unsigned Max(unsigned);
				void unknown(bool);
				bool unknown() const;
				bool stop(bool s = true);
				bool stopped() const;
			private:
				void attached(widget_reference, graph_reference)	override;
				void refresh(graph_reference)	override;
			private:
				void _m_draw_box(graph_reference);
				void _m_draw_progress(graph_reference);
				bool _m_check_changing(unsigned) const;
			private:
				static const unsigned border = 2;

				widget * widget_{nullptr};
				nana::paint::graphics* graph_{nullptr};
				unsigned draw_width_{static_cast<unsigned>(-1)};
				bool unknown_{false};
				bool stop_{false};
				unsigned max_{100};
				unsigned value_{0};
			}; //end class drawer
		}
	}//end namespace drawerbase
       /// \brief A progressbar widget with two styles: know, and unknow amount value (goal). 
       /// In unknow style the amount is ignored and the bar is scrolled when value change.
	class progress 
		: public widget_object<category::widget_tag, drawerbase::progress::trigger>
	{
	public:
		progress();
		progress(window, bool visible);
		progress(window, const rectangle & = rectangle(), bool visible = true);

		unsigned value() const;
		unsigned value(unsigned val);
		unsigned inc();
		unsigned amount() const;
		unsigned amount(unsigned value);
		void unknown(bool);
		bool unknown() const;
		bool stop(bool s=true);  ///< request stop or cancel and return previus stop status
		bool stopped() const;  
	};
}//end namespace nana
#endif
