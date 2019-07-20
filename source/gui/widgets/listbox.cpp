/*
 *	A List Box Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/listbox.cpp
 *	@contributors:
 *		Hiroshi Seki
 *		Ariel Vina-Rodriguez
 *		leobackes(pr#86,pr#97)
 *		Benjamin Navarro(pr#81)
 *		besh81(pr#130)
 *		dankan1890(pr#158)
 *		ErrorFlynn(pr#418,pr#448,pr#454)
 *
 */
#include <algorithm>
#include <list>
#include <deque>
#include <stdexcept>
#include <map>
#include <iostream>

#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/panel.hpp>	//for inline widget

#include <nana/gui/layout_utility.hpp>
#include <nana/gui/element.hpp>
#include <nana/paint/text_renderer.hpp>
#include <nana/system/dataexch.hpp>
#include <nana/system/platform.hpp>
#include "skeletons/content_view.hpp"

namespace nana
{
	static void check_range(std::size_t pos, std::size_t size)
	{
		if (!(pos < size))
			throw std::out_of_range("listbox: invalid element position");
	}

	namespace drawerbase
	{
		namespace listbox
		{
			class model_lock_guard
			{
				model_lock_guard(const model_lock_guard&) = delete;
				model_lock_guard& operator=(const model_lock_guard&) = delete;
			public:
				model_lock_guard(model_interface* model)
					: model_ptr_(model)
				{
					if (model)
						model->lock();
				}

				~model_lock_guard() noexcept
				{
					if (model_ptr_)
						model_ptr_->unlock();
				}
			private:
				model_interface* const model_ptr_;
			};


			//struct cell
				cell::format::format(const ::nana::color& bgcolor, const ::nana::color& fgcolor) noexcept
					: bgcolor{ bgcolor }, fgcolor{ fgcolor }
				{}

				cell::cell(const cell& rhs)
					:	text(rhs.text),
						custom_format{ rhs.custom_format ? new format(*rhs.custom_format) : nullptr }
				{}

				//A workaround that VS2013 does not support to define an explicit default move constructor
				cell::cell(cell&& other) noexcept
					:	text(std::move(other.text)),
						custom_format{ std::move(other.custom_format) }
				{
				}

				cell::cell(std::string text) noexcept
					: text(std::move(text))
				{}

				cell::cell(std::string text, const format& fmt)
					:	text(std::move(text)),
						custom_format(std::make_unique<format>( fmt ))
				{}

				cell& cell::operator=(const cell& rhs)
				{
					if (this != &rhs)
					{
						text = rhs.text;
						custom_format.reset(rhs.custom_format ? new format{*rhs.custom_format} : nullptr);
					}
					return *this;
				}

				cell& cell::operator=(cell&& other) noexcept
				{
					if (this != &other)
					{
						text = std::move(other.text);
						custom_format = std::move(other.custom_format);
					}
					return *this;
				}
			//end struct cell

            // Essence of the columns Header
			class es_header
			{
			public:
				struct attributes
				{
					bool movable{true};
					bool resizable{true};
					bool sortable{true};
					bool visible{true};
				};

				struct column
					: public column_interface
				{
					native_string_type caption;                     //< header title
					unsigned width_px;                              //< column width in pixels
					std::pair<unsigned, unsigned> range_width_px;   //< allowed width
					bool visible_state{ true };


					size_type index;                          //< Absolute position of column when it was created

					nana::align alignment{ nana::align::left };

					std::function<bool(const std::string&,  nana::any*,
							           const std::string&,  nana::any*,        bool reverse)> weak_ordering;

					std::shared_ptr<paint::font> font;	///< The exclusive column font
					
					column(const column&) = default;

					column& operator=(const column& other)
					{
						if (this != &other)
						{
							caption = other.caption;
							width_px = other.width_px;
							range_width_px = other.range_width_px;
							visible_state = other.visible_state;
							index = other.index;
							alignment = other.alignment;
							weak_ordering = other.weak_ordering;
							font = other.font;
						}
						return *this;
					
					}
					
					column(column&& other):
						caption(std::move(other.caption)),
						width_px(other.width_px),
						range_width_px(other.range_width_px),
						visible_state(other.visible_state),
						index(other.index),
						alignment(other.alignment),
						weak_ordering(std::move(other.weak_ordering)),
						font(std::move(other.font)),
						ess_(other.ess_)
					{
					}

					column(essence* ess, native_string_type&& text, unsigned px, size_type pos) noexcept :
						caption(std::move(text)),
						width_px(px),
						index(pos),
						ess_(ess)
					{
					}
				private:
					/// The definition is provided after essence
					void _m_refresh() noexcept;
				private:
					essence* const ess_;
				public:
					/// Implementation of column_interface
					unsigned width() const noexcept override
					{
						return width_px;
					}

					/// Sets the width and overrides the ranged width
					void width(unsigned pixels) noexcept override
					{
						width_px = pixels;
						range_width_px.first = range_width_px.second = 0;

						_m_refresh();
					}

					void width(unsigned minimum, unsigned maximum) override
					{
						//maximum must be larger than minimum, but maximum == 0 is allowed if minimum is 0
						if ((minimum >= maximum) && (minimum != 0))
							throw std::invalid_argument("listbox.column.width() minimum must be less than maximum");

						range_width_px.first = minimum;
						range_width_px.second = maximum;

						unsigned px = std::clamp(static_cast<int>(width_px), static_cast<int>(minimum), static_cast<int>(maximum));
						if (width_px != px)
						{
							width_px = px;
							_m_refresh();
						}
					}

					size_type position(bool disp_order) const noexcept override;	//< The definition is provided after essence

					std::string text() const noexcept override
					{
						return to_utf8(caption);
					}

					void text(std::string text_utf8) override
					{
						caption = to_nstring(std::move(text_utf8));
						_m_refresh();
					}

					void text_align(::nana::align align) noexcept override
					{
						if (alignment != align)
						{
							alignment = align;
							_m_refresh();
						}
					}

					//Definition is provided after essence
					void fit_content(unsigned maximize = 100000) noexcept override;

					/// Sets an exclusive font for the column
					void typeface(const paint::font& column_font) override;

					/// Returns a font
					paint::font typeface() const noexcept override;

					bool visible() const noexcept override
					{
						return visible_state;
					}

					void visible(bool is_visible) noexcept override
					{
						visible_state = is_visible;
						_m_refresh();
					}
				};
			public:
				using container = std::vector<column>;

				export_options::columns_indexs get_headers(bool only_visibles) const
				{
					export_options::columns_indexs	idx;
					for(const auto &col : cont_)
					{
						if(col.visible_state || !only_visibles)
							idx.push_back(col.index);
					}
					return idx;
				}

				std::string to_string(const export_options& exp_opt) const
				{
					std::string head_str;
					bool first{true};
					for( size_type idx{}; idx<exp_opt.columns_order.size(); ++idx)
					{
						if(first)
							first=false;
						else
							head_str += exp_opt.sep;
						head_str += this->at(exp_opt.columns_order[idx]).text();
					}
					return head_str;
				}

				const attributes& attrib() const noexcept
				{
					return attrib_;
				}

				attributes& attrib() noexcept
				{
					return attrib_;
				}

				size_type create(essence* ess, native_string_type&& text, unsigned pixels)
				{
#ifdef _nana_std_has_emplace_return_type
					return cont_.emplace_back(ess, std::move(text), pixels, static_cast<size_type>(cont_.size())).index;
#else
					cont_.emplace_back(ess, std::move(text), pixels, static_cast<size_type>(cont_.size()));
                    return cont_.back().index;
#endif
				}

				void clear()
				{
					cont_.clear();
				}

				unsigned width_px() const noexcept  ///< the visible width of the whole header
				{
					unsigned pixels = 0;
					for(auto & col : cont_)
					{
						if (col.visible_state)
							pixels += col.width_px;
					}
					return pixels;
				}

				/// Calculates the ranged columns to make the whole header fit a specified width
				/**
				 * @param width The width to be fittd
				 * @return true if the ranged columns is adjusted for the width, false otherwise.
				 */
				bool calc_ranged_columns(unsigned width) noexcept
				{
					unsigned fixed_px = 0;
					unsigned minimal_px = 0;
					unsigned maximal_px = 0;

					unsigned ranged_px = 0;
					unsigned ranged_count = 0;

					auto const & const_cont = cont_;
					for (auto & col : const_cont)
					{
						if (col.visible_state)
						{
							if (col.range_width_px.first == col.range_width_px.second)
							{
								fixed_px += col.width_px;
								continue;
							}

							minimal_px += col.range_width_px.first;
							maximal_px += col.range_width_px.second;

							ranged_px += col.width_px;
							++ranged_count;
						}
					}

					// Don't calculate because the header fits the width
					if (ranged_px + fixed_px == width)
						return true;

					//Don't calculate the ranged columns if
					//there isn't a ranged column while maximal_px == 0, or
					//the minimal ranged size is larger than width
					if ((0 == maximal_px) || (fixed_px + minimal_px > width))
						return false;

					const bool beyond = (ranged_px + fixed_px > width);
					unsigned delta_px = (beyond ? ranged_px + fixed_px - width : width - (ranged_px + fixed_px));

					while (delta_px)
					{
						for (auto & col : cont_)
						{
							if (0 == delta_px)
								break;

							if (col.visible_state && (col.range_width_px.first < col.range_width_px.second))
							{
								if (beyond)
								{
									if (col.range_width_px.first < col.width_px)
									{
										--col.width_px;
										--delta_px;
									}
								}
								else
								{
									if (col.width_px < col.range_width_px.second)
									{
										++col.width_px;
										--delta_px;
									}
								}
							}
						}
					}

					return true;
				}

				const container& cont() const noexcept
				{
					return cont_;
				}

				size_type cast(size_type pos, bool disp_order) const
				{
					check_range(pos, cont_.size());

					size_type order = 0; //order for display position
					for (auto & m : cont_)
					{
						if (!m.visible_state)
							continue;

						if (disp_order)
						{
							if (0 == pos)
								return m.index;
							--pos;
						}
						else
						{
							if (m.index == pos)
								return order;
							++order;
						}
					}

					throw std::invalid_argument("listbox: invalid header index");
				}

                /// find and return a ref to the column that originally was at position "pos" previous to any list reorganization.
				column& at(size_type pos, bool disp_order = false)
				{
					check_range(pos, cont_.size());

					//The order of cont_'s elements is the display order.
					if (!disp_order)
					{
						/// It always match the item with pos, otherwise a bug occurs.
						for (auto & m : cont_)
						{
							if (m.index == pos)
								return m;
						}
					}
					
					return cont_[pos];
				}

				const column& at(size_type pos, bool disp_order = false) const
                {
					check_range(pos, cont_.size());

					if (!disp_order)
						pos = this->cast(pos, false);

					return cont_[pos];
                }

				/// Returns the position(original index when it is creating) of the current column at point x
				size_type column_from_point(int x) const noexcept
				{
					for (const auto & col : cont_)
					{
						if (col.visible_state)
						{
							if (x < static_cast<int>(col.width_px))
								return col.index;

							x -= static_cast<int>(col.width_px);
							continue;
						}
					}
					return npos;
				}

				unsigned margin() const
				{
					return margin_;
				}

				std::pair<int, unsigned> range(size_type pos) const
				{
					int left = static_cast<int>(margin_);

					for (auto & m : cont_)
					{
						if (m.index == pos)
							return{left, m.width_px};

						if (m.visible_state)
							left += static_cast<int>(m.width_px);
					}

					return{ left, 0 };
				}

				/// return the original index of the visible col currently before(in front of) or after the col originally at index "index"
				size_type next(size_type index) const noexcept
				{
					bool found_me = false;
					for(auto i = cont_.cbegin(); i != cont_.cend(); ++i)  // in current order
					{
						if (!found_me)
						{
							if (i->index == index)
								found_me = true;
						}
						else if(i->visible_state)
							return i->index;
					}
					return npos;
				}

				/// Returns the absolute position of the first/last visible column.
				size_type boundary(bool get_front) const noexcept
				{
					size_type pos = npos;
					for (const auto & m : cont_)
					{
						if (m.visible_state)
						{
							if (get_front)
								return m.index;
							else
								pos = m.index;
						}
					}

					return pos;
				}
                

				/// move col to view pos
				void move_to_view_pos (size_type col, size_type view, bool front) noexcept
				{
					if (!front) view++;
					if (view >= cont_.size() )		return;

					auto i = std::find_if(   cont_.begin(),
							              cont_.end(),
							              [&](const column& c){return col==c.index;});

					if (i==cont_.end()) return;

					auto col_from = *i;
					cont_.erase(i);
					cont_.insert(cont_.begin()+ view, col_from);

				}
				/// move the col originally at "from" to the position currently in front (or after) the col originally at index "to" invalidating some current index
				void move(size_type from, size_type to, bool front) noexcept
				{
					if ((from == to) || (from >= cont_.size()) || (to >= cont_.size()))
						return;
					
#ifdef _MSC_VER
					for (auto i = cont_.cbegin(); i != cont_.cend(); ++i)
#else
					for (auto i = cont_.begin(); i != cont_.end(); ++i)
#endif
					{
						if (from == i->index)
						{
							auto col_from = *i;
							cont_.erase(i);

							//A workaround for old libstdc++, that some operations of vector
							//don't accept const iterator.
#ifdef _MSC_VER
							for (auto u = cont_.cbegin(); u != cont_.cend(); ++u)
#else
							for (auto u = cont_.begin(); u != cont_.end(); ++u)
#endif
							{
								if (to == u->index)
								{
									cont_.insert(front ? u : ++u, col_from);
									return;
								}
							}
							return;
						}
					}
				}
			private:
				attributes attrib_;
				unsigned	margin_{ 5 };
				container cont_;
			};


			struct item_data
			{
				using container = std::vector<cell>;

				std::unique_ptr<container> cells;
				nana::color bgcolor;
				nana::color fgcolor;
				paint::image img;
				nana::size img_show_size;

				struct inner_flags
				{
					bool selected	:1;
					bool checked	:1;
				}flags;

				mutable std::unique_ptr<nana::any> anyobj;

				item_data() noexcept
				{
					flags.selected = flags.checked = false;
				}

				item_data(const item_data& r)
					:	cells(r.cells ? std::make_unique<container>(*r.cells) : nullptr),
						bgcolor(r.bgcolor),
						fgcolor(r.fgcolor),
						img(r.img),
						flags(r.flags),
						anyobj(r.anyobj ? new nana::any(*r.anyobj) : nullptr)
				{}

				item_data(container&& cont)
					: cells(std::make_unique<container>(std::move(cont)))
				{
					flags.selected = flags.checked = false;
				}

				item_data(std::string&& s)
					: cells(std::make_unique<container>())
				{
					flags.selected = flags.checked = false;
					cells->emplace_back(std::move(s));
				}

				item_data& operator=(const item_data& r)
				{
					if (this != &r)
					{
						if (r.cells)
							cells = std::make_unique<container>(*r.cells);

						flags = r.flags;
						anyobj.reset(r.anyobj ? new nana::any(*r.anyobj) : nullptr);
						bgcolor = r.bgcolor;
						fgcolor = r.fgcolor;
						img = r.img;
					}
					return *this;
				}

				std::string to_string(const export_options& exp_opt, const std::vector<cell>* model_cells) const
				{
					std::string item_str;

					bool ignore_first = true;

					for (auto col : exp_opt.columns_order)
					{
						if (ignore_first)
							ignore_first = false;
						else
							item_str += exp_opt.sep;

						//Use the model cells instead if model cells is available
						item_str += (model_cells ? model_cells : cells.get())->operator[](col).text;
					}

                    return item_str;
				}
			};

			class inline_indicator;

			struct category_t
			{
				using container = std::deque<item_data>;

				native_string_type text;
				std::vector<std::size_t> sorted;
				container items;

				std::unique_ptr<model_interface> model_ptr;

				bool expand{ true };
				bool display_number{ true };

				//A cat may have a key object to identify the category
				std::shared_ptr<nana::detail::key_interface> key_ptr;

				std::deque<pat::cloneable<pat::abstract_factory<inline_notifier_interface>>> factories;
				std::deque<std::unique_ptr<inline_indicator>> indicators;

				category_t(native_string_type str = {}) noexcept
					:text(std::move(str))
				{}

				bool selected() const noexcept
				{
					for (auto & m : items)
					{
						if (false == m.flags.selected)
							return false;
					}
					return !items.empty();
				}

				void make_sort_order()
				{
					sorted.clear();
					for (std::size_t i = 0; i < items.size(); ++i)
						sorted.push_back(i);
				}
				
				std::vector<cell> cells(size_type pos) const
				{
					if (model_ptr)
						return model_ptr->container()->to_cells(pos);

					return *(items.at(pos).cells);
				}
			};

			struct inline_pane
			{
				::nana::panel<false> pane_bottom;	//pane for pane_widget
				::nana::panel<false> pane_widget;	//pane for placing user-define widget
				std::unique_ptr<inline_notifier_interface> inline_ptr;
				inline_indicator * indicator;
				index_pair	item_pos;				//The item index of the inline widget
				std::size_t	column_pos;
			};

			enum class view_action
			{
				auto_view,
				top_view,
				bottom_view,
			};

			class es_lister
			{
			public:
				using container = std::list<category_t>;
				using item_type = item_data;

				std::function<std::function<bool(const ::std::string&, ::nana::any*,
								const ::std::string&, ::nana::any*, bool reverse)>(std::size_t) > fetch_ordering_comparer;

				struct sort_attributes
				{
					std::size_t	column;		///< The position of the column to be sorted
					bool		resort;
					bool		reverse;	
				};

				es_lister()
				{
					//#0 is a default category
					categories_.emplace_back();

					sort_attrs_.column = npos;
					sort_attrs_.resort = true;
					sort_attrs_.reverse = false;
				}

				void bind(essence* ess, widget& wd) noexcept
				{
					ess_ = ess;
					widget_ = dynamic_cast<nana::listbox*>(&wd);
				}

				nana::listbox* wd_ptr() const noexcept
				{
					return widget_;
				}

				nana::any * anyobj(const index_pair& id, bool allocate_if_empty) const
				{
					auto& catobj = *get(id.cat);
					if(id.item < catobj.items.size())
					{
						auto& item = catobj.items[id.item];

						if(item.anyobj)
							return item.anyobj.get();

						if (allocate_if_empty)
						{
							item.anyobj.reset(new ::nana::any);
							return item.anyobj.get();
						}
					}
					return nullptr;
				}

                std::string to_string(const export_options& exp_opt) const;

				void emit_cs(const index_pair& pos, bool for_selection)
				{
					item_proxy item(ess_, pos);
					arg_listbox arg{ item };

					auto & events = wd_ptr()->events();
					
					if (for_selection)
						events.selected.emit(arg, wd_ptr()->handle());
					else
						events.checked.emit(arg, wd_ptr()->handle());

					//notify the inline pane. An item may have multiple panes, each pane is for a column.
					for (auto p : active_panes_)
					{
						if (p && (p->item_pos == pos))
						{
							if (for_selection)
								p->inline_ptr->notify_status(inline_widget_status::selecting, item.selected());
							else
								p->inline_ptr->notify_status(inline_widget_status::checking, item.checked());
						}
					}
				}

				// Definition is provided after struct essence
				unsigned column_content_pixels(size_type pos) const;

				const sort_attributes& sort_attrs() const noexcept
				{
					return sort_attrs_;
				}

