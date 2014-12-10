/*
 *	A List Box Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/listbox.hpp
 *
 */

#ifndef NANA_GUI_WIDGETS_LISTBOX_HPP
#define NANA_GUI_WIDGETS_LISTBOX_HPP
#include "widget.hpp"
#include <nana/concepts.hpp>
#include <nana/key_type.hpp>
#include <functional>
#include <initializer_list>

namespace nana
{
	class listbox;

	namespace drawerbase
	{
		namespace listbox
		{
			typedef std::size_t size_type;

			struct cell
			{
				struct format
				{
					::nana::color_t bgcolor;
					::nana::color_t fgcolor;

					format(color_t bgcolor = 0xFF000000, color_t fgcolor = 0xFF000000);
				};

				using format_ptr = std::unique_ptr < format > ;

				::nana::string	text;
				format_ptr custom_format;

				cell() = default;
				cell(const cell&);
				cell(cell&&);
				cell(nana::string);
				cell(nana::string, const format&);
				cell(nana::string, color_t bgcolor, color_t fgcolor);

				cell& operator=(const cell&);
				cell& operator=(cell&&);
			};

			class oresolver
			{
			public:
				oresolver& operator<<(bool);
				oresolver& operator<<(short);
				oresolver& operator<<(unsigned short);
				oresolver& operator<<(int);
				oresolver& operator<<(unsigned int);
				oresolver& operator<<(long);
				oresolver& operator<<(unsigned long);
				oresolver& operator<<(long long);
				oresolver& operator<<(unsigned long long);
				oresolver& operator<<(float);
				oresolver& operator<<(double);
				oresolver& operator<<(long double);

				oresolver& operator<<(const char*);
				oresolver& operator<<(const wchar_t*);
				oresolver& operator<<(const std::string&);
				oresolver& operator<<(const std::wstring&);
				oresolver& operator<<(std::wstring&&);
				oresolver& operator<<(cell);
				oresolver& operator<<(std::nullptr_t);

				std::vector<cell> && move_cells();
			private:
				std::vector<cell> cells_;
			};

			class iresolver
			{
			public:
				iresolver(const std::vector<cell>&);

				iresolver& operator>>(bool&);
				iresolver& operator>>(short&);
				iresolver& operator>>(unsigned short&);
				iresolver& operator>>(int&);
				iresolver& operator>>(unsigned int&);
				iresolver& operator>>(long&);
				iresolver& operator>>(unsigned long&);
				iresolver& operator>>(long long&);
				iresolver& operator>>(unsigned long long&);
				iresolver& operator>>(float&);
				iresolver& operator>>(double&);
				iresolver& operator>>(long double&);

				iresolver& operator>>(std::string&);
				iresolver& operator>>(std::wstring&);
				iresolver& operator>>(cell&);
				iresolver& operator>>(std::nullptr_t);
			private:
				const std::vector<cell>& cells_;
				std::size_t pos_{0};
			};

			struct index_pair
			{
				size_type cat;	//The pos of category
				size_type item;	//the pos of item in a category.

				index_pair(size_type cat_pos = 0, size_type item_pos = 0)
					:	cat(cat_pos),
						item(item_pos)
				{}

				bool empty() const
				{
					return (npos == cat);
				}

				void set_both(size_type n)
				{
					cat = item = n;
				}

				bool is_category() const
				{
					return (npos != cat && npos == item);
				}

				bool is_item() const
				{
					return (npos != cat && npos != item);
				}

				bool operator==(const index_pair& r) const
				{
					return (r.cat == cat && r.item == item);
				}

				bool operator!=(const index_pair& r) const
				{
					return !this->operator==(r);
				}

				bool operator>(const index_pair& r) const
				{
					return (cat > r.cat) || (cat == r.cat && item > r.item);
				}
			};

			typedef std::vector<index_pair> selection;

			//struct essence_t
			//@brief:	this struct gives many data for listbox,
			//			the state of the struct does not effect on member funcions, therefore all data members are public.
			struct essence_t;

			struct category_t;
			class drawer_header_impl;
			class drawer_lister_impl;

			class trigger: public drawer_trigger
			{
			public:
				trigger();
				~trigger();
				essence_t& essence();
				essence_t& essence() const;
				void draw();
			private:
				void _m_draw_border();
			private:
				void attached(widget_reference, graph_reference)	override;
				void detached()	override;
				void typeface_changed(graph_reference)	override;
				void refresh(graph_reference)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void mouse_wheel(graph_reference, const arg_wheel&)	override;
				void dbl_click(graph_reference, const arg_mouse&)	override;
				void resized(graph_reference, const arg_resized&)		override;
				void key_press(graph_reference, const arg_keyboard&)	override;
			private:
				essence_t * essence_;
				drawer_header_impl *drawer_header_;
				drawer_lister_impl *drawer_lister_;
			};//end class trigger

