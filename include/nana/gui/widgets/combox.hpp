/**
 *	A Combox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *  @file: nana/gui/widgets/combox.hpp
 */

#ifndef NANA_GUI_WIDGETS_COMBOX_HPP
#define NANA_GUI_WIDGETS_COMBOX_HPP
#include "widget.hpp"
#include "float_listbox.hpp"
#include "skeletons/text_editor_part.hpp"
#include <nana/key_type.hpp>
#include <nana/concepts.hpp>
#include <functional>

namespace nana
{
	/// A list box combined with a textbox - the list box should drop down when the user selects the arrow next to the control
	class combox;

	struct arg_combox
		: public event_arg
	{
		combox & widget;
		arg_combox(combox&);
	};

	namespace drawerbase
	{
		namespace combox
		{
			struct combox_events
				: public general_events
			{
				basic_event<arg_combox>	selected;
				basic_event<arg_combox> text_changed;
			};

			class drawer_impl;

			class trigger
				: public drawer_trigger
			{
			public:
				trigger();
				~trigger();

				drawer_impl& get_drawer_impl();
				const drawer_impl& get_drawer_impl() const;
			private:
				void attached(widget_reference, graph_reference)	override;
				void detached()	override;
				void refresh(graph_reference)	override;
				void focus(graph_reference, const arg_focus&)	override;
				void mouse_enter(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_wheel(graph_reference, const arg_wheel&)	override;
				void key_press(graph_reference, const arg_keyboard&)	override;
				void key_char(graph_reference, const arg_keyboard&)	override;
			private:
				drawer_impl * drawer_;
			};

			class item_proxy
				: public std::iterator<std::input_iterator_tag, item_proxy>
			{
			public:
				item_proxy(drawer_impl*, std::size_t pos);
				item_proxy&		text(const ::std::string&);
				::std::string	text() const;
				item_proxy&	select();
				bool		selected() const;
				item_proxy& icon(const nana::paint::image&);
				nana::paint::image icon() const;

				template<typename T>
				T * value_ptr() const
				{
					return any_cast<T>(_m_anyobj(false));
				}

				template<typename T>
				T & value() const
				{
					auto * pany = _m_anyobj(false);
					if (nullptr == pany)
						throw std::runtime_error("combox::item_proxy.value<T>() is empty");

					T * p = any_cast<T>(pany);
					if (nullptr == p)
						throw std::runtime_error("combox::item_proxy.value<T>() invalid type of value");
					return *p;
				}

				template<typename T>
				item_proxy& value(const T& t)
				{
					*_m_anyobj(true) = t;
					return *this;
				}

				template<typename T>
				item_proxy& value(T&& t)
				{
					*_m_anyobj(true) = ::std::move(t);
					return *this;
				}
			public:
				/// Behavior of Iterator's value_type
				bool operator==(const ::std::string&) const;
				bool operator==(const char*) const;

				/// Behavior of Iterator
				item_proxy & operator=(const item_proxy&);

				/// Behavior of Iterator
				item_proxy & operator++();

				/// Behavior of Iterator
				item_proxy	operator++(int);

				/// Behavior of Iterator
				item_proxy& operator*();

				/// Behavior of Iterator
				const item_proxy& operator*() const;

				/// Behavior of Iterator
				item_proxy* operator->();

				/// Behavior of Iterator
				const item_proxy* operator->() const;

				/// Behavior of Iterator
				bool operator==(const item_proxy&) const;

				/// Behavior of Iterator
				bool operator!=(const item_proxy&) const;
			private:
				nana::any * _m_anyobj(bool alloc_if_empty) const;
			private:
				drawer_impl * impl_;
				std::size_t pos_;
			};
		}//end namespace combox
	}//end namespace drawerbase

	class combox
		:	public widget_object<category::widget_tag, drawerbase::combox::trigger, drawerbase::combox::combox_events, ::nana::widgets::skeletons::text_editor_scheme>,
			public nana::concepts::any_objective<std::size_t, 1>
	{
	public:
		typedef float_listbox::item_renderer item_renderer;
		typedef drawerbase::combox::item_proxy item_proxy;

		combox();
		combox(window, bool visible);
		combox(window, ::std::string, bool visible = true);
		combox(window, const char*, bool visible = true);
		combox(window, const rectangle& r = rectangle(), bool visible = true);

		void clear();
		void editable(bool);
		bool editable() const;
		void set_accept(std::function<bool(wchar_t)>);
		combox& push_back(std::string);
		std::size_t the_number_of_options() const;
		std::size_t option() const;   ///< Index of the last selected, from drop-down list, item.
		void option(std::size_t);   ///< Select the text specified by index
		::std::string text(std::size_t) const;
		void erase(std::size_t pos);

		template<typename Key>
		item_proxy operator[](const Key& kv)
		{
			typedef typename nana::detail::type_escape<Key>::type key_t;
			std::shared_ptr<nana::detail::key_interface> p(new nana::key<key_t, std::less<key_t> >(kv), [](nana::detail::key_interface*p)
			{
				delete p;
			});

			return _m_at_key(std::move(p));
		}

		template<typename Key>
		void erase_key(const Key& kv)
		{
			typedef typename nana::detail::type_escape<Key>::type key_t;
			std::unique_ptr<nana::detail::key_interface> p(new nana::key<key_t, std::less<key_t> >(kv));
			_m_erase(p.get());
		}

		template<typename Key>
		void erase_key(Key&& kv)
		{
			typedef typename nana::detail::type_escape<Key>::type key_t;
			std::unique_ptr<nana::detail::key_interface> p(new nana::key<key_t, std::less<key_t> >(std::move(kv)));
			_m_erase(p.get());
		}

		/// \brief Set user-defined item renderer object.
		/// It is an address therefore the user should not destroy the renderer object
		/// after it is set to the combox. Passing null_ptr cancels the user-defined renderer object.
		void renderer(item_renderer*);

		void image(std::size_t, const nana::paint::image&);
		nana::paint::image image(std::size_t) const;
		void image_pixels(unsigned);  ///<Sets the width of image area. Default is 16 pixels.
	private:
		item_proxy _m_at_key(std::shared_ptr<nana::detail::key_interface>&&);
		void _m_erase(nana::detail::key_interface*);
		drawerbase::combox::drawer_impl & _m_impl();
		const drawerbase::combox::drawer_impl& _m_impl() const;
	private:
		//Overrides widget's virtual functions
		native_string_type _m_caption() const throw() override;
		void _m_caption(native_string_type&&) override;
		nana::any * _m_anyobj(std::size_t pos, bool alloc_if_empty) const override;
	};
}
#endif