                /// each sort() invalidates any existing reference from display position to absolute item, that is after sort() display offset point to different items
                void sort()
				{
					if((npos == sort_attrs_.column) || (!sort_attrs_.resort))
						return;

					auto weak_ordering_comp = fetch_ordering_comparer(sort_attrs_.column);
					if(weak_ordering_comp)
					{
						for (auto & cat : categories_)
						{
							const bool use_model = (cat.model_ptr != nullptr);

							std::stable_sort(cat.sorted.begin(), cat.sorted.end(), [&cat, &weak_ordering_comp, use_model, this](std::size_t x, std::size_t y){
								//The predicate must be a strict weak ordering.
								//!comp(x, y) != comp(x, y)
								if (use_model)
								{
									auto & mx = cat.items[x];
									auto & my = cat.items[y];

									auto mx_cells = cat.model_ptr->container()->to_cells(x);
									auto my_cells = cat.model_ptr->container()->to_cells(y);

									if (mx_cells.size() <= sort_attrs_.column || my_cells.size() <= sort_attrs_.column)
									{
										std::string a;
										if (mx_cells.size() > sort_attrs_.column)
											a = mx_cells[sort_attrs_.column].text;

										std::string b;
										if (my_cells.size() > sort_attrs_.column)
											b = my_cells[sort_attrs_.column].text;

										return weak_ordering_comp(a, mx.anyobj.get(), b, my.anyobj.get(), sort_attrs_.reverse);
									}

									return weak_ordering_comp(mx_cells[sort_attrs_.column].text, mx.anyobj.get(), my_cells[sort_attrs_.column].text, my.anyobj.get(), sort_attrs_.reverse);
								}
								
								auto & mx = cat.items[x];
								auto & my = cat.items[y];

								if (mx.cells->size() <= sort_attrs_.column || my.cells->size() <= sort_attrs_.column)
								{
									std::string a;
									if (mx.cells->size() > sort_attrs_.column)
										a = (*mx.cells)[sort_attrs_.column].text;

									std::string b;
									if (my.cells->size() > sort_attrs_.column)
										b = (*my.cells)[sort_attrs_.column].text;

									return weak_ordering_comp(a, mx.anyobj.get(), b, my.anyobj.get(), sort_attrs_.reverse);
								}

								return weak_ordering_comp((*mx.cells)[sort_attrs_.column].text, mx.anyobj.get(), (*my.cells)[sort_attrs_.column].text, my.anyobj.get(), sort_attrs_.reverse);
							});
						}
					}
					else
					{	//No user-defined comparer is provided, and default comparer is applying.
						for (auto & cat : categories_)
						{
							const bool use_model = (cat.model_ptr != nullptr);

							std::stable_sort(cat.sorted.begin(), cat.sorted.end(), [this, &cat, use_model](std::size_t x, std::size_t y){
								//The predicate must be a strict weak ordering.
								//!comp(x, y) != comp(x, y)
								if (use_model)
								{
									auto mx_cells = cat.model_ptr->container()->to_cells(x);
									auto my_cells = cat.model_ptr->container()->to_cells(y);

									if (mx_cells.size() <= sort_attrs_.column || my_cells.size() <= sort_attrs_.column)
									{
										std::string a;
										if (mx_cells.size() > sort_attrs_.column)
											a = mx_cells[sort_attrs_.column].text;

										std::string b;
										if (my_cells.size() > sort_attrs_.column)
											b = my_cells[sort_attrs_.column].text;

										return (sort_attrs_.reverse ? a > b : a < b);
									}

									auto & a = mx_cells[sort_attrs_.column].text;
									auto & b = my_cells[sort_attrs_.column].text;
									return (sort_attrs_.reverse ? a > b : a < b);
								}

								auto & mx = cat.items[x];
								auto & my = cat.items[y];

								if (mx.cells->size() <= sort_attrs_.column || my.cells->size() <= sort_attrs_.column)
								{
									std::string a;
									if (mx.cells->size() > sort_attrs_.column)
										a = (*mx.cells)[sort_attrs_.column].text;

									std::string b;
									if (my.cells->size() > sort_attrs_.column)
										b = (*my.cells)[sort_attrs_.column].text;

									return (sort_attrs_.reverse ? a > b : a < b);
								}

								auto & a = (*mx.cells)[sort_attrs_.column].text;
								auto & b = (*my.cells)[sort_attrs_.column].text;
								return (sort_attrs_.reverse ? a > b : a < b);
							});
						}
					}
				}

				/// Sorts the specified column
				/**
				 * It sorts the specified column and invalidates all existing item reference from display position to absolute position.
				 * The side effect of this method is that all display positions point to different absolute positions.
				 * @param pos The position of the specified column.
				 * @param reverse A pointer to a boolean which indicates whether to reverse sort. If this parameter is nullptr, the sort is negated to the current reverse state.
				 * @return true if the column is sorted, false otherwise.
				 */
				bool sort_column(std::size_t pos, const bool * reverse)
				{
					if (nana::npos == pos)
					{
						sort_attrs_.column = npos;
						return false;
					}

					if (reverse)
					{
						if (pos != sort_attrs_.column || *reverse != sort_attrs_.reverse)
						{
							sort_attrs_.column = pos;
							sort_attrs_.reverse = *reverse;

							sort();
						}
					}
					else
					{
						if (pos != sort_attrs_.column)
						{
							sort_attrs_.column = pos;
							sort_attrs_.reverse = false;
						}
						else
							sort_attrs_.reverse = !sort_attrs_.reverse;

						sort();
					}
					return true;
				}

				bool active_sort(bool resort) noexcept
				{
					bool prstatus = sort_attrs_.resort;
					sort_attrs_.resort = resort;
					return prstatus;
				}

				/// Scroll the selected item into the view
				void scroll_into_view(const index_pair& abs_pos, view_action vw_act);


		        /// will use the key to insert new cat before the first cat with compare less than the key, or at the end of the list of cat and return a ref to that new cat.  ?
				category_t* create_category(std::shared_ptr<nana::detail::key_interface>& ptr)
				{
					//A workaround for old version of libstdc++
					//Some operations of vector provided by libstdc++ don't accept const iterator.
#ifdef _MSC_VER
					for (auto i = categories_.cbegin(); i != categories_.cend(); ++i)
#else
					for (auto i = categories_.begin(); i != categories_.end(); ++i)
#endif
					{
						if (i->key_ptr)
						{
							if (!i->key_ptr->same_type(ptr.get()))
							{
								this->ordered_categories_ = false;
								break;
							}
							else if (ptr->compare(i->key_ptr.get()))
							{
								auto & catobj = *categories_.emplace(i);
								catobj.key_ptr = ptr;
								return &catobj;
							}
						}
					}

#ifdef _nana_std_has_emplace_return_type
					auto & last_cat = categories_.emplace_back();
					last_cat.key_ptr = ptr;
					return &last_cat;
#else
					categories_.emplace_back();
					categories_.back().key_ptr = ptr;
					return &(categories_.back());
#endif
				}
                
				/// Inserts a new category at position specified by pos
				category_t* create_category(native_string_type&& text, std::size_t pos = nana::npos)
				{
					if (::nana::npos == pos)
					{
#ifdef _nana_std_has_emplace_return_type
						return &categories_.emplace_back(std::move(text));
#else
						categories_.emplace_back(std::move(text));
						return &categories_.back();
#endif
					}

					return &(*categories_.emplace(this->get(pos), std::move(text)));
				}

				/// Insert  before item in absolute "pos" a new item with "text" in column 0, and place it in last display position of this cat
				void insert(const index_pair& pos, std::string&& text, const std::size_t columns)
				{
					auto & catobj = *get(pos.cat);

					const auto item_count = catobj.items.size();

					check_range(pos.item, item_count);

					catobj.sorted.push_back(item_count);

					if (catobj.model_ptr)
					{
						throw_if_immutable_model(catobj.model_ptr.get());
						auto container = catobj.model_ptr->container();
						std::size_t item_index;
						//
						if (pos.item < item_count)
						{
							catobj.items.emplace(catobj.items.begin() + pos.item);
							container->emplace(pos.item);
							item_index = pos.item;
						}
						else
						{
							item_index = container->size();
							catobj.items.emplace_back();
							container->emplace_back();
						}

						std::vector<cell> cells;
						cells.emplace_back(std::move(text));
						cells.resize(columns);
						container->assign(item_index, cells);

						return;
					}

					catobj.items.emplace(catobj.items.begin() + (pos.item < item_count ? pos.item : item_count), std::move(text));
				}

				/// Converts an index between display position and absolute real position.
				/**
				 * @param from An index to be converted
				 * @param from_display_order If this parameter is true, the method convert a display position to the absolute position. If the parameter
				 *							is false, the method converts an absolute position to the display position.
				 * @return A display position or absolute position that are depending on from_display_order.
				 */
				index_pair index_cast(const index_pair& from, bool from_display_order) const
				{
					auto target = index_cast_noexcept(from, from_display_order);
					if (target.empty())
						throw std::out_of_range("listbox: invalid element position");
					return target;
				}
				
				index_pair index_cast_noexcept(const index_pair& from, bool from_display_order, const index_pair& default_value = index_pair{npos, npos}) const noexcept
				{
					if (from.cat < categories_.size())
					{
						auto i = categories_.cbegin();
						std::advance(i, from.cat);

						auto & cat = *i;
						if (from.item < cat.sorted.size())
						{
							if (from_display_order)
								return index_pair{ from.cat, static_cast<size_type>(cat.sorted[from.item]) };

							for (size_type i = 0; i < cat.sorted.size(); ++i)
							{
								if (from.item == cat.sorted[i])
									return index_pair{ from.cat, i };
							}
						}
					}
					return default_value;
				}

				static void throw_if_immutable_model(model_interface* model)
				{
					if (model && model->container()->immutable())
					{
						//Precondition check for the insert/erase operation, it throws if the model is immutable
						throw std::runtime_error("nana::listbox disallow to insert/remove items because of immutable model");
					}
				}

				void throw_if_immutable_model(const index_pair& pos) const
				{
					if (pos.cat < categories_.size())
					{
						auto i = categories_.cbegin();
						std::advance(i, pos.cat);

						throw_if_immutable_model(i->model_ptr.get());
					}
				}

				void assign_model(const index_pair& pos, const std::vector<cell>& cells)
				{
					if (pos.cat < categories_.size())
					{
						auto i = categories_.cbegin();
						std::advance(i, pos.cat);
						if (i->model_ptr)
						{
							throw_if_immutable_model(i->model_ptr.get());
							i->model_ptr->container()->assign(pos.item, cells);
						}
					}
				}

				bool has_model(const index_pair& pos) const
				{
					return (get(pos.cat)->model_ptr != nullptr);
				}

				category_t::container::value_type& at_abs(const index_pair& pos)
				{
					return get(pos.cat)->items.at(pos.item);
				}

				std::vector<cell> at_model_abs(const index_pair& pos) const
				{
					auto model_ptr = get(pos.cat)->model_ptr.get();

					model_lock_guard lock(model_ptr);
					if (model_ptr)
						return model_ptr->container()->to_cells(pos.item);

					return{};
				}

				/// return a ref to the real item object at display position
				category_t::container::value_type& at(const index_pair& pos)
				{
					return get(pos.cat)->items.at(index_cast(pos, true).item);
				}

				const category_t::container::value_type& at(const index_pair& pos) const
				{
					return get(pos.cat)->items.at(index_cast(pos, true).item);
				}

				std::vector<cell> at_model(const index_pair& pos) const
				{
					auto model_ptr = get(pos.cat)->model_ptr.get();
					if (!model_ptr)
						return{};

					model_lock_guard lock(model_ptr);

					auto acc_pos = pos.item;
					if (npos != sort_attrs_.column)
						acc_pos = index_cast(pos, true).item;	//convert display position to absolute position

					return model_ptr->container()->to_cells(acc_pos);
				}

				void append_active_panes(inline_pane* p)
				{
					if (nullptr == p)
						active_panes_.clear();
					else
						active_panes_.push_back(p);
				}

				// Removes all items of a specified category
				// It throws when the category is out of range or has an immutable model.
				void clear(size_type cat)
				{
					auto& catobj = *get(cat);

					model_lock_guard lock(catobj.model_ptr.get());
					if (catobj.model_ptr)
					{
						//The immutable modal can't be cleared.
						throw_if_immutable_model(catobj.model_ptr.get());

						catobj.model_ptr->container()->clear();
					}

					catobj.items.clear();
					catobj.sorted.clear();
				}

                // Clears all items in all cat, but not the container of cat self.
				void clear()
				{
					// Check whether there is a immutable model before performing clear.
					for (auto & cat : categories_)
						throw_if_immutable_model(cat.model_ptr.get());

					for (size_type i = 0; i < categories_.size(); ++i)
						clear(i);
				}

				index_pair advance(const index_pair& pos, int n) const
				{
					const auto cat_size = categories_.size();
					index_pair dpos{ npos, npos };
		
					if (pos.cat >= cat_size || (pos.item != npos && pos.item >= size_item(pos.cat)))
						return dpos;

					if ((0 == pos.cat && npos == pos.item) || (!expand(pos.cat) && (npos != pos.item)))
						return dpos;

					if (0 == n)
						return pos;

					dpos = pos;
					if (0 < n)
					{
						//Forward
						std::size_t index = (npos == pos.item ? 0 : pos.item + 1);

						while (n)
						{
							std::size_t end = 1;
							if (expand(dpos.cat))
								end += size_item(dpos.cat);

							if (n < static_cast<int>(end - index))
								return index_pair{ dpos.cat, index + n - 1 };

							++dpos.cat;
							if (cat_size == dpos.cat)
								return index_pair{ npos, npos };

							n -= static_cast<int>(end - index);
							index = 0;
						}
						return index_pair{ dpos.cat, npos };
					}
					
					//Backward
					n = -n;
					dpos = pos;
					if (good(dpos.cat))
					{
						auto count = static_cast<int>(dpos.is_category() ? 1 : pos.item + 2);
						auto i = get(pos.cat);
						while (true)
						{
							if (count > n)
							{
								count -= n;
								dpos.item = (count == 1 ? npos : count - 2);
								return dpos;
							}

							n -= count;

							if (i == categories_.cbegin())
								break;

							--i;
							--dpos.cat;
							count = static_cast<int>(i->expand ? i->items.size() : 0) + 1;
						}
					}
					return index_pair{npos, npos};
				}

                /// change to index arg
				size_type distance(index_pair from, index_pair to) const
				{
					if(from  == to ) return 0;

					if(to.cat == from.cat)
					{
						if(from.item > to.item && from.item != npos)
							std::swap(from, to);

						return (from.item == npos ? to.item + 1 : to.item - from.item);
					}
					else if(to.cat < from.cat)
						std::swap(from, to);

					std::size_t count = 1;
					for (auto i = get(from.cat); i != get(to.cat); ++i)
					{
						if (i->expand)
							count += i->items.size() + 1;
						else
							++count;
					}

					if (npos != to.item)
						count += (1 + to.item);

					if (npos != from.item)
						count -= (1 + from.item);
					else if (0 == from.cat)
						--count;

					return count - 1;
				}

				void text(category_t* cat, size_type pos, size_type abs_col, cell&& cl, size_type columns)
				{
					if ((abs_col < columns) && (pos < cat->items.size()))
					{
						std::vector<cell> model_cells;

						model_lock_guard lock(cat->model_ptr.get());
						if (cat->model_ptr)
						{
							throw_if_immutable_model(cat->model_ptr.get());
							model_cells = cat->model_ptr->container()->to_cells(pos);
						}

						auto & cells = (cat->model_ptr ? model_cells : *(cat->items[pos].cells));

						if (abs_col < cells.size())
						{
							cells[abs_col] = std::move(cl);
							if (sort_attrs_.column == abs_col)
								sort();
						}
						else
						{	//If the index of specified sub item is over the number of sub items that item contained,
							//it fills the non-exist items.
							cells.resize(abs_col);
							cells.emplace_back(std::move(cl));
						}

						if (cat->model_ptr)
							cat->model_ptr->container()->assign(pos, model_cells);
					}
				}

				void text(category_t* cat, size_type pos, size_type abs_col, std::string&& str, size_type columns)
				{
					if ((abs_col < columns) && (pos < cat->items.size()))
					{
						std::vector<cell> model_cells;

						model_lock_guard lock(cat->model_ptr.get());
						if (cat->model_ptr)
						{
							throw_if_immutable_model(cat->model_ptr.get());
							model_cells = cat->model_ptr->container()->to_cells(pos);
						}

						auto & cells = (cat->model_ptr ? model_cells : *(cat->items[pos].cells));

						if (abs_col < cells.size())
						{
							cells[abs_col].text = std::move(str);
							if (sort_attrs_.column == abs_col)
								sort();
						}
						else
						{	//If the index of specified sub item is over the number of sub items that item contained,
							//it fills the non-exist items.
							cells.resize(abs_col);
							cells.emplace_back(std::move(str));
						}

						if (cat->model_ptr)
							cat->model_ptr->container()->assign(pos, model_cells);
					}
				}

				void erase(const index_pair& pos);

				void erase(size_type cat)
				{
					auto i = get(cat);

					//If the category is the first one, it just clears the items instead of removing whole category.
					if(0 == cat)
					{
						if (i->model_ptr)
						{
							throw_if_immutable_model(i->model_ptr.get());
							i->model_ptr->container()->clear();
						}

						i->items.clear();
						i->sorted.clear();
					}
					else
						categories_.erase(i);
				}

				void erase()
				{
					//Do not remove the first category.

					this->erase(0);

					if (categories_.size() > 1)
					{
						//A workaround for old version of libstdc++
						//Some operations of vector provided by libstdc++ don't accept const iterator.
#ifdef _MSC_VER
						categories_.erase(++categories_.cbegin(), categories_.cend());
#else
						categories_.erase(++categories_.begin(), categories_.end());
#endif
					}
				}

				bool expand(size_type cat, bool exp) noexcept
				{
					//It is allowed to expand the 1st category.
					if(good(cat) && (cat || exp))
					{
						auto & expanded = get(cat)->expand;
						if(expanded != exp)
						{
							expanded = exp;
							return true;
						}
					}
					return false;
				}

				bool expand(size_type cat) const noexcept
				{
					return (good(cat) ? get(cat)->expand : false);
				}

				container& cat_container() noexcept
				{
					return categories_;
				}

				const container& cat_container() const noexcept
				{
					return categories_;
				}

				//Enable/Disable the ordered categories
				bool enable_ordered(bool enb) noexcept
				{
					if (ordered_categories_ != enb)
					{
						if (enb)
						{
							::nana::detail::key_interface * refkey = nullptr;

							for (auto & cat : categories_)
							{
								if (!cat.key_ptr)
									continue;

								if (refkey)
								{
									if (!cat.key_ptr->same_type(refkey))
										return false;
								}
								else
									refkey = cat.key_ptr.get();
							}
						}

						ordered_categories_ = enb;
					}
					return true;
				}

				bool enable_ordered() const noexcept
				{
					return ordered_categories_;
				}

				size_type the_number_of_expanded() const noexcept
				{
					size_type n = categories_.size() - 1;
					for (auto & i : categories_)
					{
						if(i.expand)
							n += i.items.size();
					}
					return n;
				}

				/// Finds a good item or category if an item specified by pos is invalid
				index_pair find_next_good(index_pair pos, bool ignore_category) const noexcept
				{
					//Return the pos if it is good
					if (this->good(pos))
						return pos;
					
					while(pos.cat < this->categories_.size())
					{
						if ((pos.npos == pos.item) && !ignore_category)
							return pos;

						auto cat_item_size = this->get(pos.cat)->items.size();

						if (pos.item < cat_item_size)
							return pos;

						if ((pos.npos == pos.item) && (cat_item_size > 0) && ignore_category)
							return index_pair{ pos.cat, 0 };

						++pos.cat;
						pos.item = pos.npos;
					}
					return index_pair{};
				}

				template<typename Pred>
				std::vector<std::pair<index_pair, bool>> select_display_range_if(index_pair fr_abs, index_pair to_dpl, bool deselect_others, Pred pred)
				{
					fr_abs = find_next_good(fr_abs, true);
					
					if (to_dpl.empty())
					{
						if (fr_abs.empty())
							return{};

						to_dpl = this->last();
					}
					

					auto fr_dpl = (fr_abs.is_category() ? fr_abs : this->index_cast(fr_abs, false));	//Converts an absolute position to display position
                    if (fr_dpl > to_dpl)
						std::swap(fr_dpl, to_dpl);

					if (to_dpl.is_category() && this->size_item(to_dpl.cat) > 0)
						to_dpl.item = this->size_item(to_dpl.cat) - 1;

					const index_pairs already_selected = (deselect_others ? this->pick_items(true) : index_pairs{});

					const auto begin = fr_dpl;
					const auto last = to_dpl;

					//pair first: index in the range of [begin, last]
					//pair second: indicates whether the index is selected before selection.
					std::vector<std::pair<index_pair, bool>> pairs;

					for (; !(fr_dpl > to_dpl); fr_dpl = advance(fr_dpl, 1))	//fr_dpl <= to_dpl
					{
						if (fr_dpl.is_category())
						{
							if (!expand(fr_dpl.cat))
							{
								auto size = size_item(fr_dpl.cat);
								for (std::size_t i = 0; i < size; ++i)
								{
									index_pair abs_pos{ fr_dpl.cat, i };
									item_proxy m{ ess_, abs_pos };
									pairs.emplace_back(abs_pos, m.selected());

									if (pred(abs_pos))
										m.select(true);
								}

								if (fr_dpl.cat == to_dpl.cat)
									break;
							}
						}
						else
						{
							auto abs_pos = index_cast(fr_dpl, true);	//convert display position to absolute position
							item_proxy m{ ess_, abs_pos };
							pairs.emplace_back(abs_pos, m.selected());

							if (pred(abs_pos))
								m.select(true);
						}
					}

					if (deselect_others)
					{
						//Deselects the already selected which is out of range [begin, last] 
						for (auto index : already_selected)
						{
							auto disp_order = this->index_cast(index, false);	//converts an absolute position to a display position
							if (begin > disp_order || disp_order > last)
								item_proxy{ ess_, index }.select(false);
						}
					}

					return pairs;
				}

