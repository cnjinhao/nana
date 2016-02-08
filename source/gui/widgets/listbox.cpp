/*
 *	A List Box Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
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
 *		
 */

#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/scroll.hpp>
#include <nana/gui/widgets/panel.hpp>	//for inline widget

#include <nana/gui/layout_utility.hpp>
#include <nana/gui/element.hpp>
#include <list>
#include <deque>
#include <stdexcept>
#include <algorithm>
#include <nana/system/dataexch.hpp>
#include <cassert>

namespace nana
{
	namespace drawerbase
	{
		namespace listbox
		{
			//struct cell
				cell::format::format(const ::nana::color& bgcolor, const ::nana::color& fgcolor)
					: bgcolor{ bgcolor }, fgcolor{ fgcolor }
				{}

				cell::cell(const cell& rhs)
					:	text(rhs.text),
						custom_format{ rhs.custom_format ? new format(*rhs.custom_format) : nullptr }
				{}

				//A workaround that VS2013 does not support to define an explicit default move constructor
				cell::cell(cell&& other)
					:	text(std::move(other.text)),
						custom_format{ std::move(other.custom_format) }
				{
				}

				cell::cell(std::string text)
					: text(std::move(text))
				{}

				cell::cell(std::string text, const format& fmt)
					:	text(std::move(text)),
						custom_format(std::make_unique<format>( fmt ))	// or  custom_format(new format{ fmt })
				{}

				cell::cell(std::string text, const ::nana::color& bgcolor, const ::nana::color& fgcolor)
					:	text(std::move(text)),
						custom_format{std::make_unique<format>( bgcolor, fgcolor ) }	//custom_format{ new format{ bgcolor, fgcolor } }
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

				cell& cell::operator=(cell&& other)
				{
					if (this != &other)
					{
						text = std::move(other.text);
						custom_format = std::move(other.custom_format);
					}
					return *this;
				}
			//end struct cell

