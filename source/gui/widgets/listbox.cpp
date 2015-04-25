/*
 *	A List Box Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/listbox.cpp
 *	@contributors:	Hiroshi Seki, Ariel Vina-Rodriguez
 */

#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/scroll.hpp>
#include <nana/gui/element.hpp>
#include <list>
#include <deque>
#include <stdexcept>
#include <algorithm>
#include <nana/system/dataexch.hpp>

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

				cell::cell(nana::string text)
					: text(std::move(text))
				{}

				cell::cell(nana::string text, const format& fmt)
					:	text(std::move(text)),
						custom_format(new format{ fmt })	//make_unique
				{}

				cell::cell(nana::string text, const ::nana::color& bgcolor, const ::nana::color& fgcolor)
					:	text(std::move(text)),
						custom_format{ new format{ bgcolor, fgcolor } }	//make_unique
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
				cells_.emplace_back(n ? L"true" : L"false");
				return *this;
			}
			oresolver& oresolver::operator<<(short n)
			{
				cells_.emplace_back(std::to_wstring(n));
				return *this;
			}

			oresolver& oresolver::operator<<(unsigned short n)
			{
				cells_.emplace_back(std::to_wstring(n));
				return *this;
			}

			oresolver& oresolver::operator<<(int n)
			{
				cells_.emplace_back(std::to_wstring(n));
				return *this;
			}

			oresolver& oresolver::operator<<(unsigned int n)
			{
				cells_.emplace_back(std::to_wstring(n));
				return *this;
			}

			oresolver& oresolver::operator<<(long n)
			{
				cells_.emplace_back(std::to_wstring(n));
				return *this;
			}

			oresolver& oresolver::operator<<(unsigned long n)
			{
				cells_.emplace_back(std::to_wstring(n));
				return *this;
			}
			oresolver& oresolver::operator<<(long long n)
			{
				cells_.emplace_back(std::to_wstring(n));
				return *this;
			}

			oresolver& oresolver::operator<<(unsigned long long n)
			{
				cells_.emplace_back(std::to_wstring(n));
				return *this;
			}

			oresolver& oresolver::operator<<(float f)
			{
				cells_.emplace_back(std::to_wstring(f));
				return *this;
			}

			oresolver& oresolver::operator<<(double f)
			{
				cells_.emplace_back(std::to_wstring(f));
				return *this;
			}

			oresolver& oresolver::operator<<(long double f)
			{
				cells_.emplace_back(std::to_wstring(f));
				return *this;
			}

			oresolver& oresolver::operator<<(const char* text)
			{
				cells_.emplace_back(std::wstring(charset(text)));
				return *this;
			}

			oresolver& oresolver::operator<<(const wchar_t* text)
			{
				cells_.emplace_back(text);
				return *this;
			}

			oresolver& oresolver::operator<<(const std::string& text)
			{
				cells_.emplace_back(std::wstring(charset(text)));
				return *this;
			}

			oresolver& oresolver::operator<<(const std::wstring& text)
			{
				cells_.emplace_back(text);
				return *this;
			}

			oresolver& oresolver::operator<<(std::wstring&& text)
			{
				cells_.emplace_back(std::move(text));
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
				cells_.back().text.assign(1, nana::char_t(0));	//means invalid cell
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
					text = charset(cells_[pos_++].text);
				return *this;
			}

			iresolver& iresolver::operator>>(std::wstring& text)
			{
				if (pos_ < cells_.size())
					text = cells_[pos_++].text;

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

			class es_header   /// Essence of the columns Header
			{
			public:
				typedef std::size_t size_type;

				struct column_t
				{
					nana::string text;  //< "text" header of the column number "index" with weigth "pixels"
					unsigned pixels;
					bool visible{true};
					size_type index;
					std::function<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool reverse)> weak_ordering;

					column_t() = default;
					column_t(nana::string&& txt, unsigned px, size_type pos)
						: text(std::move(txt)), pixels(px), index(pos)
					{}
				};

				using container = std::vector<column_t> ;

                export_options::columns_indexs all_headers(bool only_visibles) const
                {
                    export_options::columns_indexs	idx;				
					for(auto hd : cont())
					{
						if(!only_visibles || hd.visible)  
							idx.push_back(hd.index);
					}
                    return idx;
                }

                nana::string to_string() const
                {
                    nana::string sep{STR(";")}, endl{STR("\n")}, head_str; 
                    bool first{true};
					for(auto & i: cont())
					{
						if(i.visible)
                        {
                            if(first)
                                first=false;
                            else 
                                head_str += sep;
							head_str += i.text;
                        }
					}
                    return head_str;
                }

				bool visible() const
				{
					return visible_;
				}

				bool visible(bool v)
				{
					if(visible_ != v)
					{
						visible_ = v;
						return true;
					}
					return false;
				}

				std::function<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool reverse)> fetch_comp(std::size_t index) const
				{
					if(index < cont_.size())
					{
						for(auto & m : cont_)
						{
							if(m.index == index)
								return m.weak_ordering;
						}
					}
					return{};
				}

				void create(nana::string&& text, unsigned pixels)
				{
					cont_.emplace_back(std::move(text), pixels, static_cast<size_type>(cont_.size()));
				}

				void item_width(size_type pos, unsigned width)
				{
					if (pos >= cont_.size())
						return;

					for(auto & m : cont_)
					{
						if(m.index == pos)
							m.pixels = width;
					}
				}

				unsigned item_width(size_type pos) const
				{
					for (auto & m : cont_)
					{
						if (m.index == pos)
							return m.pixels;
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

				size_type index(size_type n) const
				{
					return (n < cont_.size() ? cont_[n].index : npos);
				}

				const container& cont() const
				{
					return cont_;
				}

				column_t& column(size_type pos)
				{
					for(auto & m : cont_)
					{
						if(m.index == pos)
							return m;
					}
					throw std::out_of_range("Nana.GUI.Listbox: invalid header index.");
				}

				size_type item_by_x(int x) const
				{
					for(auto & m : cont_)
					{
						if(x < static_cast<int>(m.pixels))
							return m.index;
						x -= m.pixels;
					}
					return npos;
				}

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

				size_type neighbor(size_type index, bool front) const
				{
					size_type n = npos;
					for(auto i = cont_.cbegin(); i != cont_.cend(); ++i)
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

				size_type begin() const
				{
					for(auto & m : cont_)
					{
						if(m.visible) return m.index;
					}
					return npos;
				}

				size_type last() const
				{
					for(auto i = cont_.rbegin(); i != cont_.rend(); ++i)
					{
						if(i->visible) return i->index;
					}
					return npos;
				}

				void move(size_type index, size_type to, bool front)
				{
					if((index != to) && (index < cont_.size()) && (to < cont_.size()))
					{
						auto i = std::find_if(cont_.begin(), cont_.end(), [index](container::value_type& m){
							return (index == m.index);
						});

						if (i == cont_.end())
							return;

						auto from = *i;
						cont_.erase(i);

						i = std::find_if(cont_.begin(), cont_.end(), [to](const container::value_type& m)->bool{ return (to == m.index); } );
						if(i != cont_.end())
							cont_.insert((front ? i : ++i), from);
					}
				}
			private:
				bool visible_{true};
				container cont_;
			};

			struct essence_t;

			struct item_t
			{
				typedef std::vector<cell> container;

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

				item_t(nana::string&& s)
				{
					flags.selected = flags.checked = false;
					cells.emplace_back(std::move(s));
				}

				item_t(nana::string&& s, const nana::color& bg, const nana::color& fg)
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

                nana::string to_string(const export_options::columns_indexs& col_order) const
                {
                    nana::string sep{STR(";")}, endl{STR("\n")}, item_str; 
                    bool first{true};
                    for( size_type idx{}; idx<col_order.size(); ++idx)
					{
                            if(first)
                                first=false;
                            else 
                                item_str += sep;

							item_str += cells[col_order[idx]].text;
					}
                    return item_str;
                }
			};

			struct category_t
			{
				using container = std::deque<item_t>;

				::nana::string text;
				std::vector<std::size_t> sorted;
				container items;
				bool expand{true};

				//A cat may have a key object to identify the category
				std::shared_ptr<nana::detail::key_interface> key_ptr;

				category_t() = default;

				category_t(nana::string str)
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

				std::function<std::function<bool(const ::nana::string&, ::nana::any*,
								const ::nana::string&, ::nana::any*, bool reverse)>(std::size_t) > fetch_ordering_comparer;

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
					auto& catobj = *_m_at(id.cat);
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
                nana::string to_string() const;
                
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
										nana::string a;
										if (mx.cells.size() > sorted_index_)
											a = mx.cells[sorted_index_].text;

										nana::string b;
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
										nana::string a;
										if (item_x.cells.size() > sorted_index_)
											a = item_x.cells[sorted_index_].text;

										nana::string b;
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
					std::swap(resort, resort_);
					return resort;
				}

				bool sort_reverse() const
				{
					return sorted_reverse_;
				}

				///Append a new category with a specified name.
				category_t* create_cat(nana::string&& text)
				{
					list_.emplace_back(std::move(text));
					return &list_.back();
				}

				void create_cat(const std::initializer_list<nana::string>& args)
				{
					for (auto & arg : args)
						list_.emplace_back(arg);
				}

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

				category_t* create_cat(std::size_t pos, nana::string&& text)
				{
#if defined(NANA_LINUX) || defined(NANA_MINGW)
					//Call begin instead of cbegin, because the first parameter
					//of emplace is not const_iterator in GCC's C++ standard
					//library implementation.
					auto i = list_.begin();
#else
					auto i = list_.cbegin();
#endif
					std::advance(i, pos);
					return &(*list_.emplace(i, std::move(text)));
				}

				/// Insert  before item in absolute "pos" a new item with "text" in column 0, and place it in last display position of this cat
				bool insert(const index_pair& pos, nana::string&& text)
				{
					auto & catobj = *_m_at(pos.cat);

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
					auto & catobj = *_m_at(cat);
					if (display_order_pos >= catobj.sorted.size())
						throw std::out_of_range("listbox: Invalid item position.");

					return catobj.sorted[display_order_pos];
				}

				/// find display order for the real item but without check from current active sorting, in fact using just the last sorting !!!
				size_type display_order(size_type cat, size_type pos) const
				{
					auto & catobj = *_m_at(cat);
					if (pos >= catobj.sorted.size())
						throw std::out_of_range("listbox: Invalid item position.");

                    for (size_type i=0; i<catobj.sorted.size();++i)
                        if (pos==catobj.sorted[i])
                            return i;
					 
					return   npos ;
				}

                /// return a ref to the real item object at display!!! position pos using current sorting only if it is active, and at absolute position if no sorting is currently active.
				category_t::container::value_type& at(const index_pair& pos)
				{
					auto index = pos.item;

					if (sorted_index_ != npos)
						index = absolute(pos);

					return _m_at(pos.cat)->items.at(index);
				}
				const category_t::container::value_type& at(const index_pair& pos) const
				{
					return at(pos);
				}

				void clear(size_type cat)
				{
					auto& catobj = *_m_at(cat);
					catobj.items.clear();
					catobj.sorted.clear();
				}

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
					auto i = _m_at(from.cat);
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
					if (!cat || pos >= cat->items.size())
						throw std::out_of_range("nana::listbox: bad item position");

					return cat->items[pos].cells;
				}

				nana::string text(category_t* cat, size_type pos, size_type col) const
				{
					if (pos < cat->items.size() && (col < cat->items[pos].cells.size()))
						return cat->items[pos].cells[col].text;
					return{};
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

				void text(category_t* cat, size_type pos, size_type col, nana::string&& str, size_type columns)
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

				void erase(const index_pair& pos)
				{
					auto & catobj = *_m_at(pos.cat);
					if(pos.item < catobj.items.size())
					{
						catobj.items.erase(catobj.items.begin() + pos.item);
						catobj.sorted.erase(std::find(catobj.sorted.begin(), catobj.sorted.end(), catobj.items.size()));
						sort();
					}
				}

				void erase(size_type cat)
				{
					auto i = _m_at(cat);

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
						auto & expanded = _m_at(cat)->expand;
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
					return (good(cat) ? _m_at(cat)->expand : false);
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
						auto i = list_.begin();
						std::advance(i, except.cat);

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
					return _m_at(cat)->items.size();
				}

				bool categ_checked(size_type cat) const
				{
					auto & items = _m_at(cat)->items;
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
					auto & items = _m_at(cat)->items;
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
					auto & items = _m_at(cat)->items;
					for(auto & m : items)
						if(m.flags.selected == false)
							return false;
					return true;
				}

                /// set all items in cat to selection sel, emiting events, actualizing last_selected_abs, but not check for single_selection_
                bool categ_selected(size_type cat, bool sel)
				{
					bool changed = false;
					auto & items = _m_at(cat)->items;

					index_pair pos(cat, 0);
					for(auto & m : items)
					{
						if(m.flags.selected != sel)
						{
							m.flags.selected = sel;

							arg_listbox arg{ item_proxy(ess_, pos), sel };
							wd_ptr()->events().selected.emit(arg);
							changed = true;

							if (sel)                         // not check for single_selection_
								last_selected_abs = pos;
							else if (last_selected_abs == pos)
								last_selected_abs.set_both(npos);
						}
						++pos.item;
					}
					return changed;
				}

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
					return absolute ( last_displ() );
				}

				bool good(size_type cat) const
				{
					return (cat < list_.size());
				}

				bool good(const index_pair& pos) const
				{
					return ((pos.cat < list_.size()) && (pos.item < size_item(pos.cat)));
				}

				bool good_item(index_pair pos, index_pair& item) const
				{
					if (!good(pos.cat))
						return false;

					if (pos.is_category())
					{
						item = pos;
						if (0 == pos.cat)
							item.item = 0;

						return true;
					}

					auto i = _m_at(pos.cat);
					if (pos.item < i->items.size())
					{
						item = pos;
						return true;
					}

					if (++i == list_.end())
						return false;

					item.cat = pos.cat + 1;
					item.item = npos;
					return true;
				}

				///Translate relative position (position in display) into absolute position (original data order)
				size_type absolute(const index_pair& display_pos) const
				{
					return (sorted_index_ == npos ? display_pos.item : _m_at(display_pos.cat)->sorted[display_pos.item]);
				}
				index_pair absolute_pair(const index_pair& display_pos) const
				{
                    return {display_pos.cat, absolute( display_pos )};
				}
				
                ///Translate absolute position (original data order) into relative position (position in display)
                size_type relative(const index_pair& pos) const
				{
					if (sorted_index_ == npos) return pos.item ;

                    auto & catobj = *_m_at(pos.cat);

                    for (size_type i=0; i<catobj.sorted.size();++i)
                        if (pos.item == catobj.sorted[i])
                            return i;
					 
					return   npos ;
				}
				index_pair relative_pair(const index_pair& pos) const
				{
                    return {pos.cat, relative( pos )};
				}

                /// all arg are relative to display order, or all are absolute, but not mixed
				bool forward(index_pair from, size_type offs, index_pair& item) const
				{
					if(!good_item(from, from))
						return false;

					if(offs == 0)
					{
						item = from;
						return true;
					}

					if(list_.size() <= from.cat) return false;

					if(from.is_category())
					{
					    // this is a category, so...
						// and offs is not 0, this category would not be candidated.
						// the algorithm above to calc the offset item is always starting with a item.
						--offs;
						from.item = 0;
					}

					auto icat = _m_at(from.cat); // an iterator to category from.cat

					if(icat->expand)
					{
						std::size_t item_left_in_this_cat = icat->items.size() -1- from.item;
						if(offs <= item_left_in_this_cat )
						{
							item = from;
							item.item += offs;  // use absolute to know the real item
							return true;       // allways return here when we have only one cat. 
						}
						else
                        {
							offs -= item_left_in_this_cat ;
							item = from;
							item.item += item_left_in_this_cat ;
                        }
					}

					++from.cat;
					++icat;
					for(; icat != list_.end(); ++icat, ++from.cat)
					{
						item.cat = from.cat;
						item.item = npos;

						if(offs-- == 0)
						{
							return true;
						}

						if(icat->expand)
						{
							if(offs < icat->items.size())
							{
								//item.cat = from.cat;
								item.item = offs;
								return true;
							}
							else
								offs -= icat->items.size();
						}
					}
					return false;
				}

                /// all arg are relative to display order, or all are absolute, but not mixed
				bool backward(index_pair from, size_type offs, index_pair& item) const
				{
					if(offs == 0)
						item = from;

					if(good(from.cat))
					{
						auto i = _m_at(from.cat);
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
			private:
				/// categories iterator
                container::iterator _m_at(size_type index)
				{
					if(index >= list_.size())
						throw std::out_of_range("Nana.GUI.Listbox: invalid category index");

					auto i = list_.begin();
					std::advance(i, index);
					return i;
				}

				container::const_iterator _m_at(size_type index) const
				{
					if(index >= list_.size())
						throw std::out_of_range("Nana.GUI.Listbox: invalid category index");

					auto i = list_.cbegin();
					std::advance(i, index);
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
                ::nana::listbox::export_options def_exp_options ;

                ::nana::listbox::export_options& def_export_options()
                {
			        return def_exp_options;
                }


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

				essence_t()
				{
					scroll.offset_x = 0;
					pointer_where.first = parts::unknown;
					lister.fetch_ordering_comparer = std::bind(&es_header::fetch_comp, &header, std::placeholders::_1);
				}

                nana::string to_string() const
                {
                    nana::string sep{STR(";")}, endl{STR("\n")}; 
                    lister.to_string();
                    return header.to_string() + endl + lister.to_string() ;
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
						scroll.offset_y_abs.item = (pos_abs.cat > 0 ? npos : 0);
				}
				void scroll_y_rel(const index_pair& pos_rel)
				{
				    scroll_y_abs(lister.relative_pair(pos_rel) );
				}
				void set_scroll_y_abs(const index_pair& pos_abs)
                {
                    scroll.offset_y_abs=pos_abs;
                    scroll_y_dpl_refresh() ;
                }
                /// directly set a tested relative display pos 
				void set_scroll_y_dpl(const index_pair& pos_dpl)
                {
                    scroll.offset_y_dpl=pos_dpl;
                    scroll.offset_y_abs = lister.absolute_pair(pos_dpl);
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

                void trace_first_selected_item()
				{
					auto fs=lister.find_first_selected();
					if( ! fs.empty() ) 
                       trace_item_abs( fs );
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
					if(scroll.h.empty() == false)
					{
						unsigned width = 4 + (scroll.v.empty() ? 0 : scroll.scale - 1);
						if(width >= graph->width()) return;
						scroll.h.amount(header.pixels());
						scroll.h.range(graph->width() - width);
						scroll.h.value(scroll.offset_x);
					}

					if(scroll.v.empty() == false)
					{
						unsigned height = 2 + (scroll.h.empty() ? 0 : scroll.scale);
						if(height >= graph->width()) return;
						scroll.v.amount(lister.the_number_of_expanded());
						scroll.v.range(number_of_lister_items(false));
                        size_type off = lister.distance({0,0}, scroll.offset_y_dpl );
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

					if(h)
					{
						rectangle r(1, sz.height - scroll.scale - 1, width, scroll.scale);
						if(scroll.h.empty())
						{
							scroll.h.create(wd, r);
							API::take_active(scroll.h.handle(), false, wd);
							scroll.h.events().mouse_move.connect_unignorable([this](const nana::arg_mouse& arg){
								_m_answer_scroll(arg);
							});
							scroll.h.events().mouse_up.connect_unignorable([this](const nana::arg_mouse& arg){
								_m_answer_scroll(arg);
							});
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
							API::take_active(scroll.v.handle(), false, wd);  // install value_changed() not mouse_move ????

							scroll.v.events().value_changed([this](const ::nana::arg_scroll<true>& arg)
							{
								_m_answer_scroll_value(arg);
							});

						}
						else
							scroll.v.move(r);

					}
					else if(!scroll.v.empty())
					{
						scroll.v.close();
                        set_scroll_y_dpl({0,0}); //			scroll.offset_y.set_both(0);

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
					std::vector<size_type> seq;
					header_seq(seq, r.width);
					return (seq.size() ? (header.item_pos(seq[0], nullptr) - scroll.offset_x + r.x) : 0);
				}

				bool calc_where(int x, int y)
				{
					decltype(pointer_where) new_where;

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
							new_where.second = (y - header_visible_px() + 1) / item_size;
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

				void header_seq(std::vector<size_type> &seqs, unsigned lister_w)const
				{
					int x = - (scroll.offset_x);
					for(auto hd : header.cont())
					{
						if(false == hd.visible) continue;
						x += hd.pixels;
						if(x > 0)
							seqs.push_back(hd.index);
						if(x >= static_cast<int>(lister_w))
							break;
					}
				}
			private:
				void _m_answer_scroll(const arg_mouse& arg)
				{
					if(arg.evt_code == event_code::mouse_move && arg.left_button == false) return;

					bool update = false;
					if(arg.window_handle == scroll.v.handle())
					{
						index_pair item;
						if(lister.forward(item, scroll.v.value(), item))
						{
							if (item != scroll.offset_y_dpl)
							{
					            set_scroll_y_dpl ( item );
								update = true;
							}
						}
					}
					else if(arg.window_handle == scroll.h.handle())
					{
						if(scroll.offset_x != static_cast<int>(scroll.h.value()))
						{
							scroll.offset_x = static_cast<int>(scroll.h.value());
							update = true;
						}
					}

					if(update)
						API::refresh_window(lister.wd_ptr()->handle());
				}
				void _m_answer_scroll_value(const ::nana::arg_scroll<true>& arg)
				{
					index_pair item;
					if( !lister.forward(item, scroll.v.value(), item)) return;

					if (item == scroll.offset_y_dpl) 
                        return; 

					set_scroll_y_dpl ( item );

                    API::refresh_window(lister.wd_ptr()->handle());
				}
                
#if ( 0 )   
                void update_selection_range(index_pair to, const arg_mouse& arg)
				{
					using item_state = essence_t::item_state;
					using parts = essence_t::parts;
					bool update = false;
					index_pair item_pos;
					bool sel = true;
					if (!lister.single_selection())
					{
						if (arg.shift)
							lister.select_display_range(lister.last_selected, item_pos, sel);
						else if (arg.ctrl)
							    sel = !item_proxy(essence_, index_pair (item_pos.cat, lister.absolute(item_pos))).selected();  
							else
								lister.select_for_all(false);
					}
					else
						sel = !item_proxy(essence_, index_pair (item_pos.cat, lister.absolute(item_pos))).selected();

						item_ptr->flags.selected = sel;
						index_pair last_selected(item_pos.cat, lister.absolute(item_pos)); 

						arg_listbox arg{item_proxy{essence_, last_selected}, sel};
						lister.wd_ptr()->events().selected.emit(arg);

                        if (item_ptr->flags.selected)
						{
							lister.cancel_others_if_single_enabled(true, last_selected);
							essence_->lister.last_selected = last_selected;
								
						}
						else if (essence_->lister.last_selected == last_selected)
								essence_->lister.last_selected.set_both(npos);
					}
					else if(!lister.single_selection())
							lister.categ_selected(item_pos.cat, true);
					update = true;

				}
#endif			

};

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
            nana::string es_lister::to_string() const
            {
                nana::string sep{STR(";")}, endl{STR("\n")}, list_str; 
                auto col_order = ess_->header.all_headers(true);
                bool first{true};
				for(auto & cat: cat_container())
				{
                    if(first)
                            first=false;
                    else
 						list_str += (cat.text + endl);

                    bool first_item{true};
                    for (auto i : cat.sorted)
                    {
                        auto& it= cat.items[i] ;
                        if(it.flags.selected)
                            list_str += (it.to_string(col_order) + endl);
                    }
				}
                return list_str ;
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
						for(auto & hd : essence_->header.cont())
						{
							if(hd.visible)
							{
								if((static_cast<int>(hd.pixels)  - 2 < x) && (x < static_cast<int>(hd.pixels) + 3))
								{
									item_spliter_ = hd.index;
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
						int new_w = orig_item_width_ - (ref_xpos_ - pos.x);
						if(static_cast<int>(item.pixels) != new_w)
						{
							essence_->header.item_width(item_spliter_, (new_w < static_cast<int>(essence_->suspension_width + 20) ? essence_->suspension_width + 20 : new_w));
							auto new_w = essence_->header.pixels();
							if(new_w < (rect.width + essence_->scroll.offset_x))
							{
								essence_->scroll.offset_x = (new_w > rect.width ? new_w - rect.width : 0);
							}
							essence_->adjust_scroll_life();
							return 2;
						}
					}
					return 0;
				}

				void draw(const nana::rectangle& r)
				{
					_m_draw(essence_->header.cont(), r);

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
						
						int midpos = item_x + static_cast<int>(item_pixels / 2);

						//Get the item pos
						//if mouse pos is at left of an item middle, the pos of itself otherwise the pos of the next.
						place_front = (x <= midpos);
						x = (place_front ? item_x : essence_->header.item_pos(essence_->header.neighbor(i, false), nullptr));

						if(i != npos)
							essence_->graph->rectangle({ x - essence_->scroll.offset_x + rect.x, rect.y, 2, rect.height }, true, colors::red);

						return i;
					}
					return npos;
				}

				template<typename Container>
				void _m_draw(const Container& cont, const nana::rectangle& rect)
				{
					graph_reference graph = *(essence_->graph);
					

					int txtop = (rect.height - essence_->text_height) / 2 + rect.y;
					auto txtcolor = essence_->lister.wd_ptr()->fgcolor();

					auto state = item_state::normal;
					//check whether grabing an item, if item_spliter_ != npos, that indicates the grab item is a spliter.
					if(essence_->pointer_where.first == parts::header && (item_spliter_ == npos))
						state = essence_->ptr_state;

					const unsigned height = rect.height - 1;
					const int bottom_y = rect.bottom() - 2;
					int x = rect.x - essence_->scroll.offset_x;
					for(auto & i: cont)
					{
						if(i.visible)
						{
							int next_x = x + static_cast<int>(i.pixels);
							if(next_x > rect.x)
							{
								_m_draw_item(graph, x, rect.y, height, txtop, txtcolor, i, (i.index == essence_->pointer_where.second ? state : item_state::normal));
								graph.line({ next_x - 1, rect.y }, { next_x - 1, bottom_y }, _m_border_color());
							}

							x = next_x;
							if (x > rect.right())
								break;
						}
					}

					if (x < rect.right())
						graph.rectangle({ x, rect.y, static_cast<unsigned>(rect.right() - x), height }, true, essence_->scheme_ptr->header_bgcolor);
				}

				template<typename Item>
				void _m_draw_item(graph_reference graph, int x, int y, unsigned height, int txtop, const ::nana::color& fgcolor, const Item& item, item_state state)
				{
					essence_->scheme_ptr->header_bgcolor.get_color();
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
					_m_draw_item(ext_graph, 0, 0, essence_->header_size, txtop, colors::white, item, item_state::floated);

					int xpos = essence_->header.item_pos(item.index, nullptr) + pos.x - ref_xpos_;
					ext_graph.blend(ext_graph.size(), *(essence_->graph), nana::point(xpos - essence_->scroll.offset_x + rect.x, rect.y), 0.5);
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
					// essence_->scroll_y_dpl_refresh() ; // ????

                    internal_scope_guard lock;

					size_type n = essence_->number_of_lister_items(true);
					if(0 == n)return;
					widget * wdptr = essence_->lister.wd_ptr();
					auto bgcolor = wdptr->bgcolor();
					auto fgcolor = wdptr->fgcolor();

					unsigned header_w = essence_->header.pixels();
					essence_->graph->set_color(bgcolor);
					if(header_w - essence_->scroll.offset_x < rect.width)
						essence_->graph->rectangle(rectangle{ rect.x + static_cast<int>(header_w)-essence_->scroll.offset_x, rect.y, rect.width - (header_w - essence_->scroll.offset_x), rect.height }, true);

					es_lister & lister = essence_->lister;
					//The Tracker indicates the item where mouse placed.
					index_pair tracker(npos, npos);
					auto & ptr_where = essence_->pointer_where;  

                    //if where == lister || where == checker, 'second' indicates the offset to the  relative display-order pos of the scroll offset_y which stands for the first item to be displayed in lister.
					if((ptr_where.first == parts::lister || ptr_where.first == parts::checker) && ptr_where.second != npos)
						lister.forward(essence_->scroll.offset_y_dpl, ptr_where.second, tracker);

					std::vector<size_type> subitems;
					essence_->header_seq(subitems, rect.width);

					if(subitems.empty())
						return;

					int x = essence_->item_xpos(rect);
					int y = rect.y;
					int txtoff = (essence_->item_size - essence_->text_height) / 2;

					auto i_categ = lister.cat_container().cbegin();
					std::advance(i_categ, essence_->scroll.offset_y_dpl.cat);

					auto idx = essence_->scroll.offset_y_dpl;

					auto state = item_state::normal;

					//Here draws a root categ or a first drawing is not a categ.
					if(idx.cat == 0 || !idx.is_category())
					{
						if (idx.cat == 0 && idx.is_category())
						{
							essence_->scroll.offset_y_dpl.item = 0;
							idx.item = 0;
						}

						std::size_t size = i_categ->items.size();
						for(std::size_t offs = essence_->scroll.offset_y_dpl.item; offs < size; ++offs, ++idx.item)
						{
							if(n-- == 0)	break;
							state = (tracker == idx	? item_state::highlighted : item_state::normal);

							_m_draw_item(i_categ->items[lister.absolute(index_pair(idx.cat, offs)) ], x, y, txtoff, header_w, rect, subitems, bgcolor,fgcolor, state);
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
						for(decltype(size) pos = 0; pos < size; ++pos)
						{
							if(n-- == 0)	break;
							state = (idx == tracker ? item_state::highlighted : item_state::normal);

							_m_draw_item(i_categ->items[ lister.absolute(index_pair(idx.cat, pos))], x, y, txtoff, header_w, rect, subitems, bgcolor, fgcolor, state);
							y += essence_->item_size;
							++idx.item;
						}
					}

					if (y < rect.y + static_cast<int>(rect.height))
					{
						essence_->graph->set_color(bgcolor);
						essence_->graph->rectangle(rectangle{ rect.x, y, rect.width, rect.y + rect.height - y }, true);
					}
				}
			private:
				void _m_draw_categ(const category_t& categ, int x, int y, int txtoff, unsigned width, const nana::rectangle& r, nana::color bgcolor, item_state state) const
				{
					bool sel = categ.selected();
					if(sel && (categ.expand == false))
						bgcolor = static_cast<color_rgb>(0xD5EFFC);

					if (state == item_state::highlighted)
						bgcolor = bgcolor.blend(static_cast<color_rgb>(0x99defd), 0.8);

					auto graph = essence_->graph;
					graph->rectangle(rectangle{ x, y, width, essence_->item_size }, true, bgcolor);

					color txt_color{ static_cast<color_rgb>(0x3399) };

					facade<element::arrow> arrow("double");
					arrow.direction(categ.expand ? ::nana::direction::north : ::nana::direction::south);
					::nana::rectangle arrow_r{ x + 5, y + static_cast<int>(essence_->item_size - 16) / 2, 16, 16 };
					arrow.draw(*graph, {}, txt_color, arrow_r, element_state::normal);

					graph->string({ x + 20, y + txtoff }, categ.text, txt_color);

					::nana::string str = L'(' + std::to_wstring(categ.items.size()) + L')';

					unsigned str_w = graph->text_extent_size(str).width;

					auto text_s = graph->text_extent_size(categ.text);
					graph->string({ x + 25 + static_cast<int>(text_s.width), y + txtoff }, str);

					if (x + 35 + text_s.width + str_w < x + width)
					{
						::nana::point pos{ x + 30 + static_cast<int>(text_s.width + str_w), y + static_cast<int>(essence_->item_size) / 2 };
						graph->line(pos, { x + static_cast<int>(width)-5, pos.y }, txt_color);
					}
					//Draw selecting inner rectangle
					if(sel && categ.expand == false)
					{
						width -= essence_->scroll.offset_x;
						_m_draw_border(r.x, y, (r.width < width ? r.width : width));
					}
				}

				void _m_draw_item(const item_t& item, int x, int y, int txtoff, unsigned width, const nana::rectangle& r, const std::vector<size_type>& seqs, nana::color bgcolor, nana::color fgcolor, item_state state) const
				{
					if (item.flags.selected)
						bgcolor = essence_->scheme_ptr->item_selected;
					else if (!item.bgcolor.invisible())
						bgcolor = item.bgcolor;

					if(!item.fgcolor.invisible())
						fgcolor = item.fgcolor;

					auto graph = essence_->graph;
					if (item_state::highlighted == state)
					{
						if (item.flags.selected)
							bgcolor = bgcolor.blend(colors::black, 0.98);
						else
							bgcolor = bgcolor.blend(essence_->scheme_ptr->item_selected, 0.7);
					}

					unsigned show_w = width - essence_->scroll.offset_x;
					if(show_w >= r.width) show_w = r.width;

					//draw the background
					graph->set_color(bgcolor);
					graph->rectangle(rectangle{ r.x, y, show_w, essence_->item_size }, true);

					int item_xpos         = x;
					unsigned extreme_text = x;
					bool first = true;

					for(auto index : seqs)
					{
						const auto & header = essence_->header.column(index);
                        auto it_bgcolor = bgcolor;

						if ((item.cells.size() > index) && (header.pixels > 5))
						{
							auto cell_txtcolor = fgcolor;
							auto & m_cell = item.cells[index];
							nana::size ts = graph->text_extent_size(m_cell.text);

							if (m_cell.custom_format && (!m_cell.custom_format->bgcolor.invisible()))
							{
								it_bgcolor = m_cell.custom_format->bgcolor; 
                                if (item.flags.selected)
                                    it_bgcolor = it_bgcolor.blend( bgcolor , 0.5) ;
								if (item_state::highlighted == state)
									it_bgcolor = it_bgcolor.blend(::nana::color(0x99, 0xde, 0xfd), 0.8);
                                
                                graph->set_color(it_bgcolor);
								graph->rectangle(rectangle{ item_xpos, y, header.pixels, essence_->item_size }, true);

								cell_txtcolor = m_cell.custom_format->fgcolor;
							}

							int ext_w = 5;
							if(first && essence_->checkable)
							{
								ext_w += 18;

								element_state estate = element_state::normal;
								if(essence_->pointer_where.first == parts::checker)
								{
									switch(state)
									{
									case item_state::highlighted:
										estate = element_state::hovered;	break;
									case item_state::grabbed:
										estate = element_state::pressed;	break;
									default:	break;
									}
								}

								using state = facade<element::crook>::state;
								crook_renderer_.check(item.flags.checked ?  state::checked : state::unchecked);
								crook_renderer_.draw(*graph, bgcolor, fgcolor, essence_->checkarea(item_xpos, y), estate);
							}

							if ((0 == index) && essence_->if_image)
							{
								if (item.img)
								{
									nana::rectangle img_r(item.img_show_size);
									img_r.x = static_cast<int>(ext_w) + item_xpos + static_cast<int>(16 - item.img_show_size.width) / 2;
									img_r.y = y + static_cast<int>(essence_->item_size - item.img_show_size.height) / 2;
									item.img.stretch(item.img.size(), *graph, img_r);
								}
								ext_w += 18;
							}

							graph->set_text_color(cell_txtcolor);
							graph->string(point{ item_xpos + ext_w, y + txtoff }, m_cell.text);

							if (ts.width + ext_w > header.pixels)
							{
								//The text is painted over the next subitem
								int xpos = item_xpos + header.pixels - essence_->suspension_width;

								graph->set_color(it_bgcolor);
								graph->rectangle(rectangle{ xpos, y + 2, essence_->suspension_width, essence_->item_size - 4 }, true);
								graph->set_text_color(cell_txtcolor);
								graph->string(point{ xpos, y + 2 }, STR("..."));

								//Erase the part that over the next subitem.
								if (index + 1 < seqs.size())
                                {
                                    graph->set_color(bgcolor);
								    graph->rectangle(rectangle{item_xpos + static_cast<int>(header.pixels), y + 2, ts.width + ext_w - header.pixels, essence_->item_size - 4}, true);
                                }
                                extreme_text = std::max (extreme_text, item_xpos + ext_w + ts.width);
							}
						}

						graph->line({ item_xpos - 1, y }, { item_xpos - 1, y + static_cast<int>(essence_->item_size) - 1 }, { 0xEB, 0xF4, 0xF9 });

						item_xpos += header.pixels;
						if (index + 1 >= seqs.size() && static_cast<int>(extreme_text) > item_xpos)
                        {
                            graph->set_color(item.bgcolor);
							graph->rectangle(rectangle{item_xpos , y + 2, extreme_text - item_xpos, essence_->item_size - 4}, true);
                        }
						first = false;
					}

					//Draw selecting inner rectangle
					if(item.flags.selected)
						_m_draw_border(r.x, y, show_w);
				}

				void _m_draw_border(int x, int y, unsigned width) const
				{
					//Draw selecting inner rectangle
					auto graph = essence_->graph;
					graph->rectangle({ x, y, width, essence_->item_size }, false, { 0x99, 0xDE, 0xFD });

					graph->set_color(colors::white);
					graph->rectangle({ x + 1, y + 1, width - 2, essence_->item_size - 2 }, false);
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

				essence_t& trigger::essence()
				{
					return *essence_;
				}

				essence_t& trigger::essence() const
				{
					return *essence_;
				}

				void trigger::draw()
				{
					nana::rectangle r;

					if(essence_->header.visible() && essence_->rect_header(r))
						drawer_header_->draw(r);
					if(essence_->rect_lister(r))
						drawer_lister_->draw(r);
					_m_draw_border();
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
					essence_->text_height = graph.text_extent_size(STR("jHWn0123456789/<?'{[|\\_")).height;
					essence_->item_size = essence_->text_height + 6;
					essence_->suspension_width = graph.text_extent_size(STR("...")).width;
				}

				void trigger::refresh(graph_reference)
				{
					draw();
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

					switch(update)
					{
					case 1:
						API::update_window(essence_->lister.wd_ptr()->handle());
						break;
					case 2:
						draw();
						API::lazy_refresh();
						break;
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

						draw();
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
										lister.select_for_all(false);
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
					if (essence_->pointer_where.first == parts::header && prev_state == item_state::pressed)
					{
						if(essence_->pointer_where.second < essence_->header.cont().size())
						{
							if(essence_->lister.sort_index(essence_->pointer_where.second))
							{
								essence_->trace_item_dpl({0,0});
                                draw();
								API::lazy_refresh();
							}
						}
					}
					else if (prev_state == item_state::grabbed)
					{
						nana::point pos = arg.pos;
						essence_->widget_to_header(pos);
						drawer_header_->grab(pos, false);
						draw();
						API::lazy_refresh();
						API::capture_window(essence_->lister.wd_ptr()->handle(), false);
					}
				}

				void trigger::mouse_wheel(graph_reference graph, const arg_wheel& arg)
				{
					if(essence_->wheel(arg.upwards))
					{
						draw();
						essence_->adjust_scroll_value();
						API::lazy_refresh();
					}
				}

				void trigger::dbl_click(graph_reference graph, const arg_mouse&)
				{
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
						draw();
						API::lazy_refresh();
					}
				}

				void trigger::resized(graph_reference graph, const arg_resized&)
				{
					essence_->adjust_scroll_life();
					draw();
					API::lazy_refresh();
				}

				void trigger::key_press(graph_reference graph, const arg_keyboard& arg)
				{
					bool up = false;
					switch(arg.key)
					{
					case keyboard::os_arrow_up:
						up = true;
					case keyboard::os_arrow_down:
                                       // move_select(bool upwards=true, bool unselect_previous=true, bool trace_selected=false)
						essence_->lister.move_select(up, !arg.shift, true);
						break;

					case STR(' ') :
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
                        if (up)
                            item_proxy {essence_, essence_->scroll_y_abs()}.select(true); 
                        else 
                        {
                            index_pair idx{essence_->scroll_y_dpl()};
                            essence_->lister.forward(idx, scrl.range()-1, idx);
                            item_proxy::from_display(essence_,idx).select(true);
                        }
                        break;
                    }
                    case keyboard::os_home:
                        essence_->lister.select_for_all(false);
                        item_proxy::from_display(essence_, {0,0}).select(true);
                        essence_->trace_last_selected_item ();
                        break;
                    case keyboard::os_end:
                        essence_->lister.select_for_all(false);
                        item_proxy::from_display(essence_, essence_->lister.last()).select(true);
                        essence_->trace_last_selected_item ();
                       break;

					default:
						return;
					}
					draw();
					API::lazy_refresh();
				}

				void trigger::key_char(graph_reference graph, const arg_keyboard& arg)
				{
					switch(arg.key)
					{
                    case keyboard::copy:
                           nana::system::dataexch().set(essence_->to_string());
                           return;
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
						auto i = ess_->lister.cat_container().begin();
						std::advance(i, pos.cat);
						cat_ = &(*i);
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
					auto & m = cat_->items.at(pos_.item);       // a ref to the real item
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

				item_proxy& item_proxy::text(size_type col, nana::string str)
				{
					ess_->lister.text(cat_, pos_.item, col, std::move(str), columns());
					ess_->update();
					return *this;
				}

				nana::string item_proxy::text(size_type col) const
				{
					return ess_->lister.text(cat_, pos_.item, col);
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
				bool item_proxy::operator==(const nana::string& s) const
				{
					return (ess_->lister.text(cat_, pos_.item, 0) == s);
				}

				bool item_proxy::operator==(const char * s) const
				{
					return (ess_->lister.text(cat_, pos_.item, 0) == nana::string(nana::charset(s)));
				}

				bool item_proxy::operator==(const wchar_t * s) const
				{
					return (ess_->lister.text(cat_, pos_.item, 0) == nana::string(nana::charset(s)));
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
					if (++pos_.item < cat_->items.size())
						return *this;

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

				void cat_proxy::append(std::initializer_list<nana::string> arg)
				{
					const auto items = columns();
					push_back(nana::string{});
					item_proxy ip{ ess_, index_pair(pos_, size() - 1) };
					size_type pos = 0;
					for (auto & txt : arg)
					{
						ip.text(pos++, txt);
						if (pos >= items)
							break;
					}
				}

				auto cat_proxy::columns() const -> size_type
				{
					return ess_->header.cont().size();
				}

				cat_proxy& cat_proxy::text(nana::string s)
				{
					internal_scope_guard lock;
					if (s != cat_->text)
					{
						cat_->text.swap(s);
						ess_->update();
					}
					return *this;
				}

				nana::string cat_proxy::text() const
				{
					internal_scope_guard lock;
					return cat_->text;
				}

				void cat_proxy::push_back(nana::string s)
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

				void cat_proxy::_m_append(std::vector<cell> && cells)
				{
					//check invalid cells
					for (auto & cl : cells)
					{
						if (cl.text.size() == 1 && cl.text[0] == nana::char_t(0))
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
						ess_->update();
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

					auto i = ess_->lister.cat_container().begin();
					std::advance(i, pos_);
					cat_ = &(*i);
				}
			//class cat_proxy

			//end class cat_proxy
		}
	}//end namespace drawerbase

	arg_listbox::arg_listbox(const drawerbase::listbox::item_proxy& m, bool selected)
		: item(m), selected(selected)
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

		void listbox::auto_draw(bool ad)
		{
			_m_ess().set_auto_draw(ad);
		}

		void listbox::append_header(nana::string text, unsigned width)
		{
			auto & ess = _m_ess();
			ess.header.create(std::move(text), width);
			ess.update();
		}

		listbox& listbox::header_width(size_type pos, unsigned pixels)
		{
			auto & ess = _m_ess();
			ess.header.item_width(pos, pixels);
			ess.update();
			return *this;
		}

		unsigned listbox::header_width(size_type pos) const
		{
			return _m_ess().header.item_width(pos);
		}

		listbox::cat_proxy listbox::append(nana::string s)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_cat(std::move(s));
			ess.update();

			return cat_proxy{ &ess, new_cat_ptr };
		}

		void listbox::append(std::initializer_list<nana::string> args)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			ess.lister.create_cat(args);
			ess.update();
		}

		auto listbox::insert(cat_proxy cat, nana::string str) -> cat_proxy
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			auto new_cat_ptr = ess.lister.create_cat(cat.position(), std::move(str));
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

		/// from abs pos
        listbox::item_proxy listbox::at(const index_pair& pos_abs) const
		{
			return at(pos_abs.cat).at(pos_abs.item);
		}

		void listbox::insert(const index_pair& pos, nana::string text)
		{
			internal_scope_guard lock;
			auto & ess = _m_ess();
			if (ess.lister.insert(pos, std::move(text)))
			{
				if (! empty())
				{
					auto & item = ess.lister.at(pos);
					item.bgcolor = bgcolor();
					item.fgcolor = fgcolor();
					ess.update();
				}
			}
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
			auto pos = ess.scroll_y_dpl();
			if(pos.cat == cat)
			{
				pos.item = (pos.cat > 0 ? npos : 0);
				ess.set_scroll_y_dpl(pos);
			}
			ess.update();
		}

		void listbox::clear()
		{
			auto & ess = _m_ess();
			ess.lister.clear();
			auto pos = ess.scroll_y_dpl();
			pos.item = (pos.cat > 0 ? npos : 0);
			ess.set_scroll_y_dpl(pos);
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
			ess->lister.erase(_where);
			auto pos = ess->scroll_y_dpl();
			if((pos.cat == _where.cat) && (_where.item <= pos.item))
			{
				if(pos.item == 0)
				{
					if(ess->lister.size_item(_where.cat) == 0)
						pos.item = (pos.cat > 0 ? npos : 0);
				}
				else
					--pos.item;
				ess->set_scroll_y_dpl(pos);
			}
			ess->update();
			if(_where.item < ess->lister.size_item(_where.cat))
				return ip;
			return item_proxy(ess);
		}

		void listbox::set_sort_compare(size_type col, std::function<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool reverse)> strick_ordering)
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
			return _m_ess().def_export_options();
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
				cat = ess.lister.create_cat(nana::string{});
				cat->key_ptr = ptr;
			}
			ess.update();
			return cat;
		}

		void listbox::_m_ease_key(nana::detail::key_interface* p)
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
