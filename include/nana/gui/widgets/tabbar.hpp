/*
 *	A Tabbar implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/tabbar.hpp
 *	@brief: A tabbar contains tab items and toolbox for scrolling, closing, selecting items.
 *
 */
#ifndef NANA_GUI_WIDGET_TABBAR_HPP
#define NANA_GUI_WIDGET_TABBAR_HPP
#include "widget.hpp"
#include "../../paint/gadget.hpp"
#include <nana/pat/cloneable.hpp>
#include <nana/any.hpp>

namespace nana
{
	template<typename T> class tabbar;

	template<typename T>
	struct arg_tabbar
	{
		tabbar<T> & widget;
		T & value;
	};

	template<typename T>
	struct arg_tabbar_removed : public arg_tabbar<T>
	{
		arg_tabbar_removed(tabbar<T>& wdg, T& v)
			: arg_tabbar<T>({wdg, v})
		{}

		bool remove = true;
	};

	namespace drawerbase
	{
		namespace tabbar
		{
			template<typename T>
			struct tabbar_events
				: public general_events
			{
				typedef T value_type;

				basic_event<arg_tabbar<value_type>> added;
				basic_event<arg_tabbar<value_type>> activated;
				basic_event<arg_tabbar<value_type>> removed;
			};

			class event_agent_interface
			{
			public:
				virtual ~event_agent_interface() = 0;
				virtual void added(std::size_t) = 0;
				virtual void activated(std::size_t) = 0;
				virtual bool removed(std::size_t) = 0;
			};

			class item_renderer
			{
			public:
				typedef item_renderer item_renderer_type;
				typedef nana::paint::graphics & graph_reference;
				enum state_t{disable, normal, highlight, press};

				struct item_t
				{
					nana::rectangle r;
					nana::color_t	bgcolor;
					nana::color_t	fgcolor;
				};

				virtual ~item_renderer() = 0;
				virtual void background(graph_reference, const nana::rectangle& r, nana::color_t bgcolor) = 0;
				virtual void item(graph_reference, const item_t&, bool active, state_t) = 0;
				virtual void close_fly(graph_reference, const nana::rectangle&, bool active, state_t) = 0;

				virtual void add(graph_reference, const nana::rectangle&, state_t) = 0;
				virtual void close(graph_reference, const nana::rectangle&, state_t) = 0;
				virtual void back(graph_reference, const nana::rectangle&, state_t) = 0;
				virtual void next(graph_reference, const nana::rectangle&, state_t) = 0;
				virtual void list(graph_reference, const nana::rectangle&, state_t) = 0;
			};

			template<typename T, typename DrawerTrigger>
			class event_agent
				: public event_agent_interface
			{
			public:
				typedef ::nana::arg_tabbar<T>	arg_tabbar;

				event_agent(::nana::tabbar<T>& tb, DrawerTrigger & dtr)
					: tabbar_(tb), drawer_trigger_(dtr)
				{}

				void added(std::size_t pos) override
				{
					if(pos != npos)
					{
						drawer_trigger_.at_no_bound_check(pos) = T();
						tabbar_.events().added.emit(arg_tabbar({ tabbar_, tabbar_[pos] }));
					}
				}

				void activated(std::size_t pos) override
				{
					if(pos != npos)
						tabbar_.events().activated.emit(arg_tabbar({ tabbar_, tabbar_[pos]}));
				}

				bool removed(std::size_t pos) override
				{
					if (pos != npos)
					{
						::nana::arg_tabbar_removed<T> arg(tabbar_, tabbar_[pos]);
						tabbar_.events().removed.emit(arg);
						return arg.remove;
					}
					return true;
				}
			private:
				::nana::tabbar<T> & tabbar_;
				DrawerTrigger& drawer_trigger_;
			};

			class layouter;