				bool select_for_all(bool sel, const index_pair& except_abs = index_pair{npos, npos})
				{
					bool changed = false;
					index_pair pos;
					for (auto & cat : categories_)
					{
						pos.item = 0;
						for(auto & m : cat.items)
						{
							if (except_abs != pos)
							{
								if (m.flags.selected != sel)
								{
									changed = true;
									m.flags.selected = sel;

									this->emit_cs(pos, true);

									if (m.flags.selected)
										latest_selected_abs = pos;
									else if (latest_selected_abs == pos)
										latest_selected_abs.set_both(npos);		//make empty
								}
							}
							++pos.item;
						}
						++pos.cat;
					}
					return changed;
				}

				/// return absolute positions, no relative to display
				/**
				 * @param for_selection Indicates whether the selected items or checked items to be returned.
				 * @param find_first Indicates whether or not to return the first item which
				 * @param items_status a pointer refers to a bool object to receive the status whether the picked items are all selected or all checked, in contrast to for_selection
				 */
				index_pairs pick_items(bool for_selection, bool find_first = false, bool * items_status = nullptr) const
				{
					index_pairs results;
					index_pair id;

					if (items_status)
						*items_status = true;

					for (auto & cat : categories_)
					{
						id.item = 0;
						for (auto & m : cat.items)
						{
							if (for_selection ? m.flags.selected : m.flags.checked)
							{
								if (items_status && *items_status)
									*items_status = (for_selection ? m.flags.checked : m.flags.selected);

								results.push_back(id);  // absolute positions, no relative to display
								if (find_first)
									return results;
							}
							++id.item;
						}
						++id.cat;
					}
					return results;
				}

                ///<Selects an item besides the current selected item in the display.
                /// we are moving in display, but the selection ocurre in abs position
                void move_select(bool upwards=true, bool unselect_previous=true, bool into_view=false) noexcept;

				struct pred_cancel
				{
					const bool for_selection;

					pred_cancel(bool for_sel) noexcept : for_selection(for_sel) {}

					bool operator()(category_t::container::value_type & m) const noexcept
					{
						return (for_selection ? m.flags.selected : m.flags.checked);
					}
				};

				struct emit_cancel
				{
					es_lister* const self;
					const bool for_selection;

					emit_cancel(es_lister* self, bool for_sel) noexcept : self(self), for_selection(for_sel) {}

					void operator()(category_t::container::value_type& m, const index_pair& item_pos) const
					{
						if (for_selection)
							m.flags.selected = false;
						else
							m.flags.checked = false;

						self->emit_cs(item_pos, for_selection);
					}
				};

				void cancel_others_if_single_enabled(bool for_selection, const index_pair& except)
				{
					if (!(for_selection ? single_selection_ : single_check_))
						return;

					pred_cancel pred{ for_selection };
					emit_cancel do_cancel{ this, for_selection };

					if (for_selection ? single_selection_category_limited_ : single_check_category_limited_)
					{
						auto i = this->get(except.cat);

						std::size_t item_pos = 0;
						for (auto & m : i->items)
						{
							if ((item_pos != except.item) && pred(m))
								do_cancel(m, index_pair{ except.cat, item_pos });

							++item_pos;
						}
					}
					else
					{
						index_pair cancel_pos;
						for (auto & cat : categories_)
						{
							for (auto & m : cat.items)
							{
								if ((cancel_pos != except) && pred(m))
									do_cancel(m, cancel_pos);

								++cancel_pos.item;
							}

							++cancel_pos.cat;
							cancel_pos.item = 0;
						}
					}
				}

				bool single_status(bool for_selection) const noexcept
				{
					return (for_selection ? single_selection_ : single_check_);
				}

				void enable_single(bool for_selection, bool category_limited)
				{
					bool & single = (for_selection ? single_selection_ : single_check_);
					bool & limited = (for_selection ? single_selection_category_limited_ : single_check_category_limited_);

					if (single && (limited == category_limited))
						return;

					single = true;
					limited = category_limited;

					pred_cancel pred{ for_selection };
					emit_cancel cancel{ this, for_selection };

					std::size_t cat_pos = 0;

					bool selected = false;

					for (auto & cat : categories_)
					{
						if ((category_limited) || (!selected))
						{
							bool ignore = true;	//Ignore the first matched item
							std::size_t pos = 0;
							for (auto & m : cat.items)
							{
								if (pred(m))
								{
									selected = true;

									if (ignore)
										ignore = false;
									else
										cancel(m, index_pair{ cat_pos, pos });
								}
								++pos;
							}
							++cat_pos;
						}
						else
						{
							std::size_t skip_cat = 0;
							for (auto & cat : categories_)
							{
								if (skip_cat++ < cat_pos)
									continue;

								std::size_t pos = 0;
								for (auto & m : cat.items)
								{
									if (pred(m))
										cancel(m, index_pair{ cat_pos, pos });
									++pos;
								}
								++cat_pos;
							}
							break;
						}
					}
				}

				void disable_single(bool for_selection) noexcept
				{
					(for_selection ? single_selection_ : single_check_) = false;
				}

				bool is_single_enabled(bool for_selection) const noexcept
				{
					return (for_selection ? single_selection_ : single_check_);
				}

				size_type size_item(size_type cat) const
				{
					return get(cat)->items.size();
				}

				bool cat_status(size_type pos, bool for_selection) const
				{
					auto & items = get(pos)->items;
					for (auto & m : items)
					{
						if ((for_selection ? m.flags.selected : m.flags.checked) == false)
							return false;
					}
					return true;
				}

				bool cat_status(size_type pos, bool for_selection, bool value);

				bool cat_status_reverse(size_type pos, bool for_selection) noexcept
				{
					if (pos < categories_.size())
					{
						return cat_status(pos, !cat_status(pos, for_selection));
					}
					return false;
				}

                /// can be used as the absolute position of the last absolute item, or as the display pos of the last displayed item
                index_pair last() const noexcept
				{
					index_pair i{ categories_.size() - 1, categories_.back().items.size() };

					if (i.cat)
					{
						if (i.item && categories_.back().expand)
							--i.item;
						else
							i.item = npos;
					}
					else
					{
						if (i.item)
							--i.item;
						else
							return index_pair{ npos, npos };
					}

					return i;
				}

                /// can be used as the absolute position of the first absolute item, or as the display pos of the first displayed item
                index_pair first() const noexcept
                {
					auto i = categories_.cbegin();
					if (i->items.size())
						return index_pair{ 0, 0 };

					if (categories_.size() > 1)
						return index_pair{ 1, npos };

					return index_pair{ npos, npos };
                }
				
				bool good(size_type cat) const noexcept
				{
					return (cat < categories_.size());
				}

				bool good(const index_pair& pos) const noexcept
				{
					return ((pos.cat < categories_.size()) && (pos.item < size_item(pos.cat)));
				}

				/// categories iterator
				container::iterator get(size_type pos)
				{
					check_range(pos, categories_.size());

					auto i = categories_.begin();
					std::advance(i, pos);
					return i;
				}

				container::const_iterator get(size_type pos) const
				{
					check_range(pos, categories_.size());

					auto i = categories_.cbegin();
					std::advance(i, pos);
					return i;
				}
			public:
				index_pair latest_selected_abs;	//Stands for the latest selected item that selected by last operation. Invalid if it is empty.
			private:
				essence * ess_{nullptr};
				nana::listbox * widget_{nullptr};

				sort_attributes sort_attrs_;	//Attributes of sort
				container categories_;

				bool	ordered_categories_{false};	///< A switch indicates whether the categories are ordered.
												/// The ordered categories always creates a new category at a proper position(before the first one which is larger than it).

				bool single_selection_{ false };
				bool single_selection_category_limited_{ false };
				bool single_check_{ false };
				bool single_check_category_limited_{ false };

				std::vector<inline_pane*> active_panes_;
			};//end class es_lister

			enum class operation_states
			{
				none,
				msup_deselect
			};

			/// created and live by the trigger, holds data for listbox: the state of the struct does not effect on member functions, therefore all data members are public.
			struct essence
			{
				enum class item_state{normal, highlighted, pressed, grabbed, floated};
				enum class parts{unknown = -1, header, list, list_blank, checker};

				::nana::listbox* listbox_ptr{nullptr};
				::nana::listbox::scheme_type* scheme_ptr{nullptr};
				::nana::paint::graphics *graph{nullptr};
				bool auto_draw{true};
				bool checkable{false};
				bool if_image{false};
				unsigned text_height;

                ::nana::listbox::export_options def_exp_options;

				es_header header;
				es_lister lister;  // we have at least one empty cat. the #0

				item_state ptr_state{ item_state::normal };
				std::pair<parts, std::size_t> pointer_where;	//The 'first' stands for which object, such as header and lister, 'second' stands for item
																//if where == header, 'second' indicates the item
																//if where == lister || where == checker, 'second' indicates the offset to the scroll offset_y which stands for the first item displayed in lister.
																//if where == unknown, 'second' ignored.

				std::unique_ptr<widgets::skeletons::content_view> content_view;

				std::function<void(paint::graphics&, const rectangle&, bool)> ctg_icon_renderer;	///< Renderer for the category icon

				std::function<bool(nana::mouse)> pred_msup_deselect;

				struct operation_rep
				{
					operation_states state{operation_states::none};
					index_pair item;
				}operation;

				struct mouse_selection_part
				{
					bool	started{ false };
					bool	reverse_selection{ false };
					bool	scroll_direction;
					bool	deselect_when_start_to_move;

					point	screen_pos;
					point	begin_position;	///< Logical position to the 
					point	end_position;
					index_pairs already_selected;
					index_pairs selections;

					unsigned scroll_step{ 1 };
					unsigned mouse_move_timestamp{ 0 };

					bool is_already_selected(const index_pair& abs_pos) const noexcept
					{
						return (already_selected.cend() != std::find(already_selected.cbegin(), already_selected.cend(), abs_pos));
					}

					bool is_selected(const index_pair& abs_pos) const noexcept
					{
						return (selections.cend() != std::find(selections.cbegin(), selections.cend(), abs_pos));
					}
				}mouse_selection;


				std::map<pat::detail::abstract_factory_base*, std::deque<std::unique_ptr<inline_pane>>> inline_table, inline_buffered_table;

				essence()
				{
					pointer_where.first = parts::unknown;

					lister.fetch_ordering_comparer = [this](std::size_t pos) -> std::function<bool(const std::string&, nana::any*, const std::string&, nana::any*, bool reverse)>
					{
						return header.at(pos).weak_ordering;
					};
				}

				unsigned suspension_width() const
				{
					return (graph ? graph->text_extent_size(L"...").width : 0);
				}

				bool cs_status(index_pair abs_pos, bool for_selection) const
				{
					if (abs_pos.is_category())
						return lister.cat_status(abs_pos.cat, for_selection);
					
					auto & flags = lister.get(abs_pos.cat)->items.at(abs_pos.item).flags;
					return (for_selection ? flags.selected : flags.checked);
				}

				void resize_disp_area()
				{
					auto head_px = this->header_visible_px();

					auto r = this->content_area();
					r.y += head_px;
					
					if (r.height <= head_px)
						r.height = 0;
					else
						r.height -= head_px;

					this->content_view->disp_area(r, { -1, 0 }, { 1, -static_cast<int>(head_px) }, { 2, head_px });
				}

				size_type column_from_pos(int screen_x) const
				{
					return header.column_from_point(screen_x - content_area().x + content_view->origin().x);
				}

                std::string to_string(const export_options& exp_opt) const
                {
                    return header.to_string(exp_opt) + exp_opt.endl + lister.to_string(exp_opt) ;
                }

				int content_position(const index_pair& pos) const
				{
					return static_cast<int>(lister.distance(lister.first(), pos) * this->item_height());
				}

				index_pair first_display() const noexcept
				{
					return lister.advance(lister.first(), static_cast<int>(content_view->origin().y / item_height()));
				}

				unsigned item_height() const noexcept
				{
					auto px = (std::max)(scheme_ptr->item_height_ex + text_height, unsigned(1));
					content_view->step(px, false);
					return px;
				}

				point coordinate_cast(const point& from, bool from_screen) noexcept
				{
					rectangle orignal;
					if (!rect_lister(orignal))
						return{};

					const auto origin = this->content_view->origin();

					if (from_screen)
						return from - orignal.position() + origin;

					return from + orignal.position() - origin;
				}

				std::pair<int, int> columns_range() const
				{
					rectangle r;
					if (!rect_header(r))
						return{};

					auto origin = content_view->origin();
					return{ r.x - origin.x, r.x - origin.x + static_cast<int>(header.width_px()) };
				}

				void start_mouse_selection(const arg_mouse& arg)
				{
					auto logic_pos = coordinate_cast(arg.pos, true);

					mouse_selection.started = true;
					mouse_selection.begin_position = logic_pos;
					mouse_selection.end_position = logic_pos;
					mouse_selection.deselect_when_start_to_move = true;

					if (arg.ctrl || arg.shift)
					{
						mouse_selection.already_selected = lister.pick_items(true);
						mouse_selection.reverse_selection = arg.ctrl;
					}
					API::set_capture(*listbox_ptr, true);
				}

				void update_mouse_selection(const point& screen_pos)
				{
					//Don't update if it is not started
					if (!mouse_selection.started)
						return;

					// When the button is pressed and start to move the mouse, the listbox should deselect all items.
					// But when ctrl is clicked
					if (mouse_selection.deselect_when_start_to_move)
					{
						mouse_selection.deselect_when_start_to_move = false;
						if (mouse_selection.already_selected.empty())
							lister.select_for_all(false);

						mouse_selection.selections.clear();
					}

					mouse_selection.screen_pos = screen_pos;

					auto logic_pos = coordinate_cast(screen_pos, true);
					auto imd_area = content_view->view_area();

					if (logic_pos.y > mouse_selection.begin_position.y)
					{
						//The top of logic_pos shouldn't be less than the top of imd_area.
						//This is a feature that listbox always shows the first displayed item on the screen as a selected item when
						//move the cursor upwards.
						logic_pos.y = (std::max)(logic_pos.y, coordinate_cast(imd_area.position(), true).y);
					}

					mouse_selection.end_position = logic_pos;

					std::vector<std::pair<index_pair, bool>> selections;

					auto content_x = coordinate_cast({ columns_range().first, 0 }, true).x;
					if ((std::max)(mouse_selection.end_position.x, mouse_selection.begin_position.x) >= content_x &&
						(std::min)(mouse_selection.end_position.x, mouse_selection.begin_position.x) < content_x + static_cast<int>(header.width_px()))
					{
						auto const begin_off = (std::max)((std::min)(mouse_selection.begin_position.y, mouse_selection.end_position.y), 0) / item_height();

						auto begin = lister.advance(lister.first(), begin_off);
						if (!begin.empty())
						{
							if ((mouse_selection.end_position.y < 0) || (lister.distance(lister.first(), begin) == begin_off))
							{
								//The range [begin_off, last_off] is a range of box selection
								auto last_off = (std::max)(mouse_selection.begin_position.y, mouse_selection.end_position.y) / item_height();
								auto last = lister.advance(lister.first(), last_off);

								//Tries to select the items in the box, then returns the items with their previous selected states
								selections = lister.select_display_range_if(begin, last, false, [this](const index_pair& abs_pos) {
									if (mouse_selection.reverse_selection)
									{
										//Deselects the items in the box which has been already selected
										if(mouse_selection.is_already_selected(abs_pos))
										{
											item_proxy{ this, abs_pos }.select(false);
											return false;
										}
									}
									return true;
								});

								for (auto & pair : selections)
								{
									//Continue if the previous state is selected. It indicates the item now is not selected.
									if (pair.second)
										continue;

									//Add the item to selections container.
									if(!mouse_selection.is_selected(pair.first))
										mouse_selection.selections.push_back(pair.first);
								}

								//Deselects the items which are in mouse_selection.selections but not in selections.
								//Eq to mouse_selection.selections = selections
#ifdef _MSC_VER
								for (auto i = mouse_selection.selections.cbegin(); i != mouse_selection.selections.cend();)
#else
								for(auto i = mouse_selection.selections.begin(); i != mouse_selection.selections.end();)
#endif
								{
									auto & selpos = *i;
									if (selections.cend() == std::find_if(selections.cbegin(), selections.cend(), /*pred_mouse_selection{ *i }*/[&selpos](const std::pair<index_pair, bool>& m){
										return (selpos == m.first);
									}))
									{
										item_proxy{ this, selpos }.select(false);
										i = mouse_selection.selections.erase(i);
									}
									else
										++i;
								}
							}
						}
					}

					//Restores an already selected item if it is not in selections.
					for (auto& abs_pos : mouse_selection.already_selected)
					{
						if (selections.cend() == std::find_if(selections.cbegin(), selections.cend(), [abs_pos](const std::pair<index_pair, bool>& rhs) {
							return (abs_pos == rhs.first);
							}))
						{
							item_proxy m{ this, abs_pos };
							if (!m.selected())
							{
								m.select(true);
								//Add the item to selections container.
								if(!mouse_selection.is_selected(abs_pos))
									mouse_selection.selections.push_back(abs_pos);
							}
						}
					}

					//Deselects the item which is not in already_selected and selections but in mouse_selection.selections
					for(auto i = mouse_selection.selections.cbegin(); i != mouse_selection.selections.cend();)
					{
						auto abs_pos = *i;

						bool is_box_selected = (selections.cend() != std::find_if(selections.cbegin(), selections.cend(), [abs_pos](const std::pair<index_pair, bool>& rhs) {
							return (abs_pos == rhs.first);
							}));

						if (is_box_selected || mouse_selection.is_already_selected(abs_pos))
						{
							++i;
							continue;
						}

						item_proxy{ this, abs_pos }.select(false);
						i = mouse_selection.selections.erase(i);
					}
				}

				void stop_mouse_selection() noexcept
				{
					mouse_selection.started = false;
					API::release_capture(*listbox_ptr);
					mouse_selection.begin_position = mouse_selection.end_position;
					mouse_selection.already_selected.clear();
					mouse_selection.selections.clear();
				}

				/// Returns the number of items that are contained on on screen
				/**
				 * @pram with_rest Indicates whether the extra one item which is not completely contained in rest pixels to be included.
				 * @return The number of items
				 */
				size_type count_of_exposed(bool with_rest) const
				{
					auto lister_s = this->content_view->view_area().height;
					return (lister_s / item_height()) + (with_rest && (lister_s % item_height()) ? 1 : 0);
				}

				void update(bool ignore_auto_draw = false) noexcept
				{
					if((auto_draw || ignore_auto_draw) && lister.wd_ptr())
					{
						calc_content_size(false);
						API::refresh_window(lister.wd_ptr()->handle());
					}
				}

				::nana::size calc_content_size(bool try_update = true)
				{
					size ctt_size(
						this->header.width_px() + this->header.margin(),
						static_cast<size::value_type>(this->lister.the_number_of_expanded()) * this->item_height()
					);

					this->content_view->content_size(ctt_size, try_update);

					return ctt_size;
				}

				nana::rectangle checkarea(int x, int y) const noexcept /// move to scheme ?? 16 ?
				{
					return nana::rectangle(x + 4, y + (static_cast<int>(item_height()) - 16) / 2, 16, 16);
				}

				int item_xpos(const nana::rectangle& r) const
				{
					auto seq = ordered_columns(r.width);

					if (seq.empty())
						return 0;

					return header.range(seq.front()).first + r.x - this->content_view->origin().x;
				}

				//Returns the top of the specified item in listbox window.
				int item_window_top(const index_pair& pos) const
				{
					int top = static_cast<int>(this->lister.distance(index_pair{}, pos) * item_height()) - content_view->origin().y;

					rectangle r;
					if (rect_lister(r))
						top += r.y;

					return top;
				}

				std::pair<parts, size_t> where(const nana::point& pos) const noexcept
				{
					std::pair<parts, size_t> new_where{ parts::unknown, npos };

					const auto area = this->content_area();

					if(area.is_hit(pos))
					{   /// we are inside
						auto const origin = content_view->origin();

						if (header.attrib().visible && (pos.y < static_cast<int>(header_visible_px()) + area.y))
						{   /// we are in the header
							new_where.first = parts::header;
							new_where.second = this->column_from_pos(pos.x);
						}
						else if (area.x <= pos.x + origin.x && pos.x + origin.x < area.x + static_cast<int>(header.width_px()))
						{
							// detect if cursor is in the area of header margin
							if (pos.x < area.x - origin.x + static_cast<int>(header.margin()))
								return{ parts::list_blank, npos };

							new_where.first = parts::list;

							auto const item_h = item_height();
							//don't combine the following formula into the (pos.y - area.y - header_visible_px()) / item_h
							new_where.second = ((pos.y - area.y - header_visible_px() + origin.y) / item_h) - (origin.y / item_h);

							if (this->lister.the_number_of_expanded() < new_where.second + 1)
								return{ parts::list_blank, npos };

							if (checkable)
							{
								nana::rectangle r;
								if (rect_lister(r))
								{
									auto top = new_where.second * item_h + header_visible_px();
									if (checkarea(item_xpos(r), static_cast<int>(top)).is_hit(pos))
										new_where.first = parts::checker;
								}
							}
						}
						else
							new_where.first = parts::list_blank;
					}

					return new_where;
				}

