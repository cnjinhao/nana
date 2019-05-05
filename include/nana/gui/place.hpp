/**
 *	An Implementation of Place for Layout
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/place.cpp
 *
 *	@contributions
 *	error, width/height, min/max and splitter bar initial weight by Ariel Vina-Rodriguez.
 */

#ifndef NANA_GUI_PLACE_HPP
#define NANA_GUI_PLACE_HPP
#include <nana/push_ignore_diagnostic>
#include <nana/gui/basis.hpp>
#include <memory>
#include <functional>

namespace nana
{
	namespace paint
	{
		class graphics;
	}

	class widget;
	namespace detail
	{
		class place_agent
		{
		public:
			virtual ~place_agent() = default;
			virtual std::unique_ptr<nana::widget> create(nana::window) const = 0;
		};
	}

	template<typename Widget>
	class agent
		: public detail::place_agent
	{
	public:
		agent(std::function<void(Widget&)> initializer)
			: init_(std::move(initializer))
		{}

		agent(const char* text)
			: text_(text)
		{
			throw_not_utf8(text);
		}

		agent(std::string text, std::function<void(Widget&)> initializer = {})
			: text_(std::move(text)), init_(std::move(initializer))
		{
			throw_not_utf8(text_);
		}

	private:
		std::unique_ptr<nana::widget> create(nana::window handle) const override
		{
			std::unique_ptr<Widget> ptr(new Widget(handle));
			ptr->caption(text_);
			if (init_)
				init_(*ptr);
			return std::move(ptr);
		}
	private:
		std::string text_;
		std::function<void(Widget&)> init_;
	};

    ///  Layout management - an object of class place is attached to a widget, and it automatically positions and resizes the children widgets.
	class place
		: ::nana::noncopyable
	{
		struct implement;


		class field_interface
		{
			field_interface(const field_interface&) = delete;
			field_interface& operator=(const field_interface&) = delete;
			field_interface(field_interface&&) = delete;
			field_interface& operator=(field_interface&&) = delete;
		public:
			field_interface() = default;
			virtual ~field_interface() = default;
			virtual field_interface& operator<<(const char* label) = 0;
			virtual field_interface& operator<<(std::string label) = 0;
			virtual field_interface& operator<<(window) = 0;
			virtual field_interface& fasten(window) = 0;
			
			template<typename Widget>
			field_interface& operator<<(const agent<Widget>& ag)
			{
				_m_add_agent(ag);
				return *this;
			}
		private:
			virtual void _m_add_agent(const detail::place_agent&) = 0;
		};
	public:
		class error :public std::invalid_argument
		{
		public:
			error(	const std::string& what,
					const place& plc,
					std::string            field = "unknown",
					std::string::size_type pos = std::string::npos);
			std::string base_what;
			std::string owner_caption;  ///< truncate caption (title) of the "placed" widget
			std::string div_text;       ///< involved div_text
			std::string field;          ///< posible field where the error ocurred.  
			std::string::size_type pos; ///< posible position in the div_text where the error ocurred. npos if unknown
		};
        ///  reference to a field manipulator which refers to a field object created by place 
		using field_reference = field_interface &;

		place();
		place(window);///< Attaches to a specified widget.
		~place();

		/** @brief Bind to a window
		 *	@param handle	A handle to a window which the place wants to attach.
		 *	@remark	It will throw an exception if the place has already binded to a window.
		 */
		void bind(window handle);
		window window_handle() const;

		void splitter_renderer(std::function<void(window, paint::graphics&, mouse_action)> fn);
        
		void div(std::string div_text);			  ///< Divides the attached widget into fields. May throw placa::error
		const std::string& div() const noexcept;  ///< Returns div-text that depends on fields status.
		static bool valid_field_name(const char* name)  ///< must begin with _a-zA-Z
		{
			return name && (*name == '_' || (('a' <= *name && *name <= 'z') || ('A' <= *name && *name <= 'Z')));
		}
		void modify(const char* field_name, const char* div_text);	///< Modifies a specified field. May throw placa::error

		field_reference field(const char* name);///< Returns a field with the specified name.

		void field_visible(const char* field_name, bool visible); ///<<Shows/Hides an existing field.
		bool field_visible(const char* field_name) const;	///<Determines whether the specified field is visible.

		void field_display(const char* field_name, bool display); ///<Displays/Discards an existing field.
		bool field_display(const char* field_name) const;	///<Determines whether the specified field is displayed.

		void collocate();                     ///< Layouts the widgets.

 		void erase(window handle);				///< Erases a window from field.

		field_reference operator[](const char* name); ///< Returns a field with the specified name. Equal to field();

		/// Add a panel factory
		template<typename Panel, typename ...Args>
		place& dock(const std::string& dockname, const std::string& factory_name, Args&& ... args)
		{
			return dock(dockname, factory_name, std::bind([](window parent, Args & ... args)
			{
				return std::unique_ptr<widget>(new Panel(parent, std::forward<Args>(args)...));
			}, std::placeholders::_1, args...));
		}

		place& dock(const std::string& dockname, std::string factory_name, std::function<std::unique_ptr<widget>(window)> factory);
		widget* dock_create(const std::string& factory);
	private:
		implement * impl_;
	};

	
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif //#ifndef NANA_GUI_PLACE_HPP