			class trigger
				: public drawer_trigger
			{
			public:
				enum toolbox_button_t{ButtonAdd, ButtonScroll, ButtonList, ButtonClose};
				trigger();
				~trigger();
				void activate(std::size_t);
				std::size_t activated() const;
				nana::any& at(std::size_t) const;
				nana::any& at_no_bound_check(std::size_t) const;
				const pat::cloneable<item_renderer> & ext_renderer() const;
				void ext_renderer(const pat::cloneable<item_renderer>&);
				void set_event_agent(event_agent_interface*);
				void push_back(const nana::string&, const nana::any&);
				std::size_t length() const;
				bool close_fly(bool);
				void relate(size_t, window);
				void tab_color(std::size_t, bool is_bgcolor, nana::color_t);
				void tab_image(size_t, const nana::paint::image&);
				void text(std::size_t, const nana::string&);
				nana::string text(std::size_t) const;
				bool toolbox_button(toolbox_button_t, bool);
			private:
				void attached(widget_reference, graph_reference)	override;
				void detached()	override;
				void refresh(graph_reference)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
			private:
				layouter * layouter_;
			};
		}
	}//end namespace drawerbase
    /// Analogous to dividers in a notebook or the labels in a file cabinet
	template<typename Type>
	class tabbar
		: public widget_object<category::widget_tag, drawerbase::tabbar::trigger, drawerbase::tabbar::tabbar_events<Type>>
	{
		typedef drawerbase::tabbar::trigger drawer_trigger_t;
	public:
		typedef Type value_type;            ///< The type of element data which is stored in the tabbar.
		typedef drawerbase::tabbar::item_renderer item_renderer; ///< A user-defined item renderer should be derived from this interface.

		struct button_add{};    ///< The type identifies the add button of the tabbar's toolbox.
		struct button_scroll{}; ///< The type identifies the scroll button of the tabbar's toolbox.
		struct button_list{};   ///< The type identifies the list button of the tabbar's toolbox.
		struct button_close{};  ///< The type identifies the close button of the tabbar's toolbox.

        /// A template class identifies the buttons of the tabbar’s toolbox. Refer to notes for more details.
		template<typename ButtonAdd = nana::null_type, typename ButtonScroll = nana::null_type, typename ButtonList = nana::null_type, typename ButtonClose = nana::null_type>
		struct button_container
		{
			typedef meta::fixed_type_set<ButtonAdd, ButtonScroll, ButtonList, ButtonClose> type_set;
		};

		tabbar()
		{
			evt_agent_.reset(new drawerbase::tabbar::event_agent<value_type, drawer_trigger_t>(*this, this->get_drawer_trigger()));
			this->get_drawer_trigger().set_event_agent(evt_agent_.get());
		}

		tabbar(window wd, bool visible)
			: tabbar()
		{
			this->create(wd, rectangle(), visible);
		}

		tabbar(window wd, const nana::char_t* text, bool visible)
			: tabbar(wd, ::nana::string(text), visible)
		{
		}

		tabbar(window wd, const nana::string& text, bool visible)
			: tabbar()
		{
			this->create(wd, rectangle(), visible);
			this->caption(text);
		}

		tabbar(window wd, const rectangle& r = rectangle(), bool visible = true)
			: tabbar()
		{
			this->create(wd, r, visible);
		}

		~tabbar()
		{
			this->get_drawer_trigger().set_event_agent(nullptr);
		}

		value_type & operator[](std::size_t pos) const
		{
			return static_cast<value_type&>(this->get_drawer_trigger().at_no_bound_check(pos));
		}

		void activate(std::size_t pos)                  /// Activates a tab specified by i.
		{
			this->get_drawer_trigger().activate(pos);
		}

		std::size_t activated() const
		{
			return this->get_drawer_trigger().activated();
		}

		value_type & at(std::size_t i) const        /// Returns i'th element
		{
			return static_cast<value_type&>(this->get_drawer_trigger().at(i));
		}

		void close_fly(bool fly)                    /// Draw or not a close button in each tab.
		{
			if(this->get_drawer_trigger().close_fly(fly))
				API::refresh_window(this->handle());
		}

		pat::cloneable<item_renderer>& ext_renderer() const
		{
			return this->get_drawer_trigger().ext_renderer();
		}

		void ext_renderer(const pat::cloneable<item_renderer>& ir)
		{
			this->get_drawer_trigger().ext_renderer(ir);
		}

		std::size_t length() const                 /// Returns the number of items.
		{
			return this->get_drawer_trigger().length();
		}

		void push_back(const nana::string& text)  /// Append a new item.
		{
			auto & t = this->get_drawer_trigger();
			t.push_back(text, value_type());
			API::update_window(*this);
		}

		void relate(std::size_t pos, window wd)  /// Binds a window to an item specified by pos, if the item is selected, shows the window, otherwise, hides it.
		{
			this->get_drawer_trigger().relate(pos, wd);
		}

		void tab_bgcolor(std::size_t i, nana::color_t color)
		{
			this->get_drawer_trigger().tab_color(i, true, color);
		}

		void tab_fgcolor(std::size_t i, nana::color_t color)
		{
			this->get_drawer_trigger().tab_color(i, false, color);
		}

		void tab_image(std::size_t i, const nana::paint::image& img)
		{
			this->get_drawer_trigger().tab_image(i, img);
		}
        /// Sets buttons of the tabbar's toolbox, refer to notes for more details.
		template<typename Add, typename Scroll, typename List, typename Close>
		void toolbox(const button_container<Add, Scroll, List, Close>&, bool enable)
		{
			typedef typename button_container<Add, Scroll, List, Close>::type_set type_set;
			auto & tg = this->get_drawer_trigger();
			bool redraw = false;

			if(type_set::template count<button_add>::value)
				redraw |= tg.toolbox_button(tg.ButtonAdd, enable);

			if(type_set::template count<button_scroll>::value)
				redraw |= tg.toolbox_button(tg.ButtonScroll, enable);

			if(type_set::template count<button_list>::value)
				redraw |= tg.toolbox_button(tg.ButtonList, enable);

			if(type_set::template count<button_close>::value)
				redraw |= tg.toolbox_button(tg.ButtonClose, enable);

			if(redraw)
				API::refresh_window(this->handle());
		}

		void text(std::size_t pos, const nana::string& str) /// Sets the title of the specified item, If pos is invalid, the method throws an std::out_of_range object.
		{
			this->get_drawer_trigger().text(pos, str);
		}

		nana::string text(std::size_t pos) const /// Returns a title of a specified item, If pos is invalid, the method trhows a std::out_of_range object.
		{
			return this->get_drawer_trigger().text(pos);
		}
	private:
		std::unique_ptr<drawerbase::tabbar::event_agent<value_type, drawerbase::tabbar::trigger> > evt_agent_;
	};
}//end namespace nana
#endif
