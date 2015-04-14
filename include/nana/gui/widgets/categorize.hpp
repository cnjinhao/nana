/**
 *	A Categorize Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *  @file: nana/gui/widgets/categorize.hpp
 */

#ifndef NANA_GUI_WIDGET_CATEGORIZE_HPP
#define NANA_GUI_WIDGET_CATEGORIZE_HPP

#include <nana/gui/widgets/widget.hpp>
#include <nana/pat/cloneable.hpp>
#include <nana/any.hpp>

namespace nana
{
	template<typename T> class categorize;

	template<typename ValueType>
	struct arg_categorize
		: public event_arg
	{
		categorize<ValueType> & widget;
		ValueType & value;

		arg_categorize(categorize<ValueType> & wdg, ValueType& v)
			: widget(wdg), value(v)
		{}
	};

	namespace drawerbase
	{
		namespace categorize
		{
			template<typename T>
			struct categorize_events
				: public general_events
			{
				basic_event<arg_categorize<T>> selected;
			};

			class event_agent_interface
			{
			public:
				virtual ~event_agent_interface(){}
				virtual void selected(::nana::any&) = 0;
			};

			template<typename T>
			class event_agent
				: public event_agent_interface
			{
			public:
				event_agent(::nana::categorize<T>& wdg)
					: widget_(wdg)
				{}

				void selected(::nana::any & var)
				{
					auto vp = var.get<T>();

					T null_val;
					arg_categorize<T> arg(widget_, vp ? *vp : null_val);
					widget_.events().selected.emit(arg);
				}
			private:
				::nana::categorize<T> & widget_;
			};

			class renderer
			{
			public:
				typedef nana::paint::graphics & graph_reference;

				struct ui_element
				{
					enum t
					{
						none,	//Out of the widget
						somewhere, item_root, item_name, item_arrow
					};

					t what;
					std::size_t index;

					ui_element();
				};

				virtual ~renderer() = 0;
				virtual void background(graph_reference, window wd, const nana::rectangle&, const ui_element&) = 0;
				virtual void root_arrow(graph_reference, const nana::rectangle&, mouse_action) = 0;
				virtual void item(graph_reference, const nana::rectangle&, std::size_t index, const nana::string& name, unsigned textheight, bool has_child, mouse_action) = 0;
				virtual void border(graph_reference) = 0;
			};

			class trigger
				: public drawer_trigger
			{
				class scheme;
			public:
				typedef renderer::ui_element ui_element;

				trigger();
				~trigger();

				void insert(const nana::string&, nana::any);
				bool childset(const nana::string&, nana::any);
				bool childset_erase(const nana::string&);
				bool clear();

				//splitstr
				//@brief: Sets the splitstr. If the parameter will be ingored if it is an empty string.
				void splitstr(const nana::string&);
				const nana::string& splitstr() const;
				
				void path(const nana::string&);
				nana::string path() const;

				template<typename T>
				void create_event_agent(::nana::categorize<T>& wdg)
				{
					event_agent_.reset(new event_agent<T>(wdg));
					_m_event_agent_ready();
				}

				nana::any & value() const;
			private:
				void _m_event_agent_ready() const;
			private:
				void attached(widget_reference, graph_reference)	override;
				void detached()	override;
				void refresh(graph_reference)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
			private:
				std::unique_ptr<event_agent_interface> event_agent_;
				scheme * scheme_;
			};
		}//end namespace categorize
	}//end namespace drawerbase



	/// \brief Represent an architecture of categories and what category is chosen. 
	/// The categorize widget can be used for representing a path of a directory or the order of a hierarchy.
	template<typename T>
	class categorize
		: public widget_object<category::widget_tag, drawerbase::categorize::trigger, drawerbase::categorize::categorize_events<T>>
	{
	public:
		typedef T value_type;		///< The type of objects stored
		typedef drawerbase::categorize::renderer renderer;		///< The interface for user-defined renderer.

		categorize()
		{
			this->get_drawer_trigger().create_event_agent(*this);
		}

		categorize(window wd, const rectangle& r = rectangle(), bool visible = true)
		{
			this->get_drawer_trigger().create_event_agent(*this);
			this->create(wd, r, visible);
		}

		categorize(window wd, bool visible)
			: categorize(wd, ::nana::rectangle(), visible)
		{
		}

		categorize(window wd, const nana::string& text, bool visible = true)
			: categorize(wd, ::nana::rectangle(), visible)
		{
			this->caption(text);
		}

		categorize(window wd, const nana::char_t* text, bool visible = true)
			: categorize(wd, ::nana::rectangle(), visible)
		{
			this->caption(text);
		}

		/// Insert a new category with a specified name and the object of value type. 
		/// The new category would be inserted as a child in current category, 
		/// and after inserting, the new category is replaced of the current category as a new current one.
		categorize& insert(const nana::string& name, const value_type& value)
		{
			this->get_drawer_trigger().insert(name, value);
			API::update_window(*this);
			return *this;
		}

		/// Inserts a child category into current category.
		categorize& childset(const nana::string& name, const value_type& value)
		{
			if(this->get_drawer_trigger().childset(name, value))
				API::update_window(*this);
			return *this;
		}

		/// Erases a child category with a specified name from current category.
		categorize& childset_erase(const nana::string& name)
		{
			if(this->get_drawer_trigger().childset_erase(name))
				API::update_window(*this);
			return *this;
		}

		void clear()
		{
			if(this->get_drawer_trigger().clear())
				API::update_window(*this);
		}

		/// Sets the splitter string
		categorize& splitstr(const nana::string& sstr)
		{
			this->get_drawer_trigger().splitstr(sstr);
			return *this;
		}

		nana::string splitstr() const
		{
			return this->get_drawer_trigger().splitstr();
		}

		/// Retrieves a reference of the current category's value type object. If current category is empty, it throws a exception of std::runtime_error.
		value_type& value() const
		{
			return this->get_drawer_trigger().value();
		}
	private:
		//Overrides widget's virtual functions
		void _m_caption(nana::string&& str) override
		{
			this->get_drawer_trigger().path(str);
			API::dev::window_caption(*this, this->get_drawer_trigger().path());
		}
	};
}//end namespace nana

#endif