				bool calc_where(const point& pos) noexcept
				{
					auto new_where = where(pos);
					if (new_where == pointer_where)
						return false;

					pointer_where = new_where;
					return true;
				}

				void widget_to_header(nana::point& pos) noexcept
				{
					--pos.y;
					pos.x += this->content_view->origin().x - 2;
				}

				void draw_peripheral()
				{
					auto ctt_area = this->content_area();

					if (!API::widget_borderless(*lister.wd_ptr()))
					{
						//Draw Border
						graph->rectangle(false, static_cast<color_rgb>(0x9cb6c5));

						graph->line({ ctt_area.x - 1, ctt_area.y }, { ctt_area.x - 1, ctt_area.bottom() - 1 }, colors::white);
						graph->line({ ctt_area.right(), ctt_area.y }, { ctt_area.right(), ctt_area.bottom() - 1 });
					}

					this->content_view->draw_corner(*graph);
				}

				rectangle content_area() const
				{
					rectangle r{ graph->size() };

					if (!this->listbox_ptr->borderless())
					{
						r.x = 2;
						r.width -= (r.width > 4 ? 4 : r.width);
						
						r.y = 1;
						r.height -= (r.height > 2 ? 2 : r.height);
					}
					return r;
				}

				double header_font_px() const
				{
					unsigned max_font_px = 0;

					paint::graphics graph{ size{ 1, 1 } };
					for (auto & col : this->header.cont())
					{
						if (!col.visible())
							continue;

						graph.typeface(col.typeface());

						unsigned as, ds, ileading;
						graph.text_metrics(as, ds, ileading);

						max_font_px = (std::max)(as + ds, max_font_px);
					}

					if (0 == max_font_px)
					{
						graph.typeface(listbox_ptr->typeface());
						unsigned as, ds, ileading;
						graph.text_metrics(as, ds, ileading);

						max_font_px = (std::max)(as + ds, max_font_px);
					}

					return max_font_px;
				}

				unsigned header_visible_px() const
				{
					if(header.attrib().visible)
						return scheme_ptr->header_padding_top + scheme_ptr->header_padding_bottom + static_cast<unsigned>(header_font_px());

					return 0;
				}

				bool rect_header(nana::rectangle& r) const
				{
					if(header.attrib().visible)
					{
						r = this->content_area();

						r.height = header_visible_px();

						if (lister.wd_ptr()->borderless())
							return !r.empty();

						auto exs = this->content_view->extra_space(false);
						const unsigned ex_width = (exs ? exs - 1 : 0);

						if(r.width > ex_width)
						{
							r.width -= ex_width;
							return true;
						}
					}
					return false;
				}

				bool rect_lister(nana::rectangle& r) const
				{
					auto head_pixels = header_visible_px();

					auto exs_vert = this->content_view->extra_space(false);
					auto exs_horz = this->content_view->extra_space(true);

					unsigned extr_w = (exs_vert ? exs_vert - 1 : 0);
					unsigned extr_h = exs_horz + head_pixels;

					r = this->content_area();

					if (r.width <= extr_w || r.height <= extr_h)
						return false;

					r.y += head_pixels;

					r.width -= extr_w;
					r.height -= extr_h;

					return true;
				}

				std::vector<size_type> ordered_columns(unsigned lister_w) const
				{
					std::vector<size_type> seqs;
					int x = -content_view->origin().x;

					for (const auto& col : header.cont())
					{
						if (!col.visible_state)
							continue;

						x += col.width_px;
						if (x > 0)
						{
							seqs.push_back(col.index);

							if (x >= static_cast<int>(lister_w))
								break;
						}
					}
					return seqs;
				}

				inline_pane * open_inline(pat::abstract_factory<inline_notifier_interface>* factory, inline_indicator* indicator)
				{
					std::unique_ptr<inline_pane> pane_ptr;
					auto i = inline_buffered_table.find(factory);
					if (i != inline_buffered_table.end())
					{
						auto & panes = i->second;
						if (!panes.empty())
						{
							pane_ptr = std::move(panes.front());
							panes.pop_front();
						}
					}

					if (!pane_ptr)
					{
						pane_ptr.reset(new inline_pane);
						pane_ptr->indicator = indicator;
						pane_ptr->pane_bottom.create(this->lister.wd_ptr()->handle());
						pane_ptr->pane_widget.create(pane_ptr->pane_bottom);
						pane_ptr->inline_ptr = factory->create();
						pane_ptr->inline_ptr->create(pane_ptr->pane_widget);
					}

					auto ptr = pane_ptr.get();
					inline_table[factory].emplace_back(std::move(pane_ptr));
					return ptr;
				}
			};

			//definition of iresolver/oresolver
			oresolver::oresolver(essence* ess) noexcept
				: ess_(ess)
			{}

			oresolver& oresolver::operator<<(bool n)
			{
				cells_.emplace_back(std::string(n ? "true" : "false"));
				return *this;
			}
			oresolver& oresolver::operator<<(short n)
			{
				cells_.emplace_back(std::to_string(n));
				return *this;
			}

			oresolver& oresolver::operator<<(unsigned short n)
			{
				cells_.emplace_back(std::to_string(n));
				return *this;
			}

			oresolver& oresolver::operator<<(int n)
			{
				cells_.emplace_back(std::to_string(n));
				return *this;
			}

			oresolver& oresolver::operator<<(unsigned int n)
			{
				cells_.emplace_back(std::to_string(n));
				return *this;
			}

			oresolver& oresolver::operator<<(long n)
			{
				cells_.emplace_back(std::to_string(n));
				return *this;
			}

			oresolver& oresolver::operator<<(unsigned long n)
			{
				cells_.emplace_back(std::to_string(n));
				return *this;
			}
			oresolver& oresolver::operator<<(long long n)
			{
				cells_.emplace_back(std::to_string(n));
				return *this;
			}

			oresolver& oresolver::operator<<(unsigned long long n)
			{
				cells_.emplace_back(std::to_string(n));
				return *this;
			}

			oresolver& oresolver::operator<<(float f)
			{
				cells_.emplace_back(std::to_string(f));
				return *this;
			}

			oresolver& oresolver::operator<<(double f)
			{
				cells_.emplace_back(std::to_string(f));
				return *this;
			}

			oresolver& oresolver::operator<<(long double f)
			{
				cells_.emplace_back(std::to_string(f));
				return *this;
			}

			oresolver& oresolver::operator<<(const char* text)
			{
				cells_.emplace_back(std::string(text));
				return *this;
			}

			oresolver& oresolver::operator<<(const wchar_t* text)
			{
				cells_.emplace_back(to_utf8(text));
				return *this;
			}

			oresolver& oresolver::operator<<(const std::string& text)
			{
				cells_.emplace_back(text);
				return *this;
			}

			oresolver& oresolver::operator<<(const std::wstring& text)
			{
				cells_.emplace_back(to_utf8(text));
				return *this;
			}

			oresolver& oresolver::operator<<(std::wstring&& text)
			{
				cells_.emplace_back(to_utf8(text));
				return *this;
			}

			oresolver& oresolver::operator<<(cell cl)
			{
				cells_.emplace_back(std::move(cl));
				return *this;
			}

			oresolver& oresolver::operator<<(std::nullptr_t)
			{
#ifdef _nana_std_has_emplace_return_type
				cells_.emplace_back().text.assign(1, wchar_t{});
#else
				cells_.emplace_back();
				cells_.back().text.assign(1, wchar_t(0));	//means invalid cell
#endif
				return *this;
			}

			std::vector<cell>&& oresolver::move_cells() noexcept
			{
				return std::move(cells_);
			}

			::nana::listbox& oresolver::listbox() noexcept
			{
				return *ess_->listbox_ptr;
			}

			iresolver& iresolver::operator>>(bool& n)
			{
				if (pos_ < cells_.size())
					n = (std::stoi(cells_[pos_++].text) == 0);
				return *this;
			}

			iresolver& iresolver::operator>>(short& n)
			{
				if (pos_ < cells_.size())
					n = std::stoi(cells_[pos_++].text);
				return *this;
			}

			iresolver& iresolver::operator>>(unsigned short& n)
			{
				if (pos_ < cells_.size())
					n = static_cast<unsigned short>(std::stoul(cells_[pos_++].text));
				return *this;
			}

			iresolver& iresolver::operator>>(int& n)
			{
				if (pos_ < cells_.size())
					n = std::stoi(cells_[pos_++].text);
				return *this;
			}

			iresolver& iresolver::operator>>(unsigned int& n)
			{
				if (pos_ < cells_.size())
					n = std::stoul(cells_[pos_++].text);
				return *this;
			}

			iresolver& iresolver::operator>>(long& n)
			{
				if (pos_ < cells_.size())
					n = std::stol(cells_[pos_++].text);
				return *this;
			}

			iresolver& iresolver::operator>>(unsigned long& n)
			{
				if (pos_ < cells_.size())
					n = std::stoul(cells_[pos_++].text);
				return *this;
			}

			iresolver& iresolver::operator>>(long long& n)
			{
				if (pos_ < cells_.size())
					n = std::stoll(cells_[pos_++].text);
				return *this;
			}
			iresolver& iresolver::operator>>(unsigned long long& n)
			{
				if (pos_ < cells_.size())
					n = std::stoull(cells_[pos_++].text);
				return *this;
			}

			iresolver& iresolver::operator>>(float& f)
			{
				if (pos_ < cells_.size())
					f = std::stof(cells_[pos_++].text);
				return *this;
			}


			iresolver& iresolver::operator>>(double& f)
			{
				if (pos_ < cells_.size())
					f = std::stod(cells_[pos_++].text);
				return *this;
			}

			iresolver& iresolver::operator>>(long double& f)
			{
				if (pos_ < cells_.size())
					f = std::stold(cells_[pos_++].text);
				return *this;
			}

			iresolver& iresolver::operator>>(std::string& text)
			{
				if (pos_ < cells_.size())
					text = cells_[pos_++].text;
				return *this;
			}

			iresolver& iresolver::operator>>(std::wstring& text)
			{
				if (pos_ < cells_.size())
					text = to_wstring(cells_[pos_++].text);

				return *this;
			}

			iresolver::iresolver(std::vector<cell> cl) noexcept
				: cells_(std::move(cl))
			{}

			iresolver& iresolver::operator>>(cell& cl)
			{
				if (pos_ < cells_.size())
					cl = cells_[pos_++];
				return *this;
			}

			iresolver& iresolver::operator>>(std::nullptr_t) noexcept
			{
				++pos_;
				return *this;
			}
			//end class iresolver/oresolver

			unsigned es_lister::column_content_pixels(size_type pos) const
			{
				unsigned max_px = 0;
				
				std::unique_ptr<paint::graphics> graph_helper;
				auto graph = ess_->graph;
				if (graph->empty())
				{
					//Creates a helper if widget graph is empty(when its size is 0).
					graph_helper.reset(new paint::graphics{ nana::size{ 5, 5 } });
					graph_helper->typeface(ess_->graph->typeface());
					graph = graph_helper.get();
				}

				for (auto & cat : categories_)
				{
					for (std::size_t i = 0; i < cat.items.size(); ++i)
					{
						unsigned content_px = 0;
						if (cat.model_ptr)
						{
							auto model_cells = cat.model_ptr->container()->to_cells(i);
							if (pos >= model_cells.size())
								continue;

							content_px = graph->text_extent_size(model_cells[pos].text).width;
						}
						else
						{
							if (pos >= cat.items[i].cells->size())
								continue;

							content_px = graph->text_extent_size((*cat.items[i].cells)[pos].text).width;
						}

						if (content_px > max_px)
							max_px = content_px;
					}
				}
				return max_px;
			}

			//es_header::column member functions
			void es_header::column::_m_refresh() noexcept
			{
				ess_->update(true);
			}

			size_type es_header::column::position(bool disp_order) const noexcept
			{
				return (disp_order ? ess_->header.cast(index, false) : index);
			}

			void es_header::column::fit_content(unsigned maximize) noexcept
			{
				auto content_px = ess_->lister.column_content_pixels(index);

				if (0 == content_px)
					return;

				content_px += (ess_->scheme_ptr->text_margin * 2);	//margin at left/right end.

				if (index == 0 && ess_->checkable)    //   only before the first column (display_order=0 ?)
					content_px += 18;              // add to geom. scheme (width of the checker)  ??

				if (range_width_px.first != range_width_px.second)
				{
					if (range_width_px.first > content_px)
						content_px = range_width_px.first;

					//Use range_width_px defined max if maximize is unspecified
					if (0 == maximize)
						maximize = range_width_px.second;
				}

				if (0 == maximize)
					maximize = ess_->scheme_ptr->max_fit_content;

				//maximize is only available when it > 0
				if (maximize && (content_px > maximize))
					content_px = maximize;

				width_px = content_px;

				_m_refresh();
			}

			/// Sets an exclusive font for the column
			void es_header::column::typeface(const paint::font& column_font)
			{
				this->font.reset(new paint::font{ column_font });

				API::refresh_window(*ess_->listbox_ptr);
			}

			/// Returns a font
			paint::font es_header::column::typeface() const noexcept
			{
				//Returns the exclusive font if it is not empty
				if (this->font && !this->font->empty())
				return *this->font;

				//Returns the column font if it is not empty
				if (ess_->scheme_ptr->column_font && !ess_->scheme_ptr->column_font)
					return *(ess_->scheme_ptr->column_font);

				//If all above fonts are invalid, returns the widget font.
				return ess_->listbox_ptr->typeface();
			}
			//end es_header::column functions

			class inline_indicator
				: public ::nana::detail::inline_widget_indicator<index_pair, std::string>
			{
			public:
				using parts = essence::parts;

				inline_indicator(essence* ess, std::size_t column_pos)
					: ess_{ ess }, column_pos_{column_pos}
				{
				}

				void attach(index_type pos, inline_pane* pane)
				{
					for (auto & pn : panes_)
					{
						if (pn.first == pos)
						{
							pn.second = pane;
							return;
						}
					}
					panes_.emplace_back(std::make_pair(pos, pane));
				}

				void detach() noexcept
				{
					panes_.clear();
				}
			public:
				//Implement inline_widget_indicator
				::nana::widget& host() const override
				{
					return *ess_->lister.wd_ptr();
				}

				std::size_t column() const override
				{
					return column_pos_;
				}

				void modify(index_type pos, const value_type& value) const override
				{
					ess_->lister.throw_if_immutable_model(pos);

					auto model_cells = ess_->lister.at_model_abs(pos);
					auto & cells = ess_->lister.has_model(pos) ? model_cells : (*ess_->lister.at_abs(pos).cells);

					if (cells.size() <= column_pos_)
						cells.resize(column_pos_ + 1);

					if (cells[column_pos_].text != value)
					{
						cells[column_pos_].text = value;

						if (model_cells.size())
							ess_->lister.assign_model(pos, model_cells);

						ess_->update();
					}
				}

				void selected(index_type pos) override
				{
					if (ess_->lister.at(pos).flags.selected)
						return;
					ess_->lister.select_for_all(false);
					cat_proxy(ess_, pos.cat).at(pos.item).select(true);
				}

				void hovered(index_type pos) override
				{
					auto offset = ess_->lister.distance(ess_->first_display(), ess_->lister.index_cast(pos, false));

					if (ess_->pointer_where.first != parts::list || ess_->pointer_where.second != offset)
					{
						ess_->pointer_where.first = parts::list;
						ess_->pointer_where.second = offset;
						ess_->update();
					}
				}
			private:
				essence * const ess_;
				const std::size_t column_pos_;
				std::vector<std::pair<index_type, inline_pane*>> panes_;
			};

			void es_lister::scroll_into_view(const index_pair& abs_pos, view_action vw_act)
			{
				auto& cat = *get(abs_pos.cat);

				if ((abs_pos.item != nana::npos) && (abs_pos.item >= cat.items.size()))
					throw std::invalid_argument("listbox: invalid pos to scroll");

				if (!cat.expand)
				{
					this->expand(abs_pos.cat, true);
					ess_->calc_content_size();
				}
				else if (!ess_->auto_draw)
				{
					//force a update of content size and scrollbar status when auto_draw is false to make
					//sure that the scroll function works fine.
					ess_->calc_content_size();
				}

				auto origin = ess_->content_view->origin();

				auto off = this->distance(this->first(), this->index_cast(abs_pos, false)) * ess_->item_height();

				auto screen_px = ess_->content_view->view_area().height;

				if (view_action::auto_view == vw_act)
				{
					if (static_cast<int>(off) < origin.y)
						vw_act = view_action::top_view;
					else if (static_cast<int>(off) >= static_cast<int>(origin.y + ess_->content_view->view_area().height))
						vw_act = view_action::bottom_view;
					else
						return;
				}

				origin.y = 0;

				if (view_action::bottom_view == vw_act)
				{
					off += ess_->item_height();
					if (off >= screen_px)
						origin.y = static_cast<int>(off - screen_px);
				}
				else
				{
					auto const content_px = ess_->content_view->content_size().height;
					if (content_px - off >= screen_px)
						origin.y = static_cast<int>(off);
					else if (content_px >= screen_px)
						origin.y = static_cast<int>(content_px - screen_px);
				}

				if (ess_->content_view->move_origin(origin - ess_->content_view->origin()))
					ess_->content_view->sync(false);
			}

			void es_lister::erase(const index_pair& pos)
			{
				auto & cat = *get(pos.cat);
				if (pos.item < cat.items.size())
				{
					if (cat.model_ptr)
					{
						throw_if_immutable_model(cat.model_ptr.get());
						cat.model_ptr->container()->erase(pos.item);
					}

					cat.items.erase(cat.items.begin() + pos.item);
					cat.sorted.erase(std::find(cat.sorted.begin(), cat.sorted.end(), cat.items.size()));

					sort();
				}
			}

			void es_lister::move_select(bool upwards, bool unselect_previous, bool into_view) noexcept
			{
				auto next_selected_dpl = index_cast_noexcept(latest_selected_abs, false);	//convert absolute position to display position

				if (next_selected_dpl.empty())  // has no cat ? (cat == npos) => begins from first cat
				{
					bool good = false;
					for (size_type i = 0, size = categories_.size(); i < size; ++i) // run all cat
					{
						if(size_item(i))
						{
							//The first category which contains at least one item.
							next_selected_dpl.cat = i;
							next_selected_dpl.item = 0;
							good = true;
							break;
						}
					}
					if(! good ) return;   // items in listbox : nothing to select (and an empty but visible cat?)
				}

				//start moving
				while(true)
				{
					if(upwards == false)
					{
						if(good(next_selected_dpl.cat))
						{
							if (size_item(next_selected_dpl.cat) > next_selected_dpl.item + 1)
							{
								++next_selected_dpl.item;
							}
							else
							{
								next_selected_dpl.item = 0;
								if (categories_.size() > next_selected_dpl.cat + 1)
									++next_selected_dpl.cat;
								else
									next_selected_dpl.cat = 0;
							}
						}
						else
							next_selected_dpl.set_both(0);
					}
					else
					{
						if (0 == next_selected_dpl.item)
						{
							//there is an item at least definitely, because the start pos is an available item.
							do
							{
								if (0 == next_selected_dpl.cat)
									next_selected_dpl.cat = categories_.size() - 1;
								else
									--next_selected_dpl.cat;

							}while (0 == size_item(next_selected_dpl.cat));

							next_selected_dpl.item = size_item(next_selected_dpl.cat) - 1;
						}
						else
							--next_selected_dpl.item;
					}

					if (good(next_selected_dpl.cat))
					{
						expand(next_selected_dpl.cat, true); // revise expand

						if (good(next_selected_dpl))
						{
							if (unselect_previous && !single_selection_)
								select_for_all(false);

							/// is ignored if no change (maybe set last_selected anyway??), but if change emit event, deselect others if need ans set/unset last_selected
							item_proxy::from_display(ess_, next_selected_dpl).select(true);
						}
						break;
					}
					else break;
				}

				if (into_view && !latest_selected_abs.empty())
					this->scroll_into_view(latest_selected_abs, view_action::auto_view);
			}

			std::string es_lister::to_string(const export_options& exp_opt) const
			{
				std::string list_str;
				bool first{true};
				for(auto & cat: cat_container())
				{
					if(first)
						first=false;
					else
 						list_str += (to_utf8(cat.text) + exp_opt.endl);
	
					std::vector<cell> model_cells;

					auto const pcell = (cat.model_ptr ? &model_cells : nullptr);

					for (auto i : cat.sorted)
					{
						auto& item = cat.items[i];
						if (item.flags.selected || !exp_opt.only_selected_items)
						{
							//Test if the category have a model set.
							if (pcell)
								cat.model_ptr->container()->to_cells(i).swap(model_cells);
							
							list_str += (item.to_string(exp_opt, pcell) + exp_opt.endl);
						}
					}
				}
				return list_str ;
			}

