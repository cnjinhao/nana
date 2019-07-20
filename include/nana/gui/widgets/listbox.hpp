/**
 *	A List Box Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/listbox.hpp
 *	@contributors:
 *		Hiroshi Seki
 *		Ariel Vina-Rodriguez
 *		leobackes(pr#86,pr#97)
 *		Benjamin Navarro(pr#81)
 *		besh81(pr#130)
 *		dankan1890(pr#158)
 */

#ifndef NANA_GUI_WIDGETS_LISTBOX_HPP
#define NANA_GUI_WIDGETS_LISTBOX_HPP
#include <nana/push_ignore_diagnostic>

#include "widget.hpp"
#include "detail/inline_widget.hpp"
#include "detail/widget_iterator.hpp"
#include <nana/pat/abstract_factory.hpp>
#include <nana/concepts.hpp>
#include <nana/key_type.hpp>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <typeinfo>

namespace nana
{
	class listbox;

	namespace drawerbase
	{
		namespace listbox
		{
			using size_type = std::size_t;
			using native_string_type = ::nana::detail::native_string_type;

			/// An interface of column operations
			class column_interface
			{
			public:
				/// Destructor
				virtual ~column_interface() = default;

				/// Returns the width of column, in pixel
				virtual unsigned width() const noexcept = 0;

				/// Sets width
				/**
				 * @param pixels The pixels of width
				 */
				virtual void width(unsigned pixels) noexcept = 0;

				/// Automatically adjusted width
				/**
				 * @param minimum The minimal width of column, in pixel
				 * @param maximum The maximal width of column, in pixel
				 */
				virtual void width(unsigned minimum, unsigned maximum) = 0;

				/// Returns the position of the column
				/**
				 * @param disp_order Indicates whether the display position or absolute position to be returned
				 * @return the position of the column.
				 */
				virtual size_type position(bool disp_order) const noexcept = 0;

				/// Returns the caption of column
				virtual std::string text() const noexcept = 0;

				/// Sets the caption of column
				/**
				 * @param text_utf8 A UTF-8 string for the caption.
				 */
				virtual void text(std::string text_utf8) = 0;

				/// Sets alignment of column text
				/**
				 * @param align Alignment
				 */
				virtual void text_align(::nana::align align) noexcept = 0;

				/// Adjusts the width to fit the content
				/**
				 * The priority of max: maximum, ranged width, scheme's max_fit_content.
				 * @param maximum Sets the width of column to the maximum if the width of content is larger than maximum
				 */
				virtual void fit_content(unsigned maximum = 0) noexcept = 0;

				/// Sets an exclusive font for the column
				virtual void typeface(const paint::font& column_font) = 0;

				/// Returns a font
				virtual paint::font typeface() const noexcept = 0;

				/// Determines the visibility state of the column
				/**
				 * @return true if the column is visible, false otherwise
				 */
				virtual bool visible() const noexcept = 0;

				/// Sets the visibility state of the column
				/**
				 * @param is_visible Indicates whether to show or hide the column
				 */
				virtual void visible(bool is_visible) noexcept = 0;
			};

			class const_virtual_pointer
			{
				struct intern
				{
				public:
					virtual ~intern() noexcept = default;
				};

				template<typename T>
				struct real_pointer
					: public intern
				{
					const T * ptr;

					real_pointer(const T* p) noexcept
						: ptr(p)
					{}
				};

				const_virtual_pointer(const const_virtual_pointer&) = delete;
				const_virtual_pointer& operator=(const const_virtual_pointer&) = delete;

				const_virtual_pointer(const_virtual_pointer&&) = delete;
				const_virtual_pointer& operator=(const_virtual_pointer&&) = delete;
			public:
				template<typename Type>
				explicit const_virtual_pointer(const Type* p)
					: intern_(new real_pointer<Type>{p})
				{
				}

				~const_virtual_pointer() noexcept
				{
					delete intern_;
				}

				template<typename Type>
				const typename std::remove_const<Type>::type *get() const noexcept
				{
					using value_type = typename std::remove_const<Type>::type;
					auto target = dynamic_cast<real_pointer<value_type>*>(intern_);
					return (target ? target->ptr : nullptr);
				}
			private:
				intern * intern_;
			};

			struct cell
			{
				struct format
				{
					::nana::color bgcolor;
					::nana::color fgcolor;

					format() noexcept = default;
					format(const ::nana::color& bgcolor, const ::nana::color& fgcolor) noexcept;
				};

				using format_ptr = ::std::unique_ptr<format>;

				::std::string	text;
				format_ptr	custom_format;

				cell() = default;
				cell(const cell&);
				cell(cell&&) noexcept;
				cell(::std::string) noexcept;
				cell(::std::string, const format&);

				cell& operator=(const cell&);
				cell& operator=(cell&&) noexcept;
			};

			class container_interface
			{
				friend class model_guard;
			public:
				virtual ~container_interface() = default;

				virtual void clear() = 0;
				virtual void erase(std::size_t pos) = 0;

				virtual std::size_t size() const = 0;
				virtual bool immutable() const = 0;

				virtual void emplace(std::size_t pos) = 0;
				virtual void emplace_back() = 0;

				virtual void assign(std::size_t pos, const std::vector<cell>& cells) = 0;
				virtual std::vector<cell> to_cells(std::size_t pos) const = 0;

				virtual bool push_back(const const_virtual_pointer&) = 0;

				virtual void * pointer() = 0;
				virtual const void* pointer() const = 0;
			};

			template<typename Value>
			struct container_translator
			{
				using value_translator = std::function<Value(const std::vector<cell>& cells)>;
				using cell_translator = std::function<std::vector<cell>(const Value&)>;