			//definition of iresolver/oresolver
			oresolver& oresolver::operator<<(bool n)
			{
				cells_.emplace_back(n ? "true" : "false");
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
				cells_.emplace_back(text);
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

			std::vector<cell>&& oresolver::move_cells()
			{
				return std::move(cells_);
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

			iresolver::iresolver(const std::vector<cell>& cl)
				: cells_(cl)
			{}

			iresolver& iresolver::operator>>(cell& cl)
			{
				if (pos_ < cells_.size())
					cl = cells_[pos_++];
				return *this;
			}

			iresolver& iresolver::operator>>(std::nullptr_t)
			{
				++pos_;
				return *this;
			}
			//end class iresolver/oresolver

            /// Essence of the columns Header
			class es_header
			{
			public:

				struct column_t
				{
					native_string_type text;  ///< "text" header of the column number "index" with weigth "pixels"
					unsigned pixels;
					bool visible{true};
					size_type index;
					std::function<bool(const std::string&, nana::any*, const std::string&, nana::any*, bool reverse)> weak_ordering;

					column_t() = default;
					column_t(native_string_type&& txt, unsigned px, size_type pos)
						: text(std::move(txt)), pixels(px), index(pos)
					{}
				};

				using container = std::vector<column_t> ;

                export_options::columns_indexs all_headers(bool only_visibles) const
                {
                    export_options::columns_indexs	idx;
					for(const auto &header : cont())
					{
						if(header.visible || !only_visibles)
							idx.push_back(header.index);
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
						head_str += to_utf8(column_ref(exp_opt.columns_order[idx]).text);
					}
					return head_str;
				}

				bool visible() const
				{
					return visible_;
				}

				bool visible(bool v)
				{
					if (visible_ == v)
						return false;

					visible_ = v;
					return true;
				}

				bool sortable() const
				{
					return sortable_;
				}

				void sortable(bool enable)
				{
					sortable_ = enable;
				}

				std::function<bool(const std::string&, nana::any*, const std::string&, nana::any*, bool reverse)> fetch_comp(std::size_t pos) const
				{
					try
					{
						return column_ref(pos).weak_ordering;
					}
					catch (...)
					{
					}
					return{};
				}

				size_type create(native_string_type&& text, unsigned pixels)
				{
					cont_.emplace_back(std::move(text), pixels, static_cast<size_type>(cont_.size()));
                    return cont_.back().index;
				}

				void item_width(size_type pos, unsigned width)
				{
					column(pos).pixels = width;
				}

				unsigned item_width(size_type pos) const throw()
				{
					try
					{
						return column_ref(pos).pixels;
					}
					catch (...)
					{
					}
					return 0;
				}

				unsigned pixels() const
				{
					unsigned pixels = 0;
					for(auto & m : cont_)
					{
						if(m.visible)
							pixels += m.pixels;
					}
					return pixels;
				}

				const container& cont() const
				{
					return cont_;
				}

                /// find and return a ref to the column that originaly was at position "pos" previous to any list reorganization.
				column_t& column(size_type pos)
				{
					for(auto & m : cont_)
					{
						if(m.index == pos)
							return m;
					}
					throw std::out_of_range("Nana.GUI.Listbox: invalid header index.");
				}
				const column_t& column_ref(size_type pos) const
                {
					for(const auto & m : cont_)
					{
						if(m.index == pos)
							return m;
					}
					throw std::out_of_range("Nana.GUI.Listbox: invalid header index.");
                }

                /// return the original index of the current column at x .
				size_type item_by_x(int x) const
				{
					for(const auto & col : cont_)  // in current order
					{
						if(x < static_cast<int>(col.pixels))
							return col.index;
						x -= col.pixels;
					}
					return npos;
				}

                /// return the left position of the column originaly at index "pos" .
				int item_pos(size_type pos, unsigned * pixels) const
				{
					int left = 0;
					for (auto & m : cont_)
					{
						if (m.index == pos)
						{
							if (pixels)
								*pixels = m.pixels;
							break;
						}

						if (m.visible)
							left += m.pixels;
					}
					return left;
				}
                /// return the original index of the visible col currently before(in front of) or after the col originaly at index "index"
				size_type neighbor(size_type index, bool front) const
				{
					size_type n = npos;
					for(auto i = cont_.cbegin(); i != cont_.cend(); ++i)  // in current order
					{
						if(i->index == index)
						{
							if(front)	return n;
							for(++i; i != cont_.cend(); ++i)
							{
								if(i->visible) return i->index;
							}
							break;
						}
						else if(i->visible)
							n = i->index;
					}
					return npos;
				}
                /// return the original index of the currently first visible col
				size_type begin() const
				{
					for(const auto & m : cont_)
					{
						if(m.visible) return m.index;
					}
					return npos;
				}

                /// return the original index of the currently last visible col
				size_type last() const
				{
					for(auto i = cont_.rbegin(); i != cont_.rend(); ++i)
					{
						if(i->visible) return i->index;
					}
					return npos;
				}
                /// move the col originaly at index to the position currently in front (or after) the col originaly at index "to" invalidating some current index
				void move(size_type index, size_type to, bool front) throw()
				{
					if ((index == to) || (index >= cont_.size()) || (to >= cont_.size()))
						return;

					for (auto i = cont_.begin(); i != cont_.end(); ++i)
					{
						if (index == i->index)
						{
							column_t from = std::move(*i);
							cont_.erase(i);

							for (auto u = cont_.begin(); u != cont_.end(); ++u)
							{
								if (to == u->index)
								{
									cont_.insert(front ? u : ++u, from);
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

			struct essence_t;

			struct item_t
			{
				using container = std::vector<cell>;

				container cells;
				nana::color bgcolor;
				nana::color fgcolor;
				paint::image img;
				nana::size img_show_size;

				struct flags_tag
				{
					bool selected	:1;
					bool checked	:1;
				}flags;

				mutable std::unique_ptr<nana::any> anyobj;

				item_t()
				{
					flags.selected = flags.checked = false;
				}

				item_t(const item_t& r)
					:	cells(r.cells),
						bgcolor(r.bgcolor),
						fgcolor(r.fgcolor),
						img(r.img),
						flags(r.flags),
						anyobj(r.anyobj ? new nana::any(*r.anyobj) : nullptr)
				{}

				item_t(container&& cont)
					: cells(std::move(cont))
				{
					flags.selected = flags.checked = false;
				}

				item_t(std::string&& s)
				{
					flags.selected = flags.checked = false;
					cells.emplace_back(std::move(s));
				}

				item_t(std::string&& s, const nana::color& bg, const nana::color& fg)
					:	bgcolor(bg),
						fgcolor(fg)
				{
					flags.selected = flags.checked = false;
					cells.emplace_back(std::move(s));
				}

				item_t& operator=(const item_t& r)
				{
					if (this != &r)
					{
						cells = r.cells;
						flags = r.flags;
						anyobj.reset(r.anyobj ? new nana::any(*r.anyobj) : nullptr);
						bgcolor = r.bgcolor;
						fgcolor = r.fgcolor;
						img = r.img;
					}
					return *this;
				}

				std::string to_string(const export_options& exp_opt) const
				{
					std::string item_str;

					bool ignore_first = true;
					for (size_type idx{}; idx < exp_opt.columns_order.size(); ++idx)
					{
						if (ignore_first)
							ignore_first = false;
						else
							item_str += exp_opt.sep;

						item_str += cells[exp_opt.columns_order[idx]].text;
					}

                    return item_str;
				}
			};

			class inline_indicator;

			struct category_t
			{
				using container = std::deque<item_t>;

				native_string_type text;
				std::vector<std::size_t> sorted;
				container items;
				bool expand{true};

				//A cat may have a key object to identify the category
				std::shared_ptr<nana::detail::key_interface> key_ptr;

				std::deque<pat::cloneable<pat::abstract_factory<inline_notifier_interface>>> factories;
				std::deque<std::unique_ptr<inline_indicator>> indicators;

				category_t() = default;

				category_t(native_string_type str)
					:text(std::move(str))
				{}

				bool selected() const
				{
					for (auto & m : items)
					{
						if (m.flags.selected == false) return false;
					}
					return !items.empty();
				}
			};

			class es_lister
			{
			public:
				using container = std::list<category_t>;

				std::function<std::function<bool(const ::std::string&, ::nana::any*,
								const ::std::string&, ::nana::any*, bool reverse)>(std::size_t) > fetch_ordering_comparer;

				es_lister()
				{
					//#0 is a default category
					list_.emplace_back();
				}

				void bind(essence_t* ess, widget& wd)
				{
					ess_ = ess;
					widget_ = dynamic_cast<nana::listbox*>(&wd);
				}

				nana::listbox* wd_ptr() const
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
							item.anyobj.reset(new nana::any); //make_unique
							return item.anyobj.get();
						}
					}
					return nullptr;
				}

                std::string to_string(const export_options& exp_opt) const;

                /// each sort() ivalidate any existing reference from display position to absolute item, that is after sort() display offset point to different items
                void sort()
				{
					if((sorted_index_ == npos) || (!resort_))
						return;

					auto weak_ordering_comp = fetch_ordering_comparer(sorted_index_);
					if(weak_ordering_comp)
					{
						for(auto & cat: list_)
						{
							auto bi = std::begin(cat.sorted);
							auto ei = std::end(cat.sorted);
							std::sort(bi, ei, [&cat, &weak_ordering_comp, this](std::size_t x, std::size_t y){
									//The predicate must be a strict weak ordering.
									//!comp(x, y) != comp(x, y)
									auto & mx = cat.items[x];
									auto & my = cat.items[y];
									if (mx.cells.size() <= sorted_index_ || my.cells.size() <= sorted_index_)
									{
										std::string a;
										if (mx.cells.size() > sorted_index_)
											a = mx.cells[sorted_index_].text;

										std::string b;
										if (my.cells.size() > sorted_index_)
											b = my.cells[sorted_index_].text;

										return weak_ordering_comp(a, mx.anyobj.get(), b, my.anyobj.get(), sorted_reverse_);
									}

									return weak_ordering_comp(mx.cells[sorted_index_].text, mx.anyobj.get(), my.cells[sorted_index_].text, my.anyobj.get(), sorted_reverse_);
								});
						}
					}
					else
					{	//No user-defined comparer is provided, and default comparer is applying.
						for(auto & cat: list_)
						{
							std::sort(std::begin(cat.sorted), std::end(cat.sorted), [&cat, this](std::size_t x, std::size_t y){
									auto & item_x = cat.items[x];
									auto & item_y = cat.items[y];

									if (item_x.cells.size() <= sorted_index_ || item_y.cells.size() <= sorted_index_)
									{
										std::string a;
										if (item_x.cells.size() > sorted_index_)
											a = item_x.cells[sorted_index_].text;

										std::string b;
										if (item_y.cells.size() > sorted_index_)
											b = item_y.cells[sorted_index_].text;

										return (sorted_reverse_ ? a > b : a < b);
									}

									auto & a = item_x.cells[sorted_index_].text;
									auto & b = item_y.cells[sorted_index_].text;
									return (sorted_reverse_ ? a > b : a < b);
								});
						}
					}
                    scroll_refresh();
				}
                void scroll_refresh();

                /// sort() and ivalidate any existing reference from display position to absolute item, that is after sort() display offset point to different items
				bool sort_index(size_type index)
				{
					if (npos == index)
					{
						sorted_index_ = npos;
                        scroll_refresh();
						return false;
					}

					if(index != sorted_index_)
					{
						sorted_index_ = index;
						sorted_reverse_ = false;
					}
					else
						sorted_reverse_ = !sorted_reverse_;

					sort();
					return true;
				}

                /// sort() and ivalidate any existing reference from display position to absolute item, that is: after sort() display offset point to different items
				bool set_sort_index(std::size_t index, bool reverse)
				{
					if (npos == index)
					{
						sorted_index_ = npos;
                        scroll_refresh();
						return false;
					}

					if(index != sorted_index_ || reverse != sorted_reverse_)
					{
						sorted_index_ = index;
						sorted_reverse_ = reverse;
						sort();
					}
					return true;
				}

				std::size_t sort_index() const
				{
					return sorted_index_;
				}

				bool active_sort(bool resort)
				{
					bool prstatus = resort;
					resort_ = resort;
					return prstatus;
				}

				bool sort_reverse() const
				{
					return sorted_reverse_;
				}

				void scroll(const index_pair& pos, bool to_bottom);

				///Append a new category with a specified name and return a pointer to it.
				category_t* create_cat(native_string_type&& text)
				{
					list_.emplace_back(std::move(text));
					return &list_.back();
				}

		        /// will use the key to insert new cat before the first cat with compare less than the key, or at the end of the list of cat and return a ref to that new cat.  ?
                category_t* create_cat(std::shared_ptr<nana::detail::key_interface> ptr)
				{
					for (auto i = list_.begin(); i != list_.end(); ++i)
					{
						if (i->key_ptr && i->key_ptr->compare(ptr.get()))
						{
							i = list_.emplace(i);
							i->key_ptr = ptr;
							return &(*i);
						}
					}

					list_.emplace_back();
					list_.back().key_ptr = ptr;
					return &list_.back();
				}
                /// add a new cat created at "pos" and return a ref to it
				category_t* create_cat(std::size_t pos, native_string_type&& text)
				{
					return &(*list_.emplace(this->get(pos), std::move(text)));
				}

				/// Insert  before item in absolute "pos" a new item with "text" in column 0, and place it in last display position of this cat
				bool insert(const index_pair& pos, std::string&& text)
				{
					auto & catobj = *get(pos.cat);

					const auto n = catobj.items.size();
					if (pos.item > n)
						return false;

					catobj.sorted.push_back(n);

					if (pos.item < n)
						catobj.items.emplace(catobj.items.begin() + pos.item, std::move(text));
					else
						catobj.items.emplace_back(std::move(text));

					return true;
				}

				/// convert from display order to absolute (find the real item in that display pos) but without check from current active sorting, in fact using just the last sorting !!!
                size_type index_by_display_order(size_type cat, size_type display_order_pos) const
				{
					auto & catobj = *get(cat);
					if (display_order_pos >= catobj.sorted.size())
						throw std::out_of_range("listbox: Invalid item position.");

					return catobj.sorted[display_order_pos];
				}

				/// find display order for the real item but without check from current active sorting, in fact using just the last sorting !!!
				size_type display_order(size_type cat, size_type pos) const
				{
					auto& catobj = *get(cat);
					if (pos >= catobj.sorted.size())
						throw std::out_of_range("listbox: Invalid item position.");

                    for (size_type i=0; i<catobj.sorted.size();++i)
                        if (pos==catobj.sorted[i])
                            return i;

					return   npos ;
				}

				category_t::container::value_type& at_abs(const index_pair& pos)
				{
					return get(pos.cat)->items.at(pos.item);
				}

                /// return a ref to the real item object at display!!! position pos using current sorting only if it is active, and at absolute position if no sorting is currently active.
				category_t::container::value_type& at(const index_pair& pos)
				{
					auto index = pos.item;

					if (sorted_index_ != npos)
						index = absolute(pos);

					return get(pos.cat)->items.at(index);
				}

				const category_t::container::value_type& at(const index_pair& pos) const
				{
					auto index = pos.item;

					if (sorted_index_ != npos)
						index = absolute(pos);

					return get(pos.cat)->items.at(index);
				}

				void clear(size_type cat)
				{
					auto& catobj = *get(cat);
					catobj.items.clear();
					catobj.sorted.clear();
				}
                /// clear all items in all cat, but not the container of cat self.
				void clear()
				{
					for(auto & m : list_)
					{
						m.items.clear();
						m.sorted.clear();
					}
				}

				index_pair advance(index_pair from, size_type offset)   //    <------------- index
				{
                    index_pair dpos{npos, npos};
					if(from.cat >= size_categ() || (from.item != npos && from.item >= size_item(from.cat)))
                        return dpos;

					dpos  = from;

					while(offset)
					{
						if(dpos.item == npos)
						{
							if(expand(dpos.cat) == false)
							{
								if(dpos.cat + 1 == size_categ())
									break;
								++dpos.cat;
							}
							else
								dpos.item = 0;
							--offset;
						}
						else
						{
							size_type rest = size_item(dpos.cat) - dpos.item - 1;
							if(rest == 0)
							{
								if(dpos.cat + 1 == size_categ())
									break;
								++dpos.cat;
								dpos.item = npos;
								--offset;
							}
							else if(rest < offset)
							{
								offset -= rest;
								if(dpos.cat + 1 >= size_categ())
								{
									dpos.item += rest;
									break;
								}
								dpos.item = npos;
								++dpos.cat;
							}
							else
							{
								dpos.item += offset;
								break;
							}
						}
					}
					return dpos;
				}
                /// change to index arg
				size_type distance(index_pair from, index_pair to) const
				{
					if(from  == to ) return 0;

					if(to.cat == from.cat)
					{
						if(from.item > to.item && from.item != npos)
							std::swap(from.item, to.item);

						return (from.item == npos ? to.item + 1 : to.item - from.item);
					}
					else if(to.cat < from.cat)
						std::swap(from, to);

					size_type n = 0;
					auto i = get(from.cat);
					if(from.item == npos)
					{
						if(i->expand)
							n = i->items.size();
					}
					else
						n = i->items.size() - (from.item + 1);

					for(++i, ++from.cat; i != list_.end(); ++i, ++from.cat)
					{
						++n; //this is a category
						if(from.cat != to.cat)
						{
							if(i->expand)
								n += i->items.size();
						}
						else
						{
							if(to.item != npos)
								n += (to.item + 1);
							break;
						}
					}
					return n;
				}

				std::vector<cell>& get_cells(category_t * cat, size_type pos) const
				{
					if (!cat)
						throw std::out_of_range("nana::listbox: category is null");

					return cat->items.at(pos).cells;
				}

				void text(category_t* cat, size_type pos, size_type col, cell&& cl, size_type columns)
				{
					if ((col < columns) && (pos < cat->items.size()))
					{
						auto & cont = cat->items[pos].cells;
						if (col < cont.size())
						{
							cont[col] = std::move(cl);
							if (sorted_index_ == col)
								sort();
						}
						else
						{	//If the index of specified sub item is over the number of sub items that item contained,
							//it fills the non-exist items.
							cont.resize(col);
							cont.emplace_back(std::move(cl));
						}
					}
				}

				void text(category_t* cat, size_type pos, size_type col, std::string&& str, size_type columns)
				{
					if ((col < columns) && (pos < cat->items.size()))
					{
						auto & cont = cat->items[pos].cells;
						if (col < cont.size())
						{
							cont[col].text = std::move(str);
							if (sorted_index_ == col)
								sort();
						}
						else
						{	//If the index of specified sub item is over the number of sub items that item contained,
							//it fills the non-exist items.
							cont.resize(col);
							cont.emplace_back(std::move(str));
						}
					}
				}

				void erase(const index_pair& pos);

				void erase(size_type cat)
				{
					auto i = get(cat);

					//If the category is the first one, it just clears the items instead of removing whole category.
					if(0 == cat)
					{
						i->items.clear();
						i->sorted.clear();
					}
					else
						list_.erase(i);
				}

				void erase()
				{
					//Do not remove the first category.
					auto i = list_.begin();
					i->items.clear();
					i->sorted.clear();
					if(list_.size() > 1)
						list_.erase(++i, list_.end());
				}

				bool expand(size_type cat, bool exp)
				{
					if(good(cat))
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

				bool expand(size_type cat) const
				{
					return (good(cat) ? get(cat)->expand : false);
				}

				container& cat_container()
				{
					return list_;
				}

				const container& cat_container() const
				{
					return list_;
				}

				//Enable/Disable the ordered categories
				bool enable_ordered(bool enb)
				{
					if (ordered_categories_ == enb)
						return false;

					ordered_categories_ = enb;
					return true;
				}

				bool enable_ordered() const
				{
					return ordered_categories_;
				}

				size_type the_number_of_expanded() const
				{
					size_type n = list_.size() - 1;
					for(auto & i : list_)
					{
						if(i.expand)
							n += i.items.size();
					}
					return n;
				}

				void check_for_all(bool ck)
				{
					index_pair pos;
					for(auto & cat : list_)
					{
						pos.item = 0;
						for(auto & m : cat.items)
						{
							if(m.flags.checked != ck)
							{
								m.flags.checked = ck;

								arg_listbox arg{ item_proxy{ess_, pos}, ck};
								wd_ptr()->events().checked.emit(arg);
							}
							++pos.item;
						}
						++pos.cat;
					}
				}

				selection item_checked() const
				{
					selection vec;
					index_pair id;
					for(auto & cat : list_)
					{
						id.item = 0;
						for(auto & m : cat.items)
						{
							if(m.flags.checked)
								vec.push_back(id);
							++id.item;
						}
						++id.cat;
					}
					return vec;
				}

				void select_range(index_pair fr, index_pair to, bool sel)
				{
					if (fr > to)
						std::swap(fr, to);

					for (; fr != to; forward(fr, 1, fr))
					{
						if (fr.is_item())
							item_proxy(ess_, fr).select(sel);
					}

					if (to.is_item())
						item_proxy(ess_, to).select(sel);
				}
				void select_display_range(index_pair fr_abs, index_pair to_dpl, bool sel)
				{
					index_pair fr_dpl (fr_abs.cat, this->display_order(fr_abs.cat, fr_abs.item));
                    if (fr_dpl > to_dpl)
						std::swap(fr_dpl, to_dpl);

					for (; fr_dpl != to_dpl; forward(fr_dpl, 1, fr_dpl))
					{
						if (fr_dpl.is_item())
							item_proxy(ess_, index_pair(fr_dpl.cat, absolute( fr_dpl ) )).select(sel);
					}

					if (to_dpl.is_item())
						item_proxy(ess_, index_pair(to_dpl.cat, absolute( to_dpl ) )).select(sel);
				}

				bool select_for_all(bool sel)
				{
					bool changed = false;
					index_pair i;
					for(auto & cat : list_)
					{
						i.item = 0;
						for(auto & m : cat.items)
						{
							if(m.flags.selected != sel)
							{
								changed = true;
								m.flags.selected = sel;

								arg_listbox arg{ item_proxy(ess_, i), sel };
								wd_ptr()->events().selected.emit(arg);

								if (m.flags.selected)
									last_selected_abs = i;
								else if (last_selected_abs == i)
									last_selected_abs.set_both(npos);		//make empty
							}
							++i.item;
						}
						++i.cat;
					}
					return changed;
				}

				/// return absolute positions, no relative to display
                void item_selected(selection& vec) const  // change to selection item_selected();
				{
					index_pair id;
					for(auto & cat : list_)
					{
						id.item = 0;
						for(auto & m : cat.items)
						{
							if(m.flags.selected)
								vec.push_back(id);  // absolute positions, no relative to display
							++id.item;
						}
						++id.cat;
					}
				}

                index_pair find_first_selected()
                {
					index_pair id;
					for(auto & cat : list_)
					{
						id.item = 0;
						for(auto & m : cat.items)
						{
							if(m.flags.selected)
								return id;  // absolute positions, no relative to display
							++id.item;
						}
						++id.cat;
					}
                    return {npos,npos};
                }

				/// return absolute positions, no relative to display
				bool item_selected_all_checked(selection& vec) const
				{
					index_pair id;
					bool ck = true;

					for (auto & cat : list_)
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
                void move_select(bool upwards=true, bool unselect_previous=true, bool trace_selected=false);

				void cancel_others_if_single_enabled(bool for_selection, const index_pair& except)
				{
					if (!(for_selection ? single_selection_ : single_check_))
						return;

					auto pred = [for_selection](category_t::container::value_type & m){
						return (for_selection ? m.flags.selected : m.flags.checked);
					};

					auto do_cancel = [this, for_selection](category_t::container::value_type& m, std::size_t cat_pos, std::size_t item_pos)
					{
						arg_listbox arg{ item_proxy(ess_, index_pair(cat_pos, item_pos)), false };
						if (for_selection)
						{
							m.flags.selected = false;
							widget_->events().selected.emit(arg);
						}
						else
						{
							m.flags.checked = false;
							widget_->events().checked.emit(arg);
						}
					};

					if (for_selection ? single_selection_category_limited_ : single_check_category_limited_)
					{
						auto i = this->get(except.cat);

						std::size_t item_pos = 0;
						for (auto & m : i->items)
						{
							if ((item_pos != except.item) && pred(m))
								do_cancel(m, except.cat, item_pos);

							++item_pos;
						}
					}
					else
					{
						std::size_t cat_pos = 0;
						for (auto & cat : list_)
						{
							if (cat_pos != except.cat)
							{
								std::size_t item_pos = 0;
								for (auto & m : cat.items)
								{
									if (pred(m))
										do_cancel(m, cat_pos, item_pos);
									++item_pos;
								}
							}
							else
							{
								std::size_t item_pos = 0;
								for (auto & m : cat.items)
								{
									if ((item_pos != except.item) && pred(m))
										do_cancel(m, cat_pos, item_pos);
									++item_pos;
								}
							}
							++cat_pos;
						}
					}
				}

				bool single_selection() const
				{
					return single_selection_;
				}

				bool single_check() const
				{
					return single_check_;
				}

				void enable_single(bool for_selection, bool category_limited)
				{
					bool & single = (for_selection ? single_selection_ : single_check_);
					bool & limited = (for_selection ? single_selection_category_limited_ : single_check_category_limited_);

					if (single && (limited == category_limited))
						return;

					single = true;
					limited = category_limited;

						auto pred = [for_selection](category_t::container::value_type & m){
							return (for_selection ? m.flags.selected : m.flags.checked);
						};

						auto cancel = [this, for_selection](category_t::container::value_type& m, std::size_t cat_pos, std::size_t item_pos)
						{
							arg_listbox arg{ item_proxy(ess_, index_pair(cat_pos, item_pos)), false };
							if (for_selection)
							{
								m.flags.selected = false;
								widget_->events().selected.emit(arg);
							}
							else
							{
								m.flags.checked = false;
								widget_->events().checked.emit(arg);
							}
						};

						std::size_t cat_pos = 0;
						if (category_limited)
						{
							for (auto & cat : list_)
							{
								auto i = std::find_if(cat.items.begin(), cat.items.end(), pred);
								if (i != cat.items.end())
								{
									++i;
									for (auto end = cat.items.end(); i != end; ++i)
									{
										if (pred(*i))
											cancel(*i, cat_pos, i - cat.items.begin());
									}
								}
								++cat_pos;
							}
						}
						else
						{
							bool selected = false;
							for (auto & cat : list_)
							{
								if (!selected)
								{
									auto i = std::find_if(cat.items.begin(), cat.items.end(), pred);
									if (i != cat.items.end())
									{
										selected = true;
										++i;
										for (auto end = cat.items.end(); i != end; ++i)
										{
											if (pred(*i))
												cancel(*i, cat_pos, i - cat.items.begin());
										}
									}
								}
								else
								{
									std::size_t item_pos = 0;
									for (auto & m : cat.items)
									{
										if (pred(m))
											cancel(m, cat_pos, item_pos);

										++item_pos;
									}
								}
								++cat_pos;
							}
						}
				}

				void disable_single(bool for_selection)
				{
					(for_selection ? single_selection_ : single_check_) = false;
				}

				size_type size_categ() const
				{
					return list_.size();
				}

				size_type size_item(size_type cat) const
				{
					return get(cat)->items.size();
				}

				bool categ_checked(size_type cat) const
				{
					auto& items = get(cat)->items;
					for(auto & m : items)
					{
						if(m.flags.checked == false)
							return false;
					}
					return true;
				}

				bool categ_checked(size_type cat, bool ck)
				{
					bool changed = false;
					auto & items = get(cat)->items;
					size_type index = 0;
					for(auto & m : items)
					{
						if(m.flags.checked != ck)
						{
							m.flags.checked = ck;

							arg_listbox arg{ item_proxy(ess_, index_pair(cat, index)), ck};
							wd_ptr()->events().checked.emit(arg);

							changed = true;
						}
						++index;
					}
					return changed;
				}

				bool categ_checked_reverse(size_type cat_index)
				{
					if(list_.size() > cat_index)
						return categ_checked(cat_index, !categ_checked(cat_index));
					return false;
				}

				bool categ_selected(size_type cat) const
				{
					auto & items = get(cat)->items;
					for(auto & m : items)
						if(m.flags.selected == false)
							return false;
					return true;
				}

                /// set all items in cat to selection sel, emiting events, actualizing last_selected_abs, but not check for single_selection_
                void categ_selected(size_type cat, bool sel);

				void reverse_categ_selected(size_type categ)
				{
					categ_selected(categ, ! categ_selected(categ));
				}

                /// can be used as the absolute position of the last absolute item, or as the display pos of the last displayed item
                index_pair last() const
				{
					index_pair i{ list_.size() - 1, list_.back().items.size() };

					if (i.cat)
					{
						if (i.item && list_.back().expand)
							--i.item;
						else
							i.item = npos;
					}
					else if (i.item)
						--i.item;

					return i;
				}

                /// absolute position of the last displayed item
                index_pair last_displ() const
				{
					return absolute ( last() );
				}

                /// can be used as the absolute position of the first absolute item, or as the display pos of the first displayed item
                index_pair first() const
                {
                    index_pair fst{0,npos};
                    good_item(fst,fst);
                    return fst;
                }
                /// absolute position of the first displayed item
                index_pair first_displ() const
                {
					return absolute ( first() );
                }

				bool good(size_type cat) const
				{
					return (cat < list_.size());
				}

				bool good(const index_pair& pos) const
				{
					return ((pos.cat < list_.size()) && (pos.item < size_item(pos.cat)));
				}
                /// if good return the same item (in arg item), or just the next cat and true, but If fail return false
				bool good_item(index_pair pos, index_pair& item) const
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

					if (++i == list_.end())          // item out of range and no more cat
						return false;

					item.cat = pos.cat + 1;         // select the next cat
					item.item = npos;
					return true;
				}

				///Translate relative position (position in display) into absolute position (original data order)
				size_type absolute(const index_pair& display_pos) const
				{
					if(sorted_index_ == npos || display_pos.item == npos)
						return display_pos.item ;

					auto & catobj = *get(display_pos.cat);
					if(catobj.items.empty())
						return (display_pos == index_pair{0,0} ? 0 : npos);

					return (display_pos.item < catobj.sorted.size() ? catobj.sorted[display_pos.item] : npos);
				}
				index_pair absolute_pair(const index_pair& display_pos) const
				{
					//Returns an empty pos if item pos npos
					auto item_pos = absolute(display_pos);
					return {item_pos != npos ? display_pos.cat : npos, item_pos};
				}

                ///Translate absolute position (original data order) into relative position (position in display)
                size_type relative(const index_pair& pos) const
				{
					if (sorted_index_ == npos)
						return pos.item ;

					auto& catobj = *get(pos.cat);

					for (size_type i=0; i<catobj.sorted.size();++i)
						if (pos.item == catobj.sorted[i])
							return i;

					if (catobj.items.empty() && (pos == index_pair{ 0, 0 }))
						return 0;

					return npos;
				}
				index_pair relative_pair(const index_pair& pos) const
				{
					//Returns an empty pos if item is npos
					auto item_pos = relative(pos);
					return {(item_pos != npos ? pos.cat : npos), item_pos};
				}

				/// all arg are relative to display order, or all are absolute, but not mixed
				bool forward(index_pair from, size_type offs, index_pair& item) const
				{
					if (!good_item(from, from))
						return false;

					auto cat = get(from.cat);
					auto cat_end = list_.end();

					auto items_left = (cat->expand ? cat->items.size() : 0);

					if (from.is_category())
						items_left += 1;				//add 1 category bar
					else if (items_left >= from.item)
						items_left -= from.item;
					else
						return false;					//invalid argument

					while (offs)
					{
						if (items_left > offs)
						{
							item.cat = from.cat;
							item.item = (npos == from.item ? offs - 1 : from.item + offs);
							return true;
						}

						offs -= items_left;
						if (++cat == cat_end)
							return false;

						++from.cat;
						from.item = npos;
						items_left = (cat->expand ? cat->items.size() + 1 : 1);
					}

					item = from;
					return true;
				}

                /// all arg are relative to display order, or all are absolute, but not mixed
				bool backward(index_pair from, size_type offs, index_pair& item) const
				{
					if(offs == 0)
						item = from;

					if(good(from.cat))
					{
						auto i = get(from.cat);
						size_type n = (from.is_category() ? 1 : from.item + 2);  // ??
						if(n <= offs)
						{
							offs -= n;
						}
						else
						{
							n -=offs;
							item.cat = from.cat;
							item.item = (n == 1 ? npos : n - 2);
							return true;
						}

						while(i != list_.cbegin())
						{
							--i;
							--from.cat;

							n = (i->expand ? i->items.size() : 0) + 1;

							if(n > offs)
							{
								n -=offs;
								item.cat = from.cat;
								item.item = (n == 1 ? npos : n - 2);
								return true;
							}
							else
								offs -= n;
						}
					}
					return false;
				}

				/// categories iterator
				container::iterator get(size_type pos)
				{
					if (pos >= list_.size())
						throw std::out_of_range("nana::listbox: invalid category index");

					auto i = list_.begin();
					std::advance(i, pos);
					return i;
				}

				container::const_iterator get(size_type pos) const
				{
					if (pos >= list_.size())
						throw std::out_of_range("nana::listbox: invalid category index");

					auto i = list_.cbegin();
					std::advance(i, pos);
					return i;
				}
			public:
				index_pair last_selected_abs, last_selected_dpl;
			private:
				essence_t * ess_{nullptr};
				nana::listbox * widget_{nullptr};
				std::size_t sorted_index_{npos};		///< The index of the column used to sort
				bool	resort_{true};
				bool	sorted_reverse_{false};
				bool	ordered_categories_{false};	///< A switch indicates whether the categories are ordered.
												/// The ordered categories always creates a new category at a proper position(before the first one which is larger than it).
				container list_; // rename to categories_

				bool single_selection_{ false };
				bool single_selection_category_limited_{ false };
				bool single_check_{ false };
				bool single_check_category_limited_{ false };
			};//end class es_lister


			//struct essence_t
			//@brief:	this struct gives many data for listbox,
			//			the state of the struct does not effect on member funcions, therefore all data members are public.
			struct essence_t
			{
				enum class item_state{normal, highlighted, pressed, grabbed, floated};
				enum class parts{unknown = -1, header, lister, checker};

				::nana::listbox::scheme_type* scheme_ptr{nullptr};
				::nana::paint::graphics *graph{nullptr};
				bool auto_draw{true};
				bool checkable{false};
				bool if_image{false};
				unsigned header_size{25};
				unsigned item_size{24};
				unsigned text_height{0};
				unsigned suspension_width{0};

                ::nana::listbox::export_options def_exp_options;

				es_header header;
				es_lister lister;  // we have at least one emty cat. the #0

				item_state ptr_state{ item_state::normal };
				std::pair<parts, std::size_t> pointer_where;	//The 'first' stands for which object, such as header and lister, 'second' stands for item
																//if where == header, 'second' indicates the item
																//if where == lister || where == checker, 'second' indicates the offset to the scroll offset_y which stands for the first item displayed in lister.
																//if where == unknown, 'second' ignored.

				struct scroll_part
				{
					static const unsigned scale = 16;
					int offset_x;
					index_pair offset_y_abs, offset_y_dpl;	//cat stands for category, item stands for item. "item == npos" means that is a category.
                                                // need to be abs??? to see the same item after sort() ??
					nana::scroll<true> v;
					nana::scroll<false> h;
				}scroll;


				struct inline_pane
				{
					::nana::panel<false> pane_bottom;	//pane for pane_widget
					::nana::panel<false> pane_widget;	//pane for placing user-define widget
					std::unique_ptr<inline_notifier_interface> inline_ptr;
					inline_indicator * indicator;
					index_pair	item_pos;				//The item index of the inline widget
					std::size_t	column_pos;
					::std::string text;					//text in UTF-8 encoded
				};

				std::map<pat::detail::abstract_factory_base*, std::deque<std::unique_ptr<inline_pane>>> inline_table, inline_buffered_table;

				essence_t()
				{
					scroll.offset_x = 0;
					pointer_where.first = parts::unknown;
					lister.fetch_ordering_comparer = std::bind(&es_header::fetch_comp, &header, std::placeholders::_1);
				}

                std::string to_string(const export_options& exp_opt) const
                {
                    return header.to_string(exp_opt) + exp_opt.endl + lister.to_string(exp_opt) ;
                }

                const index_pair& scroll_y_abs() const
				{
					return scroll.offset_y_abs;
				}
				const index_pair& scroll_y_dpl() const
				{
					return scroll.offset_y_dpl;
				}
				const index_pair& scroll_y_dpl_refresh()
				{
					return scroll.offset_y_dpl = lister.relative_pair(scroll.offset_y_abs);
				}

				void scroll_y_abs(const index_pair& pos_abs)
				{
					if (!lister.good(pos_abs.cat))
						return;

					scroll.offset_y_abs.cat = pos_abs.cat;

					size_type number = lister.size_item(pos_abs.cat);
					if(pos_abs.item < number)
						scroll.offset_y_abs.item = pos_abs.item;
					else if(number)
						scroll.offset_y_abs.item = number - 1;
					else
					{
						scroll.offset_y_abs.item = (pos_abs.cat > 0 ? npos : 0);
						scroll.offset_y_dpl = scroll.offset_y_abs ;
						return ;
					}
					scroll_y_dpl_refresh() ;
				}

				/// directly set a tested relative display pos
				void set_scroll_y_dpl(const index_pair& pos_dpl)
				{
					scroll.offset_y_dpl = pos_dpl;
					if (pos_dpl.is_category())
						scroll.offset_y_abs = pos_dpl;
					else
						scroll.offset_y_abs = lister.absolute_pair(pos_dpl);

					if (scroll.offset_y_abs.empty())
						throw std::invalid_argument("nana.listbox.set_scroll_y_dpl's exception is due to invalid item, please report a bug");
				}


				//number_of_lister_item
				//@brief: Returns the number of items that are contained in pixels
				//@param,with_rest: Means whether including extra one item that is not completely contained in reset pixels.
				size_type number_of_lister_items(bool with_rest) const
				{
					unsigned lister_s = graph->height() - 2 - header_visible_px() - (scroll.h.empty() ? 0 : scroll.scale);
					return (lister_s / item_size) + (with_rest && (lister_s % item_size) ? 1 : 0);
				}

				//keep the first selected item in the display area: the distances are in display positions!
                void trace_item_dpl( index_pair dpl_pos )
                {
                    if(      dpl_pos.cat <  scroll.offset_y_dpl.cat    // in prevoious cat    ---------------- up ----> we need to move
                        || ((dpl_pos.cat == scroll.offset_y_dpl.cat) && ( scroll.offset_y_dpl.item != npos)  // is our cat, where we are an item
                                                                     && (dpl_pos.item == npos || dpl_pos.item <  scroll.offset_y_dpl.item)))
					                                                                                    // problem!!!!!!
                    {
						if(lister.expand(dpl_pos.cat) == false)
						{
							if(lister.categ_selected(dpl_pos.cat))
								dpl_pos.item = static_cast<std::size_t>(npos);
							else
								lister.expand(dpl_pos.cat, true);
						}
                        set_scroll_y_dpl(dpl_pos);     //  <------------------------- set       scroll.offset_y_dpl   &     scroll.offset_y_abs
					}
					else
					{
						size_type numbers = number_of_lister_items(false);       // revise ... ok
						size_type off = lister.distance(scroll.offset_y_dpl, dpl_pos);
						if(numbers > off) return;
						index_pair n_off = lister.advance(scroll.offset_y_dpl, (off - numbers) + 1);

						if(n_off.cat != npos)       //  <------------------------- set       scroll.offset_y_dpl   &     scroll.offset_y_abs
                            set_scroll_y_dpl(n_off);
					}

					adjust_scroll_life();  // call adjust_scroll_value(); 		//adjust_scroll_value(); // again?
                }

                void trace_item_abs( index_pair abs_pos )
                {
					if(abs_pos.item == npos && abs_pos.cat              == scroll.offset_y_abs.cat
                                            && scroll.offset_y_abs.item == npos                      ) // if item==off y and is a cat
						return;

                    trace_item_dpl( lister.relative_pair(abs_pos))  ;   //  ???   scroll_y_dpl_refresh() ;
                }

                void trace_last_selected_item( )
                {
                    trace_item_abs(lister.last_selected_abs);
                }

				void update()
				{
					if(auto_draw && lister.wd_ptr())
					{
						adjust_scroll_life();
						API::refresh_window(lister.wd_ptr()->handle());
					}
				}

				void adjust_scroll_value()
				{
					const auto graph_size = graph->size();
					if(scroll.h.empty() == false)
					{
						const auto ext_px = (4 + (scroll.v.empty() ? 0 : scroll.scale - 1));
						if (ext_px > graph_size.width)
							return;

						const auto header_px = header.pixels();
						const unsigned window_px = graph_size.width - (4 + (scroll.v.empty() ? 0 : scroll.scale - 1));
						
						if (header_px < window_px + scroll.offset_x)
						{
							scroll.offset_x = header_px - window_px;
						}

						scroll.h.amount(header_px);
						scroll.h.range(window_px);
						scroll.h.value(scroll.offset_x);
						scroll.h.step(graph->text_extent_size(L"W").width);
					}

					if(scroll.v.empty() == false)
					{
						const auto ext_px = 2 + (scroll.h.empty() ? 0 : scroll.scale);
						if (ext_px >= graph_size.height)
							return;

						const auto items = lister.the_number_of_expanded();
						const auto disp_items = number_of_lister_items(false);

						size_type off = lister.distance({ 0, 0 }, scroll.offset_y_dpl);

						if (items < disp_items + off)
						{
							index_pair pos;
							if (lister.forward({ 0, 0 }, items - disp_items, pos))
							{
								off = items - disp_items;
								set_scroll_y_dpl(pos);
							}
						}

						scroll.v.amount(lister.the_number_of_expanded());
						scroll.v.range(number_of_lister_items(false));
						scroll.v.value(off);
					}
				}

				void adjust_scroll_life()  // at end call adjust_scroll_value();
				{
					internal_scope_guard lock;

					const nana::size sz = graph->size();
					unsigned header_s = header.pixels();
					window wd = lister.wd_ptr()->handle();

					//H scroll enabled
					bool h = (header_s > sz.width - 4);

					unsigned lister_s = sz.height - 2 - header_visible_px() - (h ? scroll.scale : 0);
					size_type screen_number = (lister_s / item_size);

					//V scroll enabled
					bool v = (lister.the_number_of_expanded() > screen_number);

					if(v == true && h == false)
						h = (header_s > (sz.width - 2 - scroll.scale));

					unsigned width = sz.width - 2 - (v ? scroll.scale : 0);
					unsigned height = sz.height - 2 - (h ? scroll.scale : 0);

					//event hander for scrollbars
					auto evt_fn = [this](const arg_scroll& arg)
					{
						if (scroll.h.empty() || (scroll.h.handle() != arg.window_handle))
						{
							index_pair item;
							if (!lister.forward(item, scroll.v.value(), item)) return;

							if (item == scroll.offset_y_dpl)
								return;

							set_scroll_y_dpl(item);
						}
						else
							scroll.offset_x = static_cast<int>(scroll.h.value());

						API::refresh_window(this->lister.wd_ptr()->handle());
					};

					if(h)
					{
						rectangle r(1, sz.height - scroll.scale - 1, width, scroll.scale);
						if(scroll.h.empty())
						{
							scroll.h.create(wd, r);
							API::take_active(scroll.h.handle(), false, wd);
							scroll.h.events().value_changed.connect_unignorable(evt_fn);
						}
						else
							scroll.h.move(r);
					}
					else if(!scroll.h.empty())
						scroll.h.close();

					if(v)
					{
						rectangle r(sz.width - 1 - scroll.scale, 1, scroll.scale, height);
						if(scroll.v.empty())
						{
							scroll.v.create(wd, r);
							API::take_active(scroll.v.handle(), false, wd);
							scroll.v.events().value_changed.connect_unignorable(evt_fn);
						}
						else
							scroll.v.move(r);

					}
					else if(!scroll.v.empty())
					{
						scroll.v.close();
                        set_scroll_y_dpl({0,0});

						nana::rectangle r;
						if(rect_header(r))
						{
							if(header_s > r.width)
							{
								if((header_s - scroll.offset_x) < r.width)
									scroll.offset_x = header_s - r.width;
							}
							else
								scroll.offset_x = 0;
						}
					}
					adjust_scroll_value();
				}

				void set_auto_draw(bool ad)
				{
					if(auto_draw != ad)
					{
						auto_draw = ad;
						if(ad)
						{
							adjust_scroll_life();
							API::refresh_window(lister.wd_ptr()->handle());
						}
					}
				}

				nana::rectangle checkarea(int x, int y) const
				{
					return nana::rectangle(x + 4, y + (item_size - 16) / 2, 16, 16);
				}

				int item_xpos(const nana::rectangle& r) const
				{
					auto seq = header_seq(r.width);
					return (seq.size() ? (header.item_pos(seq[0], nullptr) - scroll.offset_x + r.x) : 0);
				}

				std::pair<parts, size_t> where(int x, int y){
                    std::pair<parts, size_t> new_where;

					if(2 < x && x < static_cast<int>(graph->width()) - 2 && 1 < y && y < static_cast<int>(graph->height()) - 1)
					{
						if(header.visible() && y < static_cast<int>(header_size + 1))
						{
							x -= (2 - scroll.offset_x);
							new_where.first = parts::header;
							new_where.second = static_cast<int>(header.item_by_x(x));
						}
						else
						{
							new_where.second = ((y + 1) - header_visible_px()) / item_size; // y>1 !
							new_where.first = parts::lister;
							if(checkable)
							{
								nana::rectangle r;
								if(rect_lister(r))
								{
									std::size_t top = new_where.second * item_size + header_visible_px();
									if(checkarea(item_xpos(r), static_cast<int>(top)).is_hit(x, y))
										new_where.first = parts::checker;
								}
							}
						}
					}
					else
					{
						new_where.first = parts::unknown;
						new_where.second = npos;
					}
                    return new_where;
				}

				bool calc_where(int x, int y)
				{
                    std::pair< parts, size_t > new_where=where(x,y);
					if (new_where == pointer_where)
						return false;

					pointer_where = new_where;
					return true;
				}

				void widget_to_header(nana::point& pos)
				{
					--pos.y;
					pos.x += (scroll.offset_x - 2);
				}

				bool rect_header(nana::rectangle& r) const
				{
					if(header.visible())
					{
						if (lister.wd_ptr()->borderless())
						{
							r = graph->size();
							r.height = header_size;
							return !r.empty();
						}

						const unsigned ex_width = 4 + (scroll.v.empty() ? 0 : scroll.scale - 1);
						if(graph->width() > ex_width)
						{
							r.x = 2;
							r.y = 1;
							r.width = graph->width() - ex_width;
							r.height = header_size;
							return true;
						}
					}
					return false;
				}

				unsigned header_visible_px() const
				{
					return (header.visible() ? header_size : 0);
				}

				bool rect_lister(nana::rectangle& r) const
				{
					auto head_pixels = header_visible_px();
					unsigned width = (scroll.v.empty() ? 0 : scroll.scale - 1);
					unsigned height = (scroll.h.empty() ? 0 : scroll.scale) + head_pixels;

					if (!lister.wd_ptr()->borderless())
					{
						width += 4;
						height += 2;

						r.x = 2;
						r.y = head_pixels + 1;
					}
					else
					{
						r.x = 0;
						r.y = head_pixels;
					}

					nana::size gsz = graph->size();
					if(gsz.width <= width || gsz.height <= height) return false;

					r.width = gsz.width - width;
					r.height = gsz.height - height;
					return true;
				}

				bool wheel(bool upwards)
				{
					if(scroll.v.empty() || !scroll.v.scrollable(upwards))
						return false;

					index_pair target;
					if(upwards == false)
						lister.forward(scroll.offset_y_dpl, 1, target);
					else
						lister.backward(scroll.offset_y_dpl, 1, target);

					if (target == scroll.offset_y_dpl)
						return false;

					set_scroll_y_dpl ( target );
					return true;
				}

				std::vector<size_type> header_seq(unsigned lister_w)const
				{
					std::vector<size_type> seqs;
					int x = -(scroll.offset_x);
					for (const auto& hd : header.cont())
					{
						if (false == hd.visible) continue;
						x += static_cast<int>(hd.pixels);
						if (x > 0)
							seqs.push_back(hd.index);
						if (x >= static_cast<int>(lister_w))
							break;
					}

					return seqs;
				}

				unsigned auto_width(size_type pos, unsigned max = 3000) /// \todo introduce parametr max_header_width
				{
					unsigned max_w{ 0 };
					for (const auto &cat : lister.cat_container())
						for (const auto &it : cat.items)
						{
							if (pos >= it.cells.size()) continue;
							// precalcule text geometry
							unsigned ts = static_cast<unsigned> (graph->text_extent_size(it.cells[pos].text).width);
							if (max_w < ts)
								max_w = ts;
						}
					if (!max_w) return 0;

					unsigned ext_w = scheme_ptr->ext_w;
					if (pos == 0 && checkable)    //   only before the first column (display_order=0 ?)
						ext_w += 18;
					header.item_width(pos, max_w + ext_w + 1 < max ? max_w + ext_w + 1 : max);
					return max_w;
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

			class inline_indicator
				: public ::nana::detail::inline_widget_indicator<index_pair, std::string>
			{
			public:
				using parts = essence_t::parts;

				inline_indicator(essence_t* ess, std::size_t column_pos)
					: ess_{ ess }, column_pos_{column_pos}
				{
				}
				
				void attach(index_type pos, essence_t::inline_pane* pane)
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

				void detach()
				{
					panes_.clear();
				}
			public:
				//Implement inline_widget_indicator
				::nana::widget& host() const override
				{
					return *ess_->lister.wd_ptr();
				}

				void modify(index_type pos, const value_type& value) const override
				{
					auto & cells = ess_->lister.at_abs(pos).cells;
					if (cells.size() <= column_pos_)
						cells.resize(column_pos_ + 1);

					if (cells[column_pos_].text != value)
					{
						for (auto & pn : panes_)
						{
							if (pn.first == pos)
							{
								pn.second->text = value;
								break;
							}
						}

						cells[column_pos_].text = value;
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
					auto offset = ess_->lister.distance(ess_->scroll.offset_y_dpl, pos);

					if (ess_->pointer_where.first != parts::lister || ess_->pointer_where.second != offset)
					{
						ess_->pointer_where.first = parts::lister;
						ess_->pointer_where.second = offset;
						ess_->update();
					}
				}
			private:
				essence_t * const ess_;
				const std::size_t column_pos_;
				std::vector<std::pair<index_type, essence_t::inline_pane*>> panes_;
			};

			void es_lister::scroll(const index_pair& pos, bool to_bottom)
			{
				auto& cat = *get(pos.cat);

				if ((pos.item != ::nana::npos) && (pos.item >= cat.items.size()))
					throw std::invalid_argument("listbox: invalid pos to scroll");

				if (!cat.expand)
				{
					this->expand(pos.cat, true);
					ess_->adjust_scroll_life();
				}

				//The number of items can be displayed on screen
				auto view_items = ess_->number_of_lister_items(false) - 1;

				index_pair start_pos;
				if (to_bottom)
				{
					//start_pos will be (0,0) if backward fails
					backward(pos, view_items, start_pos);
				}
				else
				{
					if (forward(pos, view_items, start_pos))
						start_pos = pos;
					else
					{
						index_pair last(list_.size() - 1);

						if (list_.back().expand)
						{
							if (list_.back().items.empty())
								last.item = npos;
							else
								last.item = list_.back().items.size() - 1;
						}
						else
							last.item = ::nana::npos;

						backward(last, view_items, start_pos);
					}
				}

				ess_->set_scroll_y_dpl(start_pos);
				ess_->adjust_scroll_value();
			}

			void es_lister::erase(const index_pair& pos)
			{
				auto & catobj = *get(pos.cat);
				if (pos.item < catobj.items.size())
				{
					catobj.items.erase(catobj.items.begin() + pos.item);
					catobj.sorted.erase(std::find(catobj.sorted.begin(), catobj.sorted.end(), catobj.items.size()));

					sort();
				}
			}

			void es_lister::scroll_refresh()
			{
				ess_->scroll_y_dpl_refresh();
			}

			void es_lister::move_select(bool upwards, bool unselect_previous, bool trace_selected)
			{
				auto next_selected_dpl = relative_pair ( last_selected_abs); // last_selected_dpl; // ??
				if (next_selected_dpl.empty())  // has no cat ? (cat == npos) => beging from first cat
				{
					bool good = false;
					for(size_type i = 0, size = list_.size(); i < size; ++i) // run all cat
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
								if (size_categ() > next_selected_dpl.cat + 1)
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
									next_selected_dpl.cat = size_categ() - 1;
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
							if (unselect_previous && !single_selection_ )
                                select_for_all(false);

                            /// is ignored if no change (maybe set last_selected anyway??), but if change emit event, deselect others if need ans set/unset last_selected
                            item_proxy::from_display(ess_, next_selected_dpl).select(true);

                            if (trace_selected)
                                ess_->trace_item_dpl(next_selected_dpl);
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

					for (auto i : cat.sorted)
					{
						auto& it= cat.items[i] ;
						if(it.flags.selected || !exp_opt.only_selected_items)
							list_str += (it.to_string(exp_opt) + exp_opt.endl);
					}
				}
				return list_str ;
			}

            void es_lister::categ_selected(size_type cat, bool sel)
			{
                cat_proxy cpx{ess_,cat};
                for (item_proxy &it : cpx )
                {
                    if (it.selected() != sel)
						it.select(sel);
                }
                last_selected_abs = last_selected_dpl = index_pair{cat, npos};
			}

			class drawer_header_impl
			{
			public:
				using graph_reference = nana::paint::graphics&;
				using item_state = essence_t::item_state;
				using parts = essence_t::parts;

				drawer_header_impl(essence_t* es): essence_(es){}

				size_type item_spliter() const
				{
					return item_spliter_;
				}

				void cancel_spliter()
				{
					item_spliter_ = npos;
				}

				bool mouse_spliter(const nana::rectangle& r, int x)
				{
					if(essence_->ptr_state == item_state::highlighted)
					{
						x -= (r.x - essence_->scroll.offset_x);
						for(auto & hd : essence_->header.cont()) // in current order
						{
							if(hd.visible)
							{
								if((static_cast<int>(hd.pixels) < x + 2) && (x < static_cast<int>(hd.pixels) + 3))
								{
									item_spliter_ = hd.index; // original index
									return true;
								}
								x -= hd.pixels;
							}
						}
					}
					else if(essence_->ptr_state == item_state::normal)
						item_spliter_ = npos;
					return false;
				}

				void grab(const nana::point& pos, bool is_grab)
				{
					if(is_grab)
					{
						ref_xpos_ = pos.x;
						if(item_spliter_ != npos)
							orig_item_width_ = essence_->header.column(item_spliter_).pixels;
					}
					else if(grab_terminal_.index != npos && grab_terminal_.index != essence_->pointer_where.second)
						essence_->header.move(essence_->pointer_where.second, grab_terminal_.index, grab_terminal_.place_front);
				}

				//grab_move
				//@brief: draw when an item is grabbing.
				//@return: 0 = no graphics changed, 1 = just update, 2 = refresh
				int grab_move(const nana::rectangle& rect, const nana::point& pos)
				{
					if(item_spliter_ == npos)
					{
						draw(rect);
						_m_make_float(rect, pos);

						//Draw the target strip
						grab_terminal_.index = _m_target_strip(pos.x, rect, essence_->pointer_where.second, grab_terminal_.place_front);
						return 1;
					}
					else
					{
						const auto& item = essence_->header.column(item_spliter_);
						//Resize the item specified by item_spliter_.
						auto new_w = orig_item_width_ - (ref_xpos_ - pos.x);
						if(item.pixels != new_w)
						{
							essence_->header.item_width(item_spliter_, (new_w < (essence_->suspension_width + 20) ? essence_->suspension_width + 20 : new_w));
							new_w = essence_->header.pixels();
							if(new_w < (rect.width + essence_->scroll.offset_x))
								essence_->scroll.offset_x = (new_w > rect.width ? new_w - rect.width : 0);

							essence_->adjust_scroll_life();
							return 2;
						}
					}
					return 0;
				}

				void draw(const nana::rectangle& r)
				{
					graph_reference graph = *(essence_->graph);

					int text_top = (r.height - essence_->text_height) / 2 + r.y;
					auto text_color = essence_->lister.wd_ptr()->fgcolor();

					auto state = item_state::normal;
					//check whether grabing an item, if item_spliter_ != npos, that indicates the grab item is a spliter.
					if (essence_->pointer_where.first == parts::header && (item_spliter_ == npos))
						state = essence_->ptr_state;

					const unsigned height = r.height - 1;
					const int bottom_y = r.bottom() - 2;
					int x = r.x - essence_->scroll.offset_x;

					for (auto & i : essence_->header.cont())
					{
						if (i.visible)
						{
							int next_x = x + static_cast<int>(i.pixels);
							if (next_x > r.x)
							{
								_m_draw_header_item(graph, x, r.y, height, text_top, text_color, i, (i.index == essence_->pointer_where.second ? state : item_state::normal));
								graph.line({ next_x - 1, r.y }, { next_x - 1, bottom_y }, _m_border_color());
							}

							x = next_x;
							if (x > r.right())
								break;
						}
					}

					if (x < r.right())
						graph.rectangle({ x, r.y, static_cast<unsigned>(r.right() - x), height }, true, essence_->scheme_ptr->header_bgcolor);

					const int y = r.y + r.height - 1;
					essence_->graph->line({ r.x, y }, { r.x + static_cast<int>(r.width), y }, _m_border_color());
				}
			private:
				::nana::color _m_border_color() const
				{
					return essence_->scheme_ptr->header_bgcolor.get_color().blend(colors::black, 0.8);
				}

				size_type _m_target_strip(int x, const nana::rectangle& rect, size_type grab, bool& place_front)
				{
					//convert x to header logic coordinate.
					if(x < essence_->scroll.offset_x)
						x = essence_->scroll.offset_x;
					else if(x > essence_->scroll.offset_x + static_cast<int>(rect.width))
						x = essence_->scroll.offset_x + static_cast<int>(rect.width);

					size_type i = essence_->header.item_by_x(x);
					if(i == npos)
					{
						i = (essence_->header.item_pos(grab, nullptr) < x ? essence_->header.last() : essence_->header.begin());
					}
					if(grab != i)
					{
						unsigned item_pixels = 0;
						auto item_x = essence_->header.item_pos(i, &item_pixels);

						//Get the item pos
						//if mouse pos is at left of an item middle, the pos of itself otherwise the pos of the next.
						place_front = (x <= (item_x + static_cast<int>(item_pixels / 2)));
						x = (place_front ? item_x : essence_->header.item_pos(essence_->header.neighbor(i, false), nullptr));

						if(i != npos)
							essence_->graph->rectangle({ x - essence_->scroll.offset_x + rect.x, rect.y, 2, rect.height }, true, colors::red);

						return i;
					}
					return npos;
				}

				template<typename Item>
				void _m_draw_header_item(graph_reference graph, int x, int y, unsigned height, int txtop, const ::nana::color& fgcolor, const Item& item, item_state state)
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

					graph.gradual_rectangle({ x, y, item.pixels, height }, bgcolor.blend(colors::white, 0.9), bgcolor.blend(colors::black, 0.9), true);
					graph.string({ x + 5, txtop }, item.text, fgcolor);

					if(item.index == essence_->lister.sort_index())
					{
						facade<element::arrow> arrow("hollow_triangle");
						arrow.direction(essence_->lister.sort_reverse() ? ::nana::direction::south : ::nana::direction::north);
						arrow.draw(graph, {}, colors::black, { x + static_cast<int>(item.pixels - 16) / 2, -4, 16, 16 }, element_state::normal);
					}
				}

				void _m_make_float(const nana::rectangle& rect, const nana::point& pos)
				{
					const auto & item = essence_->header.column(essence_->pointer_where.second);

					nana::paint::graphics ext_graph({ item.pixels, essence_->header_size });
					ext_graph.typeface(essence_->graph->typeface());

					int txtop = (essence_->header_size - essence_->text_height) / 2;
					_m_draw_header_item(ext_graph, 0, 0, essence_->header_size, txtop, colors::white, item, item_state::floated);

					int xpos = essence_->header.item_pos(item.index, nullptr) + pos.x - ref_xpos_;
					ext_graph.blend(rectangle{ ext_graph.size() }, *(essence_->graph), nana::point(xpos - essence_->scroll.offset_x + rect.x, rect.y), 0.5);
				}

			private:
				int			ref_xpos_;
				unsigned	orig_item_width_;
				size_type	item_spliter_{npos};
				struct grab_terminal
				{
					size_type index;
					bool place_front;
				}grab_terminal_;
				essence_t * essence_;
			};

			class drawer_lister_impl
			{
			public:
				using item_state = essence_t::item_state;
				using parts = essence_t::parts;

				drawer_lister_impl(essence_t * es)
					:essence_(es)
				{}

				void draw(const nana::rectangle& rect) const
				{
                    internal_scope_guard lock;

					size_type n = essence_->number_of_lister_items(true);
					if(0 == n)return;
					widget * wdptr = essence_->lister.wd_ptr();
					auto bgcolor = wdptr->bgcolor();
					auto fgcolor = wdptr->fgcolor();

					unsigned header_w = essence_->header.pixels();
					essence_->graph->palette(false, bgcolor);
					if(header_w - essence_->scroll.offset_x < rect.width)
						essence_->graph->rectangle(rectangle{ point(rect.x + static_cast<int>(header_w)-essence_->scroll.offset_x,  rect.y),
                                                              size(static_cast<int>(rect.width)  + essence_->scroll.offset_x - static_cast<int>(header_w),  rect.height) },
                                                    true);

					es_lister & lister = essence_->lister;
					//The Tracker indicates the item where mouse placed.
					index_pair tracker(npos, npos);
					auto & ptr_where = essence_->pointer_where;

                    //if where == lister || where == checker, 'second' indicates the offset to the  relative display-order pos of the scroll offset_y which stands for the first item to be displayed in lister.
					if((ptr_where.first == parts::lister || ptr_where.first == parts::checker) && ptr_where.second != npos)
						lister.forward(essence_->scroll.offset_y_dpl, ptr_where.second, tracker);

					auto subitems = essence_->header_seq(rect.width);

					if(subitems.empty())
						return;

					int x = essence_->item_xpos(rect);
					int y = rect.y;
					int txtoff = (essence_->item_size - essence_->text_height) / 2;

					auto i_categ = lister.get(essence_->scroll.offset_y_dpl.cat);

					auto idx = essence_->scroll.offset_y_dpl;

					auto state = item_state::normal;

					essence_->inline_buffered_table.swap(essence_->inline_table);

					for(auto & cat : lister.cat_container())
						for (auto & ind : cat.indicators)
						{
							if (ind)
								ind->detach();
						}

					//Here we draw the root categ (0) or a first item if the first drawing is not a categ.(item!=npos))
					if(idx.cat == 0 || !idx.is_category())
					{
						if (idx.cat == 0 && idx.is_category())  // the 0 cat
						{
							essence_->scroll.offset_y_dpl.item = 0;  // no, we draw the first item of cat 0, not the 0 cat itself
							idx.item = 0;
						}

						std::size_t size = i_categ->items.size();
						index_pair item_index{ idx.cat, 0 };
						for(std::size_t offs = essence_->scroll.offset_y_dpl.item; offs < size; ++offs, ++idx.item)
						{
							if(n-- == 0)	break;
							state = (tracker == idx	? item_state::highlighted : item_state::normal);

							item_index.item = offs;
							item_index = lister.absolute_pair(item_index);

							_m_draw_item(*i_categ, item_index, x, y, txtoff, header_w, rect, subitems, bgcolor,fgcolor, state);
							y += essence_->item_size;
						}

						++i_categ;
						++idx.cat;
					}

					for(; i_categ != lister.cat_container().end(); ++i_categ, ++idx.cat)
					{
						if(n-- == 0) break;
						idx.item = 0;

						state = (tracker.is_category() && (idx.cat == tracker.cat) ? item_state::highlighted : item_state::normal);

						_m_draw_categ(*i_categ, rect.x - essence_->scroll.offset_x, y, txtoff, header_w, rect, bgcolor, state);
						y += essence_->item_size;

						if(false == i_categ->expand)
							continue;

						auto size = i_categ->items.size();
						index_pair item_pos{ idx.cat, 0 };
						for(decltype(size) pos = 0; pos < size; ++pos)
						{
							if(n-- == 0)	break;
							state = (idx == tracker ? item_state::highlighted : item_state::normal);

							item_pos.item = pos;
							item_pos.item = lister.absolute(item_pos);

							_m_draw_item(*i_categ, item_pos, x, y, txtoff, header_w, rect, subitems, bgcolor, fgcolor, state);
							y += essence_->item_size;
							++idx.item;
						}
					}

					essence_->inline_buffered_table.clear();

					if (y < rect.bottom())
						essence_->graph->rectangle(rectangle{ rect.x, y, rect.width, static_cast<unsigned>(rect.bottom() - y) }, true, bgcolor);
				}
			private:
				void _m_draw_categ(const category_t& categ, int x, int y, int txtoff, unsigned width, const nana::rectangle& r, nana::color bgcolor, item_state state) const
				{
					const bool sel = categ.selected();
					if (sel && (categ.expand == false))
						bgcolor = static_cast<color_rgb>(0xD5EFFC);

					if (state == item_state::highlighted)
						bgcolor = bgcolor.blend(static_cast<color_rgb>(0x99defd), 0.8);

					auto graph = essence_->graph;
					graph->rectangle(rectangle{ x, y, width, essence_->item_size }, true, bgcolor);

					color txt_color{ static_cast<color_rgb>(0x3399) };

					facade<element::arrow> arrow("double");
					arrow.direction(categ.expand ? ::nana::direction::north : ::nana::direction::south);
					arrow.draw(	*graph, {}, txt_color,
								{ x + 5, y + static_cast<int>(essence_->item_size - 16) / 2, 16, 16 },
								element_state::normal);

					graph->string({ x + 20, y + txtoff }, categ.text, txt_color);

					native_string_type str = to_nstring('(' + std::to_string(categ.items.size()) + ')');

					auto text_s = graph->text_extent_size(categ.text).width;
					auto extend_text_w = text_s + graph->text_extent_size(str).width;

					graph->string({ x + 25 + static_cast<int>(text_s), y + txtoff }, str);

					if (x + 35 + extend_text_w < x + width)
					{
						::nana::point pos{ x + 30 + static_cast<int>(extend_text_w), y + static_cast<int>(essence_->item_size) / 2 };
						graph->line(pos, { x + static_cast<int>(width)-5, pos.y }, txt_color);
					}

					//Draw selecting inner rectangle
					if (sel && (categ.expand == false))
					{
						width -= essence_->scroll.offset_x;
						_m_draw_border(r.x, y, (r.width < width ? r.width : width));
					}
				}

				//Draws an item
				//@param content_r the rectangle of list content
				void _m_draw_item(const category_t& cat, const index_pair& item_pos, const int x, const int y, const int txtoff, unsigned width, const nana::rectangle& content_r, const std::vector<size_type>& seqs, nana::color bgcolor, nana::color fgcolor, item_state state) const
				{
					auto & item = cat.items[item_pos.item];

					if (item.flags.selected)                                    // fetch the "def" colors
						bgcolor = essence_->scheme_ptr->item_selected;
					else if (!item.bgcolor.invisible())
						bgcolor = item.bgcolor;

					if(!item.fgcolor.invisible())
						fgcolor = item.fgcolor;

					if (item_state::highlighted == state)                          // and blend it if "highlighted"
					{
						if (item.flags.selected)
							bgcolor = bgcolor.blend(colors::black, 0.98);           // or "selected"
						else
							bgcolor = bgcolor.blend(essence_->scheme_ptr->item_selected, 0.7);
					}

					unsigned show_w = width - essence_->scroll.offset_x;
					if(show_w >= content_r.width) show_w = content_r.width;

					auto graph = essence_->graph;
					//draw the background
					graph->rectangle(rectangle{ content_r.x, y, show_w, essence_->item_size }, true, bgcolor);

					int item_xpos         = x;
					unsigned extreme_text = x;

					for (size_type display_order{ 0 }; display_order < seqs.size(); ++display_order)  // get the cell (column) index in the order headers are displayed
					{
						const auto column_pos = seqs[display_order];
						const auto & header = essence_->header.column(column_pos);     // deduce the corresponding header which is in a kind of dislay order
						auto it_bgcolor = bgcolor;

						if (header.pixels > 5)
						{
							int content_pos = 5;

							//Draw the image in the 1st column in display order
							if (0 == display_order)
							{
								if (essence_->checkable)
								{
									content_pos += 18;

									element_state estate = element_state::normal;
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
									crook_renderer_.draw(*graph, bgcolor, fgcolor, essence_->checkarea(item_xpos, y), estate);
								}

								if (essence_->if_image)
								{
									//Draw the image in the 1st column in display order
									if (item.img)
									{
										nana::rectangle img_r(item.img_show_size);
										img_r.x = content_pos + item_xpos + static_cast<int>(16 - item.img_show_size.width) / 2;
										img_r.y = y + static_cast<int>(essence_->item_size - item.img_show_size.height) / 2;
										item.img.stretch(rectangle{ item.img.size() }, *graph, img_r);
									}
									content_pos += 18;
								}
							}

							bool draw_column = true;

							if (static_cast<unsigned>(content_pos) < header.pixels)
							{
								auto inline_wdg = _m_get_inline_pane(cat, column_pos);
								if (inline_wdg)
								{
									//Make sure the user-define inline widgets in right visible rectangle.
									rectangle pane_r;
									auto wdg_x = item_xpos + content_pos;
									auto wdg_w = header.pixels - static_cast<unsigned>(content_pos);

									bool visible_state = true;
									if (::nana::overlap(content_r, { wdg_x, y, wdg_w, essence_->item_size }, pane_r))
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

									::nana::size sz{ wdg_w, essence_->item_size };
									inline_wdg->pane_widget.size(sz);
									inline_wdg->inline_ptr->resize(sz);

									draw_column = inline_wdg->inline_ptr->whether_to_draw();

									inline_wdg->item_pos = item_pos;
									inline_wdg->column_pos = column_pos;
									inline_wdg->inline_ptr->activate(*inline_wdg->indicator, item_pos);
									
									inline_wdg->indicator->attach(item_pos, inline_wdg);

									//To reduce the memory usage, the cells may not be allocated
									if (item.cells.size() > column_pos)
									{
										auto & text = item.cells[column_pos].text;
										if (text != inline_wdg->text)
										{
											inline_wdg->text = text;
											inline_wdg->inline_ptr->set(text);
										}
									}
									else
									{
										inline_wdg->text.clear();
										inline_wdg->inline_ptr->set({});
									}

									API::show_window(inline_wdg->pane_bottom, visible_state);
								}
							}

							if (item.cells.size() > column_pos)        // process only if the cell is visible
							{
								auto cell_txtcolor = fgcolor;
								auto & m_cell = item.cells[column_pos];
								nana::size ts = graph->text_extent_size(m_cell.text);        // precalcule text geometry

								if (m_cell.custom_format && (!m_cell.custom_format->bgcolor.invisible()))  // adapt to costum format if need
								{
									it_bgcolor = m_cell.custom_format->bgcolor;
									if (item.flags.selected)
										it_bgcolor = it_bgcolor.blend(bgcolor, 0.5);
									if (item_state::highlighted == state)
										it_bgcolor = it_bgcolor.blend(static_cast<color_rgb>(0x99defd), 0.8);

									graph->rectangle(rectangle{ item_xpos, y, header.pixels, essence_->item_size }, true, it_bgcolor);

									cell_txtcolor = m_cell.custom_format->fgcolor;
								}

								if (draw_column)
								{
									graph->string(point{ item_xpos + content_pos, y + txtoff }, m_cell.text, cell_txtcolor); // draw full text of the cell index (column)

									if (static_cast<int>(ts.width) > static_cast<int>(header.pixels) - (content_pos + item_xpos))	// it was an excess
									{
										//The text is painted over the next subitem                // here beging the ...
										int xpos = item_xpos + static_cast<int>(header.pixels) - static_cast<int>(essence_->suspension_width);

										// litter rect with the  item bg end ...
										graph->rectangle(rectangle{ xpos, y + 2, essence_->suspension_width, essence_->item_size - 4 }, true, it_bgcolor);
										graph->string(point{ xpos, y + 2 }, L"...");

										//Erase the part that over the next subitem only if the right of column is less than right of listbox
										if (item_xpos + content_pos < content_r.right() - static_cast<int>(header.pixels))
										{
											graph->palette(false, bgcolor);       // we need to erase the excess, because some cell may not draw text over
											graph->rectangle(rectangle{ item_xpos + static_cast<int>(header.pixels), y + 2,
												ts.width + static_cast<unsigned>(content_pos)-header.pixels, essence_->item_size - 4 }, true);
										}
										extreme_text = (std::max)(extreme_text, item_xpos + content_pos + ts.width);
									}
								}
							}

							graph->line({ item_xpos - 1, y }, { item_xpos - 1, y + static_cast<int>(essence_->item_size) - 1 }, static_cast<color_rgb>(0xEBF4F9));

						}

						item_xpos += static_cast<int>(header.pixels);
						if (display_order + 1 >= seqs.size() && static_cast<int>(extreme_text) > item_xpos)
						{
							graph->rectangle(rectangle{item_xpos , y + 2, extreme_text - item_xpos, essence_->item_size - 4}, true, item.bgcolor);
						}
					}

					//Draw selecting inner rectangle
					if(item.flags.selected)
						_m_draw_border(content_r.x, y, show_w);
				}

				essence_t::inline_pane * _m_get_inline_pane(const category_t& cat, std::size_t column_pos) const
				{
					if (column_pos < cat.factories.size())
					{
						auto & factory = cat.factories[column_pos];
						if (factory)
							return essence_->open_inline(factory.get(), cat.indicators[column_pos].get());
					}
					return nullptr;
				}

				essence_t::inline_pane* _m_find_inline_pane(const index_pair& pos, std::size_t column_pos) const
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

				void _m_draw_border(int x, int y, unsigned width) const
				{
					//Draw selecting inner rectangle
					auto graph = essence_->graph;
					graph->rectangle({ x, y, width, essence_->item_size }, false, static_cast<color_rgb>(0x99defd));

					graph->rectangle({ x + 1, y + 1, width - 2, essence_->item_size - 2 }, false, colors::white);
					graph->set_pixel(x, y);
					graph->set_pixel(x, y + essence_->item_size - 1);
					graph->set_pixel(x + width - 1, y);
					graph->set_pixel(x + width - 1, y + essence_->item_size - 1);
				}
			private:
				essence_t * essence_;
				mutable facade<element::crook> crook_renderer_;
			};

			//class trigger: public drawer_trigger
				trigger::trigger()
					:	essence_(new essence_t),
						drawer_header_(new drawer_header_impl(essence_)),
						drawer_lister_(new drawer_lister_impl(essence_))
				{}

				trigger::~trigger()
				{
					delete drawer_lister_;
					delete drawer_header_;
					delete essence_;
				}

				essence_t& trigger::essence() const
				{
					return *essence_;
				}

				void trigger::_m_draw_border()
				{
					if (API::widget_borderless(*essence_->lister.wd_ptr()))
						return;

					auto & graph = *essence_->graph;
					auto size = graph.size();
					//Draw Border
					graph.rectangle(false, static_cast<color_rgb>(0x9cb6c5));
					graph.line({ 1, 1 }, {1, static_cast<int>(size.height) - 2}, colors::white);
					graph.line({ static_cast<int>(size.width) - 2, 1 }, { static_cast<int>(size.width) - 2, static_cast<int>(size.height) - 2 });

					if ((essence_->scroll.h.empty() == false) && (essence_->scroll.v.empty() == false))
						graph.rectangle({ static_cast<int>(size.width - 1 - essence_->scroll.scale),
											static_cast<int>(size.height - 1 - essence_->scroll.scale),
											essence_->scroll.scale,
											essence_->scroll.scale },
										true, colors::button_face);
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					essence_->scheme_ptr = static_cast<::nana::listbox::scheme_type*>(API::dev::get_scheme(widget));
					essence_->graph = &graph;
					typeface_changed(graph);

					essence_->lister.bind(essence_, widget);
					widget.bgcolor(colors::white);
				}

				void trigger::detached()
				{
					essence_->graph = nullptr;
				}

				void trigger::typeface_changed(graph_reference graph)
				{
					essence_->text_height = graph.text_extent_size(L"jHWn0123456789/<?'{[|\\_").height;
					essence_->item_size = essence_->text_height + 6;
					essence_->suspension_width = graph.text_extent_size(L"...").width;
				}

				void trigger::refresh(graph_reference)
				{
					if (API::is_destroying(essence_->lister.wd_ptr()->handle()))
						return;

					nana::rectangle r;

					if (essence_->header.visible() && essence_->rect_header(r))
						drawer_header_->draw(r);
					if (essence_->rect_lister(r))
						drawer_lister_->draw(r);
					_m_draw_border();
				}

				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					using item_state = essence_t::item_state;
					using parts = essence_t::parts;
					int update = 0; //0 = nothing, 1 = update, 2 = refresh
					if(essence_->ptr_state == item_state::pressed)
					{
						if(essence_->pointer_where.first == parts::header)
						{
							essence_->ptr_state = item_state::grabbed;
							nana::point pos = arg.pos;
							essence_->widget_to_header(pos);
							drawer_header_->grab(pos, true);
							API::capture_window(essence_->lister.wd_ptr()->handle(), true);
							update = 2;
						}
					}

                    if(essence_->ptr_state == item_state::grabbed)
					{
						nana::point pos = arg.pos;
						essence_->widget_to_header(pos);

						nana::rectangle r;
						essence_->rect_header(r);
						update = drawer_header_->grab_move(r, pos);
					}
					else if(essence_->calc_where(arg.pos.x, arg.pos.y))
					{
						essence_->ptr_state = item_state::highlighted;
						update = 2;
					}

					bool set_spliter = false;
					if(essence_->pointer_where.first == parts::header)
					{
						nana::rectangle r;
						if(essence_->rect_header(r))
						{
							if(drawer_header_->mouse_spliter(r, arg.pos.x))
							{
								set_spliter = true;
								essence_->lister.wd_ptr()->cursor(cursor::size_we);
							}
						}
					}
					if(set_spliter == false && essence_->ptr_state != item_state::grabbed)
					{
						if((drawer_header_->item_spliter() != npos) || (essence_->lister.wd_ptr()->cursor() == cursor::size_we))
						{
							essence_->lister.wd_ptr()->cursor(cursor::arrow);
							drawer_header_->cancel_spliter();
							update = 2;
						}
					}

					if (update)
					{
						if (2 == update)
							refresh(graph);

						API::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference graph, const arg_mouse&)
				{
					using item_state = essence_t::item_state;
					using parts = essence_t::parts;
					if((essence_->pointer_where.first != parts::unknown) || (essence_->ptr_state != item_state::normal))
					{
						if (essence_->ptr_state != item_state::grabbed)
						{
							essence_->pointer_where.first = parts::unknown;
							essence_->ptr_state = item_state::normal;
						}

						refresh(graph);
						API::lazy_refresh();
					}
				}

				void trigger::mouse_down(graph_reference, const arg_mouse& arg)
				{
					using item_state = essence_t::item_state;
					using parts = essence_t::parts;
					bool update = false;
					auto & ptr_where = essence_->pointer_where;
					if((ptr_where.first == parts::header) && (ptr_where.second != npos || (drawer_header_->item_spliter() != npos)))
					{
						essence_->ptr_state = item_state::pressed;
						nana::rectangle r;
						if(essence_->rect_header(r))
						{
							drawer_header_->draw(r);
							update = true;
						}
					}
					else if(ptr_where.first == parts::lister || ptr_where.first == parts::checker)
					{
						auto & lister = essence_->lister;
						index_pair item_pos;
						if (lister.forward(essence_->scroll.offset_y_dpl, ptr_where.second, item_pos))
						{
							auto * item_ptr = (item_pos.is_item() ? &lister.at(item_pos) : nullptr);
							if(ptr_where.first == parts::lister)
							{
								bool sel = true;
								if (!lister.single_selection())
								{
									if (arg.shift)
										lister.select_display_range(lister.last_selected_abs , item_pos, sel);
									else if (arg.ctrl)
										sel = !item_proxy(essence_, index_pair (item_pos.cat, lister.absolute(item_pos))).selected();
									else
										lister.select_for_all(false);	//cancel all selections
								}
								else
									sel = !item_proxy(essence_, index_pair (item_pos.cat, lister.absolute(item_pos))).selected();

								if(item_ptr)
								{
									item_ptr->flags.selected = sel;
									index_pair last_selected(item_pos.cat, lister.absolute(item_pos));

									arg_listbox arg{item_proxy{essence_, last_selected}, sel};
									lister.wd_ptr()->events().selected.emit(arg);

									if (item_ptr->flags.selected)
									{
										lister.cancel_others_if_single_enabled(true, last_selected);
										essence_->lister.last_selected_abs = last_selected;

									}
									else if (essence_->lister.last_selected_abs == last_selected)
										essence_->lister.last_selected_abs.set_both(npos);
								}
								else if(!lister.single_selection())
									lister.categ_selected(item_pos.cat, true);
							}
							else
							{
								if(item_ptr)
								{
									item_ptr->flags.checked = ! item_ptr->flags.checked;

									index_pair abs_pos{ item_pos.cat, lister.absolute(item_pos) };
									arg_listbox arg{ item_proxy{ essence_, abs_pos }, item_ptr->flags.checked };
									lister.wd_ptr()->events().checked.emit(arg);

									if (item_ptr->flags.checked)
										lister.cancel_others_if_single_enabled(false, abs_pos);
								}
								else if (! lister.single_check())
									lister.categ_checked_reverse(item_pos.cat);
							}
							update = true;
						}
						else
							update = lister.select_for_all(false); //unselect all items due to the blank area being clicked

						if(update)
						{
							nana::rectangle r;
							update = essence_->rect_lister(r);
							if(update)
								drawer_lister_->draw(r);
						}
					}

					if(update)
					{
						_m_draw_border();
						API::lazy_refresh();
					}
				}


				void trigger::mouse_up(graph_reference graph, const arg_mouse& arg)
				{
					using item_state = essence_t::item_state;
					using parts = essence_t::parts;

					auto prev_state = essence_->ptr_state;
					essence_->ptr_state = item_state::highlighted;
					//Do sort
					if (essence_->header.sortable() && essence_->pointer_where.first == parts::header && prev_state == item_state::pressed)
					{
						if(essence_->pointer_where.second < essence_->header.cont().size())
						{
							if(essence_->lister.sort_index(essence_->pointer_where.second))
							{
								essence_->trace_item_dpl({0,0});
								refresh(graph);
								API::lazy_refresh();
							}
						}
					}
					else if (prev_state == item_state::grabbed)
					{
						nana::point pos = arg.pos;
						essence_->widget_to_header(pos);
						drawer_header_->grab(pos, false);
						refresh(graph);
						API::lazy_refresh();
						API::capture_window(essence_->lister.wd_ptr()->handle(), false);
					}
				}

				void trigger::mouse_wheel(graph_reference graph, const arg_wheel& arg)
				{
					if(essence_->wheel(arg.upwards))
					{
						refresh(graph);
						essence_->adjust_scroll_value();
						API::lazy_refresh();
					}
				}

				void trigger::dbl_click(graph_reference graph, const arg_mouse& arg)
				{
					if (essence_->pointer_where.first == essence_t::parts::header)
						if (cursor::size_we == essence_->lister.wd_ptr()->cursor())
                        {
 			               if (essence(). auto_width(drawer_header_->item_spliter() )) // ? in order
			                   essence().update();
                           return;
                       }

					if (essence_->pointer_where.first != essence_t::parts::lister)
						return;

					index_pair item_pos;
					auto & offset_y = essence_->scroll.offset_y_dpl;
					auto & lister = essence_->lister;
					//Get the item which the mouse is placed.
					if (lister.forward(offset_y, essence_->pointer_where.second, item_pos))
					{
						if (!item_pos.is_category())	//being the npos of item.second is a category
							return;

                        arg_category ai(cat_proxy(essence_, item_pos.cat));
                        lister.wd_ptr()->events().category_dbl_click.emit(ai);

                        if(!ai.category_change_blocked()){
                            bool do_expand = (lister.expand(item_pos.cat) == false);
                            lister.expand(item_pos.cat, do_expand);

                            if(false == do_expand)
                            {
                                auto last = lister.last();
                                size_type n = essence_->number_of_lister_items(false);
                                if (lister.backward(last, n, last))
                                    offset_y = last;
                            }
                            essence_->adjust_scroll_life();
                            refresh(graph);
                            API::lazy_refresh();
                        }
					}
				}

				void trigger::resized(graph_reference graph, const arg_resized&)
				{
					essence_->adjust_scroll_life();
					refresh(graph);
					API::lazy_refresh();
				}

				void trigger::key_press(graph_reference graph, const arg_keyboard& arg)
				{
					bool up = false;

                    if (essence_->lister.size_categ()==1 && essence_->lister.size_item(0)==0)
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
							selection s;
							bool ck = ! essence_->lister.item_selected_all_checked(s);
							for(auto i : s)
								item_proxy(essence_, i).check(ck);
						}
						break;

                    case keyboard::os_pageup :
						up = true;
                    case keyboard::os_pagedown:
                    {
					    auto& scrl = essence_->scroll.v;
                        if (! scrl.make_page_scroll(!up))
                            return;
                        essence_->lister.select_for_all(false);

                        index_pair idx{essence_->scroll_y_dpl()};
                        if (!up)
                            essence_->lister.forward(idx, scrl.range()-1, idx);

                        if (idx.is_item())
                           item_proxy::from_display(essence_, idx).select(true);
                        else
                            if(!essence_->lister.single_selection())
								essence_->lister.categ_selected(idx.cat, true);

                        essence_->trace_last_selected_item ();

                        break;
                    }
                    case keyboard::os_home:
                    {
                        essence_->lister.select_for_all(false);

                        index_pair frst{essence_->lister.first()};
                        if (frst.is_item())
                           item_proxy::from_display(essence_, frst).select(true);
                        else
                            if(!essence_->lister.single_selection())
								essence_->lister.categ_selected(frst.cat, true);

                        essence_->trace_last_selected_item ();
                        break;
                    }
                    case keyboard::os_end:
                        essence_->lister.select_for_all(false);
                        item_proxy::from_display(essence_, essence_->lister.last()).select(true);
                        essence_->trace_last_selected_item ();
                       break;

					default:
						return;
					}
					refresh(graph);
					API::lazy_refresh();
				}

				void trigger::key_char(graph_reference graph, const arg_keyboard& arg)
				{
					switch(arg.key)
					{
                    case keyboard::copy:
                    {
                        export_options exp_opt {essence_->def_exp_options};
                        exp_opt.columns_order = essence_->header.all_headers(true);
                        exp_opt.only_selected_items = true;
                        ::nana::system::dataexch().set(essence_->to_string(exp_opt));
                        return;
                    }
                    case keyboard::select_all :
                        essence_->lister.select_for_all(true);
						refresh(graph);
					    API::lazy_refresh();
                        break;

					default:
						return;
					}
				}

			//end class trigger

			//class item_proxy
				item_proxy::item_proxy(essence_t * ess)
					:	ess_(ess)
				{}

				item_proxy::item_proxy(essence_t * ess, const index_pair& pos)
					:	ess_(ess),
						pos_(pos)
				{
					if (ess)
					{
						auto i = ess_->lister.get(pos.cat);
						cat_ = &(*i);       // what is pos is a cat?
					}
				}

                /// the main porpose of this it to make obvious that item_proxy operate with absolute positions, and dont get moved during sort()
                item_proxy item_proxy::from_display(essence_t *ess, const index_pair &relative)
                {
                    return item_proxy{ess, ess->lister.absolute_pair(relative)};
                }
                item_proxy item_proxy::from_display(const index_pair &relative) const
                {
                    return item_proxy{ess_, ess_->lister.absolute_pair(relative)};
                }

                /// posible use: last_selected_display = last_selected.to_display().item; use with caution, it get invalidated after a sort()
                index_pair item_proxy::to_display() const
                {
                    return ess_->lister.relative_pair(pos_);
                }

                bool item_proxy::empty() const
				{
					return !ess_;
				}

				item_proxy & item_proxy::check(bool ck)
				{
					auto & m = cat_->items.at(pos_.item);
					if(m.flags.checked != ck)
					{
						m.flags.checked = ck;
						arg_listbox arg{*this, ck};
						ess_->lister.wd_ptr()->events().checked.emit(arg);
						ess_->update();
					}
					return *this;
				}

				bool item_proxy::checked() const
				{
					return cat_->items.at(pos_.item).flags.checked;
				}

                /// is ignored if no change (maybe set last_selected anyway??), but if change emit event, deselect others if need ans set/unset last_selected
                item_proxy & item_proxy::select(bool s)
				{
					auto & m = cat_->items.at(pos_.item);       // a ref to the real item     // what is pos is a cat?
					if(m.flags.selected == s) return *this;     // ignore if no change
					m.flags.selected = s;                       // actually change selection

                    arg_listbox arg{*this, s};
					ess_->lister.wd_ptr()->events().selected.emit(arg);

					if (m.flags.selected)
					{
						ess_->lister.cancel_others_if_single_enabled(true, pos_);	//Cancel all selections except pos_ if single_selection is enabled.
						ess_->lister.last_selected_abs = pos_;
					}
					else if (ess_->lister.last_selected_abs == pos_)
							ess_->lister.last_selected_abs.set_both(npos);

					ess_->update();

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

				std::size_t item_proxy::columns() const
				{
					return ess_->header.cont().size();
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

				item_proxy& item_proxy::text(size_type col, std::wstring str)
				{
					ess_->lister.text(cat_, pos_.item, col, to_utf8(str), columns());
					ess_->update();
					return *this;
				}

				std::string item_proxy::text(size_type col) const
				{
					return ess_->lister.get_cells(cat_, pos_.item).at(col).text;
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
					return (ess_->lister.get_cells(cat_, pos_.item).at(0).text == s);
				}

				bool item_proxy::operator==(const std::wstring& s) const
				{
					return (ess_->lister.get_cells(cat_, pos_.item).at(0).text == to_utf8(s));
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
				essence_t * item_proxy::_m_ess() const
				{
					return ess_;
				}

				index_pair item_proxy::pos() const
				{
					return pos_;
				}

				auto item_proxy::_m_cells() const -> std::vector<cell>&
				{
					return ess_->lister.get_cells(cat_, pos_.item);
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
				cat_proxy::cat_proxy(essence_t * ess, size_type pos)
					:	ess_(ess),
						pos_(pos)
				{
					_m_cat_by_pos();
				}

				cat_proxy::cat_proxy(essence_t* ess, category_t* cat)
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

				void cat_proxy::append(std::initializer_list<std::string> arg)
				{
					const auto items = columns();
					push_back(std::string{});
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
					push_back(std::string{});
					item_proxy ip{ ess_, index_pair(pos_, size() - 1) };
					size_type pos = 0;
					for (auto & txt : arg)
					{
						ip.text(pos++, to_utf8(txt));
						if (pos >= items)
							break;
					}
				}

				cat_proxy & cat_proxy::select(bool sel)
                {
                    for (item_proxy &it : *this )
                        it.select(sel);

                    ess_->lister.last_selected_abs = ess_->lister.last_selected_dpl =  index_pair {this->pos_, npos};

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
						cat_->text.swap(text);
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
						cat_->text.swap(text);
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

					cat_->sorted.push_back(cat_->items.size());
					cat_->items.emplace_back(std::move(s));

					auto wd = ess_->lister.wd_ptr();
					if(wd && !(API::empty_window(wd->handle())))
					{
						auto & m = cat_->items.back();
						m.bgcolor = wd->bgcolor();
						m.fgcolor = wd->fgcolor();
						ess_->update();
					}
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
					if(pos_abs >= size())
						throw std::out_of_range("listbox.cat_proxy.at() invalid position");
					return item_proxy(ess_, index_pair(pos_, pos_abs));
				}

				item_proxy cat_proxy::back() const
				{
					if (cat_->items.empty())
						throw std::runtime_error("listbox.back() no element in the container.");

					return item_proxy(ess_, index_pair(pos_, cat_->items.size() - 1));
				}

				/// convert from display order to absolute (find the real item in that display pos) but without check from current active sorting, in fact using just the last sorting !!!
				size_type cat_proxy::index_by_display_order(size_type display_order_pos) const
				{
					return ess_->lister.index_by_display_order(pos_, display_order_pos);
				}

        		/// find display order for the real item but without check from current active sorting, in fact using just the last sorting !!!
				size_type cat_proxy::display_order(size_type pos) const
				{
					return ess_->lister.display_order(pos_, pos);
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
					if (column >= ess_->header.cont().size())
						throw std::out_of_range("listbox.cat_proxy.inline_factory: invalid column index");

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

					cat_->sorted.push_back(cat_->items.size());
					cells.resize(columns());
					cat_->items.emplace_back(std::move(cells));

					auto wd = ess_->lister.wd_ptr();
					if (wd && !(API::empty_window(wd->handle())))
					{
						auto & m = cat_->items.back();
						m.bgcolor = wd->bgcolor();
						m.fgcolor = wd->fgcolor();
					}
				}

				void cat_proxy::_m_cat_by_pos()
				{
					if (pos_ >= ess_->lister.size_categ())
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
				void cat_proxy::_m_update() {
					ess_->update();
				}

			//class cat_proxy

			//end class cat_proxy
		}
	}//end namespace drawerbase

	arg_listbox::arg_listbox(const drawerbase::listbox::item_proxy& m, bool selected) noexcept
		: item(m), selected(selected)
	{
	}


	//Implementation of arg_category
	//Contributed by leobackes(pr#97)
	arg_category::arg_category ( const nana::drawerbase::listbox::cat_proxy& cat ) noexcept
		: category(cat), block_change_(false)
    {
    }

    void arg_category::block_category_change() const noexcept
	{
		block_change_ = true;
    }

    bool arg_category::category_change_blocked() const noexcept
	{
		return block_change_;
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

		void listbox::auto_draw(bool ad)
		{
			_m_ess().set_auto_draw(ad);
		}

		void listbox::scroll(bool to_bottom, size_type cat_pos)
		{
			auto & ess = _m_ess();
			auto cats = ess.lister.size_categ();

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
				if (0 == items)
					pos.item = ::nana::npos;
				else
					pos.item = items - 1;
			}
			else
				pos.item = ess.lister.size_item(cat_pos) ? 0 : ::nana::npos;

			ess.lister.scroll(pos, to_bottom);
			ess.update();
		}

		void listbox::scroll(bool to_bottom, const index_pair& pos)
		{
			auto & ess = _m_ess();
			ess.lister.scroll(pos, to_bottom);
			ess.update();
		}

		listbox::size_type listbox::append_header(std::string s, unsigned width)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto pos = ess.header.create(to_nstring(std::move(s)), width);
			ess.update();
            return pos;
		}

		listbox::size_type listbox::append_header(std::wstring s, unsigned width)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto pos = ess.header.create(to_nstring(std::move(s)), width);
			ess.update();
			return pos;
		}

		listbox& listbox::header_width(size_type pos, unsigned pixels)
		{
			auto & ess = _m_ess();
			ess.header.item_width(pos, pixels);
			ess.update();
			return *this;
		}
		unsigned listbox::auto_width(size_type pos, unsigned max)
		{
            auto & ess = _m_ess();
			unsigned max_w = ess.auto_width(pos, max);
			ess.update();
			return max_w;
		}

		unsigned listbox::header_width(size_type pos) const
		{
			return _m_ess().header.item_width(pos);
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

		void listbox::append(std::initializer_list<std::string> args)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();

			for (auto & arg : args)
				ess.lister.create_cat(native_string_type(to_nstring(arg)));
			ess.update();
		}

		void listbox::append(std::initializer_list<std::wstring> args)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();

			for (auto & arg : args)
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

		listbox::cat_proxy listbox::at(size_type pos) const
		{
			auto & ess = _m_ess();
			if(pos >= ess.lister.size_categ())
				throw std::out_of_range("Nana.Listbox.at(): invalid position");

			return cat_proxy(&ess, pos);
		}

		listbox& listbox::ordered_categories(bool enable_ordered)
		{
			internal_scope_guard lock;

			auto & ess = _m_ess();
			if (ess.lister.enable_ordered(enable_ordered))
				ess.update();

			return *this;
		}

        listbox::item_proxy listbox::at(const index_pair& pos_abs) const
		{
			return at(pos_abs.cat).at(pos_abs.item);
		}


		// Contributed by leobackes(pr#97)
        listbox::index_pair listbox::at ( const point& pos ) const
        {
            auto & ess=_m_ess();
            auto _where=ess.where(pos.x, pos.y);
            index_pair item_pos{npos,npos};
            if(_where.first==drawerbase::listbox::essence_t::parts::lister){
                auto & offset_y = ess.scroll.offset_y_dpl;
                ess.lister.forward(offset_y, _where.second, item_pos);
            }
            return item_pos;
        }

		//Contributed by leobackes(pr#97)
        listbox::columns_indexs listbox::column_from_pos ( const point& pos )
        {
            auto & ess=_m_ess();
            columns_indexs col=ess.header.item_by_x(pos.x - 2 - ess.scroll.offset_x);
            return col;
        }



		void listbox::insert(const index_pair& pos, std::string text)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			if (ess.lister.insert(pos, std::move(text)))
			{
				if (!empty())
				{
					auto & item = ess.lister.at(pos);
					item.bgcolor = bgcolor();
					item.fgcolor = fgcolor();
					ess.update();
				}
			}
		}

		void listbox::insert(const index_pair& pos, std::wstring text)
		{
			insert(pos, to_utf8(text));
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

		auto listbox::checked() const -> selection
		{
			return _m_ess().lister.item_checked();
		}

		void listbox::clear(size_type cat)
		{
			auto & ess = _m_ess();
			ess.lister.clear(cat);
            //unsort();   // ??

			  // from current display position
			  // move to the cat self if not in first cat
              // move to first item ?? if in first cat
            ess.scroll_y_abs(ess.scroll_y_abs());

			ess.update();
		}

		void listbox::clear()
		{
			auto & ess = _m_ess();
			ess.lister.clear();
            unsort();   // apperar to be espected

			  // from current display position
			  // move to the cat self if not in first cat
              // move to first item ?? if in first cat
            ess.scroll_y_abs(ess.scroll_y_abs());

			ess.update();
		}

		void listbox::erase(size_type cat)
		{
			auto & ess = _m_ess();
			ess.lister.erase(cat);
			if(cat)
			{
				auto pos = ess.scroll_y_dpl();
				if(cat <= pos.cat)
				{
					if(pos.cat == ess.lister.size_categ())
						--pos.cat;
					pos.item = npos;
					ess.set_scroll_y_dpl(pos);
				}
			}
			else
				ess.set_scroll_y_dpl(index_pair());
			ess.update();
		}

		void listbox::erase()
		{
			auto & ess = _m_ess();
			ess.lister.erase();
			ess.scroll_y_abs(index_pair());
			ess.update();
		}

		listbox::item_proxy listbox::erase(item_proxy ip)
		{
			if(ip.empty())
				return ip;

			auto * ess = ip._m_ess();
			auto _where = ip.pos();

			auto pos_before = ess->scroll_y_dpl();
			ess->lister.erase(_where);
			auto pos = ess->scroll_y_dpl();
			if (!pos.empty())
			{
				if ((pos.cat == _where.cat) && (_where.item <= pos.item))
				{
					if (pos.item == 0)
					{
						if (ess->lister.size_item(_where.cat) == 0)
							pos.item = (pos.cat > 0 ? npos : 0);
					}
					else
						--pos.item;
					ess->set_scroll_y_dpl(pos);
				}
			}
			else
			{
				if (pos_before.item)
					--pos_before.item;
				ess->set_scroll_y_dpl(pos_before);
			}
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
			_m_ess().header.column(col).weak_ordering = std::move(strick_ordering);
		}

        /// sort() and ivalidate any existing reference from display position to absolute item, that is: after sort() display offset point to different items
        void listbox::sort_col(size_type col, bool reverse)
		{
			_m_ess().lister.set_sort_index(col, reverse);
		}

		auto listbox::sort_col() const -> size_type
		{
			return _m_ess().lister.sort_index();
		}

        /// potencially ivalidate any existing reference from display position to absolute item, that is: after sort() display offset point to different items
		void listbox::unsort()
		{
			_m_ess().lister.set_sort_index(npos, false);
		}

		bool listbox::freeze_sort(bool freeze)
		{
			return !_m_ess().lister.active_sort(!freeze);
		}

		auto listbox::selected() const -> selection   // change to: selection selected();
		{
			selection s;
			_m_ess().lister.item_selected(s);   // absolute positions, no relative to display
			return std::move(s);
		}

		void listbox::show_header(bool sh)
		{
			auto & ess = _m_ess();
			ess.header.visible(sh);
			ess.update();
		}

		bool listbox::visible_header() const
		{
			return _m_ess().header.visible();
		}

		void listbox::move_select(bool upwards)  ///<Selects an item besides the current selected item in the display.
		{
			auto & ess = _m_ess();
			ess.lister.move_select(upwards);
			ess.update();
		}

		listbox::size_type listbox::size_categ() const
		{
			return _m_ess().lister.size_categ();
		}

		listbox::size_type listbox::size_item() const
		{
			return size_item(0);
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

		drawerbase::listbox::essence_t & listbox::_m_ess() const
		{
			return get_drawer_trigger().essence();
		}

		nana::any* listbox::_m_anyobj(size_type cat, size_type index, bool allocate_if_empty) const
		{
			return _m_ess().lister.anyobj(index_pair{cat, index}, allocate_if_empty);
		}

		drawerbase::listbox::category_t* listbox::_m_at_key(std::shared_ptr<nana::detail::key_interface> ptr)
		{
			auto & ess = _m_ess();

			internal_scope_guard lock;

			for (auto & m : ess.lister.cat_container())
			{
				if (m.key_ptr && nana::detail::pred_equal_by_less(ptr.get(), m.key_ptr.get()))
					return &m;
			}

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

		void listbox::_m_erase_key(nana::detail::key_interface* p)
		{
			auto & cont = _m_ess().lister.cat_container();

			internal_scope_guard lock;
			for (auto i = cont.begin(); i != cont.end(); ++i)
			{
				if (i->key_ptr && nana::detail::pred_equal_by_less(p, i->key_ptr.get()))
				{
					cont.erase(i);
					return;
				}
			}
		}
	//end class listbox
}//end namespace nana