			class item_proxy
				: public std::iterator<std::input_iterator_tag, item_proxy>
			{
			public:
				item_proxy(essence_t*);
				item_proxy(essence_t*, const index_pair&);

				bool empty() const;

				item_proxy & check(bool ck);
				bool checked() const;

				item_proxy & select(bool);
				bool selected() const;

				item_proxy & bgcolor(nana::color_t);
				nana::color_t bgcolor() const;

				item_proxy& fgcolor(nana::color_t);
				nana::color_t fgcolor() const;

				index_pair pos() const;

				size_type columns() const;

				item_proxy&		text(size_type col, cell);
				item_proxy&		text(size_type col, nana::string);
				nana::string	text(size_type col) const;

				void icon(const nana::paint::image&);

				template<typename T>
				item_proxy & resolve_from(const T& t)
				{
					oresolver ores;
					ores << t;
					auto && cells = ores.move_cells();
					auto cols = columns();
					cells.resize(cols);
					for (auto pos = 0; pos < cols; ++pos)
					{
						auto & el = cells[pos];
						if (el.text.size() == 1 && el.text[0] == nana::char_t(0))
							continue;
						text(pos, std::move(el));
					}
					
					return *this;
				}

				template<typename T>
				void resolve_to(T& t) const
				{
					iresolver ires(_m_cells());
					ires >> t;
				}

				template<typename T>
				T* value_ptr() const
				{
					auto * pany = _m_value();
					return (pany ? pany->get<T>() : nullptr);
				}

				template<typename T>
				T & value() const
				{
					auto * pany = _m_value();
					if(nullptr == pany)
						throw std::runtime_error("listbox::item_proxy.value<T>() is empty");

					T * p = pany->get<T>();
					if(nullptr == p)
						throw std::runtime_error("listbox::item_proxy.value<T>() invalid type of value");
					return *p;
				}

				template<typename T>
				item_proxy & value(T&& t)
				{
					*_m_value(true) = std::forward<T>(t);
					return *this;
				}

				/// Behavior of Iterator's value_type
				bool operator==(const nana::string& s) const;
				bool operator==(const char * s) const;
				bool operator==(const wchar_t * s) const;

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

				//Undocumented method
				essence_t * _m_ess() const;
			private:
				std::vector<cell> & _m_cells() const;
				nana::any * _m_value(bool alloc_if_empty);
				const nana::any * _m_value() const;
			private:
				essence_t * ess_;
				category_t*	cat_{nullptr};
				index_pair	pos_;
			};

			class cat_proxy
				: public std::iterator < std::input_iterator_tag, cat_proxy >
			{
			public:
				cat_proxy() = default;
				cat_proxy(essence_t*, size_type pos);
				cat_proxy(essence_t*, category_t*);

				/// Append an item at end of the category, set_value determines whether assign T object to the value of item.
				template<typename T>
				item_proxy append(T&& t, bool set_value = false)
				{
					oresolver ores;
					if (set_value)
						ores << t;	//copy it if it is rvalue and set_value is true.
					else
						ores << std::forward<T>(t);

					_m_append(ores.move_cells());

					item_proxy iter{ ess_, index_pair(pos_, size() - 1) };
					if (set_value)
						iter.value(std::forward<T>(t));
					return iter;
				}

				void append(std::initializer_list<nana::string>);

				size_type columns() const;

				cat_proxy& text(nana::string);
				nana::string text() const;

				/// Behavior of a container
				void push_back(nana::string);

				item_proxy begin() const;
				item_proxy end() const;
				item_proxy cbegin() const;
				item_proxy cend() const;

				item_proxy at(size_type pos) const;
				item_proxy back() const;

				/// Returns the index of a item by its display pos, the index of the item isn't changed after sorting.
				size_type index_by_display_order(size_type disp_order) const;
				size_type display_order(size_type pos) const;
				size_type position() const;

				/// Returns the number of items
				size_type size() const;

				/// Behavior of Iterator
				cat_proxy& operator=(const cat_proxy&);

				/// Behavior of Iterator
				cat_proxy & operator++();

				/// Behavior of Iterator
				cat_proxy	operator++(int);

				/// Behavior of Iterator
				cat_proxy& operator*();

				/// Behavior of Iterator
				const cat_proxy& operator*() const;

				/// Behavior of Iterator
				cat_proxy* operator->();

				/// Behavior of Iterator
				const cat_proxy* operator->() const;

				/// Behavior of Iterator
				bool operator==(const cat_proxy&) const;

				/// Behavior of Iterator
				bool operator!=(const cat_proxy&) const;
			private:
				void _m_append(std::vector<cell> && cells);
				void _m_cat_by_pos();
			private:
				essence_t*	ess_{nullptr};
				category_t*	cat_{nullptr};
				size_type	pos_{0};
			};
		}
	}//end namespace drawerbase

	struct arg_listbox
	{
		mutable drawerbase::listbox::item_proxy item;
		bool	selected;
	};