			bool es_lister::cat_status(size_type pos, bool for_selection, bool value)
			{
				bool changed = false;

				if (for_selection)
				{
					cat_proxy cpx{ ess_, pos };
					for (item_proxy &it : cpx)
						it.select(value);

					latest_selected_abs.cat = pos;
					latest_selected_abs.item = npos;

					return true;
				}
				else
				{
					auto & items = get(pos)->items;
					size_type index = 0;
					for (auto & m : items)
					{
						if (m.flags.checked != value)
						{
							m.flags.checked = value;
							this->emit_cs(index_pair{ pos, index }, false);
							changed = true;
						}
						++index;
					}
				}
				return changed;
			}

			class drawer_header_impl
			{
			public:
				using graph_reference = nana::paint::graphics&;
				using item_state = essence::item_state;
				using parts = essence::parts;

				struct column_rendering_parameter
				{
					unsigned margin;
					unsigned height;
					double max_font_px;
					paint::font wdg_font;
				};

				drawer_header_impl(essence* es) noexcept: essence_(es){}

				size_type splitter() const noexcept
				{
					return grabs_.splitter;
				}

				void cancel_splitter() noexcept
				{
					grabs_.splitter = npos;
				}

				// Detects a header spliter, return true if x is in the splitter area after that header item (column)
				bool detect_splitter(int x) noexcept
				{
					nana::rectangle r;
					if (!essence_->rect_header(r))
						return false;

					if(essence_->ptr_state == item_state::highlighted)
					{
						x -= r.x - essence_->content_view->origin().x + static_cast<int>(essence_->header.margin());

						for(auto & col : essence_->header.cont()) // in current order
						{
							if(col.visible_state)
							{
								if ((static_cast<int>(col.width_px) < x + static_cast<int>(essence_->scheme_ptr->header_splitter_area_before))
									&& (x < static_cast<int>(col.width_px) + static_cast<int>(essence_->scheme_ptr->header_splitter_area_after)))
								{
									grabs_.splitter = col.index; // original index
									return true;
								}
								x -= static_cast<int>(col.width_px);
							}
						}
					}
					else if(essence_->ptr_state == item_state::normal)
						grabs_.splitter = npos;
					return false;
				}

				void grab(const nana::point& pos, bool is_grab)
				{
					if(is_grab)
					{
						grabs_.start_pos = pos.x;
						if(grabs_.splitter != npos)  // resize header item, not move it
							grabs_.item_width = essence_->header.at(grabs_.splitter).width_px;
					}
					else if((grab_terminal_.index != npos) && (grab_terminal_.index != essence_->pointer_where.second))
						essence_->header.move(essence_->pointer_where.second, grab_terminal_.index, grab_terminal_.place_front);
				}

				//grab_move
				/// @brief draw when an item is grabbing.
				/// @return true if refresh is needed, false otherwise
				bool grab_move(const nana::point& pos)
				{
					if(npos == grabs_.splitter)
					{  // move column, not resize it
						options_.grab_column = true;
						options_.grab_column_position = pos;
						return true;
					}
					else
					{   // resize column, not move it
						auto& col = essence_->header.at(grabs_.splitter);

						auto delta_px = (grabs_.start_pos - pos.x);

						//Resize the item specified by item_spliter_.
						auto new_w = static_cast<int>(grabs_.item_width) > delta_px ?  grabs_.item_width - delta_px : 0;

						//Check the minimized and maximized value
						if (col.range_width_px.first != col.range_width_px.second)
						{
							//Column ranged width
							new_w = std::clamp(static_cast<int>(new_w), static_cast<int>(col.range_width_px.first), static_cast<int>(col.range_width_px.second));
						}
						else
						{
							//Default scheme
							new_w = (std::max)(new_w, essence_->suspension_width() + essence_->scheme_ptr->min_column_width);
						}

						if(col.width_px != new_w)
						{
							col.width_px = new_w;
							essence_->calc_content_size();
							return true;
						}
					}
					return false;
				}

				void draw(graph_reference graph, const nana::rectangle& r)
				{
					const auto border_color = essence_->scheme_ptr->header_bgcolor.get_color().blend(colors::black, 0.2);

					auto text_color = essence_->scheme_ptr->header_fgcolor.get_color();

					auto state = item_state::normal;
					//check whether grabbing an item, if item_spliter_ != npos, that indicates the grabbed item is a splitter.
					if ((parts::header == essence_->pointer_where.first) && (npos == grabs_.splitter))
						state = essence_->ptr_state;

					rectangle column_r{
						r.x - essence_->content_view->origin().x, r.y,
						0, r.height - 1
					};

					column_rendering_parameter crp;

					//The first item includes the margin
					crp.margin = essence_->header.margin();

					crp.height = essence_->header_visible_px();
					crp.max_font_px = essence_->header_font_px();
					crp.wdg_font = graph.typeface();

					for (auto & col : essence_->header.cont())
					{
						if (col.visible_state)
						{
							column_r.width = col.width_px + crp.margin;

							const auto right_pos = column_r.right();

							//Make sure the column is in the display area.
							if (right_pos > r.x)
							{
								_m_draw_header_item(graph, crp, column_r, text_color, col, (col.index == essence_->pointer_where.second ? state : item_state::normal));
								graph.line({ right_pos - 1, r.y }, { right_pos - 1, r.bottom() - 2 }, border_color);
							}

							crp.margin = 0;

							column_r.x = right_pos;
							if (right_pos > r.right())
								break;
						}
					}

					//If the last rendered column's right point doesn't reach at r.right, fill the spare space.
					if (column_r.x < r.right())
					{
						column_r.width = (r.right() - column_r.x);
						if(API::dev::copy_transparent_background(essence_->listbox_ptr->handle(), column_r, graph, column_r.position()))
							graph.blend(column_r, essence_->scheme_ptr->header_bgcolor, 0.5);
						else
							graph.rectangle(column_r, true, essence_->scheme_ptr->header_bgcolor);
					}

					const int y = r.bottom() - 1;
					graph.line({ r.x, y }, { r.right(), y }, border_color);

					if (options_.grab_column)
					{
						_m_make_float(r, options_.grab_column_position);   // now draw one floating header item

						//Draw the target strip
						grab_terminal_.index = _m_target_strip(options_.grab_column_position.x, r, essence_->pointer_where.second, grab_terminal_.place_front);

						options_.grab_column = false;
					}
				}
			private:
				size_type _m_target_strip(int x, const nana::rectangle& rect, size_type grab, bool& place_front)
				{
					//convert x to header logic coordinate.
					auto const x_offset = essence_->content_view->origin().x;

					x = std::clamp(x, x_offset, x_offset + static_cast<int>(rect.width));

					auto i = essence_->header.column_from_point(x);

					if (i == npos)
						i = essence_->header.boundary(essence_->header.range(grab).first >= x);

					if(grab != i)
					{
						auto item_rg = essence_->header.range(i);

						//Get the item pos
						//if mouse pos is at left of an item middle, the pos of itself otherwise the pos of the next.
						place_front = (x <= (item_rg.first + static_cast<int>(item_rg.second / 2)));
						x = (place_front ? item_rg.first : essence_->header.range(essence_->header.next(i)).first);

						if (npos != i)
						{
							if (place_front && (0 == essence_->header.cast(i, false)))
								x -= static_cast<int>(essence_->header.margin());
							essence_->graph->rectangle({ x - x_offset + rect.x, rect.y, 2, rect.height }, true, colors::red);
						}

						return i;
					}
					return npos;
				}

				void _m_draw_header_item(graph_reference graph, const column_rendering_parameter& crp, const rectangle& column_r, const ::nana::color& fgcolor, const es_header::column& column, item_state state)
				{
					::nana::color bgcolor;

					switch(state)
					{
					case item_state::normal:		bgcolor = essence_->scheme_ptr->header_bgcolor.get_color(); break;
					case item_state::highlighted:	bgcolor = essence_->scheme_ptr->header_bgcolor.get_color().blend(colors::white, 0.5); break;
					case item_state::pressed:
					case item_state::grabbed:		bgcolor = essence_->scheme_ptr->header_grabbed.get_color(); break;
					case item_state::floated:		bgcolor = essence_->scheme_ptr->header_floated.get_color();	break;
					}

					if(API::dev::copy_transparent_background(essence_->listbox_ptr->handle(), column_r, graph, column_r.position()))
					{
						paint::graphics grad_graph{column_r.dimension()};
						grad_graph.gradual_rectangle(rectangle{column_r.dimension()}, bgcolor.blend(colors::white, 0.1), bgcolor.blend(colors::black, 0.1), true);

						graph.blend(column_r, grad_graph, {}, 0.8);
					}
					else
						graph.gradual_rectangle(column_r, bgcolor.blend(colors::white, 0.1), bgcolor.blend(colors::black, 0.1), true);

					paint::aligner text_aligner{ graph, column.alignment, column.alignment };

					auto text_margin = essence_->scheme_ptr->text_margin;

					if (text_margin < column_r.width)
					{
						graph.palette(true, fgcolor);

						//Set column font
						graph.typeface(column.typeface());

						point text_pos{
							column_r.x + static_cast<int>(crp.margin),
							static_cast<int>(essence_->scheme_ptr->header_padding_top)
						};

						if (align::left == column.alignment)
							text_pos.x += text_margin;
						else if (align::center == column.alignment)
							text_margin = 0;

						text_aligner.draw(column.caption, text_pos, column_r.width - text_margin);

						//Restores widget font
						graph.typeface(crp.wdg_font);
					}

					auto & sort = essence_->lister.sort_attrs();

					if (column.index == sort.column)
					{
						facade<element::arrow> arrow("hollow_triangle");
						arrow.direction(sort.reverse ? ::nana::direction::south : ::nana::direction::north);
						arrow.draw(graph, {}, colors::black, { column_r.x + (static_cast<int>(column_r.width) - 16) / 2, -4, 16, 16 }, element_state::normal); // geometric scheme?
					}
				}

				void _m_make_float(const nana::rectangle& rect, const nana::point& pos)
				{
					const auto & col = essence_->header.at(essence_->pointer_where.second);

					column_rendering_parameter crp;
					crp.margin = 0;
					crp.height = essence_->header_visible_px();
					crp.max_font_px = essence_->header_font_px();
					crp.wdg_font = essence_->listbox_ptr->typeface();

					if (&essence_->header.at(0, true) == &col)
						crp.margin = essence_->header.margin();

					paint::graphics fl_graph({ col.width_px + crp.margin, crp.height });

					fl_graph.typeface(essence_->graph->typeface());

					_m_draw_header_item(fl_graph, crp, rectangle{ fl_graph.size()}, colors::white, col, item_state::floated);

					auto xpos = essence_->header.range(col.index).first + pos.x - grabs_.start_pos;
					essence_->graph->blend(rectangle{ point{ xpos - essence_->content_view->origin().x + rect.x, rect.y } , fl_graph.size() }, fl_graph, {}, 0.5);
				}

			private:
				essence * const essence_;

				struct grab_variables
				{
					int start_pos;
					unsigned item_width;

					size_type splitter{ npos };
				}grabs_;

				struct grab_terminal
				{
					size_type index;
					bool place_front;
				}grab_terminal_;

				struct options
				{
					bool	grab_column{ false };
					point	grab_column_position;
				}options_;
			};

			class drawer_lister_impl
			{
			public:
				using item_state = essence::item_state;
				using parts = essence::parts;
				using status_type = inline_notifier_interface::status_type;

				drawer_lister_impl(essence * es) noexcept
					:essence_(es)
				{}

				void draw(const nana::rectangle& visual_r)
				{
                    internal_scope_guard lock;

					//clear active panes
					essence_->lister.append_active_panes(nullptr);

					//The count of items to be drawn
					auto item_count = essence_->count_of_exposed(true);
					if (0 == item_count)
						return;

					auto const self = essence_->lister.wd_ptr();
					auto const bgcolor = self->bgcolor();
					auto const fgcolor = self->fgcolor();

					essence_->graph->palette(false, bgcolor);

					auto const header_w = essence_->header.width_px();
					auto const item_height_px = essence_->item_height();

					auto const origin = essence_->content_view->origin();

					auto const header_margin = essence_->header.margin();
					if (header_w + header_margin < origin.x + visual_r.width)
					{
						rectangle r{ point{ visual_r.x + static_cast<int>(header_w + header_margin) - origin.x, visual_r.y },
							size{ visual_r.width + origin.x - header_w, visual_r.height } };
						
						if (!API::dev::copy_transparent_background(essence_->listbox_ptr->handle(), r, *essence_->graph, r.position()))
							essence_->graph->rectangle(r, true);
					}

					if (header_margin > 0)
					{
						rectangle r = visual_r;
						r.width = header_margin;

						if (!API::dev::copy_transparent_background(essence_->listbox_ptr->handle(), r, *essence_->graph, r.position()))
							essence_->graph->rectangle(r, true);
					}

					es_lister & lister = essence_->lister;

					auto & ptr_where = essence_->pointer_where;

					auto first_disp = essence_->first_display();

					point item_coord{
						essence_->item_xpos(visual_r),
						visual_r.y - static_cast<int>(origin.y % item_height_px)
					};

					essence_->inline_buffered_table.swap(essence_->inline_table);

					// The first display is empty when the listbox is empty.
					if (!first_disp.empty())
					{
						index_pair hoverred_pos(npos, npos);	//the hovered item.

						//if where == lister || where == checker, 'second' indicates the offset to the  relative display-order pos of the scroll offset_y which stands for the first item to be displayed in lister.
						if ((ptr_where.first == parts::list || ptr_where.first == parts::checker) && ptr_where.second != npos)
						{
							hoverred_pos = lister.advance(first_disp, static_cast<int>(ptr_where.second));
						}

						auto const columns = essence_->ordered_columns(visual_r.width);

						if (columns.empty())
							return;

						auto const txtoff = static_cast<int>(essence_->scheme_ptr->item_height_ex) / 2;

						for (auto & cat : lister.cat_container())
							for (auto & ind : cat.indicators)
							{
								if (ind)
									ind->detach();
							}

						auto idx = first_disp;
						for (auto i_categ = lister.get(first_disp.cat); i_categ != lister.cat_container().end(); ++i_categ)
						{
							if (item_coord.y > visual_r.bottom())
								break;

							if (idx.cat > 0 && idx.is_category())
							{
								_m_draw_categ(*i_categ, visual_r.x - origin.x, item_coord.y, txtoff, header_w, bgcolor,
									(hoverred_pos.is_category() && (idx.cat == hoverred_pos.cat) ? item_state::highlighted : item_state::normal)
									);
								item_coord.y += static_cast<int>(item_height_px);
								idx.item = 0;
							}

							if (i_categ->expand)
							{
								auto size = i_categ->items.size();
								for (; idx.item < size; ++idx.item)
								{
									if (item_coord.y > visual_r.bottom())
										break;

									auto item_pos = lister.index_cast(index_pair{ idx.cat, idx.item }, true);	//convert display position to absolute position

									_m_draw_item(*i_categ, item_pos, item_coord, txtoff, header_w, visual_r, columns, bgcolor, fgcolor,
										(idx == hoverred_pos ? item_state::highlighted : item_state::normal)
										);

									item_coord.y += static_cast<int>(item_height_px);
								}
							}

							++idx.cat;
							idx.item = nana::npos;
						}
					}

					essence_->inline_buffered_table.clear();

					if (item_coord.y < visual_r.bottom())
					{
						rectangle bground_r{ visual_r.x, item_coord.y, visual_r.width, static_cast<unsigned>(visual_r.bottom() - item_coord.y) };
						if (!API::dev::copy_transparent_background(essence_->listbox_ptr->handle(), bground_r, *essence_->graph, bground_r.position()))
							essence_->graph->rectangle(bground_r, true, bgcolor);
					}

					//Draw mouse selection
					//Check if the mouse selection box presents.
					if (essence_->mouse_selection.begin_position != essence_->mouse_selection.end_position)
					{
						point box_position{
							std::min(essence_->mouse_selection.begin_position.x, essence_->mouse_selection.end_position.x),
							std::min(essence_->mouse_selection.begin_position.y, essence_->mouse_selection.end_position.y)
						};
						size box_size{
								static_cast<size::value_type>(std::abs(essence_->mouse_selection.begin_position.x - essence_->mouse_selection.end_position.x)),
								static_cast<size::value_type>(std::abs(essence_->mouse_selection.begin_position.y - essence_->mouse_selection.end_position.y))
						};

						paint::graphics box_graph{ box_size };
						box_graph.rectangle(true, essence_->scheme_ptr->selection_box.get_color().blend(colors::white, 0.6));
						box_graph.rectangle(false, essence_->scheme_ptr->selection_box.get_color());

						essence_->graph->blend(rectangle{ essence_->coordinate_cast(box_position, false), box_size }, box_graph, {}, 0.5);
					}
				}
			private:
				void _m_draw_categ(const category_t& categ, int x, int y, int txtoff, unsigned width, nana::color bgcolor, item_state state)
				{
					const auto item_height = essence_->item_height();

					rectangle bground_r{ x + static_cast<int>(essence_->header.margin()), y, width, item_height };
					auto graph = essence_->graph;

					item_data item;
					item.flags.selected = categ.selected();

					this->_m_draw_item_bground(bground_r, bgcolor, {}, state, item);

					color txt_color{ static_cast<color_rgb>(0x3399) };

					//Area of category icon
					rectangle rt_ctg_icon{ x + 5, y + static_cast<int>(item_height - 16) / 2, 16, 16 };

					if (essence_->ctg_icon_renderer)
					{
						essence_->ctg_icon_renderer(*graph, rt_ctg_icon, categ.expand);
					}
					else
					{
						facade<element::arrow> arrow("double");
						arrow.direction(categ.expand ? ::nana::direction::south : ::nana::direction::east);
						arrow.draw(*graph, {}, txt_color, rt_ctg_icon, element_state::normal);
					}

					graph->string({ x + 20, y + txtoff }, categ.text, txt_color);

					auto text_px = graph->text_extent_size(categ.text).width;
					if (categ.display_number)
					{
						//Display the number of items in the category
						native_string_type str = to_nstring('(' + std::to_string(categ.items.size()) + ')');
						graph->string({ x + 25 + static_cast<int>(text_px), y + txtoff }, str);
						text_px += graph->text_extent_size(str).width;
					}

					if (35 + text_px < width)
					{
						::nana::point pos{ x + 30 + static_cast<int>(text_px), y + static_cast<int>(item_height) / 2 };

						graph->line(pos, { x + static_cast<int>(width) - 5, pos.y }, txt_color);
					}

					//Draw selecting inner rectangle
					if (item.flags.selected && (categ.expand == false))
						_m_draw_item_border(y);
				}

				color _m_draw_item_bground(const rectangle& bground_r, color bgcolor, color cell_color, item_state state, const item_data& item)
				{
					auto graph = essence_->graph;

					auto const is_transparent = API::dev::copy_transparent_background(essence_->listbox_ptr->handle(), bground_r, *graph, bground_r.position());

					if (is_transparent)
						bgcolor = color{};

					if (item.flags.selected)
					{
						bgcolor = essence_->scheme_ptr->item_selected;

						if (!cell_color.invisible())
							bgcolor = bgcolor.blend(cell_color, 0.5);
					}
					else if (!cell_color.invisible())
						bgcolor = cell_color;
					else if (!item.bgcolor.invisible())
						bgcolor = item.bgcolor;

					if (item_state::highlighted == state)
					{
						if (item.flags.selected)
							bgcolor = bgcolor.blend(essence_->scheme_ptr->item_highlighted, 0.5);
						else
							bgcolor = bgcolor.blend(essence_->scheme_ptr->item_highlighted, 0.7);
					}

					if (is_transparent)
					{
						if(!bgcolor.invisible())
							graph->blend(bground_r, bgcolor, 0.2);
					}
					else
					{
						graph->rectangle(bground_r, true, bgcolor);

					}

					return bgcolor;
				}

