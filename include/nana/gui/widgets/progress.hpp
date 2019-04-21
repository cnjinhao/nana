/**
 *	A Progress Indicator Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
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
			struct scheme
				: public widget_geometrics
			{
				scheme();

				color_proxy gradient_bgcolor{ colors::button_face_shadow_start };
				color_proxy gradient_fgcolor{ static_cast<color_rgb>(0x6FFFA8) };
			};

			class substance;

			class trigger
				: public drawer_trigger
			{
			public:
				trigger();
				~trigger();

				substance* progress() const;
			private:
				void attached(widget_reference, graph_reference) override;
				void refresh(graph_reference) override;
			private:
				substance* const progress_;
			};
		}
	}//end namespace drawerbase
       /// \brief A progressbar widget with two styles: know, and unknown amount value (goal). 
       /// In unknown style the amount is ignored and the bar is scrolled when value change.
	class progress
		: public widget_object<category::widget_tag, drawerbase::progress::trigger, ::nana::general_events, drawerbase::progress::scheme>
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
	};
}//end namespace nana
#endif