	namespace drawerbase
	{
		namespace listbox
		{
			struct listbox_events
				: public general_events
			{
				basic_event<arg_listbox> checked;
				basic_event<arg_listbox> selected;
			};
		}
	}//end namespace drawerbase

/*! \brief A rectangle containing a list of strings from which the user can select. This widget contain a list of \a categories, with in turn contain \a items. 
A category is a text with can be \a selected, \a checked and \a expanded to show the items.
An item is formed by \a column-fields, each corresponding to one of the \a headers. 
An item can be \a selected and \a checked.
The user can \a drag the header to \a reisize it or to \a reorganize it. 
By \a clicking on a header the list get \a reordered, first up, and then down alternatively,
*/
	class listbox
		:	public widget_object<category::widget_tag, drawerbase::listbox::trigger, drawerbase::listbox::listbox_events>,
			public concepts::any_objective<drawerbase::listbox::size_type, 2>
	{
	public:
		using size_type		= drawerbase::listbox::size_type;
		using index_pair	= drawerbase::listbox::index_pair;
		using cat_proxy		= drawerbase::listbox::cat_proxy;
		using item_proxy	= drawerbase::listbox::item_proxy;
		using selection = drawerbase::listbox::selection;    ///<A container type for items.
		using iresolver = drawerbase::listbox::iresolver;
		using oresolver = drawerbase::listbox::oresolver;
		using cell = drawerbase::listbox::cell;
	public:
		listbox() = default;
		listbox(window, bool visible);
		listbox(window, const rectangle& = {}, bool visible = true);

		void auto_draw(bool);                                ///<Set state: Redraw automatically after an operation?

		void append_header(nana::string, unsigned width = 120);///<Appends a new column with a header text and the specified width at the end

		cat_proxy append(nana::string);          ///<Appends a new category at the end
		void append(std::initializer_list<nana::string>); ///<Appends categories at the end
		cat_proxy insert(cat_proxy, nana::string);
		cat_proxy at(size_type pos) const;
		listbox& ordered_categories(bool);

		template<typename Key>
		cat_proxy operator[](const Key & ck)
		{
			typedef typename nana::detail::type_escape<Key>::type key_t;
			std::shared_ptr<nana::detail::key_interface> p(new nana::key<key_t, std::less<key_t>>(ck), [](nana::detail::key_interface* p)
			{
				delete p;
			});

			return cat_proxy(&_m_ess(), _m_at_key(p));
		}

		template<typename Key>
		cat_proxy operator[](Key && ck)
		{
			typedef typename nana::detail::type_escape<Key>::type key_t;
			std::shared_ptr<nana::detail::key_interface> p(new nana::key<key_t, std::less<key_t>>(std::move(ck)), [](nana::detail::key_interface* p)
			{
				delete p;
			});

			return cat_proxy(&_m_ess(), _m_at_key(p));
		}

		item_proxy at(const index_pair&) const;

		void insert(const index_pair&, nana::string);         ///<Insert a new item with a text in the first column.

		void checkable(bool);
		selection checked() const;                         ///<Returns the items which are checked.                       

		void clear(size_type cat);                         ///<Removes all the items from the specified category
		void clear();                                      ///<Removes all the items from all categories
		void erase(size_type cat);                         ///<Erases a category
		void erase();                                      ///<Erases all categories.
		item_proxy erase(item_proxy);

		template<typename Key>
		void erase_key(const Key& kv)
		{
			typedef typename nana::detail::type_escape<Key>::type key_t;
			nana::key<key_t, std::less<key_t> > key(kv);
			_m_ease_key(&key);
		}

		template<typename Key>
		void erase_key(Key&& kv)
		{
			typedef typename nana::detail::type_escape<Key>::type key_t;
			nana::key<key_t, std::less<key_t> > key(std::move(kv));
			_m_ease_key(&key);
		}

		            ///Sets a strick weak ordering comparer for a column
		void set_sort_compare(size_type col, std::function<bool(const nana::string&, nana::any*,
				                                        const nana::string&, nana::any*, bool reverse)> strick_ordering);

		void sort_col(size_type col, bool reverse = false);
		size_type sort_col() const;
		void unsort();
		bool freeze_sort(bool freeze);

		selection selected() const;                         ///<Get the indexs of all the selected items

		void show_header(bool);
		bool visible_header() const;
		void move_select(bool upwards);                     ///<Selects an item besides the current selected item.

		size_type size_categ() const;                   ///<Get the number of categories
		size_type size_item() const;                    ///<The number of items in the default category
		size_type size_item(size_type cat) const;          ///<The number of items in category "cat"
	private:
		drawerbase::listbox::essence_t & _m_ess() const;
		nana::any* _m_anyobj(size_type cat, size_type index, bool allocate_if_empty) const;
		size_type _m_headers() const;
		drawerbase::listbox::category_t* _m_at_key(std::shared_ptr<nana::detail::key_interface>);
		void _m_ease_key(nana::detail::key_interface*);
	};
}//end namespace nana
#endif