				value_translator	to_value;
				cell_translator		to_cell;
			};

			template<typename STLContainer>
			class basic_container
				: public container_interface
			{
			};

			template<typename STLContainer>
			class standalone_container
				: public basic_container<STLContainer>
			{
				using value_type = typename STLContainer::value_type;
				using value_translator = typename container_translator<value_type>::value_translator;
				using cell_translator = typename container_translator<value_type>::cell_translator;
			public:
				standalone_container(STLContainer&& cont, value_translator vtrans, cell_translator ctrans)
					:	container_(std::move(cont)),
						translator_({ vtrans, ctrans })
				{}

				standalone_container(const STLContainer& cont, value_translator vtrans, cell_translator ctrans)
					:	container_(cont),
						translator_({ vtrans, ctrans })
				{}
			private:
				void clear() override
				{
					container_.clear();
				}

				void erase(std::size_t pos) override
				{
					auto i = container_.begin();
					std::advance(i, pos);
					container_.erase(i);
				}

				std::size_t size() const override
				{
					return container_.size();
				}

				bool immutable() const override
				{
					return false;
				}

				void emplace(std::size_t pos) override
				{
					auto i = container_.begin();
					std::advance(i, pos);

					container_.emplace(i);
				}

				void emplace_back() override
				{
					container_.emplace_back();
				}
			
				void assign(std::size_t pos, const std::vector<cell>& cells) override
				{
					container_.at(pos) = translator_.to_value(cells);
				}

				std::vector<cell> to_cells(std::size_t pos) const override
				{
					return translator_.to_cell(container_.at(pos));
				}

				bool push_back(const const_virtual_pointer& dptr) override
				{
					auto value = dptr.get<value_type>();
					if (value)
					{
						container_.push_back(*value);
						return true;
					}
					return false;
				}

				void* pointer() override
				{
					return &container_;
				}

				const void* pointer() const override
				{
					return &container_;
				}
			private:
				STLContainer container_;
				container_translator<value_type> translator_;
			};


			template<typename STLContainer>
			class shared_container
				: public basic_container<STLContainer>
			{
				using value_type = typename STLContainer::value_type;
				using value_translator = typename container_translator<value_type>::value_translator;
				using cell_translator = typename container_translator<value_type>::cell_translator;

			public:
				using container_reference = STLContainer&;

				
				shared_container(container_reference cont, value_translator vtrans, cell_translator ctrans)
					: container_(cont), translator_({ vtrans, ctrans })
				{
					
				}
			private:
				void clear() override
				{
					container_.clear();
				}

				void erase(std::size_t pos) override
				{
					auto i = container_.begin();
					std::advance(i, pos);
					container_.erase(i);
				}

				std::size_t size() const override
				{
					return container_.size();
				}

				bool immutable() const override
				{
					return false;
				}

				void emplace(std::size_t pos) override
				{
					auto i = container_.begin();
					std::advance(i, pos);

					container_.emplace(i);
				}

				void emplace_back() override
				{
					container_.emplace_back();
				}

				void assign(std::size_t pos, const std::vector<cell>& cells) override
				{
					container_.at(pos) = translator_.to_value(cells);
				}

				std::vector<cell> to_cells(std::size_t pos) const override
				{
					return translator_.to_cell(container_.at(pos));
				}

				bool push_back(const const_virtual_pointer& dptr) override
				{
					auto value = dptr.get<value_type>();
					if (value)
					{
						container_.push_back(*value);
						return true;
					}
					return false;
				}

				void* pointer() override
				{
					return &container_;
				}

				const void* pointer() const override
				{
					return &container_;
				}
			private:
				container_reference container_;
				container_translator<value_type> translator_;
			};

			template<typename STLContainer>
			class shared_immutable_container
				: public basic_container<STLContainer>
			{
				using value_type = typename STLContainer::value_type;
				using cell_translator = typename container_translator<value_type>::cell_translator;


			public:
				using container_reference = const STLContainer&;


				shared_immutable_container(container_reference cont, cell_translator ctrans)
					: container_(cont), ctrans_(ctrans)
				{
				}
			private:
				void clear() override
				{
					throw std::runtime_error("nana::listbox disallow to remove items because of immutable model");
				}

				void erase(std::size_t /*pos*/) override
				{
					throw std::runtime_error("nana::listbox disallow to remove items because of immutable model");
				}

				std::size_t size() const override
				{
					return container_.size();
				}

				bool immutable() const override
				{
					return true;
				}

				void emplace(std::size_t /*pos*/) override
				{
					throw std::runtime_error("nana::listbox disallow to remove items because of immutable model");
				}

				void emplace_back() override
				{
					throw std::runtime_error("nana::listbox disallow to remove items because of immutable model");
				}

				void assign(std::size_t /*pos*/, const std::vector<cell>& /*cells*/) override
				{
					throw std::runtime_error("nana::listbox disallow to remove items because of immutable model");
				}

				std::vector<cell> to_cells(std::size_t pos) const override
				{
					return ctrans_(container_.at(pos));
				}

				bool push_back(const const_virtual_pointer& /*dptr*/) override
				{
					throw std::runtime_error("nana::listbox disallow to remove items because of immutable model");
				}

				void* pointer() override
				{
					return nullptr;
				}

				const void* pointer() const override
				{
					return &container_;
				}
			private:
				container_reference container_;
				cell_translator ctrans_;
			};

			class model_interface
			{
			public:
				virtual ~model_interface() = default;

				virtual void lock() = 0;
				virtual void unlock() = 0;

				virtual container_interface* container() noexcept = 0;
				virtual const container_interface* container() const noexcept = 0;
			};

