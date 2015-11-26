/**
 *	A Slider Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/slider.hpp
 */
#ifndef NANA_GUI_WIDGETS_SLIDER_HPP
#define NANA_GUI_WIDGETS_SLIDER_HPP
#include "widget.hpp"
#include <nana/pat/cloneable.hpp>

namespace nana
{
	class slider;

	struct arg_slider
		: public event_arg
	{
		slider & widget;

		arg_slider(slider&);
	};

	namespace drawerbase
	{
		namespace slider
		{
			struct slider_events
				: public general_events
			{
				basic_event<arg_slider> value_changed;
			};

			enum class seekdir
			{
				bilateral, forward, backward
			};

			class provider
			{
			public:
				virtual ~provider() = default;
				virtual std::string adorn_trace(unsigned vmax, unsigned vadorn) const = 0;
			};

			class renderer
			{
			public:
				typedef ::nana::paint::graphics & graph_reference;

				struct bar_t
				{
					bool horizontal;
					nana::rectangle r;		//the rectangle of bar.
					unsigned border_size;	//border_size of bar.
				};

				struct slider_t
				{
					bool horizontal;
					int pos;
					unsigned border;
					unsigned scale;
				};

				struct adorn_t
				{
					bool horizontal;
					nana::point bound;
					int fixedpos;
					unsigned block;
					unsigned vcur_scale;	//pixels of vcur scale.
				};

				virtual ~renderer() = default;

				virtual void background(window, graph_reference, bool isglass) = 0;
				virtual void adorn(window, graph_reference, const adorn_t&) = 0;
				virtual void adorn_textbox(window, graph_reference, const ::std::string&, const nana::rectangle&) = 0;
				virtual void bar(window, graph_reference, const bar_t&) = 0;
				virtual void slider(window, graph_reference, const slider_t&) = 0;
			};

			class controller;

			class trigger
				: public drawer_trigger
			{
			public:
				typedef controller controller_t;

				trigger();
				~trigger();
				controller_t* ctrl() const;
			private:
				void attached(widget_reference, graph_reference)	override;
				void detached()	override;
				void refresh(graph_reference)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void resized(graph_reference, const arg_resized&)		override;
			private:
				controller_t * impl_;
			};
		}//end namespace slider
	}//end namespace drawerbase
    /// A slider widget wich the user can drag for tracking
	class slider
		: public widget_object<category::widget_tag, drawerbase::slider::trigger, drawerbase::slider::slider_events>
	{
	public:
		typedef drawerbase::slider::renderer renderer;       ///< The interface for user-defined renderer.
		typedef drawerbase::slider::provider provider;       ///< The interface for user-defined provider.
		typedef drawerbase::slider::seekdir seekdir;         ///< Defines the slider seek direction.

		slider();
		slider(window, bool visible);
		slider(window, const rectangle& = rectangle(), bool visible = true);

		void seek(seekdir);                                  ///< Define the direction that user can seek by using mouse.
		void vertical(bool);
		bool vertical() const;
		void vmax(unsigned);
		unsigned vmax() const;
		void value(unsigned);
		unsigned value() const;
		unsigned move_step(bool forward);                         ///< Increase or decrease the value of slider.
		unsigned adorn() const;

		pat::cloneable<renderer>& ext_renderer();                 ///< Refers to the current renderer that slider is using.
		void ext_renderer(const pat::cloneable<renderer>&);       ///< Set the current renderer.
		void ext_provider(const pat::cloneable<provider>&);
		void transparent(bool);
		bool transparent() const;
	};
}//end namespace nana

#endif
