/**
 *	A Slider Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/slider.hpp
 */
#ifndef NANA_GUI_WIDGETS_SLIDER_HPP
#define NANA_GUI_WIDGETS_SLIDER_HPP

#include <nana/push_ignore_diagnostic>

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

			struct scheme_impl
				: public widget_geometrics
			{
				/// Colors
				color_proxy color_adorn		{ static_cast<color_rgb>(0x3da3ce) };
				color_proxy color_bar		{ static_cast<color_rgb>(0x878787) };
				color_proxy color_slider	{ static_cast<color_rgb>(0x606060) };
				color_proxy color_slider_highlighted{ static_cast<color_rgb>(0x2d93be) };
				color_proxy color_vernier		{ colors::red };
				color_proxy color_vernier_text	{ colors::white };

				/// Geometrical parameters
				unsigned vernier_text_margin{ 8 };

			};

			struct slider_events
				: public general_events
			{
				basic_event<arg_slider> value_changed;
			};

			enum class seekdir
			{
				bilateral, forward, backward
			};


			class renderer_interface
			{
			public:
				using graph_reference = ::nana::paint::graphics&;
				using scheme = scheme_impl;

				struct data_bar
				{
					bool vert;		///< Indicates whether the slider is vertical.
					::nana::rectangle area;	///< Position and size of bar.
					unsigned border_weight;	///< The border weight in pixels.
				};

				struct data_slider
				{
					bool		vert;	///< Indicates whether the slider is vertical.
					double		pos;
					unsigned	border_weight;
					unsigned	weight;
				};

				struct data_adorn
				{
					bool vert;	///< Indicates whether the slider is vertical.
					::nana::point bound;
					int fixedpos;
					unsigned block;
					unsigned vcur_scale;
				};

				struct data_vernier
				{
					bool		vert;	///< Indicates whether the slider is vertical.
					int			position;
					int			end_position;
					unsigned	knob_weight;

					std::string text;
				};

				virtual ~renderer_interface() = default;

				virtual void background(window, graph_reference, bool transparent, const scheme&) = 0;
				virtual void adorn(window, graph_reference, const data_adorn&, const scheme&) = 0;
				virtual void vernier(window, graph_reference, const data_vernier&, const scheme&) = 0;
				virtual void bar(window, graph_reference, const data_bar&, const scheme&) = 0;
				virtual void slider(window, graph_reference, mouse_action, const data_slider&, const scheme&) = 0;
			};

			class trigger
				: public drawer_trigger
			{
				class model;
			public:
				trigger();
				~trigger();
				model* get_model() const;
			private:
				void attached(widget_reference, graph_reference)	override;
				void refresh(graph_reference)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void resized(graph_reference, const arg_resized&)		override;
			private:
				model * model_ptr_;
			};
		}//end namespace slider
	}//end namespace drawerbase


    /// A slider widget which the user can drag for tracking
	class slider
		: public widget_object<category::widget_tag, drawerbase::slider::trigger, drawerbase::slider::slider_events, drawerbase::slider::scheme_impl>
	{
	public:
		using renderer_interface = drawerbase::slider::renderer_interface;	///< The interface for customized renderer.
		using seekdir = drawerbase::slider::seekdir;						///< Defines the slider seek direction.

		slider();
		slider(window, bool visible);
		slider(window, const rectangle& = rectangle(), bool visible = true);

		void seek(seekdir);                                  ///< Define the direction that user can seek by using mouse.
		void vertical(bool);
		bool vertical() const;
		void maximum(unsigned);
		unsigned maximum() const;

		/** Set slider value
            @param[in] v new value for slider.
            v will be clipped to the range 0 to maximum
        */
		void value(int );

		unsigned value() const;
		unsigned move_step(bool forward);                         ///< Increase or decrease the value of slider.
		unsigned adorn() const;

		const pat::cloneable<renderer_interface>& renderer();                 ///< Refers to the current renderer that slider is using.
		void renderer(const pat::cloneable<renderer_interface>&);       ///< Set the current renderer.

		void vernier(std::function<std::string(unsigned maximum, unsigned cursor_value)> provider);
		void transparent(bool);
		bool transparent() const;
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