			class model_guard
			{
				model_guard(const model_guard&) = delete;
				model_guard& operator=(const model_guard&) = delete;
			public:
				model_guard(model_interface* model)
					: model_(model)
				{
					model->lock();
				}

				model_guard(model_guard&& other)
					: model_(other.model_)
				{
					other.model_ = nullptr;
				}

				~model_guard() noexcept
				{
					if (model_)
						model_->unlock();
				}

				model_guard& operator=(model_guard&& other)
				{
					if (this != &other)
					{
						if (model_)
							model_->unlock();

						model_ = other.model_;
						other.model_ = nullptr;
					}
					return *this;
				}

				template<typename STLContainer>
				STLContainer& container()
				{
					using stlcontainer = typename std::decay<STLContainer>::type;

					if (!model_)
						throw std::runtime_error("nana::listbox empty model_guard");

					using type = basic_container<stlcontainer>;
					auto p = dynamic_cast<type*>(model_->container());
					if (nullptr == p)
						throw std::invalid_argument("invalid listbox model container type");

					if (nullptr == p->pointer())
						throw std::runtime_error("the modal is immutable, please declare model_guard with const");

					return *static_cast<stlcontainer*>(p->pointer());
				}

				template<typename STLContainer>
				const STLContainer& container() const
				{
					using stlcontainer = typename std::decay<STLContainer>::type;

					if (!model_)
						throw std::runtime_error("nana::listbox empty model_guard");

					using type = basic_container<stlcontainer>;
					auto p = dynamic_cast<const type*>(model_->container());
					if (nullptr == p)
						throw std::invalid_argument("invalid listbox model container type");

					return *static_cast<const stlcontainer*>(p->pointer());
				}
			private:
				model_interface* model_;
			};

			template<typename STLContainer, typename Mutex>
			class standalone_model_container
				: public model_interface
			{
			public:
				using value_translator = typename container_translator<typename STLContainer::value_type>::value_translator;
				using cell_translator = typename container_translator<typename STLContainer::value_type>::cell_translator;

				standalone_model_container(STLContainer&& container, value_translator vtrans, cell_translator ctrans)
					: container_(std::move(container), std::move(vtrans), std::move(ctrans))
				{
				}

				standalone_model_container(const STLContainer& container, value_translator vtrans, cell_translator ctrans)
					: container_(container, std::move(vtrans), std::move(ctrans))
				{
				}

				void lock() override
				{
					mutex_.lock();
				}

				void unlock() override
				{
					mutex_.unlock();
				}

				container_interface* container() noexcept override
				{
					return &container_;
				}

				const container_interface* container() const noexcept override
				{
					return &container_;
				}
			private:
				Mutex mutex_;
				standalone_container<STLContainer> container_;
			};

			template<typename STLContainer, typename Mutex>
			class shared_model_container
				: public model_interface
			{
			public:
				using value_translator = typename container_translator<typename STLContainer::value_type>::value_translator;
				using cell_translator = typename container_translator<typename STLContainer::value_type>::cell_translator;

				shared_model_container(STLContainer& container, value_translator vtrans, cell_translator ctrans)
					: container_ptr_(new shared_container<STLContainer>(container, std::move(vtrans), std::move(ctrans)))
				{
				}

				shared_model_container(const STLContainer& container, cell_translator ctrans)
					: container_ptr_(new shared_immutable_container<STLContainer>(container, std::move(ctrans)))
				{
				}

				void lock() override
				{
					mutex_.lock();
				}

				void unlock() override
				{
					mutex_.unlock();
				}

				container_interface* container() noexcept override
				{
					return container_ptr_.get();
				}

				const container_interface* container() const noexcept override
				{
					return container_ptr_.get();
				}
			private:
				Mutex mutex_;
				std::unique_ptr<container_interface> container_ptr_;
			};


			/// useful for both absolute and display (sorted) positions
			struct index_pair
			{
				constexpr static const size_type npos = ::nana::npos;

				size_type cat;	//The pos of category
				size_type item;	//the pos of item in a category.

				explicit index_pair(size_type cat_pos = 0, size_type item_pos = 0) noexcept
					: cat(cat_pos), item(item_pos)
				{}

				bool empty() const noexcept
				{
					return (npos == cat);
				}

				void set_both(size_type n) noexcept
				{
					cat = item = n;
				}

				bool is_category() const noexcept
				{
					return (npos != cat && npos == item);
				}

				bool operator==(const index_pair& r) const noexcept
				{
					return (r.cat == cat && r.item == item);
				}

				bool operator!=(const index_pair& r) const noexcept
				{
					return !this->operator==(r);
				}

				bool operator<(const index_pair& r) const noexcept
				{
					return (cat < r.cat) || ((cat == r.cat) && (r.item != npos) && ((item == npos) || (item < r.item)));
				}

				bool operator>(const index_pair& r) const noexcept
				{
					return (cat > r.cat) || ((cat == r.cat) && (item != npos) && ((r.item == npos) || (item > r.item)));
				}
			};

			using index_pairs = ::std::vector<index_pair>;

			enum class inline_widget_status{
				checked,
				checking,
				selected,
				selecting
			};

			using inline_notifier_interface = detail::inline_widget_notifier_interface<index_pair, inline_widget_status, ::std::string>;

			// struct essence
			//@brief:	this struct gives many data for listbox,
			//			the state of the struct does not effect on member functions, therefore all data members are public.
			struct essence;

			class oresolver
			{
			public:
				oresolver(essence*) noexcept;
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

				oresolver& operator<<(const char* text_utf8);
				oresolver& operator<<(const wchar_t*);
				oresolver& operator<<(const std::string& text_utf8);
				oresolver& operator<<(const std::wstring&);
				oresolver& operator<<(std::wstring&&);
				oresolver& operator<<(cell);
				oresolver& operator<<(std::nullptr_t);

