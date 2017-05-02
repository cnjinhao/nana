/*
 *	A List Box Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
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
 *
 */

#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/panel.hpp>	//for inline widget

#include <nana/gui/layout_utility.hpp>
#include <nana/gui/element.hpp>
#include <nana/paint/text_renderer.hpp>
#include <nana/system/dataexch.hpp>
#include <nana/system/platform.hpp>
#include "skeletons/content_view.hpp"

#include <algorithm>
#include <list>
#include <deque>
#include <stdexcept>
#include <map>

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
				struct column
					: public column_interface
				{
					native_string_type caption;
					unsigned width_px;
					std::pair<unsigned, unsigned> range_width_px;
					bool visible_state{ true };

					/// Absolute position of column when it was creating
					size_type index;

					nana::align alignment{ nana::align::left };

					std::function<bool(const std::string&, nana::any*, const std::string&, nana::any*, bool reverse)> weak_ordering;


					column() = default;
					
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
					//The definition is provided after essence
					void _m_refresh() noexcept;
				private:
					essence* const ess_;
				public:
					//Implementation of column_interface
					unsigned width() const noexcept override
					{
						return width_px;
					}

					// Sets the width and overrides the ranged width
					void width(unsigned pixels) noexcept override
					{
						width_px = pixels;
						range_width_px.first = range_width_px.second = 0;

						_m_refresh();
					}

					void width(unsigned minimum, unsigned maximum)
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

					size_type position(bool disp_order) const noexcept override;	//The definition is provided after essence

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

				bool visible() const noexcept
				{
					return visible_;
				}

				bool visible(bool v) noexcept
				{
					if (visible_ == v)
						return false;

					visible_ = v;
					return true;
				}

				bool sortable() const noexcept
				{
					return sortable_;
				}

				void sortable(bool enable) noexcept
				{
					sortable_ = enable;
				}

				size_type create(essence* ess, native_string_type&& text, unsigned pixels)
				{
					cont_.emplace_back(ess, std::move(text), pixels, static_cast<size_type>(cont_.size()));
                    return cont_.back().index;
				}

				unsigned pixels() const noexcept  ///< the visible width of the whole header
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

					for (auto & col : cont_)
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

                /// find and return a ref to the column that originaly was at position "pos" previous to any list reorganization.
				column& at(size_type pos, bool disp_order = false)
				{
					check_range(pos, cont_.size());

					if (!disp_order)
						pos = this->cast(pos, false);
					
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

				/// Returns the left point position and width(in variable *pixels) of column originaly at position pos.
				int position(size_type pos, unsigned * pixels) const
				{
					int left = 0;
					for (auto & m : cont_)
					{
						if (m.index == pos)
						{
							if (pixels)
								*pixels = m.width_px;
							break;
						}

						if (m.visible_state)
							left += m.width_px;
					}
					return left;
				}

				/// return the original index of the visible col currently before(in front of) or after the col originaly at index "index"
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
                
				/// move the col originaly at "from" to the position currently in front (or after) the col originaly at index "to" invalidating some current index
				void move(size_type from, size_type to, bool front) noexcept
				{
					if ((from == to) || (from >= cont_.size()) || (to >= cont_.size()))
						return;
					
					for (auto i = cont_.begin(); i != cont_.end(); ++i)
					{
						if (from == i->index)
						{
							auto col_from = std::move(*i);
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
				bool visible_{true};
				bool sortable_{true};
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

						//Use the model cells instead if model cells is avaiable
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

				bool expand{true};

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

				std::vector<inline_pane*> get_inline_pane(const index_pair& item_pos)
				{
					std::vector<inline_pane*> panes;
					for (auto p : active_panes_)
					{
						if (p && (p->item_pos == item_pos))
						{
							panes.emplace_back(p);
						}
					}
					return panes;
				}

				void emit_cs(const index_pair& pos, bool for_selection)
				{
					item_proxy i(ess_, pos);
					arg_listbox arg{ i };

					auto & events = wd_ptr()->events();
					
					if (for_selection)
						events.selected.emit(arg, wd_ptr()->handle());
					else
						events.checked.emit(arg, wd_ptr()->handle());

					auto panes = get_inline_pane(pos);
					for (auto p : panes)
					{
						if(for_selection)
							p->inline_ptr->notify_status(inline_widget_status::selecting, i.selected());
						else
							p->inline_ptr->notify_status(inline_widget_status::checking, i.checked());
					}
				}

				// Definition is provided after struct essence
				unsigned column_content_pixels(size_type pos) const;

				const sort_attributes& sort_attrs() const noexcept
				{
					return sort_attrs_;
				}

                /// each sort() ivalidate any existing reference from display position to absolute item, that is after sort() display offset point to different items
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
					if (npos == pos)
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

				void scroll(const index_pair& pos, bool to_bottom);

				/// Append a new category with a specified name and return a pointer to it.
				category_t* create_cat(native_string_type&& text)
				{
					categories_.emplace_back(std::move(text));
					return &categories_.back();
				}

		        /// will use the key to insert new cat before the first cat with compare less than the key, or at the end of the list of cat and return a ref to that new cat.  ?
				category_t* create_cat(std::shared_ptr<nana::detail::key_interface>& ptr)
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

					categories_.emplace_back();
					categories_.back().key_ptr = ptr;
					return &(categories_.back());
				}
                
				/// add a new cat created at "pos" and return a ref to it
				category_t* create_cat(std::size_t pos, native_string_type&& text)
				{
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
					auto cat = get(from.cat);
					if (from.item < cat->sorted.size())
					{
						if (from_display_order)
							return index_pair{ from.cat, static_cast<size_type>(cat->sorted[from.item]) };

						for (size_type i = 0; i < cat->sorted.size(); ++i)
						{
							if (from.item == cat->sorted[i])
								return index_pair{ from.cat, i };
						}
					}
					throw std::out_of_range("listbox: invalid item position");
				}
				
				index_pair index_cast_noexcpt(const index_pair& from, bool from_display_order, const index_pair& default_value = index_pair{npos, npos}) const noexcept
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

                /// return a ref to the real item object at display!!! position pos using current sorting only if it is active, and at absolute position if no sorting is currently active.
				category_t::container::value_type& at(const index_pair& pos)
				{
					auto acc_pos = pos.item;
					if (npos != sort_attrs_.column)
						acc_pos = index_cast(pos, true).item;	//convert display position to absolute position

					return get(pos.cat)->items.at(acc_pos);
				}

				const category_t::container::value_type& at(const index_pair& pos) const
				{
					auto acc_pos = pos.item;
					if (npos != sort_attrs_.column)
						acc_pos = index_cast(pos, true).item;	//convert display position to absolute position

					return get(pos.cat)->items.at(acc_pos);
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
					if(good(cat) && cat)
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

				template<typename Pred>
				std::vector<std::pair<index_pair, bool>> select_display_range_if(index_pair fr_abs, index_pair to_dpl, bool unselect_others, Pred pred)
				{
					const auto already_selected = this->pick_items(true);

					auto fr_dpl = this->index_cast(fr_abs, false);	//Converts an absolute position to display position
                    if (fr_dpl > to_dpl)
						std::swap(fr_dpl, to_dpl);

					const auto begin = fr_dpl;
					const auto last = to_dpl;

					//pair first: index in the range of [begin, last]
					//pair second: indicates whether the index is selected before selection.
					std::vector<std::pair<index_pair, bool>> pairs;

					for (; fr_dpl != to_dpl; fr_dpl = advance(fr_dpl, 1))
					{
						if (!fr_dpl.is_category())
						{
							auto abs_pos = index_cast(fr_dpl, true);	//convert display position to absolute position
							item_proxy m{ ess_, abs_pos };
							pairs.emplace_back(abs_pos, m.selected());

							if (pred(abs_pos))
								m.select(true);
						}
					}

					if (!to_dpl.is_category())
					{
						auto abs_pos = index_cast(to_dpl, true);	//convert display position to absolute position

						item_proxy m(ess_, abs_pos);
						pairs.emplace_back(abs_pos, m.selected());

						if (pred(abs_pos))
							m.select(true);
					}

					if (unselect_others)
					{
						//Unselects the already selected which is out of range [begin, last] 
						for (auto index : already_selected)
						{
							auto disp_order = this->index_cast(index, false);	//converts an absolute position to a display position
							if (begin > disp_order || disp_order > last)
								item_proxy{ ess_, index }.select(false);
						}
					}

					return pairs;
				}

				bool select_for_all(bool sel, const index_pair& except = index_pair{npos, npos})
				{
					bool changed = false;
					index_pair pos;
					for (auto & cat : categories_)
					{
						pos.item = 0;
						for(auto & m : cat.items)
						{
							if (except != pos)
							{
								if (m.flags.selected != sel)
								{
									changed = true;
									m.flags.selected = sel;

									this->emit_cs(pos, true);

									if (m.flags.selected)
										last_selected_abs = pos;
									else if (last_selected_abs == pos)
										last_selected_abs.set_both(npos);		//make empty
								}
							}
							++pos.item;
						}
						++pos.cat;
					}
					return changed;
				}

				/// return absolute positions, no relative to display
				index_pairs pick_items(bool for_selection) const
				{
					index_pairs results;
					index_pair id;

					for (auto & cat : categories_)
					{
						id.item = 0;
						for (auto & m : cat.items)
						{
							if (for_selection ? m.flags.selected : m.flags.checked)
								results.push_back(id);  // absolute positions, no relative to display
							++id.item;
						}
						++id.cat;
					}
					return results;
				}

				/// return absolute positions, no relative to display
				bool item_selected_all_checked(index_pairs& vec) const
				{
					index_pair id;
					bool ck = true;

					for (auto & cat : categories_)
					{
						id.item = 0;
						for (auto & m : cat.items)
						{
							if (m.flags.selected)
							{
								vec.push_back(id);  // absolute positions, no relative to display
								ck &= m.flags.checked;
							}
							++id.item;
						}
						++id.cat;
					}

					//Just returns true when the all selected items are checked.
					return ck;
				}

                ///<Selects an item besides the current selected item in the display.
                /// we are moving in display, but the selection ocurre in abs position
                void move_select(bool upwards=true, bool unselect_previous=true, bool trace_selected=false) noexcept;

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
					else if (i.item)
						--i.item;

					return i;
				}

                /// can be used as the absolute position of the first absolute item, or as the display pos of the first displayed item
                index_pair first() const noexcept
                {
                    index_pair fst{0,npos};
                    good_item(fst,fst);
                    return fst;
                }
				
				bool good(size_type cat) const noexcept
				{
					return (cat < categories_.size());
				}

				bool good(const index_pair& pos) const noexcept
				{
					return ((pos.cat < categories_.size()) && (pos.item < size_item(pos.cat)));
				}

                /// if good return the same item (in arg item), or just the next cat and true, but If fail return false
				bool good_item(const index_pair& pos, index_pair& item) const noexcept
				{
					if (!good(pos.cat))
						return false;       // cat out of range

					if (pos.is_category())
					{
						item = pos;          // return the cat self
						if (0 == pos.cat)    // but for cat 0 return first item
							item.item = 0;   // let check this is good
                        else
						    return true;
					}

					auto i = get(pos.cat);         // pos is not a cat and i point to it cat
					if (pos.item < i->items.size())
					{
						item = pos;                // good item, return it
						return true;
					}

					if (++i == categories_.cend())          // item out of range and no more cat
						return false;

					item.cat = pos.cat + 1;         // select the next cat
					item.item = npos;
					return true;
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
				index_pair last_selected_abs;
			private:
				essence * ess_{nullptr};
				nana::listbox * widget_{nullptr};

				sort_attributes sort_attrs_;	//Attributes of sort

				bool	ordered_categories_{false};	///< A switch indicates whether the categories are ordered.
												/// The ordered categories always creates a new category at a proper position(before the first one which is larger than it).
				container categories_;

				bool single_selection_{ false };
				bool single_selection_category_limited_{ false };
				bool single_check_{ false };
				bool single_check_category_limited_{ false };

				std::vector<inline_pane*> active_panes_;
			};//end class es_lister


			/// created and live by the trigger, holds data for listbox: the state of the struct does not effect on member funcions, therefore all data members are public.
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
				es_lister lister;  // we have at least one emty cat. the #0

				item_state ptr_state{ item_state::normal };
				std::pair<parts, std::size_t> pointer_where;	//The 'first' stands for which object, such as header and lister, 'second' stands for item
																//if where == header, 'second' indicates the item
																//if where == lister || where == checker, 'second' indicates the offset to the scroll offset_y which stands for the first item displayed in lister.
																//if where == unknown, 'second' ignored.

				std::unique_ptr<widgets::skeletons::content_view> content_view;

				struct mouse_selection_part
				{
					bool	started{ false };
					bool	reverse_selection{ false };

					point	screen_pos;
					point	begin_position;	///< Logical position to the 
					point	end_position;
					index_pairs already_selected;
					index_pairs selections;

					bool scroll_direction;
					unsigned scroll_step{ 1 };
					unsigned mouse_move_timestamp{ 0 };
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
					auto offset_display = content_view->origin().y / item_height();
					return lister.advance(lister.first(), offset_display);
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

				struct pred_mouse_selection
				{
					index_pair pos_;
					pred_mouse_selection(index_pair pos) noexcept
						: pos_(pos)
					{}

					bool operator()(const std::pair<index_pair, bool>& m) const noexcept
					{
						return (pos_ == m.first);
					}
				};

				std::pair<int, int> columns_range() const
				{
					rectangle r;
					if (!rect_header(r))
						return{};

					auto origin = content_view->origin();
					return{ r.x - origin.x, r.x - origin.x + static_cast<int>(header.pixels()) };
				}

				void start_mouse_selection(const point& screen_pos)
				{
					auto logic_pos = coordinate_cast(screen_pos, true);

					mouse_selection.started = true;
					mouse_selection.begin_position = logic_pos;
					mouse_selection.end_position = logic_pos;

					mouse_selection.already_selected = lister.pick_items(true);

					API::set_capture(*listbox_ptr, true);
				}

				void update_mouse_selection(const point& screen_pos)
				{
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

					auto content_x = coordinate_cast({ columns_range().first, 0 }, true).x;
					if ((std::max)(mouse_selection.end_position.x, mouse_selection.begin_position.x) <= content_x ||
						(std::min)(mouse_selection.end_position.x, mouse_selection.begin_position.x) >= content_x + static_cast<int>(header.pixels())
						)
					{
						lister.select_for_all(false);
						return;
					}

					auto begin_off = (std::max)((std::min)(mouse_selection.begin_position.y, mouse_selection.end_position.y), 0) / item_height();
					auto last_off = (std::max)(mouse_selection.begin_position.y, mouse_selection.end_position.y) / item_height();

					auto begin = lister.advance(lister.first(), begin_off);
					auto last = lister.advance(lister.first(), last_off);

					if (!lister.good(last))
						last = lister.last();

					if (lister.good(begin) && ((mouse_selection.end_position.y < 0) || (lister.distance(lister.first(), begin) == begin_off)))
					{
						auto selections = lister.select_display_range_if(begin, last, false, [this](const index_pair& abs_pos) {
							if (this->mouse_selection.reverse_selection)
							{
								if(mouse_selection.already_selected.cend() != std::find(mouse_selection.already_selected.cbegin(), mouse_selection.already_selected.cend(), abs_pos))
								{
									item_proxy{ this, abs_pos }.select(false);
									return false;
								}
							}
							return true;
						});

						for (auto & pair : selections)
						{
							if (pair.second)
								continue;

							if (mouse_selection.selections.cend() ==
								std::find(mouse_selection.selections.cbegin(), mouse_selection.selections.cend(), pair.first))
							{
								mouse_selection.selections.push_back(pair.first);
							}
						}

#ifdef _MSC_VER
						for(auto i = mouse_selection.selections.cbegin(); i != mouse_selection.selections.cend();)
#else
						for(auto i = mouse_selection.selections.begin(); i != mouse_selection.selections.end();)

#endif
						{
							if (selections.cend() == std::find_if(selections.cbegin(), selections.cend(), pred_mouse_selection{*i}))
							{
								item_proxy{ this, *i }.select(false);
								i = mouse_selection.selections.erase(i);
							}
							else
								++i;
						}

						if (mouse_selection.reverse_selection)
						{
							for (auto & abs_pos : mouse_selection.already_selected)
							{
								if (selections.cend() == std::find_if(selections.cbegin(), selections.cend(), pred_mouse_selection{abs_pos}))
								{
									item_proxy{ this, abs_pos }.select(true);
								}
							}
						}
					}
					else
					{
						for (auto & pos : mouse_selection.selections)
						{
							item_proxy{ this, pos }.select(false);
						}

						mouse_selection.selections.clear();

						if (mouse_selection.reverse_selection)
						{
							for (auto & abs_pos : mouse_selection.already_selected)
							{
								item_proxy{ this, abs_pos }.select(true);
							}
						}
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
						this->header.pixels() + 5,
						static_cast<size::value_type>(this->lister.the_number_of_expanded())
					);

					ctt_size.height *= this->item_height();

					this->content_view->content_size(ctt_size, try_update);

					return ctt_size;
				}

				nana::rectangle checkarea(int x, int y) const noexcept /// move to scheme ?? 16 ?
				{
					return nana::rectangle(x + 4, y + (static_cast<int>(item_height()) - 16) / 2, 16, 16);
				}

				int item_xpos(const nana::rectangle& r) const
				{
					auto seq = header_seq(r.width);

					if (seq.empty())
						return 0;

					return (header.position(seq[0], nullptr) - this->content_view->origin().x + r.x);
				}

				//Returns the absolute coordinate of the specified item in the window
				point item_coordinate(const index_pair& pos) const
				{
					auto top = static_cast<int>(this->lister.distance(index_pair{}, pos) * item_height()) - content_view->origin().y;

					rectangle r;
					if (rect_lister(r))
						top += r.y;

					return{ top, top + static_cast<int>(item_height()) };
				}

				std::pair<parts, size_t> where(const nana::point& pos) const noexcept
				{
					std::pair<parts, size_t> new_where{ parts::unknown, npos };

					const auto area = this->content_area();

					if(area.is_hit(pos))
					{   /// we are inside
						auto const origin = content_view->origin();

						if(header.visible() && (pos.y < static_cast<int>(scheme_ptr->header_height) + area.y))
						{   /// we are in the header
							new_where.first = parts::header;
							new_where.second = this->column_from_pos(pos.x);
						}
						else if (area.x <= pos.x + origin.x && pos.x + origin.x < area.x + static_cast<int>(header.pixels()))
						{
							new_where.first = parts::list;

							auto const item_h = item_height();
							//don't combine the following formula into the (pos.y - area.y - header_visible_px()) / item_h
							new_where.second = ((pos.y - area.y - header_visible_px() + origin.y) / item_h) - (origin.y / item_h);

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

				unsigned header_visible_px() const
				{
					return (header.visible() ? scheme_ptr->header_height : 0);
				}

				bool rect_header(nana::rectangle& r) const
				{
					if(header.visible())
					{
						r = this->content_area();

						r.height = scheme_ptr->header_height;

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

				std::vector<size_type> header_seq(unsigned lister_w) const
				{
					std::vector<size_type> seqs;
					int x = -content_view->origin().x;

					for (const auto& col : header.cont())
					{
						if (!col.visible_state)
							continue;

						x += col.width_px;
						if (x > 0)
							seqs.push_back(col.index);

						if (x >= static_cast<int>(lister_w))
							break;
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
				cells_.emplace_back();
				cells_.back().text.assign(1, wchar_t(0));	//means invalid cell
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

							content_px = ess_->graph->text_extent_size(model_cells[pos].text).width;
						}
						else
						{
							if (pos >= cat.items[i].cells->size())
								continue;

							content_px = ess_->graph->text_extent_size((*cat.items[i].cells)[pos].text).width;
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

				void hovered(index_type /*pos*/) override
				{
					auto offset = ess_->content_view->origin().y / ess_->item_height();

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

			void es_lister::scroll(const index_pair& pos, bool to_bottom)
			{
				auto& cat = *get(pos.cat);

				if ((pos.item != nana::npos) && (pos.item >= cat.items.size()))
					throw std::invalid_argument("listbox: invalid pos to scroll");

				if (!cat.expand)
				{
					this->expand(pos.cat, true);
					ess_->calc_content_size();
				}

				ess_->content_view->turn_page(to_bottom, false);
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

			void es_lister::move_select(bool upwards, bool unselect_previous, bool /*trace_selected*/) noexcept
			{
				auto next_selected_dpl = index_cast_noexcpt(last_selected_abs, false);	//convert absolute position to display position

				if (next_selected_dpl.empty())  // has no cat ? (cat == npos) => beging from first cat
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

					last_selected_abs.cat = pos;
					last_selected_abs.item = npos;

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
				bool detect_splitter(const nana::rectangle& r, int x) noexcept
				{
					if(essence_->ptr_state == item_state::highlighted)
					{
						x -= r.x - essence_->content_view->origin().x;

						for(auto & col : essence_->header.cont()) // in current order
						{
							if(col.visible_state)
							{
								auto col_pixels = static_cast<int>(col.width_px);

								if ((col_pixels < x + static_cast<int>(essence_->scheme_ptr->header_splitter_area_before))
									&& (x < col_pixels + static_cast<int>(essence_->scheme_ptr->header_splitter_area_after)))
								{
									grabs_.splitter = col.index; // original index
									return true;
								}
								x -= col_pixels;
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
							new_w = (std::max)(new_w, essence_->scheme_ptr->suspension_width + essence_->scheme_ptr->min_column_width);
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
					//check whether grabing an item, if item_spliter_ != npos, that indicates the grab item is a spliter.
					if ((parts::header == essence_->pointer_where.first) && (npos == grabs_.splitter))
						state = essence_->ptr_state;

					rectangle column_r{
						r.x - essence_->content_view->origin().x, r.y,
						0, r.height - 1
					};

					for (auto & col : essence_->header.cont())
					{
						if (col.visible_state)
						{
							column_r.width = col.width_px;

							const auto right_pos = column_r.right();

							//Make sure the column is in the display area.
							if (right_pos > r.x)
							{
								_m_draw_header_item(graph, column_r, text_color, col, (col.index == essence_->pointer_where.second ? state : item_state::normal));
								graph.line({ right_pos - 1, r.y }, { right_pos - 1, r.bottom() - 2 }, border_color);
							}

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

					if(i == npos)
						i = essence_->header.boundary(essence_->header.position(grab, nullptr) >= x);

					if(grab != i)
					{
						unsigned item_pixels = 0;
						auto item_x = essence_->header.position(i, &item_pixels);

						//Get the item pos
						//if mouse pos is at left of an item middle, the pos of itself otherwise the pos of the next.
						place_front = (x <= (item_x + static_cast<int>(item_pixels / 2)));
						x = (place_front ? item_x : essence_->header.position(essence_->header.next(i), nullptr));

						if (npos != i)
							essence_->graph->rectangle({x - x_offset + rect.x, rect.y, 2, rect.height}, true, colors::red);

						return i;
					}
					return npos;
				}

				void _m_draw_header_item(graph_reference graph, const rectangle& column_r, const ::nana::color& fgcolor, const es_header::column& column, item_state state)
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

						point text_pos{ column_r.x, (static_cast<int>(essence_->scheme_ptr->header_height) - static_cast<int>(essence_->text_height)) / 2 };

						if (align::left == column.alignment)
							text_pos.x += text_margin;
						else if (align::center == column.alignment)
							text_margin = 0;

						text_aligner.draw(column.caption, text_pos, column_r.width - text_margin);
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

					paint::graphics fl_graph({ col.width_px, essence_->scheme_ptr->header_height });

					fl_graph.typeface(essence_->graph->typeface());

					_m_draw_header_item(fl_graph, rectangle{ fl_graph.size()}, colors::white, col, item_state::floated);

					auto xpos = essence_->header.position(col.index, nullptr) + pos.x - grabs_.start_pos;
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

				void draw(const nana::rectangle& rect)
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

					auto const header_w = essence_->header.pixels();
					auto const item_height_px = essence_->item_height();

					auto origin = essence_->content_view->origin();
					if (header_w < origin.x + rect.width)
					{
						rectangle r{ point{ rect.x + static_cast<int>(header_w)-origin.x, rect.y },
							size{ rect.width + origin.x - header_w, rect.height } };
						
						if (!API::dev::copy_transparent_background(essence_->listbox_ptr->handle(), r, *essence_->graph, r.position()))
							essence_->graph->rectangle(r, true);
					}

					es_lister & lister = essence_->lister;

					auto & ptr_where = essence_->pointer_where;

					int item_top = rect.y - (origin.y % item_height_px);
					auto first_disp = essence_->first_display();

					// The first display is empty when the listbox is empty.
					if (!first_disp.empty())
					{
						index_pair hoverred_pos(npos, npos);	//the hoverred item.

						//if where == lister || where == checker, 'second' indicates the offset to the  relative display-order pos of the scroll offset_y which stands for the first item to be displayed in lister.
						if ((ptr_where.first == parts::list || ptr_where.first == parts::checker) && ptr_where.second != npos)
						{
							hoverred_pos = lister.advance(first_disp, static_cast<int>(ptr_where.second));
						}

						auto subitems = essence_->header_seq(rect.width);

						if (subitems.empty())
							return;

						int txtoff = essence_->scheme_ptr->item_height_ex / 2;

						auto i_categ = lister.get(first_disp.cat);

						auto idx = first_disp;

						essence_->inline_buffered_table.swap(essence_->inline_table);

						for (auto & cat : lister.cat_container())
							for (auto & ind : cat.indicators)
							{
								if (ind)
									ind->detach();
							}

						const int x = essence_->item_xpos(rect);

						//Here we draw the root categ (0) or a first item if the first drawing is not a categ.(item!=npos))
						if (idx.cat == 0 || !idx.is_category())
						{
							if (idx.cat == 0 && idx.is_category())  // the 0 cat
							{
								first_disp.item = 0;
								idx.item = 0;
							}

							std::size_t size = i_categ->items.size();
							for (std::size_t offs = first_disp.item; offs < size; ++offs, ++idx.item)
							{
								if (item_top >= rect.bottom())
									break;

								auto item_pos = lister.index_cast(index_pair{ idx.cat, offs }, true);	//convert display position to absolute position

								_m_draw_item(*i_categ, item_pos, x, item_top, txtoff, header_w, rect, subitems, bgcolor, fgcolor,
									(hoverred_pos == idx ? item_state::highlighted : item_state::normal)
								);

								item_top += item_height_px;
							}

							++i_categ;
							++idx.cat;
						}

						for (; i_categ != lister.cat_container().end(); ++i_categ, ++idx.cat)
						{
							if (item_top > rect.bottom())
								break;

							idx.item = 0;

							_m_draw_categ(*i_categ, rect.x - origin.x, item_top, txtoff, header_w, rect, bgcolor, 
									(hoverred_pos.is_category() && (idx.cat == hoverred_pos.cat) ? item_state::highlighted : item_state::normal)
								);
							item_top += item_height_px;

							if (false == i_categ->expand)
								continue;

							auto size = i_categ->items.size();
							for (decltype(size) pos = 0; pos < size; ++pos)
							{
								if (item_top > rect.bottom())
									break;

								auto item_pos = lister.index_cast(index_pair{ idx.cat, pos }, true);	//convert display position to absolute position

								_m_draw_item(*i_categ, item_pos, x, item_top, txtoff, header_w, rect, subitems, bgcolor, fgcolor,
									(idx == hoverred_pos ? item_state::highlighted : item_state::normal)
								);

								item_top += item_height_px;
								if (item_top >= rect.bottom())
									break;

								++idx.item;
							}
						}

						essence_->inline_buffered_table.clear();
					}

					if (item_top < rect.bottom())
					{
						rectangle bground_r{ rect.x, item_top, rect.width, static_cast<unsigned>(rect.bottom() - item_top) };
						if (!API::dev::copy_transparent_background(essence_->listbox_ptr->handle(), bground_r, *essence_->graph, bground_r.position()))
							essence_->graph->rectangle(bground_r, true, bgcolor);
					}

					//Draw mouse selection
					//Check if the mouse selection box is present.
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
				void _m_draw_categ(const category_t& categ, int x, int y, int txtoff, unsigned width, const nana::rectangle& r, nana::color bgcolor, item_state state)
				{
					const auto item_height = essence_->item_height();

					rectangle bground_r{ x, y, width, item_height };
					auto graph = essence_->graph;

					item_data item;
					item.flags.selected = categ.selected();

					this->_m_draw_item_bground(bground_r, bgcolor, {}, state, item);

					color txt_color{ static_cast<color_rgb>(0x3399) };

					facade<element::arrow> arrow("double");
					arrow.direction(categ.expand ? ::nana::direction::north : ::nana::direction::south);
					arrow.draw(	*graph, {}, txt_color,
								{ x + 5, y + static_cast<int>(item_height - 16) / 2, 16, 16 },
								element_state::normal);

					graph->string({ x + 20, y + txtoff }, categ.text, txt_color);

					native_string_type str = to_nstring('(' + std::to_string(categ.items.size()) + ')');

					auto text_s = graph->text_extent_size(categ.text).width;
					auto extend_text_w = text_s + graph->text_extent_size(str).width;

					graph->string({ x + 25 + static_cast<int>(text_s), y + txtoff }, str);

					if (35 + extend_text_w < width)
					{
						::nana::point pos{ x + 30 + static_cast<int>(extend_text_w), y + static_cast<int>(item_height) / 2 };

						graph->line(pos, { x + static_cast<int>(width) - 5, pos.y },
									txt_color);
					}

					//Draw selecting inner rectangle
					if (item.flags.selected && (categ.expand == false))
						_m_draw_item_border(r.x, y, (std::min)(r.width, width - essence_->content_view->origin().x));
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
					              const int x,                           ///< left coordinate ?
					              const int y,                           ///< top coordinate 
					              const int txtoff,                      ///< below y to print the text
					              unsigned width, 
					              const nana::rectangle& content_r,      ///< the rectangle where the full list content have to be drawn
					              const std::vector<size_type>& seqs,    ///< columns to print
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

					const unsigned show_w = (std::min)(content_r.width, width - essence_->content_view->origin().x);

					auto graph = essence_->graph;

					//draw the background for the whole item
					rectangle bground_r{ content_r.x, y, show_w, essence_->item_height() };
					auto const state_bgcolor = this->_m_draw_item_bground(bground_r, bgcolor, {}, state, item);

					int column_x = x;

					for (size_type display_order{ 0 }; display_order < seqs.size(); ++display_order)  // get the cell (column) index in the order headers are displayed
					{
						const auto column_pos = seqs[display_order];
						const auto & col = essence_->header.at(column_pos);     // deduce the corresponding header which is in a kind of dislay order

						if (col.width_px > essence_->scheme_ptr->text_margin)
						{
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
										img_r.x = content_pos + column_x + (16 - static_cast<int>(item.img_show_size.width)) / 2;  // center in 16 - geom scheme?
										img_r.y = y + (static_cast<int>(essence_->item_height()) - static_cast<int>(item.img_show_size.height)) / 2; // center
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
									if (::nana::overlap(content_r, { wdg_x, y, wdg_w, essence_->item_height() }, pane_r))
									{
										::nana::point pane_pos;
										if (wdg_x < content_r.x)
											pane_pos.x = wdg_x - content_r.x;

										if (y < content_r.y)
											pane_pos.y = y - content_r.y;

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

									bground_r = rectangle{ column_x, y, col.width_px, essence_->item_height() };
									col_bgcolor = this->_m_draw_item_bground(bground_r, bgcolor, m_cell.custom_format->bgcolor, state, item);
								}
								else
									col_bgcolor = state_bgcolor;

								if (draw_column)
								{
									paint::aligner text_aligner{*graph, col.alignment};

									unsigned text_margin_right = 0;
									if (align::left == col.alignment)
										content_pos += essence_->scheme_ptr->text_margin;
									else if (align::right == col.alignment)
										text_margin_right = essence_->scheme_ptr->text_margin;

									graph->palette(true, col_fgcolor);
									text_aligner.draw(m_cell.text, { column_x + content_pos, y + txtoff }, col.width_px - content_pos - text_margin_right);
								}
							}

							if (0 == display_order)
							{
								if (essence_->checkable)
									crook_renderer_.draw(*graph, col_bgcolor, col_fgcolor, essence_->checkarea(column_x, y), estate);
								if (item.img)
									item.img.stretch(rectangle{ item.img.size() }, *graph, img_r);
							}

							graph->line({ column_x - 1, y }, { column_x - 1, y + static_cast<int>(essence_->item_height()) - 1 }, static_cast<color_rgb>(0xEBF4F9));
						}

						column_x += col.width_px;
					}

					//Draw selecting inner rectangle
					if(item.flags.selected)
						_m_draw_item_border(content_r.x, y, show_w);
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

				void _m_draw_item_border(int x, int y, unsigned width) const
				{
					//Draw selecting inner rectangle
					rectangle r{ x, y, width, essence_->item_height() };

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
					//essence_->text_height = graph.text_extent_size(L"jHWn0123456789/<?'{[|\\_").height;

					essence_->text_height = 0;
					unsigned as, ds, il;
					if (graph.text_metrics(as, ds, il))
						essence_->text_height = as + ds;

					essence_->scheme_ptr->suspension_width = graph.text_extent_size("...").width;
					essence_->calc_content_size(true);
				}

				void trigger::refresh(graph_reference graph)
				{
					if (API::is_destroying(essence_->lister.wd_ptr()->handle()))
						return;

					nana::rectangle r;

					if (essence_->rect_lister(r))
						drawer_lister_->draw(r);

					if (essence_->header.visible() && essence_->rect_header(r))
						drawer_header_->draw(graph, r);

					essence_->draw_peripheral();
				}

				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					using item_state = essence::item_state;
					using parts = essence::parts;

					bool need_refresh = false;

					point pos_in_header = arg.pos;
					essence_->widget_to_header(pos_in_header);

					if(essence_->ptr_state == item_state::pressed)
					{
						if(essence_->pointer_where.first == parts::header)
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

					bool set_splitter = false;
					if(essence_->pointer_where.first == parts::header)
					{
						nana::rectangle r;
						if(essence_->rect_header(r))
						{
							if(drawer_header_->detect_splitter(r, arg.pos.x))
							{
								set_splitter = true;
								essence_->lister.wd_ptr()->cursor(cursor::size_we);
							}
						}
					}

					if((!set_splitter) && (essence_->ptr_state != item_state::grabbed))
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

							const auto abs_item_pos = lister.index_cast_noexcpt(item_pos, true, item_pos);	//convert display position to absolute position

							if(ptr_where.first == parts::list)
							{
								//adjust the display of selected into the list rectangle if the part of the item is beyond the top/bottom edge
								if (good_list_r)
								{
									auto item_coord = this->essence_->item_coordinate(abs_item_pos);	//item_coord.x = top, item_coord.y = bottom
									if (item_coord.x < list_r.y && list_r.y < item_coord.y)
										essence_->content_view->move_origin({ 0, item_coord.x - list_r.y });
									else if (item_coord.x < list_r.bottom() && list_r.bottom() < item_coord.y)
										essence_->content_view->move_origin({ 0, item_coord.y - list_r.bottom() });

									essence_->content_view->sync(false);
								}

								bool sel = true;

								//no single selected
								if (!lister.single_status(true))
								{
									if (arg.shift)
									{
										//Set the first item as the begin of selected item if there
										//is not a last selected item.(#154 reported by RenaudAlpes)
										if (lister.last_selected_abs.empty() || lister.last_selected_abs.is_category())
											lister.last_selected_abs.set_both(0);

										auto before = lister.last_selected_abs;

										lister.select_display_range_if(lister.last_selected_abs, item_pos, true, [](const index_pair&)
										{
											return true;
										});

										lister.last_selected_abs = before;
									}
									else if (arg.ctrl)
									{
										essence_->mouse_selection.reverse_selection = true;
										sel = !item_proxy(essence_, abs_item_pos).selected();
									}
									else
									{
										if (nana::mouse::right_button == arg.button)
										{
											//Unselects all selected items if the current item is not selected before selecting.
											auto selected = lister.pick_items(true);
											if (selected.cend() == std::find(selected.cbegin(), selected.cend(), item_pos))
												lister.select_for_all(false, item_pos);
										}
										else
										{
											//Unselects all selected items except current item if right button clicked.
											lister.select_for_all(false, item_pos);	//cancel all selections
										}
									}
								}
								else
								{
									//Clicking on a category is ignored when single selection is enabled.
									//Fixed by Greentwip(issue #121)
									if (item_ptr)
										sel = !item_proxy(essence_, abs_item_pos).selected();
								}

								if(item_ptr)
								{
									if (item_ptr->flags.selected != sel)
									{
										item_ptr->flags.selected = sel;
										lister.emit_cs(abs_item_pos, true);

										if (item_ptr->flags.selected)
										{
											lister.cancel_others_if_single_enabled(true, abs_item_pos);
											essence_->lister.last_selected_abs = abs_item_pos;
										}
										else if (essence_->lister.last_selected_abs == abs_item_pos)
											essence_->lister.last_selected_abs.set_both(npos);
									}
								}
								else
								{
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
						else
						{
							//Blank area is clicked

							bool unselect_all = true;
							if (!lister.single_status(true))	//not single selected
							{
								if (arg.ctrl || arg.shift)
								{
									essence_->mouse_selection.reverse_selection = arg.ctrl;
									unselect_all = false;
								}
							}

							if(unselect_all)
								update = lister.select_for_all(false); //unselect all items due to the blank area being clicked
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
							essence_->start_mouse_selection(arg.pos);

						lister.select_for_all(false);
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
					//Do sort
					if (essence_->header.sortable() && essence_->pointer_where.first == parts::header && prev_state == item_state::pressed)
					{
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
					bool up = false;

                    if (essence_->lister.cat_container().size() == 1 && essence_->lister.size_item(0)==0)
                       return ;

					switch(arg.key)
					{
					case keyboard::os_arrow_up:
						up = true;
					case keyboard::os_arrow_down:
						essence_->lister.move_select(up, !arg.shift, true);
						break;
					case L' ':
						{
							index_pairs s;
							bool ck = ! essence_->lister.item_selected_all_checked(s);
							for(auto i : s)
								item_proxy(essence_, i).check(ck);
						}
						break;
					case keyboard::os_pageup :
						up = true;
					case keyboard::os_pagedown:
						{
							//Turns page, then returns if no change occurs
							if (!essence_->content_view->turn_page(!up, false))
								return;

							essence_->lister.select_for_all(false);

							auto idx = essence_->first_display();

							if (!up)
								idx = essence_->lister.advance(idx, static_cast<int>(essence_->count_of_exposed(false)) - 1);

							if (!idx.is_category())
								item_proxy::from_display(essence_, idx).select(true);
							else if (!essence_->lister.single_status(true))	//not selected
								essence_->lister.cat_status(idx.cat, true, true);

							break;
						}
					case keyboard::os_home:
						{
							essence_->lister.select_for_all(false);

							index_pair frst{essence_->lister.first()};
							if (! frst.is_category())
								item_proxy::from_display(essence_, frst).select(true);
							else if (!essence_->lister.single_status(true))	//not selected
								essence_->lister.cat_status(frst.cat, true, true);

							break;
						}
					case keyboard::os_end:
						essence_->lister.select_for_all(false);
						item_proxy::from_display(essence_, essence_->lister.last()).select(true);
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
						essence_->lister.select_for_all(true);
						refresh(graph);
					    API::dev::lazy_refresh();
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

				/// the main porpose of this it to make obvious that item_proxy operate with absolute positions, and dont get moved during sort()
				item_proxy item_proxy::from_display(essence *ess, const index_pair &relative)
				{
					return item_proxy{ ess, ess->lister.index_cast(relative, true) };
				}

				item_proxy item_proxy::from_display(const index_pair &relative) const
				{
					return item_proxy{ess_, ess_->lister.index_cast(relative, true)};
				}

				/// posible use: last_selected_display = last_selected.to_display().item; use with caution, it get invalidated after a sort()
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
								ess_->lister.scroll(pos_, !(ess_->first_display() > this->to_display()));
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
					if(m.flags.selected == s)
						return *this;     // ignore if no change

					m.flags.selected = s;                       // actually change selection

					ess_->lister.emit_cs(this->pos_, true);

					if (m.flags.selected)
					{
						ess_->lister.cancel_others_if_single_enabled(true, pos_);	//Cancel all selections except pos_ if single_selection is enabled.
						ess_->lister.last_selected_abs = pos_;
					}
					else if (ess_->lister.last_selected_abs == pos_)
							ess_->lister.last_selected_abs.set_both(npos);

					if (scroll_view)
					{
						if (ess_->lister.get(pos_.cat)->expand)
							ess_->lister.get(pos_.cat)->expand = false;

						if (!this->displayed())
							ess_->lister.scroll(pos_, !(ess_->first_display() > this->to_display()));
					}

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
					return (text(pos_.item) == s);
				}

				bool item_proxy::operator==(const std::wstring& s) const
				{
					return (text(pos_.item) == to_utf8(s));
				}

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

                    ess_->lister.last_selected_abs =  index_pair {this->pos_, npos};

                    return *this;
                }

				bool cat_proxy::selected() const
                {
                    for (item_proxy &it : *this )
                        if (!it.selected())
                            return false;
                    return true;
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

						ess_->update(true);
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
				_m_ess().update();

			return true;
		}

		void listbox::auto_draw(bool enabled) noexcept
		{
			auto & ess = _m_ess();
			if (ess.auto_draw != enabled)
			{
				ess.auto_draw = enabled;
				ess.update();
			}
		}

		void listbox::scroll(bool to_bottom, size_type cat_pos)
		{
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

			ess.lister.scroll(pos, to_bottom);
			ess.update();
		}

		void listbox::scroll(bool to_bottom, const index_pair& pos)
		{
			_m_ess().lister.scroll(pos, to_bottom);
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

		listbox::cat_proxy listbox::append(std::string s)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_cat(to_nstring(std::move(s)));
			ess.update();

			return cat_proxy{ &ess, new_cat_ptr };
		}

		listbox::cat_proxy listbox::append(std::wstring s)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_cat(to_nstring(std::move(s)));
			ess.update();
			return cat_proxy{ &ess, new_cat_ptr };
		}

		void listbox::append(std::initializer_list<std::string> categories)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();

			for (auto & arg : categories)
				ess.lister.create_cat(native_string_type(to_nstring(arg)));
			ess.update();
		}

		void listbox::append(std::initializer_list<std::wstring> categories)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();

			for (auto & arg : categories)
				ess.lister.create_cat(native_string_type(to_nstring(arg)));
			ess.update();
		}

		auto listbox::insert(cat_proxy cat, std::string str) -> cat_proxy
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_cat(cat.position(), to_nstring(std::move(str)));
			return cat_proxy{ &ess, new_cat_ptr };
		}

		auto listbox::insert(cat_proxy cat, std::wstring str) -> cat_proxy
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_cat(cat.position(), to_nstring(std::move(str)));
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

		listbox::cat_proxy listbox::at(size_type pos)
		{
			check_range(pos, size_categ());
			return{ &_m_ess(), pos };
		}

		const listbox::cat_proxy listbox::at(size_type pos) const
		{
			check_range(pos, size_categ());
			return{ &_m_ess(), pos };
		}

		listbox::item_proxy listbox::at(const index_pair& abs_pos)
		{
			return at(abs_pos.cat).at(abs_pos.item);
		}

		const listbox::item_proxy listbox::at(const index_pair& pos_abs) const
		{
			return at(pos_abs.cat).at(pos_abs.item);
		}

		// Contributed by leobackes(pr#97)
		listbox::index_pair listbox::cast( const point& pos ) const
		{
			auto & ess=_m_ess();
			auto _where = ess.where(pos);

			if (drawerbase::listbox::essence::parts::list == _where.first)
				return ess.lister.advance(ess.first_display(), static_cast<int>(_where.second));
	
			return index_pair{ npos, npos };
		}

		auto listbox::column_at(size_type pos, bool disp_order) -> column_interface&
		{
			return _m_ess().header.at(pos, disp_order);
		}

		auto listbox::column_at(size_type pos, bool disp_order) const -> const column_interface&
		{
			return _m_ess().header.at(pos, disp_order);
		}

		auto listbox::column_size() const ->size_type
		{
			return _m_ess().header.cont().size();
		}

		//Contributed by leobackes(pr#97)
		listbox::size_type listbox::column_from_pos ( const point& pos ) const
		{
			return _m_ess().column_from_pos(pos.x);
		}

		void listbox::checkable(bool chkable)
		{
			auto & ess = _m_ess();
			if(ess.checkable != chkable)
			{
				ess.checkable = chkable;
				ess.update();
			}
		}

		auto listbox::checked() const -> index_pairs
		{
			return _m_ess().lister.pick_items(false);
		}

		void listbox::clear(size_type cat)
		{
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
			auto & ess = _m_ess();

			ess.lister.clear();
            unsort();   // apperar to be espected

			ess.calc_content_size(false);
			ess.content_view->change_position(0, false, false);
			ess.content_view->sync(false);

			ess.update();
		}

		void listbox::erase(size_type cat)
		{
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
			auto & ess = _m_ess();
			ess.lister.erase();
			//ess.first_display(ess.lister.first());
			ess.calc_content_size();
			ess.update();
		}
		
		void listbox::erase(index_pairs indexes)
		{
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
			return _m_ess().header.sortable();
		}

		void listbox::sortable(bool enable)
		{
			_m_ess().header.sortable(enable);
		}

		void listbox::set_sort_compare(size_type col, std::function<bool(const std::string&, nana::any*, const std::string&, nana::any*, bool reverse)> strick_ordering)
		{
			_m_ess().header.at(col).weak_ordering = std::move(strick_ordering);
		}

        /// sort() and ivalidate any existing reference from display position to absolute item, that is: after sort() display offset point to different items
        void listbox::sort_col(size_type col, bool reverse)
		{
			_m_ess().lister.sort_column(col, &reverse);
		}

		auto listbox::sort_col() const -> size_type
		{
			return _m_ess().lister.sort_attrs().column;
		}

        /// potencially ivalidate any existing reference from display position to absolute item, that is: after sort() display offset point to different items
		void listbox::unsort()
		{
			this->sort_col(npos, false);
		}

		bool listbox::freeze_sort(bool freeze)
		{
			return !_m_ess().lister.active_sort(!freeze);
		}

		auto listbox::selected() const -> index_pairs
		{
			return _m_ess().lister.pick_items(true);   // absolute positions, no relative to display
		}

		void listbox::show_header(bool sh)
		{
			_m_ess().header.visible(sh);
			_m_ess().update();
		}

		bool listbox::visible_header() const
		{
			return _m_ess().header.visible();
		}

		void listbox::move_select(bool upwards)  ///<Selects an item besides the current selected item in the display.
		{
			_m_ess().lister.move_select(upwards);
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
			_m_ess().lister.disable_single(for_selection);
		}

        listbox::export_options& listbox::def_export_options()
        {
			return _m_ess().def_exp_options;
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
				cat = ess.lister.create_cat(ptr);
			}
			else
			{
				cat = ess.lister.create_cat(native_string_type{});
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
	//end class listbox
}//end namespace nana