				/// Draws an item
				void _m_draw_item(const category_t& cat,
					              const index_pair& item_pos, 
								  const point& coord,
					              const int txtoff,						///< below y to print the text
					              unsigned header_width,				///< width of all visible columns, in pixel 
					              const nana::rectangle& content_r,		///< the rectangle where the full list content have to be drawn
					              const std::vector<size_type>& seqs,	///< columns to print
					              nana::color bgcolor, 
					              nana::color fgcolor, 
					              item_state state
					)
				{
					auto & item = cat.items[item_pos.item];

					std::vector<cell> model_cells;
					if (cat.model_ptr)
					{
						model_cells = cat.model_ptr->container()->to_cells(item_pos.item);
					}

					auto & cells = (cat.model_ptr ? model_cells : *item.cells);

					if(!item.fgcolor.invisible())
						fgcolor = item.fgcolor;

					const unsigned columns_shown_width = (std::min)(content_r.width, header_width - essence_->content_view->origin().x);

					auto graph = essence_->graph;

					//draw the background for the whole item
					rectangle bground_r{
						content_r.x + static_cast<int>(essence_->header.margin()) - essence_->content_view->origin().x,
						coord.y,
						columns_shown_width + essence_->content_view->origin().x,
						essence_->item_height() };
					auto const state_bgcolor = this->_m_draw_item_bground(bground_r, bgcolor, {}, state, item);

					//The position of column in x-axis.
					int column_x = coord.x;

					for (size_type display_order{ 0 }; display_order < seqs.size(); ++display_order)  // get the cell (column) index in the order headers are displayed
					{
						const auto column_pos = seqs[display_order];
						const auto & col = essence_->header.at(column_pos);     // deduce the corresponding header which is in a kind of display order

						if (col.width_px > essence_->scheme_ptr->text_margin)
						{
							//The column text position, it is a offset to column_x.
							int content_pos = 0;

							element_state estate = element_state::normal;
							nana::rectangle img_r;

							//Draw the image in the 1st column in display order
							if (0 == display_order)
							{
								if (essence_->checkable)
								{
									content_pos += 18;   // checker width, geom scheme?

									if (essence_->pointer_where.first == parts::checker)
									{
										switch (state)
										{
										case item_state::highlighted:
											estate = element_state::hovered;	break;
										case item_state::grabbed:
											estate = element_state::pressed;	break;
										default:	break;
										}
									}

									using state = facade<element::crook>::state;
									crook_renderer_.check(item.flags.checked ? state::checked : state::unchecked);
								}

								if (essence_->if_image)
								{
									//Draw the image in the 1st column in display order
									if (item.img)
									{
										nana::rectangle imgt(item.img_show_size);
										img_r = imgt;
										img_r.x = content_pos + coord.x + 2 + (16 - static_cast<int>(item.img_show_size.width)) / 2;  // center in 16 - geom scheme?
										img_r.y = coord.y + (static_cast<int>(essence_->item_height()) - static_cast<int>(item.img_show_size.height)) / 2; // center
									}
									content_pos += 18;  // image width, geom scheme?
								}
							}

							bool draw_column = true;

							if ( content_pos + essence_->scheme_ptr->text_margin < col.width_px) // we have room
							{
								auto inline_wdg = _m_get_inline_pane(cat, column_pos);
								if (inline_wdg)
								{
									//Make sure the user-define inline widgets is in the right visible rectangle.
									rectangle pane_r;

									const auto wdg_x = column_x + content_pos;
									const auto wdg_w = col.width_px - static_cast<unsigned>(content_pos);

									bool visible_state = true;
									if (::nana::overlap(content_r, { wdg_x, coord.y, wdg_w, essence_->item_height() }, pane_r))
									{
										::nana::point pane_pos;
										if (wdg_x < content_r.x)
											pane_pos.x = wdg_x - content_r.x;

										if (coord.y < content_r.y)
											pane_pos.y = coord.y - content_r.y;

										inline_wdg->pane_widget.move(pane_pos);
										inline_wdg->pane_bottom.move(pane_r);
									}
									else
										visible_state = false;

									draw_column = inline_wdg->inline_ptr->whether_to_draw();

									inline_wdg->item_pos = item_pos;
									inline_wdg->column_pos = column_pos;
									inline_wdg->inline_ptr->activate(*inline_wdg->indicator, item_pos);

									::nana::size sz{ wdg_w, essence_->item_height() };
									inline_wdg->pane_widget.size(sz);
									inline_wdg->inline_ptr->resize(sz);

									inline_wdg->inline_ptr->notify_status(status_type::selected, item.flags.selected);
									inline_wdg->inline_ptr->notify_status(status_type::checked, item.flags.checked);
									
									inline_wdg->indicator->attach(item_pos, inline_wdg);

									//To reduce the memory usage, the cells may not be allocated
									if (cells.size() > column_pos)
										inline_wdg->inline_ptr->set(cells[column_pos].text);
									else
										inline_wdg->inline_ptr->set({});

									API::show_window(inline_wdg->pane_bottom, visible_state);

									essence_->lister.append_active_panes(inline_wdg);
								}
							}

							auto col_bgcolor = bgcolor;
							auto col_fgcolor = fgcolor;

							if (cells.size() > column_pos)        // process only if the cell is visible
							{
								auto & m_cell = cells[column_pos];
								review_utf8(m_cell.text);

								if (m_cell.custom_format)  // adapt to costum format if need
								{
									col_fgcolor = m_cell.custom_format->fgcolor;

									bground_r = rectangle{ column_x, coord.y, col.width_px, essence_->item_height() };
									col_bgcolor = this->_m_draw_item_bground(bground_r, bgcolor, m_cell.custom_format->bgcolor, state, item);
								}
								else
									col_bgcolor = state_bgcolor;

								if (draw_column)
								{
									//Draw item text
									paint::aligner text_aligner{*graph, col.alignment};

									unsigned text_margin_right = 0;
									if (align::left == col.alignment)
										content_pos += essence_->scheme_ptr->text_margin;
									else if (align::right == col.alignment)
										text_margin_right = essence_->scheme_ptr->text_margin;

									graph->palette(true, col_fgcolor);
									text_aligner.draw(m_cell.text, { column_x + content_pos, coord.y + txtoff }, col.width_px - content_pos - text_margin_right);
								}
							}

							if (0 == display_order)
							{
								if (essence_->checkable)
									crook_renderer_.draw(*graph, col_bgcolor, col_fgcolor, essence_->checkarea(column_x, coord.y), estate);
								if (item.img)
									item.img.stretch(rectangle{ item.img.size() }, *graph, img_r);
							}

							if (display_order > 0)
								graph->line({ column_x - 1, coord.y }, { column_x - 1, coord.y + static_cast<int>(essence_->item_height()) - 1 }, static_cast<color_rgb>(0xEBF4F9));
						}

						column_x += col.width_px;
					}

					//Draw selecting inner rectangle
					if (item.flags.selected)
						_m_draw_item_border(coord.y);
				}

				inline_pane * _m_get_inline_pane(const category_t& cat, std::size_t column_pos) const
				{
					if (column_pos < cat.factories.size())
					{
						auto & factory = cat.factories[column_pos];
						if (factory)
						{
							return essence_->open_inline(factory.get(), cat.indicators[column_pos].get());
						}
					}
					return nullptr;
				}

				inline_pane* _m_find_inline_pane(const index_pair& pos, std::size_t column_pos) const
				{
					auto & cat = *essence_->lister.get(pos.cat);

					if (column_pos >= cat.factories.size())
						return nullptr;

					auto& factory = cat.factories[column_pos];
					if (!factory)
						return nullptr;

					auto i = essence_->inline_table.find(factory.get());
					if (i == essence_->inline_table.end())
						return nullptr;

					for (auto & inl_widget : i->second)
					{
						if (inl_widget->item_pos == pos && inl_widget->column_pos == column_pos)
							return inl_widget.get();
					}
					return nullptr;
				}

				void _m_draw_item_border(int item_top) const
				{
					//Draw selecting inner rectangle
					rectangle r{
						essence_->content_area().x - essence_->content_view->origin().x + static_cast<int>(essence_->header.margin()),
						item_top,
						essence_->header.width_px(),
						essence_->item_height()
					};

					essence_->graph->rectangle(r, false, static_cast<color_rgb>(0x99defd));

					essence_->graph->palette(false, colors::white);
					paint::draw(*essence_->graph).corner(r, 1);

					essence_->graph->rectangle(r.pare_off(1), false);
				}
			private:
				essence * const essence_;
				mutable facade<element::crook> crook_renderer_;
			};

			//class trigger: public drawer_trigger
				trigger::trigger()
					:	essence_(new essence),
						drawer_header_(new drawer_header_impl(essence_)),
						drawer_lister_(new drawer_lister_impl(essence_))
				{}

				trigger::~trigger()
				{
					delete drawer_lister_;
					delete drawer_header_;
					delete essence_;
				}

				essence& trigger::ess() const noexcept
				{
					return *essence_;
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					essence_->listbox_ptr = static_cast<nana::listbox*>(&widget);
					essence_->scheme_ptr = static_cast<::nana::listbox::scheme_type*>(API::dev::get_scheme(widget));
					essence_->graph = &graph;

					essence_->lister.bind(essence_, widget);
					widget.bgcolor(colors::white);

					essence_->content_view.reset(new widgets::skeletons::content_view{ widget.handle() });
					essence_->resize_disp_area();

					//Set the content_view wheel speed with the listbox scheme.
					essence_->content_view->set_wheel_speed([this] {
						return essence_->scheme_ptr->mouse_wheel.lines;
					});

					essence_->content_view->events().hover_outside = [this](const point& cur_pos) {
						essence_->update_mouse_selection(cur_pos);
					};
				}

				void trigger::detached()
				{
					essence_->graph = nullptr;
					essence_->listbox_ptr = nullptr;
				}

				void trigger::typeface_changed(graph_reference graph)
				{
					essence_->text_height = 0;
					unsigned as, ds, il;
					if (graph.text_metrics(as, ds, il))
						essence_->text_height = as + ds;

					essence_->calc_content_size(true);
				}

				void trigger::refresh(graph_reference graph)
				{
					if (API::is_destroying(essence_->lister.wd_ptr()->handle()))
						return;

					nana::rectangle r;

					if (essence_->rect_lister(r))
						drawer_lister_->draw(r);

					if (essence_->header.attrib().visible && essence_->rect_header(r))
						drawer_header_->draw(graph, r);

					essence_->draw_peripheral();
				}

				// In mouse move event, it cancels the msup_deselect if the listbox is draggable or it started the mouse selection.
				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					using item_state = essence::item_state;
					using parts = essence::parts;

					//Don't deselect the items if the listbox is draggable
					if ((operation_states::msup_deselect == essence_->operation.state) && API::dev::window_draggable(arg.window_handle))
						essence_->operation.state = operation_states::none;

					bool need_refresh = false;

					point pos_in_header = arg.pos;
					essence_->widget_to_header(pos_in_header);

					if(essence_->ptr_state == item_state::pressed)
					{
						if((essence_->pointer_where.first == parts::header) && essence_->header.attrib().movable)
						{   // moving a pressed header : grab it
							essence_->ptr_state = item_state::grabbed;

							//Start to move a header column or resize a header column(depends on item_spliter_)
							drawer_header_->grab(pos_in_header, true);
							
							essence_->lister.wd_ptr()->set_capture(true);
							need_refresh = true;
						}
					}

                    if(essence_->ptr_state == item_state::grabbed)
					{
						// moving a grabbed header 
						need_refresh = drawer_header_->grab_move(pos_in_header);
					}
					else if(essence_->calc_where(arg.pos))
					{
						essence_->ptr_state = item_state::highlighted;
						need_refresh = true;
					}

					//Detects column splitter
					if(essence_->header.attrib().resizable &&
						(essence_->pointer_where.first == parts::header) &&
						drawer_header_->detect_splitter(arg.pos.x))
					{
						essence_->lister.wd_ptr()->cursor(cursor::size_we);
					}
					else if(essence_->ptr_state != item_state::grabbed)
					{
						if((drawer_header_->splitter() != npos) || (essence_->lister.wd_ptr()->cursor() == cursor::size_we))
						{
							essence_->lister.wd_ptr()->cursor(cursor::arrow);
							drawer_header_->cancel_splitter();
							need_refresh = true;
						}
					}

					if (essence_->mouse_selection.started)
					{
						essence_->update_mouse_selection(arg.pos);

						//Don't deselect items if the mouse selection is started
						essence_->operation.state = operation_states::none;
						need_refresh = true;
					}