				std::vector<cell> && move_cells() noexcept;

				::nana::listbox& listbox() noexcept;
			private:
				essence* const ess_;
				std::vector<cell> cells_;
			};

			class iresolver
			{
			public:
				iresolver(std::vector<cell>) noexcept;

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

				iresolver& operator>>(std::string& text_utf8);
				iresolver& operator>>(std::wstring&);
				iresolver& operator>>(cell&);
				iresolver& operator>>(std::nullptr_t) noexcept;
			private:
				std::vector<cell> cells_;
				std::size_t pos_{0};
			};


			struct category_t;
			class drawer_header_impl;
			class drawer_lister_impl;

			/// mostly works on display positions
			class trigger: public drawer_trigger
			{
			public:
				trigger();
				~trigger();
				essence& ess() const noexcept;
			private:
				void attached(widget_reference, graph_reference)	override;
				void detached()	override;
				void typeface_changed(graph_reference)	override;
				void refresh(graph_reference)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void dbl_click(graph_reference, const arg_mouse&)	override;
				void resized(graph_reference, const arg_resized&)		override;
				void key_press(graph_reference, const arg_keyboard&)	override;
				void key_char(graph_reference, const arg_keyboard&)	override;
			private:
				essence * essence_;
				drawer_header_impl *drawer_header_;
				drawer_lister_impl *drawer_lister_;
			};//end class trigger

