/**
 *	A Categorize Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2021 Jinhao(cnjinhao@hotmail.com)
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
#include <nana/push_ignore_diagnostic>
#include <any>

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

	namespace drawerbase::categorize
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
				virtual void selected(::std::any&) = 0;
			};

			template<typename T>
			class event_agent
				: public event_agent_interface
			{
			public:
				event_agent(::nana::categorize<T>& wdg)
					: widget_(wdg)
				{}

				void selected(::std::any & var)
				{
					auto vp = std::any_cast<T>(&var);

					T null_val;
					arg_categorize<T> arg(widget_, vp ? *vp : null_val);
					widget_.events().selected.emit(arg, widget_.handle());
				}
			private:
				::nana::categorize<T> & widget_;
			};

			class renderer
			{
			public:
				using graph_reference = paint::graphics&;

				enum class elements
				{
					none,	//Out of the widget
					somewhere, item_root, item_name, item_arrow
				};

				struct ui_element
				{
					elements what{ elements::none };
					std::size_t index{ 0 };
				};

				virtual ~renderer() = 0;
				virtual void background(graph_reference, window wd, const nana::rectangle&, const ui_element&) = 0;
				virtual void root_arrow(graph_reference, const nana::rectangle&, mouse_action) = 0;
				virtual void item(graph_reference, const nana::rectangle&, std::size_t index, const ::std::string& name, unsigned textheight, bool has_child, mouse_action) = 0;
				virtual void border(graph_reference) = 0;
			};

			class tree_wrapper;

			class trigger
				: public drawer_trigger
			{
				class scheme;
			public:
				using elements = renderer::elements;

				trigger();
				~trigger();

				void insert(const std::string&, std::any);
				bool childset(const std::string&, std::any);

				//Erases the child node which specified by name. If name is empty, it clears all nodes.
				bool erase(const std::string& name);

				//splitstr
				//@brief: Sets the splitstr. If the parameter will be ignored if it is an empty string.
				void splitstr(const ::std::string&);
				const ::std::string& splitstr() const noexcept;

				void set_caption(std::string);

				template<typename T>
				void create_event_agent(::nana::categorize<T>& wdg)
				{
					event_agent_.reset(new event_agent<T>(wdg));
					_m_event_agent_ready();
				}

				std::any & value() const;
			private:
				void _m_event_agent_ready();
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
				scheme * scheme_{nullptr};
			};
	}//end namespace drawerbase::categorize



	/// \brief Represent an architecture of categories and what category is chosen. 
	/// The categorize widget can be used for representing a path of a directory or the order of a hierarchy.
	template<typename T>
	class categorize
		: public widget_object<category::widget_tag, drawerbase::categorize::trigger, drawerbase::categorize::categorize_events<T>>
	{
	public:
		using native_string_type = widget::native_string_type;
		using value_type = T;		///< The type of objects stored
		using renderer = drawerbase::categorize::renderer;		///< The interface for user-defined renderer.

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

		categorize(window wd, std::string_view title, bool visible = true)
			: categorize(wd, ::nana::rectangle(), visible)
		{
			this->caption(title);
		}

		categorize(window wd, const std::wstring_view title, bool visible = true)
			: categorize(wd, ::nana::rectangle(), visible)
		{
			this->caption(title);
		}

#ifdef __cpp_char8_t
		categorize(window wd, const std::u8string_view title, bool visible = true)
			: categorize(wd, ::nana::rectangle(), visible)
		{
			this->caption(title);
		}			
#endif

		/// Insert a new category with a specified name and the object of value type. 
		/// The new category would be inserted as a child in current category, 
		/// and after inserting, the new category is replaced of the current category as a new current one.
		categorize& insert(const std::string& name, const value_type& value)
		{
			throw_not_utf8(name);

			internal_scope_guard lock;
			this->get_drawer_trigger().insert(name, value);
			api::update_window(*this);
			return *this;
		}

		/// Inserts a child category into current category.
		categorize& childset(const std::string& name, const value_type& value)
		{
			throw_not_utf8(name);

			internal_scope_guard lock;
			if(this->get_drawer_trigger().childset(name, value))
				api::update_window(*this);
			return *this;
		}

		/// Erases a child category with a specified name from current category.
		categorize& childset_erase(const std::string& name)
		{
			throw_not_utf8(name);

			internal_scope_guard lock;
			if(this->get_drawer_trigger().erase(name))
				api::update_window(*this);
			return *this;
		}

#ifdef __cpp_char8_t
		/// Insert a new category with a specified name and the object of value type. 
		/// The new category would be inserted as a child in current category, 
		/// and after inserting, the new category is replaced of the current category as a new current one.
		categorize& insert(std::u8string_view name, const value_type& value)
		{
			internal_scope_guard lock;
			this->get_drawer_trigger().insert(to_string(name), value);
			api::update_window(*this);
			return *this;
		}

		/// Inserts a child category into current category.
		categorize& childset(std::u8string_view name, const value_type& value)
		{
			internal_scope_guard lock;
			if(this->get_drawer_trigger().childset(to_string(name), value))
				api::update_window(*this);
			return *this;
		}

		/// Erases a child category with a specified name from current category.
		categorize& childset_erase(std::u8string_view name)
		{
			internal_scope_guard lock;
			if(this->get_drawer_trigger().erase(to_string(name)))
				api::update_window(*this);
			return *this;
		}		
#endif

		void clear()
		{
			internal_scope_guard lock;
			if (this->get_drawer_trigger().erase({}))
				api::update_window(*this);
		}

		/// Sets the splitter string
		categorize& splitstr(const std::string& sstr)
		{
			internal_scope_guard lock;
			this->get_drawer_trigger().splitstr(sstr);
			return *this;
		}

#ifdef __cpp_char8_t
		/// Sets the splitter string
		categorize& splitstr(std::u8string_view sstr)
		{
			internal_scope_guard lock;
			this->get_drawer_trigger().splitstr(to_string(sstr));
			return *this;
		}
#endif

		std::string splitstr() const
		{
			internal_scope_guard lock;
			return this->get_drawer_trigger().splitstr();
		}

		/// Retrieves a reference of the current category's value type object. If current category is empty, it throws a exception of std::runtime_error.
		value_type& value() const
		{
			internal_scope_guard lock;
			return std::any_cast<value_type&>(this->get_drawer_trigger().value());
		}
	private:
		//Overrides widget's virtual functions
		void _m_caption(native_string_type&& str) override
		{
			this->get_drawer_trigger().set_caption(to_utf8(str));
		}
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>
#endif