					if (need_refresh)
					{
						refresh(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference graph, const arg_mouse&)
				{
					using item_state = essence::item_state;
					using parts = essence::parts;

					if((essence_->pointer_where.first != parts::unknown) || (essence_->ptr_state != item_state::normal))
					{
						if (essence_->ptr_state != item_state::grabbed)
						{
							essence_->pointer_where.first = parts::unknown;
							essence_->ptr_state = item_state::normal;
						}

						refresh(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_down(graph_reference graph, const arg_mouse& arg)
				{
					using item_state = essence::item_state;
					using parts = essence::parts;
					bool update = false;

					essence_->mouse_selection.reverse_selection = false;

					auto & lister = essence_->lister;

					rectangle head_r, list_r;
					auto const good_head_r = essence_->rect_header(head_r);
					auto const good_list_r = essence_->rect_lister(list_r);

					auto & ptr_where = essence_->pointer_where;
					if((ptr_where.first == parts::header) && (ptr_where.second != npos || (drawer_header_->splitter() != npos)))
					{
						essence_->ptr_state = item_state::pressed;
						if(good_head_r)
						{
							drawer_header_->draw(graph, head_r);
							update = true;
						}
					}
					else if(ptr_where.first == parts::list || ptr_where.first == parts::checker)
					{
						index_pair item_pos = lister.advance(essence_->first_display(), static_cast<int>(ptr_where.second));

						if ((essence_->column_from_pos(arg.pos.x) != npos) && !item_pos.empty())
						{
							auto * item_ptr = (item_pos.is_category() ? nullptr : &lister.at(item_pos));

							const auto abs_item_pos = lister.index_cast_noexcept(item_pos, true, item_pos);	//convert display position to absolute position

							if(ptr_where.first == parts::list)
							{
								//adjust the display of selected into the list rectangle if the part of the item is beyond the top/bottom edge
								if (good_list_r)
								{
									auto const item_top = this->essence_->item_window_top(abs_item_pos);
									auto const item_bottom = item_top + static_cast<int>(essence_->item_height());

									int move_top = 0;

									if (item_top < list_r.y && list_r.y < item_bottom)
										move_top = item_top - list_r.y;
									else if (item_top < list_r.bottom() && list_r.bottom() < item_bottom)
										move_top = item_bottom - list_r.bottom();

									if (0 != move_top)
									{
										essence_->content_view->move_origin({ 0, move_top });
										essence_->content_view->sync(false);
									}
								}

								bool new_selected_status = true;

								if (!lister.single_status(true))	//multiply selection enabled
								{
									if (arg.shift)
									{
										//Set the first item as the begin of selected item if there
										//is not a latest selected item.(#154 reported by RenaudAlpes)
										if (lister.latest_selected_abs.empty())
											lister.latest_selected_abs = lister.first();

										auto before = lister.latest_selected_abs;

										lister.select_display_range_if(lister.latest_selected_abs, item_pos, true, [](const index_pair&)
										{
											return true;
										});

										lister.latest_selected_abs = before;
									}
									else if (arg.ctrl)
									{
										essence_->mouse_selection.reverse_selection = true;
										new_selected_status = !essence_->cs_status(abs_item_pos, true);
									}
									else
									{
										auto selected = lister.pick_items(true);
										if (selected.cend() != std::find(selected.cbegin(), selected.cend(), abs_item_pos))
										{
											//If the current selected one has been selected before selecting, remains the selection states for all
											//selected items. But these items will be unselected when the mouse is released.

											//Other items will be unselected if multiple items are selected.
											if (selected.size() > 1)
											{
												essence_->operation.item = abs_item_pos;

												//Don't deselect the selections, let it determine in mouse_move event depending on whether dnd is enabled.
												essence_->operation.state = operation_states::msup_deselect;
											}
										}
										else
											lister.select_for_all(false, abs_item_pos);

										lister.latest_selected_abs = abs_item_pos;
									}
								}
								else
								{
									//Clicking on a category is ignored when single selection is enabled.
									//Fixed by Greentwip(issue #121)
									if (item_ptr)
										new_selected_status = !item_proxy(essence_, abs_item_pos).selected();
								}

								if(item_ptr)
								{
									if (item_ptr->flags.selected != new_selected_status)
									{
										if (new_selected_status)
										{
											//Deselects the previously selected item.
											lister.cancel_others_if_single_enabled(true, abs_item_pos);
											essence_->lister.latest_selected_abs = abs_item_pos;
										}
										else if (essence_->lister.latest_selected_abs == abs_item_pos)
											essence_->lister.latest_selected_abs.set_both(npos);

										item_ptr->flags.selected = new_selected_status;
										lister.emit_cs(abs_item_pos, true);
									}
								}
								else
								{
									//A category was clicked. Sets all child items to be selected only if multiply selection is enabled.
									if(!lister.single_status(true))
										lister.cat_status(item_pos.cat, true, true);
								}
							}
							else
							{
								if (item_ptr)
								{
									item_ptr->flags.checked = !item_ptr->flags.checked;
									lister.emit_cs(abs_item_pos, false);

									if (item_ptr->flags.checked)
										lister.cancel_others_if_single_enabled(false, abs_item_pos);
								}
								else if (!lister.single_status(false))	//not single checked
									lister.cat_status_reverse(item_pos.cat, false);
							}
							update = true;
						}

						if(update)
						{
							if (good_list_r)
							{
								drawer_lister_->draw(list_r);
								if (good_head_r)
									drawer_header_->draw(graph, head_r);
							}
							else
								update = false;
						}
					}
					else if (ptr_where.first == parts::list_blank)	//not selected
					{
						//Start box selection if mulit-selection is enabled
						if (arg.is_left_button() && (!lister.single_status(true)))
							essence_->start_mouse_selection(arg);

						//Deselecting all items is deferred to the mouse up event when ctrl or shift is not pressed
						//Pressing ctrl or shift is to selects other items without deselecting current selections.
						if (!(arg.ctrl || arg.shift))
						{
							essence_->operation.state = operation_states::msup_deselect;
							essence_->operation.item = index_pair{nana::npos, nana::npos};
						}
					}

					if(update)
					{
						essence_->draw_peripheral();
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference graph, const arg_mouse& arg)
				{
					using item_state = essence::item_state;
					using parts = essence::parts;

					auto prev_state = essence_->ptr_state;
					essence_->ptr_state = item_state::highlighted;

					bool need_refresh = false;

					//Don't sort the column when the mouse is due to released for stopping resizing column.
					if ((drawer_header_->splitter() == npos) && essence_->header.attrib().sortable && essence_->pointer_where.first == parts::header && prev_state == item_state::pressed)
					{
						//Try to sort the column
						if(essence_->pointer_where.second < essence_->header.cont().size())
							need_refresh = essence_->lister.sort_column(essence_->pointer_where.second, nullptr);
					}
					else if (item_state::grabbed == prev_state)
					{
						nana::point pos = arg.pos;
						essence_->widget_to_header(pos);
						drawer_header_->grab(pos, false);
						need_refresh = true;
						essence_->lister.wd_ptr()->release_capture();
					}

					if (essence_->mouse_selection.started)
					{
						essence_->stop_mouse_selection();
						need_refresh = true;
					}

					if (operation_states::msup_deselect == essence_->operation.state)
					{
						essence_->operation.state = operation_states::none;

						//Don't deselect if the predicate returns false
						if(!(essence_->pred_msup_deselect && !essence_->pred_msup_deselect(arg.button)))
							need_refresh |= essence_->lister.select_for_all(false, essence_->operation.item);
					}

					if (need_refresh)
					{
						refresh(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::dbl_click(graph_reference graph, const arg_mouse&)
				{
					using parts = essence::parts;

					if (parts::header == essence_->pointer_where.first)
					{
						if (cursor::size_we == essence_->lister.wd_ptr()->cursor())
						{
							//adjust the width of column to fit its content.
							auto split_pos = drawer_header_->splitter();
							if (split_pos != npos)
							{
								essence_->header.at(split_pos).fit_content();
								refresh(graph);
								API::dev::lazy_refresh();
							}
							return;
						}
					}

					if (parts::list != essence_->pointer_where.first)
						return;

					auto & lister = essence_->lister;
					//Get the item which the mouse is placed.

					auto item_pos = lister.advance(essence_->first_display(), static_cast<int>(essence_->pointer_where.second));
					if (!item_pos.empty())
					{
						if (!item_pos.is_category())	//being the npos of item.second is a category
							return;

						arg_listbox_category arg_cat(cat_proxy(essence_, item_pos.cat));
						lister.wd_ptr()->events().category_dbl_click.emit(arg_cat, lister.wd_ptr()->handle());

						if (!arg_cat.block_operation)
						{
                            bool do_expand = (lister.expand(item_pos.cat) == false);
                            lister.expand(item_pos.cat, do_expand);

							essence_->calc_content_size(false);
							essence_->content_view->sync(false);
							refresh(graph);
							API::dev::lazy_refresh();
                        }
					}
				}

				void trigger::resized(graph_reference graph, const arg_resized&)
				{
					essence_->resize_disp_area();

					refresh(graph);
					API::dev::lazy_refresh();
				}

				void trigger::key_press(graph_reference graph, const arg_keyboard& arg)
				{
					auto & list = essence_->lister;
					// Exit if list is empty
					if (list.first().empty())
						return;

					switch(arg.key)
					{
					case keyboard::os_arrow_up:
					case keyboard::os_arrow_down:
						list.move_select((keyboard::os_arrow_up == arg.key), !arg.shift, true);
						break;
					case L' ':
						{
							bool items_checked{ false };
							auto items = list.pick_items(true, false, &items_checked);
							items_checked = !items_checked;
							for (auto i : items)
								item_proxy(essence_, i).check(items_checked);
						}
						break;
					case keyboard::os_pageup :
					case keyboard::os_pagedown:
						{
							auto const upward = (keyboard::os_pageup == arg.key);
							auto const item_px = essence_->item_height();
							auto picked_items = list.pick_items(true, true);
							index_pair init_idx = (picked_items.empty() ? list.first() : picked_items[0]);

							essence_->lister.select_for_all(false);

							//Get the pixels between the init item and top edge or bottom edge
							auto logic_top = static_cast<int>(list.distance(list.first(), init_idx) * item_px);

							auto const screen_top = essence_->content_view->origin().y;
							auto const screen_bottom = screen_top + essence_->content_view->view_area().height;
							index_pair target_idx;

							//Check if it scrolls in current screen window
							//condition: top of target item is not less than top edge of content view and
							//the bottom of target item is not greater than bottom edge of content view.
							if ((screen_top + static_cast<int>(item_px) <= logic_top) && (logic_top + static_cast<int>(item_px) + static_cast<int>(item_px) <= static_cast<int>(screen_bottom)))
							{
								int offset = (static_cast<int>(upward ? screen_top : screen_bottom - item_px) - logic_top) / static_cast<int>(item_px);
								target_idx = list.advance(init_idx, offset);
							}
							else
							{
								//turn page
								auto page_item_count = (std::max)(1, static_cast<int>(essence_->count_of_exposed(false)));

								auto origin = essence_->content_view->origin();
								if (upward)
								{
									target_idx = list.advance(init_idx, -page_item_count);
									if (target_idx.empty())
										target_idx = list.first();

									origin.y = static_cast<int>(list.distance(list.first(), target_idx) * item_px);
								}
								else
								{
									target_idx = list.advance(init_idx, page_item_count);
									if (target_idx.empty())
										target_idx = list.last();

									origin.y = static_cast<int>((list.distance(list.first(), target_idx) + 1) * item_px);
									origin.y = (std::max)(origin.y - static_cast<int>(screen_bottom - screen_top), 0);
								}

								essence_->content_view->move_origin(origin - essence_->content_view->origin());
							}
						
							if (!target_idx.is_category())
								item_proxy::from_display(essence_, target_idx).select(true);
							else if (!list.single_status(true))	//not selected
								list.cat_status(target_idx.cat, true, true);
						}
						break;
					case keyboard::os_home:
					case keyboard::os_end:
						{
							list.select_for_all(false);

							auto pos = (keyboard::os_home == arg.key ? list.first() : list.last());
							if (!pos.empty())
							{
								//When the pos indicates an empty category, then search forwards/backwards(depending on arg.key whether it is Home or End) for a non empty category.
								//When a non-empty category is found, assign the pos to the first/last item of the category if the category is expanded.
								if (pos.is_category())
								{
									if (keyboard::os_home == arg.key)
									{
										while (0 == list.size_item(pos.cat))
										{
											if (++pos.cat >= list.cat_container().size())
											{
												pos = index_pair{ npos, npos };
												break;
											}
										}
									}
									else
									{
										while (0 == list.size_item(pos.cat))
										{
											if (pos.cat-- == 0)
											{
												pos = index_pair{ npos, npos };
												break;
											}
										}
									}

									if (!pos.empty())
									{
										if (list.expand(pos.cat))
											pos.item = 0;
									}
								}

								if (!pos.empty())
								{
									if (pos.is_category())
									{
										if (!list.single_status(true)) //multiple selection is not enabled
											list.cat_status(pos.cat, true, true);
									}
									else
										item_proxy::from_display(essence_, pos).select(true);

									list.scroll_into_view(pos, view_action::auto_view);
								}
							}
							
						}
						break;
					default:
						return;
					}
					refresh(graph);
					API::dev::lazy_refresh();
				}

				void trigger::key_char(graph_reference graph, const arg_keyboard& arg)
				{
					switch(arg.key)
					{
					case keyboard::copy:
						{
							auto exp_opt = essence_->def_exp_options;
							exp_opt.columns_order = essence_->header.get_headers(true);
							exp_opt.only_selected_items = true;
							::nana::system::dataexch().set(essence_->to_string(exp_opt), API::root(essence_->listbox_ptr->handle()));
							return;
						}
					case keyboard::select_all :
						if (!essence_->lister.single_status(true))
						{
							essence_->lister.select_for_all(true);
							refresh(graph);
							API::dev::lazy_refresh();
						}
                        break;
					default:
						return;
					}
				}
			//end class trigger

			//class item_proxy
				item_proxy::item_proxy(essence * ess, const index_pair& pos)
					:	ess_(ess),
						pos_(pos)
				{
					//get the cat of the item specified by pos
					if (ess && !pos.empty())
						cat_ = &(*ess->lister.get(pos.cat));
				}

				/// the main purpose of this it to make obvious that item_proxy operate with absolute positions, and don't get moved during sort()
				item_proxy item_proxy::from_display(essence *ess, const index_pair &relative)
				{
					return item_proxy{ ess, ess->lister.index_cast(relative, true) };
				}

				item_proxy item_proxy::from_display(const index_pair &relative) const
				{
					return item_proxy{ess_, ess_->lister.index_cast(relative, true)};
				}

				/// possible use: last_selected_display = last_selected.to_display().item; use with caution, it get invalidated after a sort()
				index_pair item_proxy::to_display() const
				{
					return ess_->lister.index_cast(pos_, false);	//convert absolute position to display position
				}

				bool item_proxy::displayed() const
				{
					if (!ess_->lister.get(pos_.cat)->expand)
						return false;

					auto pos = to_display();
					if (ess_->first_display() > pos)
						return false;

					auto last = ess_->lister.advance(ess_->first_display(), static_cast<int>(ess_->count_of_exposed(false)));

					return (last > pos || last == pos);
				}

				bool item_proxy::empty() const noexcept
				{
					return !ess_;
				}

				item_proxy & item_proxy::check(bool ck, bool scroll_view)
				{
					internal_scope_guard lock;
					auto & m = cat_->items.at(pos_.item);
					if(m.flags.checked != ck)
					{
						m.flags.checked = ck;
						ess_->lister.emit_cs(pos_, false);
						if (scroll_view)
						{
							if (ess_->lister.get(pos_.cat)->expand)
								ess_->lister.get(pos_.cat)->expand = false;

							if (!this->displayed())
								ess_->lister.scroll_into_view(pos_, (ess_->first_display() > this->to_display() ? view_action::top_view : view_action::bottom_view));
						}

						ess_->update();
					}
					return *this;
				}

				bool item_proxy::checked() const
				{
					return cat_->items.at(pos_.item).flags.checked;
				}

				/// is ignored if no change (maybe set last_selected anyway??), but if change emit event, deselect others if need ans set/unset last_selected
				item_proxy & item_proxy::select(bool s, bool scroll_view)
				{
					internal_scope_guard lock;

					//pos_ never represents a category if this item_proxy is available.
					auto & m = cat_->items.at(pos_.item);       // a ref to the real item

					//ignore if no change
					if(m.flags.selected == s)
						return *this;

					m.flags.selected = s;                       // actually change selection

					ess_->lister.emit_cs(this->pos_, true);

					if (m.flags.selected)
					{
						ess_->lister.cancel_others_if_single_enabled(true, pos_);	//Cancel all selections except pos_ if single_selection is enabled.
						ess_->lister.latest_selected_abs = pos_;
					}
					else if (ess_->lister.latest_selected_abs == pos_)
						ess_->lister.latest_selected_abs.set_both(npos);

					if (scroll_view && (!this->displayed()))
						ess_->lister.scroll_into_view(pos_, (ess_->first_display() > this->to_display() ? view_action::top_view : view_action::bottom_view));

					ess_->update();
					return *this;
				}

				bool item_proxy::selected() const
				{
					return cat_->items.at(pos_.item).flags.selected;
				}

				item_proxy & item_proxy::bgcolor(const nana::color& col)
				{
					cat_->items.at(pos_.item).bgcolor = col;
					ess_->update();
					return *this;
				}

				nana::color item_proxy::bgcolor() const
				{
					return cat_->items.at(pos_.item).bgcolor;
				}

				item_proxy& item_proxy::fgcolor(const nana::color& col)
				{
					cat_->items.at(pos_.item).fgcolor = col;
					ess_->update();
					return *this;
				}

				nana::color item_proxy::fgcolor() const
				{
					return cat_->items.at(pos_.item).fgcolor;
				}

				std::size_t item_proxy::columns() const noexcept
				{
					return ess_->header.cont().size();
				}

				size_type item_proxy::column_cast(size_type pos, bool disp_order) const
				{
					return ess_->header.cast(pos, disp_order);
				}

				item_proxy& item_proxy::text(size_type col, cell cl)
				{
					ess_->lister.text(cat_, pos_.item, col, std::move(cl), columns());
					ess_->update();
					return *this;
				}

				item_proxy& item_proxy::text(size_type col, std::string str)
				{
					ess_->lister.text(cat_, pos_.item, col, std::move(str), columns());
					ess_->update();
					return *this;
				}

				item_proxy& item_proxy::text(size_type col, const std::wstring& str)
				{
					ess_->lister.text(cat_, pos_.item, col, to_utf8(str), columns());
					ess_->update();
					return *this;
				}

				std::string item_proxy::text(size_type col) const
				{
					return cat_->cells(pos_.item).at(col).text;
				}

				void item_proxy::icon(const nana::paint::image& img)
				{
					if (img)
					{
						auto & item = cat_->items.at(pos_.item);
						item.img = img;
						nana::fit_zoom(img.size(), nana::size(16, 16), item.img_show_size);

						ess_->if_image = true;
						ess_->update();
					}
				}

				//Behavior of Iterator's value_type
#ifdef _nana_std_has_string_view
				bool item_proxy::operator==(std::string_view sv) const
				{
					return (text(0) == sv);
				}

				bool item_proxy::operator==(std::wstring_view sv) const
				{
					return (text(0) == to_utf8(sv));
				}
#else
				bool item_proxy::operator==(const char * s) const
				{
					return this->operator==(std::string(s));
				}

				bool item_proxy::operator==(const wchar_t * s) const
				{
					return this->operator==(std::wstring(s));
				}

				bool item_proxy::operator==(const std::string& s) const
				{
					return (text(0) == s);
				}

				bool item_proxy::operator==(const std::wstring& s) const
				{
					return (text(0) == to_utf8(s));
				}
#endif

				item_proxy & item_proxy::operator=(const item_proxy& rhs)
				{
					if(this != &rhs)
					{
						ess_ = rhs.ess_;
						cat_ = rhs.cat_;
						pos_ = rhs.pos_;
					}
					return *this;
				}

				// Behavior of Iterator
				item_proxy & item_proxy::operator++()
				{
					if (++pos_.item >= cat_->items.size())
						cat_ = nullptr;

					return *this;
				}

				// Behavior of Iterator
				item_proxy	item_proxy::operator++(int)
				{
					item_proxy ip(*this);

					if (++pos_.item >= cat_->items.size())
						cat_ = nullptr;
					return ip;
				}

				// Behavior of Iterator
				item_proxy& item_proxy::operator*()
				{
					return *this;
				}

				// Behavior of Iterator
				const item_proxy& item_proxy::operator*() const
				{
					return *this;
				}

				// Behavior of Iterator
				item_proxy* item_proxy::operator->()
				{
					return this;
				}

				// Behavior of Iterator
				const item_proxy* item_proxy::operator->() const
				{
					return this;
				}

				// Behavior of Iterator
				bool item_proxy::operator==(const item_proxy& rhs) const
				{
					if((ess_ != rhs.ess_) || (cat_ != rhs.cat_))
						return false;

					//They both are end iterator when cat_ == 0
					return (!cat_ || (pos_ == rhs.pos_));
				}

				// Behavior of Iterator
				bool item_proxy::operator!=(const item_proxy& rhs) const
				{
					return ! this->operator==(rhs);
				}

				//Undocumented methods
				essence * item_proxy::_m_ess() const noexcept
				{
					return ess_;
				}

				index_pair item_proxy::pos() const noexcept
				{
					return pos_;
				}

				auto item_proxy::_m_cells() const -> std::vector<cell>
				{
					return cat_->cells(pos_.item);
				}

				nana::any * item_proxy::_m_value(bool alloc_if_empty)
				{
					return ess_->lister.anyobj(pos_, alloc_if_empty);
				}

				const nana::any * item_proxy::_m_value() const
				{
					return ess_->lister.anyobj(pos_, false);
				}
			//end class item_proxy

			//class cat_proxy

			//the member cat_ is used for fast accessing to the category
				cat_proxy::cat_proxy(essence * ess, size_type pos) noexcept
					:	ess_(ess),
						pos_(pos)
				{
					_m_cat_by_pos();
				}

				cat_proxy::cat_proxy(essence* ess, category_t* cat) noexcept
					:	ess_(ess),
						cat_(cat)
				{
					for (auto & m : ess->lister.cat_container())
					{
						if (&m == cat)
							break;
						++pos_;
					}
				}

				model_guard cat_proxy::model()
				{
					if (!cat_->model_ptr)
						throw std::runtime_error("nana::listbox has not a model for the category");

					return{ cat_->model_ptr.get() };
				}

				void cat_proxy::append(std::initializer_list<std::string> arg)
				{
					const auto items = columns();
					push_back({});
					item_proxy ip{ ess_, index_pair(pos_, size() - 1) };
					size_type pos = 0;
					for (auto & txt : arg)
					{
						ip.text(pos++, txt);
						if (pos >= items)
							break;
					}
				}

				void cat_proxy::append(std::initializer_list<std::wstring> arg)
				{
					const auto items = columns();
					push_back({});
					item_proxy ip{ ess_, index_pair(pos_, size() - 1) };
					size_type pos = 0;
					for (auto & txt : arg)
					{
						ip.text(pos++, txt);
						if (pos >= items)
							break;
					}
				}

				cat_proxy & cat_proxy::select(bool sel)
                {
                    for (item_proxy &it : *this )
                        it.select(sel);

                    ess_->lister.latest_selected_abs = index_pair {this->pos_, npos};

                    return *this;
                }

				bool cat_proxy::selected() const
                {
                    for (item_proxy &it : *this )
                        if (!it.selected())
                            return false;
                    return true;
                }

				cat_proxy& cat_proxy::display_number(bool display)
				{
					if (cat_->display_number != display)
					{
						cat_->display_number = display;
						ess_->update();
					}
					return *this;
				}

				bool cat_proxy::expanded() const
				{
					return cat_->expand;
				}

				cat_proxy& cat_proxy::expanded(bool expand)
				{
					//The first category isn't allowed to be collapsed
					if ((expand != cat_->expand) && pos_)
					{
						cat_->expand = expand;
						ess_->update();
					}
					return *this;
				}

				auto cat_proxy::columns() const -> size_type
				{
					return ess_->header.cont().size();
				}

				cat_proxy& cat_proxy::text(std::string s)
				{
					auto text = to_nstring(s);
					internal_scope_guard lock;
					if (text != cat_->text)
					{
						cat_->text = std::move(text);
						ess_->update();
					}
					return *this;
				}

				cat_proxy& cat_proxy::text(std::wstring s)
				{
					auto text = to_nstring(s);
					internal_scope_guard lock;
					if (text != cat_->text)
					{
						cat_->text = std::move(text);
						ess_->update();
					}
					return *this;
				}

				std::string cat_proxy::text() const
				{
					internal_scope_guard lock;
					return to_utf8(cat_->text);
				}

				void cat_proxy::push_back(std::string s)
				{
					internal_scope_guard lock;

					ess_->lister.throw_if_immutable_model(index_pair{ pos_ });

					cat_->sorted.push_back(cat_->items.size());

					if (cat_->model_ptr)
					{
						const auto cont = cat_->model_ptr->container();
						auto pos = cont->size();
						cont->emplace_back();
						auto cells = cont->to_cells(pos);
						if (cells.size())
							cells.front().text = std::move(s);
						else
							cells.emplace_back(std::move(s));

						cont->assign(pos, cells);
						cat_->items.emplace_back();
					}
					else
						cat_->items.emplace_back(std::move(s));

					ess_->update();
				}

				//Behavior of a container
				item_proxy cat_proxy::begin() const
				{
					auto i = ess_->lister.get(pos_);
					if (i->items.empty())
						return end();

					return item_proxy(ess_, index_pair(pos_, 0));
				}

				//Behavior of a container
				item_proxy cat_proxy::end() const
				{
					return item_proxy(ess_);
				}

				//Behavior of a container
				item_proxy cat_proxy::cbegin() const
				{
					return begin();
				}

				//Behavior of a container
				item_proxy cat_proxy::cend() const
				{
					return end();
				}

				item_proxy cat_proxy::at(size_type pos_abs) const
				{
					check_range(pos_abs, size());
					return item_proxy(ess_, index_pair(pos_, pos_abs));
				}

				item_proxy cat_proxy::back() const
				{
					if (cat_->items.empty())
						throw std::runtime_error("listbox.back() no element in the container.");

					return item_proxy(ess_, index_pair(pos_, cat_->items.size() - 1));
				}

				size_type cat_proxy::index_cast(size_type from, bool from_display_order) const
				{
					return ess_->lister.index_cast(index_pair{ pos_, from }, from_display_order).item;
				}

				size_type cat_proxy::position() const
				{
					return pos_;
				}

				size_type cat_proxy::size() const
				{
					return cat_->items.size();
				}

				// Behavior of Iterator
				cat_proxy& cat_proxy::operator=(const cat_proxy& r)
				{
					if(this != &r)
					{
						ess_ = r.ess_;
						cat_ = r.cat_;
						pos_ = r.pos_;
					}
					return *this;
				}

				// Behavior of Iterator
				cat_proxy & cat_proxy::operator++()
				{
					++pos_;
					_m_cat_by_pos();

					return *this;
				}

				// Behavior of Iterator
				cat_proxy	cat_proxy::operator++(int)
				{
					cat_proxy ip(*this);
					++pos_;
					_m_cat_by_pos();

					return ip;
				}

				// Behavior of Iterator
				cat_proxy& cat_proxy::operator*()
				{
					return *this;
				}

				// Behavior of Iterator
				const cat_proxy& cat_proxy::operator*() const
				{
					return *this;
				}

				/// Behavior of Iterator
				cat_proxy* cat_proxy::operator->()
				{
					return this;
				}

				/// Behavior of Iterator
				const cat_proxy* cat_proxy::operator->() const
				{
					return this;
				}

				// Behavior of Iterator
				bool cat_proxy::operator==(const cat_proxy& r) const
				{
					if(ess_ != r.ess_)
						return false;

					if(ess_)	//Not empty
						return (pos_ == r.pos_);

					return true;	//Both are empty
				}

				// Behavior of Iterator
				bool cat_proxy::operator!=(const cat_proxy& r) const
				{
					return ! this->operator==(r);
				}

				void cat_proxy::inline_factory(size_type column, pat::cloneable<pat::abstract_factory<inline_notifier_interface>> factory)
				{
					check_range(column, this->columns());

					if (column >= cat_->factories.size())
					{
						cat_->factories.resize(column + 1);
						cat_->indicators.resize(column + 1);
					}

					cat_->factories[column] = std::move(factory);
					cat_->indicators[column].reset(new inline_indicator(ess_, column));
				}

				void cat_proxy::_m_append(std::vector<cell> && cells)
				{
					//check invalid cells
					for (auto & cl : cells)
					{
						if (cl.text.size() == 1 && cl.text[0] == wchar_t(0))
						{
							cl.text.clear();
							cl.custom_format.reset();
						}
					}

					internal_scope_guard lock;

					if (cat_->model_ptr)
					{
						es_lister::throw_if_immutable_model(cat_->model_ptr.get());

						auto container = cat_->model_ptr->container();
	
						auto item_index = container->size();
						cat_->items.emplace_back();
						container->emplace_back();

						container->assign(item_index, cells);
					}
					else
					{
						cells.resize(columns());
						cat_->items.emplace_back(std::move(cells));
					}

					cat_->sorted.push_back(cat_->items.size() - 1);
				}

				void cat_proxy::_m_try_append_model(const const_virtual_pointer& dptr)
				{
					//Throws when appends an object to a listbox which should have a model.
					if (!cat_->model_ptr)
						throw std::runtime_error("nana::listbox hasn't a model");

					ess_->lister.throw_if_immutable_model(cat_->model_ptr.get());

					if (!cat_->model_ptr->container()->push_back(dptr))
						throw std::invalid_argument("nana::listbox, the type of operand object is mismatched with model container value_type");

					cat_->sorted.push_back(cat_->items.size());
					cat_->items.emplace_back();
				}

				void cat_proxy::_m_cat_by_pos() noexcept
				{
					if (pos_ >= ess_->lister.cat_container().size())
					{
						ess_ = nullptr;
						cat_ = nullptr;
						return;
					}

					auto i = ess_->lister.get(pos_);
					cat_ = &(*i);
				}

				//A fix for auto_draw, to make sure the inline widget set() issued after value() and value_ptr() are actually set.
				//Fixed by leobackes(pr#86)
				void cat_proxy::_m_update() noexcept
				{
					ess_->update();
				}

				void cat_proxy::_m_reset_model(model_interface* p)
				{
					if (ess_->listbox_ptr)
					{
						cat_->model_ptr.reset(p);
						cat_->items.clear();

						cat_->items.resize(cat_->model_ptr->container()->size());

						cat_->make_sort_order();
						ess_->lister.sort();

						//Don't ignore the auto-draw flag for performance enhancement.
						ess_->update();
					}
				}
			//end class cat_proxy
		}
	}//end namespace drawerbase

	arg_listbox::arg_listbox(const drawerbase::listbox::item_proxy& m) noexcept
		: item(m)
	{
	}


	//Implementation of arg_listbox_category
	//Contributed by leobackes(pr#97)
	arg_listbox_category::arg_listbox_category(const nana::drawerbase::listbox::cat_proxy& cat) noexcept
		: category(cat)
    {
    }

	//class listbox

		listbox::listbox(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		listbox::listbox(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		bool listbox::assoc_ordered(bool enable)
		{
			internal_scope_guard lock;

			if (_m_ess().lister.enable_ordered(enable))
			{
				_m_ess().update();
				return true;
			}

			return false;
		}

		void listbox::auto_draw(bool enabled) noexcept
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			if (ess.auto_draw != enabled)
			{
				ess.auto_draw = enabled;
				ess.update();
			}
		}

		void listbox::scroll(bool to_bottom, size_type cat_pos)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto cats = this->size_categ();

			if (::nana::npos != cat_pos)
			{
				if (cat_pos >= cats)
					throw std::invalid_argument("listbox: invalid category");
			}
			else
				cat_pos = cats - 1;
			
			index_pair pos(cat_pos);
			if (to_bottom)
			{
				auto items = ess.lister.size_item(cat_pos);
				pos.item = (0 == items ? ::nana::npos : items - 1);
			}
			else
				pos.item = ess.lister.size_item(cat_pos) ? 0 : ::nana::npos;

			this->scroll(to_bottom, pos);
		}

		void listbox::scroll(bool to_bottom, const index_pair& abs_pos)
		{
			using view_action = drawerbase::listbox::view_action;
			_m_ess().lister.scroll_into_view(abs_pos, (to_bottom ? view_action::bottom_view : view_action::top_view));
			_m_ess().update();
		}

		listbox::size_type listbox::append_header(std::string s, unsigned width)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto pos = ess.header.create(&ess, to_nstring(std::move(s)), width);
			ess.update();
            return pos;
		}

		listbox::size_type listbox::append_header(std::wstring s, unsigned width)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto pos = ess.header.create(&ess, to_nstring(std::move(s)), width);
			ess.update();
			return pos;
		}

		void listbox::clear_headers()
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			ess.lister.erase();
			ess.header.clear();
			ess.update();
		}

		listbox::cat_proxy listbox::append(std::string s)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_category(to_nstring(std::move(s)));
			ess.update();

			return cat_proxy{ &ess, new_cat_ptr };
		}

		listbox::cat_proxy listbox::append(std::wstring s)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_category(to_nstring(std::move(s)));
			ess.update();
			return cat_proxy{ &ess, new_cat_ptr };
		}

		void listbox::append(std::initializer_list<std::string> categories)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();

			for (auto & arg : categories)
				ess.lister.create_category(native_string_type(to_nstring(arg)));
			ess.update();
		}