			/// operate with absolute positions and contain only the position but maintain pointers to parts of the real items 
			/// item_proxy self, it references and iterators are not invalidated by sort()
			class item_proxy
				: public ::nana::widgets::detail::widget_iterator<std::input_iterator_tag, item_proxy>
			{
			public:
				item_proxy(essence*, const index_pair& = index_pair{npos, npos});

				/// the main purpose of this it to make obvious that item_proxy operate with absolute positions, and don't get moved during sort()
				static item_proxy from_display(essence *, const index_pair &relative) ;
				item_proxy from_display(const index_pair &relative) const;

				/// possible use: last_selected_display = last_selected.to_display().item; use with caution, it get invalidated after a sort()
				index_pair to_display() const;

				/// Determines whether the item is displayed on the screen
				bool displayed() const;

				bool empty() const noexcept;

				/// Checks/unchecks the item
				/**
				* @param chk Indicates whether to check or uncheck the item
				* @param scroll_view Indicates whether to scroll the view to the item. It is ignored if the item is displayed.
				* @return the reference of *this.
				*/
				item_proxy & check(bool chk, bool scroll_view = false);

				/// Determines whether the item is checked
				bool checked() const;

				/// Selects/deselects the item
				/**
				 * @param sel Indicates whether to select or deselect the item
				 * @param scroll_view Indicates whether to scroll the view to the item. It is ignored if the item is displayed.
				 * @return the reference of *this.
				 */
				item_proxy & select(bool sel, bool scroll_view = false);

				/// Determines whether he item is selected
				bool selected() const;

				item_proxy & bgcolor(const nana::color&);
				nana::color bgcolor() const;

				item_proxy& fgcolor(const nana::color&);
				nana::color fgcolor() const;

				index_pair pos() const noexcept;

				size_type columns() const noexcept;

				/// Converts a position of column between display position and absolute position
				/**
				 * @param col The display position or absolute position.
				 * @param disp_order Indicates whether the col is a display position or absolute position. If this parameter is true, the col is display position
				 * @return absolute position if disp_order is false, display position otherwise. 
				 */
				size_type column_cast(size_type col, bool disp_order) const;

				item_proxy&		text(size_type abs_col, cell);
				item_proxy&		text(size_type abs_col, std::string);
				item_proxy&		text(size_type abs_col, const std::wstring&);
				std::string	text(size_type abs_col) const;

				void icon(const nana::paint::image&);

				template<typename T>
				item_proxy & resolve_from(const T& t)
				{
					oresolver ores(_m_ess());
					ores << t;
					auto && cells = ores.move_cells();
					auto cols = columns();
					cells.resize(cols);
					for (auto pos = 0u; pos < cols; ++pos)
					{
						auto & el = cells[pos];
						if (el.text.size() == 1 && el.text[0] == '\0')
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
				T const * value_ptr() const
				{
					return any_cast<T>(_m_value());
				}

				template<typename T>
				T & value() const
				{
					auto * pany = _m_value();
					if(nullptr == pany)
						throw std::runtime_error("listbox::item_proxy.value<T>() is empty");

					T * p = any_cast<T>(_m_value());
					if(nullptr == p)
						throw std::runtime_error("listbox::item_proxy.value<T>() invalid type of value");
					return *p;
				}
				template<typename T>
				T & value() 
				{
					auto * pany = _m_value();
					if (nullptr == pany)
						throw std::runtime_error("listbox::item_proxy.value<T>() is empty");

					T * p = any_cast<T>(_m_value(false));
					if (nullptr == p)
						throw std::runtime_error("listbox::item_proxy.value<T>() invalid type of value");
					return *p;
				}
				template<typename T>
				item_proxy & value(T&& t)
				{
					*_m_value(true) = ::std::forward<T>(t);
					return *this;
				}

				/// Behavior of Iterator's value_type
#ifdef _nana_std_has_string_view
				bool operator==(::std::string_view sv) const;
				bool operator==(::std::wstring_view sv) const;
#else
				bool operator==(const char * s) const;
				bool operator==(const wchar_t * s) const;
				bool operator==(const ::std::string& s) const;
				bool operator==(const ::std::wstring& s) const;
#endif

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
				essence * _m_ess() const noexcept;
			private:
				std::vector<cell> _m_cells() const;
				nana::any		* _m_value(bool alloc_if_empty);
				const nana::any	* _m_value() const;
			private:
				essence * ess_;
				category_t*	cat_{nullptr};

				index_pair	pos_; //Position of an item, it never represents a category when item proxy is available.
			};

			class cat_proxy
				: public ::nana::widgets::detail::widget_iterator<std::input_iterator_tag, cat_proxy>
			{
			public:
				using inline_notifier_interface = drawerbase::listbox::inline_notifier_interface;
				template<typename Value> using value_translator = typename container_translator<Value>::value_translator;
				template<typename Value> using cell_translator = typename container_translator<Value>::cell_translator;

				cat_proxy() noexcept = default;
				cat_proxy(essence*, size_type pos) noexcept;
				cat_proxy(essence*, category_t*) noexcept;

				/// Append an item at the end of this category using the oresolver to generate the texts to be put in each column.
                ///
                /// First you have to make sure there is an overload of the operator<<() of the oresolver for the type of the object used here
                /// If a listbox have a model set, try call append_model instead.
                template<typename T>
				item_proxy append(  T&& t,                  ///< Value used by the resolver to generate the texts to be put in each column of the item
                                    bool set_value = false) ///< determines whether to set the object as the value of this item.
				{
					oresolver ores(ess_);

					//Troubleshoot:
					//If a compiler error that no operator<< overload found for type T occurs, please define a overload operator<<(oresolver&, const T&).
					//If a listbox have a model set, try call append_model instead.
					if (set_value)
						ores << t;	//copy it if it is rvalue and set_value is true.
					else
						ores << std::forward<T>(t);

					_m_append(ores.move_cells());

					item_proxy iter{ ess_, index_pair(pos_, size() - 1) };
					if (set_value)
						iter.value(std::forward<T>(t));

					_m_update();

					return iter;
				}

				template<typename T>
				void append_model(const T& t)
				{
					nana::internal_scope_guard lock;
					_m_try_append_model(const_virtual_pointer{ &t });
					_m_update();
				}

				template<typename Mutex, typename STLContainer, typename ValueTranslator, typename CellTranslator>
				void model(STLContainer&& container, ValueTranslator vtrans, CellTranslator ctrans)
				{
					_m_reset_model(new standalone_model_container<typename std::decay<STLContainer>::type, Mutex>(std::forward<STLContainer>(container), std::move(vtrans), std::move(ctrans)));
				}

				template<typename Mutex, typename STLContainer, typename ValueTranslator, typename CellTranslator>
				void shared_model(STLContainer& container, ValueTranslator vtrans, CellTranslator ctrans)
				{
					_m_reset_model(new shared_model_container<typename std::decay<STLContainer>::type, Mutex>(container, std::move(vtrans), std::move(ctrans)));
				}

				template<typename Mutex, typename STLContainer, typename CellTranslator>
				void shared_model(const STLContainer& container, CellTranslator ctrans)
				{
					_m_reset_model(new shared_model_container<typename std::decay<STLContainer>::type, Mutex>(container, std::move(ctrans)));
				}

				model_guard model();

				/// Appends one item at the end of this category with the specifies texts in the column fields
				void append(std::initializer_list<std::string> texts_utf8);
				void append(std::initializer_list<std::wstring> texts);

				size_type columns() const;

				cat_proxy& text(std::string);
				cat_proxy& text(std::wstring);
				std::string text() const;

				cat_proxy & select(bool);
				bool selected() const;

				/// Enables/disables the number of items in the category to be displayed behind the category title
				cat_proxy& display_number(bool display);

				/// Determines whether the category is expanded.
				bool expanded() const;

				/// Expands/collapses the category
				/**
				 * @param expand Indicates whether to expand or collapse the category. If this parameter is true, it expands the category. If the parameter is false, it collapses the category.
				 * @return the reference of *this.
				 */
				cat_proxy& expanded(bool expand);

				/// Behavior of a container
				void push_back(std::string text_utf8);

				item_proxy begin() const;
				item_proxy end() const;
				item_proxy cbegin() const;
				item_proxy cend() const;

				item_proxy at(size_type pos_abs) const;
				item_proxy back() const;

				/// Converts the index between absolute position and display position
				/**
				 * @param from	The index to be converted
				 * @param from_display_order	If this parameter is true, the method converts a display position to an absolute position.
				 *								If the parameter is false, the method converts an absolute position to a display position.
				 * @return a display position or an absolute position that are depending on from_display_order.
				 */
				size_type index_cast(size_type from, bool from_display_order) const;
				
				/// this cat position
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

				void inline_factory(size_type column, pat::cloneable<pat::abstract_factory<inline_notifier_interface>> factory);
			private:
				void _m_append(std::vector<cell> && cells);
				void _m_try_append_model(const const_virtual_pointer&);
				void _m_cat_by_pos() noexcept;
				void _m_update() noexcept;
				void _m_reset_model(model_interface*);
			private:
				essence*	ess_{nullptr};
				category_t*	cat_{nullptr};
				size_type	pos_{0};  ///< Absolute position, not relative to display, and dont change during sort()
			};
		
			struct export_options
			{
				std::string sep = ::std::string {"\t"}, 
							 endl= ::std::string {"\n"};
				bool only_selected_items{true}, 
					 only_checked_items {false},
					 only_visible_columns{true};

				using columns_indexs = std::vector<size_type>;
				columns_indexs columns_order;
			};
		}
	}//end namespace drawerbase

	struct arg_listbox
		: public event_arg
	{
		mutable drawerbase::listbox::item_proxy item;

		arg_listbox(const drawerbase::listbox::item_proxy&) noexcept;
	};