		void listbox::append(std::initializer_list<std::wstring> categories)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();

			for (auto & arg : categories)
				ess.lister.create_category(native_string_type(to_nstring(arg)));
			ess.update();
		}

		rectangle listbox::content_area() const
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto carea = ess.content_area();
			carea.x += ess.header.margin();
			carea.width -= (std::min)(carea.width, ess.header.margin());
			return carea;
		}

		auto listbox::insert(cat_proxy cat, std::string str) -> cat_proxy
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_category(to_nstring(std::move(str)), cat.position());
			return cat_proxy{ &ess, new_cat_ptr };
		}

		auto listbox::insert(cat_proxy cat, std::wstring str) -> cat_proxy
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_category(to_nstring(std::move(str)), cat.position());
			return cat_proxy{ &ess, new_cat_ptr };
		}


		void listbox::insert_item(const index_pair& pos, std::string text)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			ess.lister.insert(pos, std::move(text), this->column_size());
			
			if (!empty())
			{
				auto & item = ess.lister.at(pos);
				item.bgcolor = bgcolor();
				item.fgcolor = fgcolor();
				ess.update();
			}
		}

		void listbox::insert_item(const index_pair& pos, const std::wstring& text)
		{
			insert_item(pos, to_utf8(text));
		}

		void listbox::insert_item(index_pair abs_pos, const listbox& rhs, const index_pairs& indexes)
		{
			auto const columns = (std::min)(this->column_size(), rhs.column_size());

			if (0 == columns)
				return;

			item_proxy it_new = this->at(abs_pos.cat).end();
			for (auto & idx : indexes)
			{
				auto it_src = rhs.at(idx.cat).at(idx.item);

				if (abs_pos.item < this->at(abs_pos.cat).size())
				{
					this->insert_item(abs_pos, it_src.text(0));
					it_new = this->at(abs_pos);
				}
				else
				{
					it_new = this->at(abs_pos.cat).append(it_src.text(0));
				}

				for (std::size_t col = 1; col < columns; ++col)
					it_new.text(col, it_src.text(col));

				++abs_pos.item;
			}
		}

		listbox::cat_proxy listbox::at(size_type pos)
		{
			internal_scope_guard lock;
			check_range(pos, size_categ());
			return{ &_m_ess(), pos };
		}

		const listbox::cat_proxy listbox::at(size_type pos) const
		{
			internal_scope_guard lock;
			check_range(pos, size_categ());
			return{ &_m_ess(), pos };
		}

		listbox::item_proxy listbox::at(const index_pair& abs_pos)
		{
			internal_scope_guard lock;
			return at(abs_pos.cat).at(abs_pos.item);
		}

		const listbox::item_proxy listbox::at(const index_pair& pos_abs) const
		{
			internal_scope_guard lock;
			return at(pos_abs.cat).at(pos_abs.item);
		}

		// Contributed by leobackes(pr#97)
		listbox::index_pair listbox::cast( const point& pos ) const
		{
			internal_scope_guard lock;
			auto & ess=_m_ess();
			auto _where = ess.where(pos);

			if (drawerbase::listbox::essence::parts::list == _where.first)
				return ess.lister.advance(ess.first_display(), static_cast<int>(_where.second));
	
			return index_pair{ npos, npos };
		}

		listbox::index_pair listbox::hovered(bool return_end) const
		{
			using parts = drawerbase::listbox::essence::parts;

			internal_scope_guard lock;

			auto cur_pos = API::cursor_position();
			API::calc_window_point(handle(), cur_pos);

			auto pt_where = _m_ess().where(cur_pos);

			if ((pt_where.first == parts::list || pt_where.first == parts::checker) && pt_where.second != npos)
			{
				auto pos = _m_ess().lister.advance(_m_ess().first_display(), static_cast<int>(pt_where.second));
				if (return_end && pos.is_category())
				{
					if (0 < pos.cat)
						--pos.cat;
					pos.item = this->size_item(pos.cat);
				}
				return pos;

			}
			else if (return_end)
				return index_pair{ this->size_categ() - 1, this->size_item(this->size_categ() - 1) };

			return index_pair{ npos, npos };
		}

		auto listbox::column_at(size_type pos, bool disp_order) -> column_interface&
		{
			internal_scope_guard lock;
			return _m_ess().header.at(pos, disp_order);
		}

		auto listbox::column_at(size_type pos, bool disp_order) const -> const column_interface&
		{
			internal_scope_guard lock;
			return _m_ess().header.at(pos, disp_order);
		}

		auto listbox::column_size() const ->size_type
		{
			internal_scope_guard lock;
			return _m_ess().header.cont().size();
		}


		void listbox::column_resizable(bool resizable)
		{
			internal_scope_guard lock;
			_m_ess().header.attrib().resizable = resizable;
		}

		bool listbox::column_resizable() const
		{
			internal_scope_guard lock;
			return _m_ess().header.attrib().resizable;
		}

		void listbox::column_movable(bool movable)
		{
			internal_scope_guard lock;
			_m_ess().header.attrib().movable = movable;
		}

		bool listbox::column_movable() const
		{
			internal_scope_guard lock;
			return _m_ess().header.attrib().movable;
		}

		//Contributed by leobackes(pr#97)
		listbox::size_type listbox::column_from_pos ( const point& pos ) const
		{
			internal_scope_guard lock;
			return _m_ess().column_from_pos(pos.x);
		}

		void listbox::checkable(bool chkable)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			if(ess.checkable != chkable)
			{
				ess.checkable = chkable;
				ess.update();
			}
		}

		auto listbox::checked() const -> index_pairs
		{
			internal_scope_guard lock;
			return _m_ess().lister.pick_items(false);
		}

		void listbox::clear(size_type cat)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();

			auto origin = ess.content_view->origin();

			int new_pos = origin.y;

			auto start_pos = static_cast<int>(ess.lister.distance(ess.lister.first(), index_pair{cat, npos}) * ess.item_height());
			auto count = static_cast<int>(ess.lister.size_item(cat) * ess.item_height());
			if (start_pos + count <= origin.y)
				new_pos = origin.y - static_cast<int>(count);
			else if (start_pos < origin.y && origin.y < start_pos + count)
				new_pos = start_pos + ess.item_height();

			ess.lister.clear(cat);

			ess.calc_content_size(false);
			ess.content_view->change_position(new_pos, false, false);
			ess.content_view->sync(false);
			ess.update();
		}

		void listbox::clear()
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();

			ess.lister.clear();
            unsort();   // appear to be expected

			ess.calc_content_size(false);
			ess.content_view->change_position(0, false, false);
			ess.content_view->sync(false);

			ess.update();
		}

		void listbox::erase(size_type cat)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();

			auto origin = ess.content_view->origin();

			auto start_pos = ess.content_position(index_pair{ cat, npos });

			int new_pos = origin.y;
			auto count = static_cast<int>((ess.lister.size_item(cat) + 1) * ess.item_height());
			if (start_pos + count <= origin.y)
				new_pos = origin.y - static_cast<int>(count);
			else if (start_pos < origin.y && origin.y < start_pos + count)
				new_pos = start_pos;

			ess.lister.erase(cat);

			ess.calc_content_size(false);
			ess.content_view->change_position(new_pos, false, false);
			ess.content_view->sync(false);

			ess.update();
		}

		void listbox::erase()
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			ess.lister.erase();
			ess.calc_content_size();
			ess.update();
		}
		
		void listbox::erase(index_pairs indexes)
		{
			internal_scope_guard lock;

			std::sort(indexes.begin(), indexes.end(), [](const index_pair& pos1, const index_pair& pos2)
			{
				return (pos1 > pos2);
			});

			auto & ess = _m_ess();

			auto const origin = ess.content_view->origin();

			for (auto & pos : indexes)
			{
				auto & cat = *ess.lister.get(pos.cat);
				if (pos.item < cat.items.size())
				{
					if (cat.model_ptr)
					{
						drawerbase::listbox::es_lister::throw_if_immutable_model(cat.model_ptr.get());
						cat.model_ptr->container()->erase(pos.item);
					}

					cat.items.erase(cat.items.begin() + pos.item);
				}
			}

			//Rebuild the sorted order
			std::size_t this_cat = this->size_categ();
			for (auto & pos : indexes)
			{
				if (this_cat != pos.cat)
				{
					this_cat = pos.cat;
					ess.lister.get(this_cat)->make_sort_order();
				}
			}

			ess.calc_content_size(false);
			ess.content_view->change_position(origin.y, false, false);
			ess.content_view->sync(false);

			ess.lister.sort();
			
			ess.update();
		}

		listbox::item_proxy listbox::erase(item_proxy ip)
		{
			if(ip.empty())
				return ip;

			internal_scope_guard lock;
			auto * ess = ip._m_ess();
			auto _where = ip.pos();

			auto origin = ess->content_view->origin();
			auto start_pos = ess->content_position(_where);
			if (start_pos < origin.y)
				origin.y -= ess->item_height();

			ess->lister.erase(_where);

			ess->calc_content_size(false);
			ess->content_view->change_position(origin.y, false, false);
			ess->content_view->sync(false);

			ess->update();
			if(_where.item < ess->lister.size_item(_where.cat))
				return ip;
			return item_proxy(ess);
		}

		bool listbox::sortable() const
		{
			internal_scope_guard lock;
			return _m_ess().header.attrib().sortable;
		}

		void listbox::sortable(bool enable)
		{
			internal_scope_guard lock;
			_m_ess().header.attrib().sortable = enable;
		}

		void listbox::set_sort_compare(size_type col, std::function<bool(const std::string&, nana::any*, const std::string&, nana::any*, bool reverse)> strick_ordering)
		{
			internal_scope_guard lock;
			_m_ess().header.at(col).weak_ordering = std::move(strick_ordering);
		}

        /// sort() and invalidates any existing reference from display position to absolute item, that is: after sort() display offset point to different items
        void listbox::sort_col(size_type col, bool reverse)
		{
			internal_scope_guard lock;
			_m_ess().lister.sort_column(col, &reverse);
		}

		auto listbox::sort_col() const -> size_type
		{
			internal_scope_guard lock;
			return _m_ess().lister.sort_attrs().column;
		}

        /// potentially invalidates any existing reference from display position to absolute item, that is: after sort() display offset point to different items
		void listbox::unsort()
		{
			internal_scope_guard lock;
			this->sort_col(npos, false);
		}

		bool listbox::freeze_sort(bool freeze)
		{
			internal_scope_guard lock;
			return !_m_ess().lister.active_sort(!freeze);
		}

		auto listbox::selected() const -> index_pairs
		{
			internal_scope_guard lock;
			return _m_ess().lister.pick_items(true);   // absolute positions, no relative to display
		}

		void listbox::show_header(bool sh)
		{
			internal_scope_guard lock;
			_m_ess().header.attrib().visible = sh;
			_m_ess().update();
		}

		bool listbox::visible_header() const
		{
			internal_scope_guard lock;
			return _m_ess().header.attrib().visible;
		}

		void listbox::move_select(bool upwards)  ///<Selects an item besides the current selected item in the display.
		{
			internal_scope_guard lock;
			_m_ess().lister.move_select(upwards, true, true);
			_m_ess().update();
		}

		listbox::size_type listbox::size_categ() const
		{
			return _m_ess().lister.cat_container().size();
		}

		listbox::size_type listbox::size_item(size_type categ) const
		{
			return _m_ess().lister.size_item(categ);
		}

		void listbox::enable_single(bool for_selection, bool category_limited)
		{
			internal_scope_guard lock;
			_m_ess().lister.enable_single(for_selection, category_limited);
		}

		void listbox::disable_single(bool for_selection)
		{
			internal_scope_guard lock;
			_m_ess().lister.disable_single(for_selection);
		}

		bool listbox::is_single_enabled(bool for_selection) const noexcept
		{
			internal_scope_guard lock;
			return _m_ess().lister.is_single_enabled(for_selection);
		}

        listbox::export_options& listbox::def_export_options()
        {
			return _m_ess().def_exp_options;
        }

		listbox& listbox::category_icon(std::function<void(paint::graphics& graph, const rectangle& rt_icon, bool expanded)> icon_renderer)
		{
			internal_scope_guard lock;
			_m_ess().ctg_icon_renderer.swap(icon_renderer);
			_m_ess().update();
			return *this;
		}

		listbox& listbox::category_icon(const paint::image& img_expanded, const paint::image& img_collapsed)
		{
			internal_scope_guard lock;
			_m_ess().ctg_icon_renderer = [img_expanded, img_collapsed](paint::graphics& graph, const rectangle& rt_icon, bool expanded)
			{
				if (expanded)
				{
					img_expanded.stretch(rectangle{ img_expanded.size() }, graph, rt_icon);
				}
				else
				{
					img_collapsed.stretch(rectangle{ img_collapsed.size() }, graph, rt_icon);
				}
			};

			_m_ess().update();
			return *this;
		}

		auto listbox::first_visible() const ->index_pair
		{
			return _m_ess().first_display();
		}

		auto listbox::last_visible() const -> index_pair
		{
			return _m_ess().lister.advance(_m_ess().first_display(), static_cast<int>(_m_ess().count_of_exposed(true)));
		}

		auto listbox::visibles() const -> index_pairs
		{
			index_pairs indexes;

			auto idx = _m_ess().first_display();

			auto n = _m_ess().count_of_exposed(true);
			while (n > 0)
			{
				if (idx.empty())
					break;

				if (idx.is_category())
				{
					indexes.push_back(idx);
					--n;
				}
				else
				{
					auto const count = (std::min)(_m_ess().lister.size_item(idx.cat) - idx.item, n);
					for (std::size_t i = 0; i < count; ++i)
					{
						indexes.push_back(idx);
						++idx.item;
					}
					if (count)
					{
						n -= count;
						--idx.item;
					}
				}

				idx = _m_ess().lister.advance(idx, 1);
			}

			return indexes;
		}

		void listbox::set_deselect(std::function<bool(nana::mouse)> predicate)
		{
			_m_ess().pred_msup_deselect = std::move(predicate);
		}

		unsigned listbox::suspension_width() const
		{
			nana::internal_scope_guard lock;
			return _m_ess().suspension_width();
		}

		drawerbase::listbox::essence & listbox::_m_ess() const
		{
			return get_drawer_trigger().ess();
		}

		nana::any* listbox::_m_anyobj(size_type cat, size_type index, bool allocate_if_empty) const
		{
			return _m_ess().lister.anyobj(index_pair{cat, index}, allocate_if_empty);
		}

		drawerbase::listbox::category_t* listbox::_m_assoc(std::shared_ptr<nana::detail::key_interface> ptr, bool create_if_not_exists)
		{
			auto & ess = _m_ess();

			internal_scope_guard lock;

			for (auto & m : ess.lister.cat_container())
			{
				if (m.key_ptr && nana::detail::pred_equal(ptr.get(), m.key_ptr.get()))
					return &m;
			}

			if (!create_if_not_exists)
				return nullptr;

			drawerbase::listbox::category_t* cat;

			if (ess.lister.enable_ordered())
			{
				cat = ess.lister.create_category(ptr);
			}
			else
			{
				cat = ess.lister.create_category(native_string_type{});
				cat->key_ptr = ptr;
			}
			ess.update();
			return cat;
		}

		void listbox::_m_erase_key(nana::detail::key_interface* p) noexcept
		{
			auto & cont = _m_ess().lister.cat_container();

			internal_scope_guard lock;
			for (auto i = cont.begin(); i != cont.end(); ++i)
			{
				if (i->key_ptr && nana::detail::pred_equal(p, i->key_ptr.get()))
				{
					cont.erase(i);
					return;
				}
			}
		}

		std::shared_ptr<scroll_operation_interface> listbox::_m_scroll_operation()
		{
			internal_scope_guard lock;
			return _m_ess().content_view->scroll_operation();
		}

		/// Move column to view_position
		void listbox::move_column(size_type abs_pos, size_type view_pos)
		{
			internal_scope_guard lock;
			return _m_ess().header.move_to_view_pos(abs_pos, view_pos, true);
			_m_ess().update();
		}

		/// Sort columns in range first_col to last_col inclusive using a row
		void listbox::reorder_columns(size_type first_col,
									  size_type last_col,
									  index_pair row, bool reverse,
									  std::function<bool(const std::string &cell1, size_type col1,
														 const std::string &cell2, size_type col2,
														 const nana::any *rowval,
														 bool reverse)> comp)
		{
			if (last_col <= first_col || last_col >= column_size())
				return;
			
			std::vector<size_type> new_idx;
			for(size_type i=first_col; i<=last_col; ++i) new_idx.push_back(i);

			internal_scope_guard lock;
			auto ip_row = this->at(row);
			auto pnany=_m_ess().lister.anyobj(row,false);
			std::sort(new_idx.begin(), new_idx.end(), [&](size_type col1,
														  size_type col2)
			{
				return comp(ip_row.text(col1), col1,
						    ip_row.text(col2), col2,
						    pnany, reverse);
			});

			//Only change the view position of columns
			for(size_t i=0; i<new_idx.size(); ++i)
			{
				_m_ess().header.move_to_view_pos(new_idx[i], i + first_col, true);
			}
			_m_ess().update();
		}

	//end class listbox
}//end namespace nana