	/// The event parameter type for listbox's category_dbl_click
	struct arg_listbox_category
		: public event_arg
	{
		drawerbase::listbox::cat_proxy category;

		/// A flag that indicates whether or not to block expansion/shrink of category when it is double clicked.
		mutable bool block_operation{ false };

		arg_listbox_category(const drawerbase::listbox::cat_proxy&) noexcept;
	};

	namespace drawerbase
	{
		namespace listbox
		{
			struct listbox_events
				: public general_events
			{
				/// An event occurs when the toggle of a listbox item is checked.
				basic_event<arg_listbox> checked;

				/// An event occurs when a listbox item is clicked.
				basic_event<arg_listbox> selected;

				/// An event occurs when a listbox category is double clicking.
				basic_event<arg_listbox_category> category_dbl_click;
			};

			struct scheme
				: public widget_geometrics
			{
				color_proxy header_bgcolor{static_cast<color_rgb>(0xf1f2f4)};
				color_proxy header_fgcolor{ colors::black };
				color_proxy header_grabbed{ static_cast<color_rgb>(0x8BD6F6)};
				color_proxy header_floated{ static_cast<color_rgb>(0xBABBBC)};
				color_proxy item_selected{ static_cast<color_rgb>(0xCCE8FF) };
				color_proxy item_highlighted{ static_cast<color_rgb>(0xE5F3FF) };

				color_proxy selection_box{ static_cast<color_rgb>(0x3399FF) };	///< Color of selection box border.


				std::shared_ptr<paint::font> column_font;	///< Renderer draws column texts with the font if it is not a nullptr.

				/// The max column width which is generated by fit_content is allowed. It is ignored when it is 0, or a max value is passed to fit_content.
				unsigned max_fit_content{ 0 };

				unsigned min_column_width{ 20 };  ///< def=20 . non counting suspension_width

				unsigned text_margin{ 5 };  ///<  def= 5. Additional or extended with added (before) to the text width to determine the cell width. cell_w = text_w + ext_w +1

				unsigned item_height_ex{ 6 };  ///< Set !=0 !!!!  def=6. item_height = text_height + item_height_ex
				unsigned header_splitter_area_before{ 2 }; ///< def=2. But 4 is better... IMO
				unsigned header_splitter_area_after{ 3 }; ///< def=3. But 4 is better...
				unsigned header_padding_top{ 3 };
				unsigned header_padding_bottom{ 3 };

				::nana::parameters::mouse_wheel mouse_wheel{}; ///< The number of lines/characters to scroll when vertical/horizontal mouse wheel is moved.
			};
		}
	}//end namespace drawerbase

/*! \class listbox
\brief A rectangle containing a list of strings from which the user can select. 
This widget contain a list of \a categories, with in turn contain a list of \a items. 
A \a category is a text with can be \a selected, \a checked and \a expanded to show the \a items.
An \a item is formed by \a column-fields, each corresponding to one of the \a headers. 
An \a item can be \a selected and \a checked.
The user can \a drag the header to \a resize it or to \a reorganize it. 
By \a clicking on one header the list get \a reordered, first up, and then down alternatively.

1. The resolver is used to resolute an object of the specified type into (or back from) a listbox item.
3. nana::listbox creates the category 0 by default. 
   This is an special category, because it is invisible, while the associated items are visible. 
   The optional, user-created categories begin at index 1 and are visible.
   The member functions without the categ parameter operate the items that belong to category 0.
4. A sort compare is used for sorting the items. It is a strict weak ordering comparer that must meet the requirement:
		Irreflexivity (comp(x, x) returns false) 
	and 
		Antisymmetry(comp(a, b) != comp(b, a) returns true)
	A simple example.
		bool sort_compare( const std::string& s1, nana::any*, 
						   const std::string& s2, nana::any*, bool reverse)
		{
			return (reverse ? s1 > s2 : s1 < s2);
		}
		listbox.set_sort_compare(0, sort_compare);
	The listbox supports attaching a customer's object for each item, therefore the items can be 
	sorted by comparing these customer's object.
		bool sort_compare( const std::string&, nana::any* o1, 
						   const std::string&, nana::any* o2, bool reverse)
		{
			if(o1 && o2) 	//some items may not attach a customer object.
			{
				int * i1 = any_cast<int>(*o1);
				int * i2 = any_cast<int>(*o2);
				return (i1 && i2 && (reverse ? *i1 > *i2 : *i1 < *i2));
 					  // ^ some types may not be int.
			}
			return false;
		}
		auto cat = listbox.at(0);
		cat.at(0).value(10); //10 is custom data.
		cat.at(1).value(20); //20 is custom data.
5. listbox is a widget_object, with template parameters drawerbase::listbox::trigger and drawerbase::listbox::scheme 
among others.
That means that listbox have a member trigger_ constructed first and accessible with get_drawer_trigger() and
a member (unique pointer to) scheme_ accessible with scheme_type& scheme() created in the constructor 
with API::dev::make_scheme<Scheme>() which call API::detail::make_scheme(::nana::detail::scheme_factory<Scheme>())
which call restrict::bedrock.make_scheme(static_cast<::nana::detail::scheme_factory_base&&>(factory));
which call pi_data_->scheme.create(std::move(factory));
which call factory.create(scheme_template(std::move(factory)));
which call (new Scheme(static_cast<Scheme&>(other)));
and which in create is set with: API::dev::set_scheme(handle_, scheme_.get()); which save the scheme pointer in 
the nana::detail::basic_window member pointer scheme
\todo doc: actualize this example listbox.at(0)...
\see nana::drawerbase::listbox::cat_proxy
\see nana::drawerbase::listbox::item_proxy
\example listbox_Resolver.cpp
*/
	class listbox
		:	public widget_object<category::widget_tag,
		                         drawerbase::listbox::trigger,
		                         drawerbase::listbox::listbox_events,
		                         drawerbase::listbox::scheme>,
			public concepts::any_objective<drawerbase::listbox::size_type, 2>
	{
	public:
		/// An unsigned integral type
		using size_type		= drawerbase::listbox::size_type;

		/// The representation of a category/item
		using index_pair	= drawerbase::listbox::index_pair;

		/// A index_pair package
		using index_pairs	= drawerbase::listbox::index_pairs;

		/// Iterator to access category
		using cat_proxy		= drawerbase::listbox::cat_proxy;

		/// Iterator to access item
		using item_proxy	= drawerbase::listbox::item_proxy;

		/// The input resolver that converts an object to an item
		using iresolver = drawerbase::listbox::iresolver;

		/// The output resolver that converts an item to an object
		using oresolver = drawerbase::listbox::oresolver;

		/// The representation of an item
		using cell		= drawerbase::listbox::cell;

		/// The options of exporting items into a string variable
		using export_options = drawerbase::listbox::export_options;

		/// The interface for user-defined inline widgets
		using inline_notifier_interface = drawerbase::listbox::inline_notifier_interface;

		/// Column operations
		using column_interface = drawerbase::listbox::column_interface;
	public:

		/// Constructors
		listbox() = default;
		listbox(window, bool visible);
		listbox(window, const rectangle& = {}, bool visible = true);

	//Element access

		/// Returns the category at specified location pos, with bounds checking.
		cat_proxy at(size_type pos);
		const cat_proxy at(size_type pos) const;

		/// Returns the item at specified absolute position
		item_proxy at(const index_pair& abs_pos);
		const item_proxy at(const index_pair &abs_pos) const;


		/// Returns the category at specified location pos, no bounds checking is performed.
		cat_proxy operator[](size_type pos);
		const cat_proxy operator[](size_type pos) const;

		/// Returns the item at specified absolute position, no bounds checking is performed.
		item_proxy operator[](const index_pair& abs_pos);
		const item_proxy operator[](const index_pair &abs_pos) const;

	//Associative category access

		/// Returns a proxy to the category of the key or create a new one in the right order
		/**
		* @param key The key of category to find
		* @return A category proxy
		*/
		template<typename Key>
		cat_proxy assoc(Key&& key)
		{
			using key_type = typename ::nana::detail::type_escape<const typename std::decay<Key>::type>::type;

			auto p = std::make_shared<nana::key<key_type, std::less<key_type>>>(std::forward<Key>(key));
			return cat_proxy(&_m_ess(), _m_assoc(p, true));
		}

		/// Returns a proxy to the category of the key or create a new one in the right order
		/**
		* @param key The key of category to find
		* @return A category proxy
		*/
		template<typename Key>
		cat_proxy assoc_at(Key&& key)
		{
			using key_type = typename ::nana::detail::type_escape<const typename std::decay<Key>::type>::type;

			auto p = std::make_shared<nana::key<key_type, std::less<key_type>>>(std::forward<Key>(key));

			auto categ = _m_assoc(p, false);
			if (nullptr == categ)
				throw std::out_of_range("listbox: invalid key.");

			return cat_proxy(&_m_ess(), categ);
		}

		/// Removes a category which is associated with the specified key
		/**
		* @param key The key of category to remove
		*/
		template<typename Key>
		void assoc_erase(Key&& key)
		{
			using key_type = typename ::nana::detail::type_escape<const typename std::decay<Key>::type>::type;

			::nana::key<key_type, std::less<key_type>> wrap(key);
			_m_erase_key(&wrap);
		}

		bool assoc_ordered(bool);


		void auto_draw(bool) noexcept;		///< Set state: Redraw automatically after an operation

		template<typename Function>
		void avoid_drawing(Function fn)
		{
			this->auto_draw(false);
			try
			{
				fn();
			}
			catch (...)
			{
				this->auto_draw(true);
				throw;
			}
			this->auto_draw(true);
		}

		/// Scrolls the view to the first or last item of a specified category
		void scroll(bool to_bottom, size_type cat_pos = ::nana::npos);

		/// Scrolls the view to show an item specified by absolute position at top/bottom of the listbox.
		void scroll(bool to_bottom, const index_pair& abs_pos);

		/// Appends a new column with a header text and the specified width at the end, and return it position
		size_type append_header(std::string text_utf8, unsigned width = 120);
		size_type append_header(std::wstring text, unsigned width = 120);

		void clear_headers();					///< Removes all the columns.

		cat_proxy append(std::string category);		///< Appends a new category to the end
		cat_proxy append(std::wstring category);		///< Appends a new category to the end
		void append(std::initializer_list<std::string> categories); ///< Appends categories to the end
		void append(std::initializer_list<std::wstring> categories); ///< Appends categories to the end

		/// Access a column at specified position
		/**
		 * @param pos Position of column
		 * @param disp_order Indicates whether the pos is display position or absolute position.
		 * @return Reference to the requested column
		 * @except std::out_of_range if !(pos < columns())
		 */
		column_interface & column_at(size_type pos, bool disp_order = false);

		/// Access a column at specified position
		/**
		* @param pos Position of column
		* @param disp_order Indicates whether the pos is display position or absolute position.
		* @return Constant reference to the requested column
		* @except std::out_of_range if !(pos < columns())
		*/
		const column_interface & column_at(size_type pos, bool disp_order = false) const;

		/// Returns the number of columns
		size_type column_size() const;

		/// Move column to view_position
        void move_column(size_type abs_pos, size_type view_pos);

        /// Sort columns in range first_col to last_col inclusive using the values from a row
        void reorder_columns(size_type first_col,
							 size_type last_col,
							 index_pair row, bool reverse,
							 std::function<bool(const std::string &cell1, size_type col1,
												const std::string &cell2, size_type col2,
												const nana::any *rowval,
												bool reverse)> comp);

        void column_resizable(bool resizable);
		bool column_resizable() const;
		void column_movable(bool);
		bool column_movable() const;

		/// Returns a rectangle in where the content is drawn.
		rectangle content_area() const;

		cat_proxy insert(cat_proxy, ::std::string);
		cat_proxy insert(cat_proxy, ::std::wstring);

		/// Inserts an item before a specified position
		/**
		 * @param abs_pos The absolute position before which an item will be inserted.
		 * @param text Text of the first column, in UTF-8 encoded.
		 */
		void insert_item(const index_pair& abs_pos, ::std::string text);

		/// Inserts an item before a specified position
		/**
		 * @param abs_pos The absolute position before which an item will be inserted.
		 * @param text Text of the first column.
		 */
		void insert_item(const index_pair& abs_pos, const ::std::wstring& text);


		void insert_item(index_pair abs_pos, const listbox& rhs, const index_pairs& indexes);

		/// Returns an index of item which contains the specified point.
		index_pair cast(const point & screen_pos) const;

		/// Returns the item which is hovered
		/**
		 * @param return_end Indicates whether to return an end position instead of empty position if an item is not hovered.
		 * @return The position of the hovered item. If return_end is true, it returns the position next to the last item of last category if an item is not hovered.
		 */
		index_pair hovered(bool return_end) const;

		/// Returns the absolute position of column which contains the specified point.
		size_type column_from_pos(const point & pos) const;

		void checkable(bool);
		index_pairs checked() const;                         ///<Returns the items which are checked.

		void clear(size_type cat);			///<Removes all the items from the specified category
		void clear();						///<Removes all the items from all categories
		void erase(size_type cat);			///<Erases a category
		void erase();						///<Erases all categories.
		void erase(index_pairs indexes);	///<Erases specified items.
		item_proxy erase(item_proxy);

		bool sortable() const;
		void sortable(bool enable);
		
		///Sets a strict weak ordering comparer for a column
		void set_sort_compare(	size_type col,
								std::function<bool(const std::string&, nana::any*,
								                   const std::string&, nana::any*, bool reverse)> strick_ordering);

		/// sort() and invalidates any existing reference from display position to absolute item, that is: after sort() display offset point to different items
		void sort_col(size_type col, bool reverse = false);
		size_type sort_col() const;

		/// potentially invalidates any existing reference from display position to absolute item, that is: after sort() display offset point to different items
		void unsort();
		bool freeze_sort(bool freeze);

		index_pairs selected() const;		///<Get the absolute indexs of all the selected items

		void show_header(bool);
		bool visible_header() const;
		void move_select(bool upwards);		///<Selects an item besides the current selected item in the display.

		size_type size_categ() const;                   ///<Get the number of categories
		size_type size_item(size_type cat) const;       ///<The number of items in category "cat"

		void enable_single(bool for_selection, bool category_limited);
		void disable_single(bool for_selection);
		bool is_single_enabled(bool for_selection) const noexcept;	///< Determines whether the single selection/check is enabled.
		export_options& def_export_options();


		/// Sets a renderer for category icon
		/**
		 * @param icon_renderer The renderer of category icon
		 * @return the reference of *this.
		 */
		listbox& category_icon(std::function<void(paint::graphics& graph, const rectangle& rt_icon, bool expanded)> icon_renderer);

		/// Sets category icons
		/**
		 * @param img_expanded An icon displayed in front of category title when the category is expanded.
		 * @param img_collapsed An icon displayed in front of category title when the category is collapsed.
		 * @return the reference of *this.
		 */
		listbox& category_icon(const paint::image& img_expanded, const paint::image& img_collapsed);

		/// Returns first visible element
		/**
		 * It may return an item or a category item.
		 * @return the index of first visible element.
		 */
		index_pair first_visible() const;

		/// Returns last visible element
		/**
		 * It may return an item or a category item.
		 * @return the index of last visible element.
		 */
		index_pair last_visible() const;

		/// Returns all visible items
		/**
		 * It returns all visible items that are displayed in listbox window.
		 * @return index_pairs containing all visible items.
		 */
		index_pairs visibles() const;

		/// Sets a predicate that indicates whether to deselect items when mouse_up is triggered.
		/**
		 * The predicate is called before the listbox attempts to deselect the selected items in the mouse_up event. Other situations,
		 * the predicates isn't called, for example, releasing mouse button after user performed a box selection, because listbox doesn't deselect the items during this operation.
		 * @param predicate Decides to deselect the items.
		 *	The paramater of predicate indicates the mouse button which is releasing.
		 *	It returns true to deselect the selected items. It returns false to cancel to deselect the selected items.
		 */
		void set_deselect(std::function<bool(nana::mouse)> predicate);

		unsigned suspension_width() const;
	private:
		drawerbase::listbox::essence & _m_ess() const;
		nana::any* _m_anyobj(size_type cat, size_type index, bool allocate_if_empty) const override;
		drawerbase::listbox::category_t* _m_assoc(std::shared_ptr<nana::detail::key_interface>, bool create_if_not_exists);
		void _m_erase_key(nana::detail::key_interface*) noexcept;
		std::shared_ptr<scroll_operation_interface> _m_scroll_operation() override;
	};
}//end namespace nana

#include <nana/pop_ignore_diagnostic>
#endif
