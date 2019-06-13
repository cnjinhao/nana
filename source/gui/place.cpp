/**
 *	An Implementation of Place for Layout
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/place.cpp
 *	@contributors	Ariel Vina-Rodriguez
 *					dankan1890(PR#156)
 */

#include <cfloat>
#include <cmath>
#include <map>
#include <set>
#include <algorithm>
#include <nana/push_ignore_diagnostic>
#include <nana/deploy.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/programming_interface.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/dragger.hpp>
#include <nana/gui/drawing.hpp>

#include <memory>
#include <limits>	//numeric_limits
#include <cstdlib>	//std::abs
#include <cstring>	//std::memset
#include <cctype>	//std::isalpha/std::isalnum

#include "place_parts.hpp"

namespace nana
{
	struct badname: place::error
	{
		explicit badname(  ::std::string           what,
							const place&           plc,
							const char*            name   = "unknown",
							std::string::size_type pos    = std::string::npos)
			:place::error(what + ": bad field name '" + (name ? name : "nullptr") + "'.",
				          plc, (name ? name : "nullptr"), pos)
		{}
	};
	typedef place_parts::number_t number_t;
	typedef place_parts::repeated_array repeated_array;

	namespace place_parts
	{
		class tokenizer
		{
		public:
			/// \todo add member full_what and overrider what() in internal exeptions
			struct error : std::invalid_argument
			{
				error(std::string           what,
					  const tokenizer&      tok)

					: std::invalid_argument{ what + " from tokenizer "  },
					  pos{tok.pos()},
					  div_str(tok.divstr_)
				{}	
				std::string::size_type pos;
				std::string            div_str;
		    };

			enum class token
			{
				div_start, div_end, splitter,
				identifier, dock, fit, hfit, vfit, vert, grid, number, array, reparray,
				weight, width, height, gap, margin, arrange, variable, repeated, min_px, max_px, left, right, top, bottom, undisplayed, invisible, switchable,
				collapse, parameters,
				equal,
				eof, error
			};

			tokenizer(const char* div_text) noexcept
				: divstr_(div_text), sp_(div_text)
			{}

			const std::string& idstr() const noexcept
			{
				return idstr_;
			}

			const number_t& number() const noexcept
			{
				return number_;
			}

			std::vector<number_t>& array() noexcept
			{
				return array_;
			}

			repeated_array&& reparray() noexcept
			{
				return std::move(reparray_);
			}

			std::vector<number_t>& parameters() noexcept
			{
				return parameters_;
			}

			std::string::size_type pos() const noexcept
			{
				return static_cast<std::string::size_type>(sp_ - divstr_);
			}

			token read()
			{
				sp_ = _m_eat_whitespace(sp_);

				std::size_t readbytes = 0;
				switch (*sp_)
				{
				case '\0':
					return token::eof;
				case '|':
					++sp_;
					readbytes = _m_number(sp_, false);
					sp_ += readbytes;
					return token::splitter;
				case '=':
					++sp_;
					return token::equal;
				case '<':
					++sp_;
					return token::div_start;
				case '>':
					++sp_;
					return token::div_end;
				case '[':
					array_.clear();
					sp_ = _m_eat_whitespace(sp_ + 1);
					if (*sp_ == ']')
					{
						++sp_;
						return token::array;
					}
					else
					{
						//When search the repeated.
						bool repeated = false;

						while (true)
						{
							sp_ = _m_eat_whitespace(sp_);
							auto tk = read();   // try ??

							if (   token::number   != tk 
								&& token::variable != tk 
								&& token::repeated != tk)

								throw error("invalid array element. Expected a number, variable or repaet", *this);

							if (!repeated)
							{
								switch (tk)
								{
								case token::number:
									array_.push_back(number_);
									break;
								case token::variable:
									array_.push_back({});
									break;
								default:
									repeated = true;
									reparray_.repeated();
									reparray_.assign(std::move(array_));
								}
							}

							sp_ = _m_eat_whitespace(sp_);
							char ch = *sp_++;

							if (ch == ']')
								return (repeated ? token::reparray : token::array);

							if (ch != ',')
								throw error("invalid array", *this);
						}
					}
					break;
				case '(':
					parameters_.clear();
					sp_ = _m_eat_whitespace(sp_ + 1);
					if (*sp_ == ')')
					{
						++sp_;
						return token::parameters;
					}

					while (true)
					{
						if (token::number == read())
							parameters_.push_back(number_);
						else
							throw error("invalid parameter. Expected a number", *this);

						sp_ = _m_eat_whitespace(sp_);
						char ch = *sp_++;

						if (ch == ')')
							return token::parameters;

						if (ch != ',')
							throw error("invalid parameter. Expected a ','", *this);
					}
					break;
				case '.': case '-':
					if (*sp_ == '-')
					{
						readbytes = _m_number(sp_ + 1, true);
						if (readbytes)
							++readbytes;
					}
					else
						readbytes = _m_number(sp_, false);

					if (readbytes)
					{
						sp_ += readbytes;
						return token::number;
					}
					else
						throw error("invalid character '" + std::string(1, *sp_) + "'", *this);
					break;
				default:
					if ('0' <= *sp_ && *sp_ <= '9')
					{
						readbytes = _m_number(sp_, false);
						if (readbytes)
						{
							sp_ += readbytes;
							return token::number;
						}
					}
					break;
				}

				if ('_' == *sp_ || std::isalpha(*sp_))
				{
					const char * idstart = sp_++;

					while ('_' == *sp_ || std::isalpha(*sp_) || std::isalnum(*sp_))
						++sp_;

					idstr_.assign(idstart, sp_);

					if (    "weight" == idstr_ 
					     || "min" == idstr_ 
					     || "max" == idstr_
					     || "width" == idstr_ 
					     || "height" == idstr_
						)
					{
						auto c3 = idstr_[2], c1 =idstr_[0];
						_m_attr_number_value();
						switch (c3)
						{
						case 'i': return c1=='w'? token::weight : token::height;
						case 'n': return token::min_px;
						case 'x': return token::max_px;
						case 'd': return token::width;
						}
					}
					else if ("arrange" == idstr_ || "hfit" == idstr_ || "vfit" == idstr_ || "gap" == idstr_)
					{
						auto ch = idstr_[0];
						_m_attr_reparray();
						switch (ch)
						{
						case 'a': return token::arrange;
						case 'h': return token::hfit;
						case 'v': return token::vfit;
						case 'g': return token::gap;
						default: break;
						}
					}
					else if ("grid" == idstr_ || "margin" == idstr_)
					{
						auto idstr = idstr_;
						if (token::equal != read())
							throw error("an equal sign is required after '" + idstr + "'", *this);

						return ('g' == idstr[0] ? token::grid : token::margin);
					}
					else if ("collapse" == idstr_)
					{
						if (token::parameters != read())
							throw error("a parameter list is required after 'collapse'", *this);
						return token::collapse;
					}
					else if (!idstr_.empty())
					{
						switch (idstr_.front())
						{
						case 'b':
							if ("bottom" == idstr_)
								return token::bottom;
							break;
						case 'd':
							if ("dock" == idstr_)
								return token::dock;
							break;
						case 'f':
							if ("fit" == idstr_)
								return token::fit;
							break;
						case 'i':
							if ("invisible" == idstr_)
								return token::invisible;
							break;
						case 'l':
							if ("left" == idstr_)
								return token::left;
							break;
						case 'r':
							if ("repeated" == idstr_)
								return token::repeated;
							else if ("right" == idstr_)
								return token::right;
							break;
						case 's':
							if ("switchable" == idstr_)
								return token::switchable;
							break;
						case 't':
							if ("top" == idstr_)
								return token::top;
							break;
						case 'u':
							if ("undisplayed" == idstr_)
								return token::undisplayed;
							break;
						case 'v':
							if ("vertical" == idstr_ || "vert" == idstr_)
								return token::vert;
							else if ("variable" == idstr_)
								return token::variable;
							break;
						}
					}

					return token::identifier;
				}

				throw error("invalid character '" + std::string(1, *sp_) + "'", *this);
				return token::error;	//Useless, just for syntax correction.
			}
		private:
			void _m_attr_number_value()
			{
				if (token::equal != read())
					throw error("an equal sign is required after '" + idstr_ + "'", *this);

				auto p = _m_eat_whitespace(sp_);

				auto neg_ptr = p;
				if ('-' == *p)
					++p;

				auto len = _m_number(p, neg_ptr != p);
				if (0 == len)
					throw error("the '" + idstr_ + "' requires a number (integer, real or percent)", *this);

				sp_ += len + (p - sp_);
			}

			void _m_attr_reparray()
			{
				auto idstr = idstr_;
				if (token::equal != read())
					throw error("an equal sign is required after '" + idstr + "'", *this);

				sp_ = _m_eat_whitespace(sp_);

				reparray_.reset();
				auto tk = read();
				switch (tk)
				{
				case token::number:
					reparray_.push(number());
					reparray_.repeated();
					break;
				case token::array:
					reparray_.assign(std::move(array_));
					break;
				case token::reparray:
					break;
				default:
					throw error("a (repeated) array is required after '" + idstr + "'", *this);
				}
			}

			static const char* _m_eat_whitespace(const char* sp) noexcept
			{
				while (*sp && !std::isgraph(int(*sp)))
					++sp;
				return sp;
			}

			std::size_t _m_number(const char* sp, bool negative) noexcept
			{
				/// \todo use std::from_char<int>() etc.

				const char* allstart = sp;
				sp = _m_eat_whitespace(sp);

				number_.assign(0);

				bool gotcha = false;
				int integer = 0;
				double real = 0;
				//read the integral part.
				const char* istart = sp;
				while ('0' <= *sp && *sp <= '9')
				{
					integer = integer * 10 + (*sp - '0');
					++sp;
				}
				const char* iend = sp;

				if ('.' == *sp)
				{
					double div = 1;
					const char* rstart = ++sp;
					while ('0' <= *sp && *sp <= '9')
					{
						real += (*sp - '0') / (div *= 10);
						++sp;
					}

					if (rstart != sp)
					{
						real += integer;
						number_.assign(negative ? -real : real);
						gotcha = true;
					}
				}
				else if (istart != iend)
				{
					number_.assign(negative ? -integer : integer);
					gotcha = true;
				}

				if (gotcha)
				{
					sp = _m_eat_whitespace(sp);
					if ('%' != *sp)
						return sp - allstart;
					
					switch (number_.kind_of())
					{
					case number_t::kind::integer:
						number_.assign_percent(number_.integer());
						break;
					case number_t::kind::real:
						number_.assign_percent(number_.real());
						break;
					default:
						break;
					}

					return sp - allstart + 1;
				}
				number_.reset();
				return 0;
			}
		private:
			const char* divstr_{};   
			const char* sp_{};
			std::string idstr_;
			number_t number_;
			std::vector<number_t> array_;
			repeated_array		reparray_;
			std::vector<number_t> parameters_;
		};	//end class tokenizer
	}

	static bool is_vert_dir(::nana::direction dir)
	{
		return (dir == ::nana::direction::north || dir == ::nana::direction::south);
	}

	static int horz_point(bool vert, const point& pos)
	{
		return (vert ? pos.y : pos.x);
	}
	
	static bool is_idchar(int ch) noexcept
	{
		return ('_' == ch || std::isalnum(ch));
	}

	static std::size_t find_idstr(const std::string& text, const char* idstr, std::size_t off = 0)
	{
		if (!idstr) return text.npos;   /// ??
		const auto len = std::strlen(idstr);

		size_t pos;
		while ((pos = text.find(idstr, off)) != text.npos)
		{
			if (!is_idchar(text[pos + len]))
			{
				if (pos == 0 || !is_idchar(text[pos - 1]))
					return pos;
			}

			off = pos + len; // occurrence not found, advancing the offset and try again
		}
		return text.npos;
	}

	//Find the text boundary of a field. The parameter start_pos is one of bound characters of the field whose bound will be returned.
	//The boundary doesn't include the tag characters.
	static std::pair<std::size_t, std::size_t> get_field_boundary(const std::string& div, std::size_t start_pos)
	{
		int depth = 0;
		if ('<' == div[start_pos])
		{
			auto off = start_pos + 1;
			while (off < div.length())
			{
				auto pos = div.find_first_of("<>", off);
				if (div.npos == pos)
					break;

				if ('<' == div[pos])
				{
					++depth;
					off = pos + 1;
					continue;
				}

				if (0 == depth)
					return{ start_pos + 1, pos };

				--depth;
				off = pos + 1;
			}
		}
		else if (('>' == div[start_pos]) && (start_pos > 0))
		{
			auto off = start_pos - 1;
			while (true)
			{
				auto pos = div.find_last_of("<>", off);
				if (div.npos == pos)
					break;

				if ('>' == div[pos])
				{
					++depth;
					if (0 == pos)
						break;

					off = pos - 1;
				}

				if (0 == depth)
					return{ pos + 1, start_pos};

				if (0 == pos)
					break;

				off = pos - 1;
			}
		}

		return{};
	}

	// Returns the bound of a specified field excluding tag characters.
	static std::pair<std::size_t, std::size_t> get_field_boundary(const std::string& div, const char* idstr, int depth)
	{
		auto start_pos = find_idstr(div, idstr);

		if (depth < 0 || start_pos >= div.length())
			return{};

		while (depth >= 0)
		{
			auto pos = div.find_last_of("<>", start_pos);
			if (div.npos == pos)
				return {0, div.length()};

			start_pos = pos - 1;
			if (div[pos] == '>')
			{
				++depth;
				continue;
			}

			--depth;
		}

		//The second parameter is the index of a tag character
		return get_field_boundary(div, start_pos + 1);
	}

	//struct implement
	struct place::implement
	{
		/// usefull ??
		struct error : std::invalid_argument
		{
			error(std::string            what,
				  std::string            field = "unknown",
			 	  std::string::size_type pos   = std::string::npos)

				: std::invalid_argument{ what + " from place implementation " },
				  pos{ pos },
				  field(field.empty() ? "unnamed" : field)
			{}
			std::string::size_type pos;
			std::string            field;
		};
		class field_gather;
		class field_dock;

		class division;
		class div_arrange;
		class div_grid;
		class div_splitter;
		class div_dock;
		class div_dockpane;
		class div_switchable;

		window window_handle{nullptr};
		event_handle event_size_handle{nullptr};

		std::string div_text;
		std::unique_ptr<division> root_division;
		std::map<std::string, field_gather*> fields;
		std::map<std::string, field_dock*> docks;
		std::map<std::string, field_dock*> dock_factoris;

		std::function<void(window, paint::graphics&, nana::mouse_action)> split_renderer;
		std::set<div_splitter*> splitters;

		//A temporary pointer used to refer to a specified div object which
		//will be deleted in modification process.
		std::unique_ptr<division> tmp_replaced;

		//The following functions are defined behind the definition of class division.
		//because the class division here is an incomplete type.
		~implement();

		void collocate();

		static division * search_div_name(division* start, const std::string&) noexcept;

		std::unique_ptr<division> scan_div(place_parts::tokenizer&, bool implicitly_started, const std::string& ignore_duplicate = {});
		void check_unique(const division*) const;

		//connect the field/dock with div object
		void connect(division* start);
		void disconnect() noexcept;
	};	//end struct implement

	class place::implement::field_gather
		: public place::field_interface
	{
	public:

        /// \todo introduce a place::implement::field_gather::error ??
		struct element_t
		{
			window handle;
			event_handle evt_destroy;

			element_t(window h, event_handle event_destroy) noexcept
				:handle(h), evt_destroy(event_destroy)
			{}
		};

		field_gather(place * p) noexcept
			: place_ptr_(p)
		{}

		~field_gather() noexcept
		{
			for (auto & e : elements)
				API::umake_event(e.evt_destroy);

			for (auto & e : fastened)
				API::umake_event(e.evt_destroy);
		}

		void visible(bool vsb, bool sync_fastened = true)
		{
			for (auto & e : elements)
				API::show_window(e.handle, vsb);

			if (sync_fastened)
			{
				for (auto & e : fastened)
					API::show_window(e.handle, vsb);
			}
		}

		static event_handle erase_element(std::vector<element_t>& elements, window handle) noexcept
		{
			for (auto i = elements.begin(); i != elements.end(); ++i)
			{
				if (i->handle == handle)
				{
					auto evt_destroy = i->evt_destroy;
					elements.erase(i);
					return evt_destroy;
				}
			}
			return nullptr;
		}
	private:
		void _m_insert_widget(window wd, bool to_fasten)  /// \todo better errors caption of failed windows, field
		{
			if (API::empty_window(wd))
				throw place::error("Failed to insert an invalid window handle.", *place_ptr_);

			if (API::get_parent_window(wd) != place_ptr_->window_handle())
				throw place::error("Failed to insert a window which is not a child of the place-binded window", *place_ptr_);

			//Listen to destroy of a window
			//It will delete the element and recollocate when the window destroyed.	
			auto evt = API::events(wd).destroy.connect([this, to_fasten](const arg_destroy& arg)
			{
				if (!to_fasten)
				{
					if (erase_element(elements, arg.window_handle))
					{
						if (!API::is_destroying(API::get_parent_window(arg.window_handle)))
							place_ptr_->collocate();
					}
				}
				else
					erase_element(fastened, arg.window_handle);
			});

			(to_fasten ? &fastened : &elements)->emplace_back(wd, evt);
		}

		field_interface& operator<<(const char* label_text) override
		{
			return static_cast<field_interface*>(this)->operator<<(agent<label>(label_text ? label_text : ""));
		}

		field_interface& operator<<(std::string label_text) override
		{
			return static_cast<field_interface*>(this)->operator<<(agent<label>(label_text));
		}

		field_interface& operator<<(window wd) override
		{
			_m_insert_widget(wd, false);
			return *this;
		}

		field_interface& fasten(window wd) override
		{
			_m_insert_widget(wd, true);
			return *this;
		}

		void _m_add_agent(const detail::place_agent& ag) override
		{
#ifdef _nana_std_has_emplace_return_type
			this->operator<<(
					widgets_.emplace_back(ag.create(place_ptr_->window_handle()))->handle()
				);
#else
			widgets_.emplace_back(ag.create(place_ptr_->window_handle()));
			this->operator<<(widgets_.back()->handle());
#endif
		}
	public:
		division* attached{ nullptr };
		std::vector<std::unique_ptr<nana::widget>> widgets_;
		std::vector<element_t>	elements;
		std::vector<element_t>	fastened;
	private:
		place * place_ptr_;
	};//end class field_gather

	class place::implement::field_dock
	{
	public:
		div_dockpane * attached{ nullptr };					//attached div object
		std::unique_ptr<place_parts::dockarea> dockarea;	//the dockable widget
		std::map<std::string, std::function<std::unique_ptr<widget>(window)>> factories;	//factories for dockpane
	};//end class field_dock


	enum class fit_policy
	{
		none,	//Doesn't fit the content
		both,	//Fits both width and height of content
		horz,	//Fits width of content with a specified height
		vert	//Fits height of content with a specified width
	};

	class place::implement::division
	{
	public:
		enum class kind{ arrange, vertical_arrange, grid, splitter, dock, dockpane, switchable};
		using token = place_parts::tokenizer::token;

		division(kind k, std::string&& n) noexcept
			: kind_of_division(k),
			name(std::move(n))
		{}

		virtual ~division()
		{
			//detach the field
			if (field)
				field->attached = nullptr;
		}

		static unsigned calc_number(const place_parts::number_t& number, unsigned area_px, double adjustable_px, double& precise_px)
		{
			switch (number.kind_of())
			{
			case number_t::kind::integer:
				return static_cast<unsigned>(number.integer());
			case number_t::kind::real:
				return static_cast<unsigned>(number.real());
			case number_t::kind::percent:
			case number_t::kind::none:
				break;
			default:
				return 0; //Useless
			}

			if(number_t::kind::percent == number.kind_of())
				adjustable_px = area_px * number.real() + precise_px;
			else
				adjustable_px += precise_px;

			auto const px = static_cast<unsigned>(adjustable_px);
			precise_px = adjustable_px - px;
			return px;
		}

		std::pair<double, double> calc_weight_floor()
		{
			std::pair<double, double> floor;
			run_.fit_extents.clear();

			run_.weight_floor = floor;

			if (this->display)
			{
				double ratio = 0;

				for (auto & child : children)
				{
					auto child_floor = child->calc_weight_floor();

					if(child->is_percent())
					{
						ratio += child->weight.real();
					}
					else
					{
						floor.first += child_floor.first;
						floor.second += child_floor.second;
					}
				}
				
				auto const vert_fields = (kind::vertical_arrange == this->kind_of_division);
				auto const vert_div = (this->div_owner && (kind::vertical_arrange == this->div_owner->kind_of_division));
				double& fv = (vert_div ? floor.second : floor.first);

				if((ratio > 0.001) && (fv > 0))
					fv /= ratio;

				int set_weight = 0;

				if ((fit_policy::none != this->fit) && this->field)
				{
					std::size_t fit_count = 0;

					unsigned max_value = 0;
					auto const fit_horz = (fit_policy::vert == this->fit);

					std::size_t pos = 0;

					for (auto & elm : this->field->elements)
					{
						++pos;

						unsigned edge_px = 0;
						if (fit_policy::both != this->fit)
						{
							auto fit_val = this->fit_parameters.at(pos - 1);
							if (fit_val.empty())
								continue;

							edge_px = fit_val.integer();
						}

						auto extent = API::content_extent(elm.handle, edge_px, fit_horz);
						if (extent)
						{
							run_.fit_extents[elm.handle] = extent->second;
							++fit_count;
							if (vert_fields)
								floor.second += extent->second.height;
							else
								floor.first += extent->second.width;

							max_value = (std::max)(max_value, (vert_fields ? extent->second.width : extent->second.height));

							if (0 == set_weight)
								set_weight = 1;
						}
						else
							set_weight = -1;
					}

					if (max_value)
					{
						if (vert_fields)
							floor.first = max_value;
						else
							floor.second = max_value;
					}

					//reverse deduction with gap
					if (fit_count > 1)
					{
						double percent = 0;
						for (std::size_t i = 0; i < fit_count - 1; ++i)
						{
							auto gap_value = gap.at(i);
							if (gap_value.kind_of() == number_t::kind::percent)
							{
								percent += gap_value.real();
							}
							else
							{
								double precise_px = 0;
								fv += calc_number(gap_value, 100, 0, precise_px);
							}
						}

						fv *= (1 + percent);
					}

					//reverse deduction with margin
					double margin_per = _m_extend_with_margin(0, floor.second);
					margin_per += _m_extend_with_margin(2, floor.second);

					if (margin_per < 1)
						floor.second /= (1 - margin_per);

					margin_per = _m_extend_with_margin(1, floor.first);
					margin_per += _m_extend_with_margin(3, floor.first);
					if (margin_per < 1)
						floor.first /= (1 - margin_per);
				}
				
				if (!this->weight.empty())
				{
					if (!this->is_percent())
					{
						//Cancel to set weight
						if (fv <= this->weight.real())
							set_weight = -1;
					}
				}

				if (1 == set_weight)
					this->weight.assign(static_cast<int>(fv));

				run_.weight_floor = floor;
				
			}

			return floor;
		}

		void set_visible(bool vsb)
		{
			if (field)
				field->visible(vsb);

			_m_visible_for_child(this, vsb);
			visible = vsb;
		}

		void set_display(bool dsp)
		{
			set_visible(dsp);
			display = dsp;

			if (kind::splitter != kind_of_division)
			{
				//Don't display the previous one, if it is a splitter
				auto div = previous();
				if (div && (kind::splitter == div->kind_of_division))
				{
					if (dsp)
					{
						auto leaf = div->previous();
						if (leaf && leaf->display)
							div->set_display(true);
					}
					else
						div->set_display(false);
				}

				//Don't display the next one, if it is a splitter
				if (div_next && (kind::splitter == div_next->kind_of_division))
				{
					if (dsp)
					{
						auto leaf = div_next->div_next;
						if (leaf && leaf->display)
							div_next->set_display(true);
					}
					else
						div_next->set_display(false);
				}
			}
			else
			{
				//This is a splitter, it only checks when it is being displayed
				if (dsp)
				{
					//Left field of splitterbar
					auto left = this->previous();
					if (left && !left->display)
						left->set_display(true);

					//Right field of splitterbar
					if (div_next && !div_next->display)
						div_next->set_display(true);
				}
			}

			if (display)
			{
				//If the field is a child of switchable field, hides other child fields.
				if (this->div_owner && (kind::switchable == this->div_owner->kind_of_division))
				{
					for (auto & child : this->div_owner->children)
					{
						if (child.get() != this)
							child->set_display(false);
					}
				}
			}
		}

		bool is_back(const division* div) const noexcept
		{
			const division * last = nullptr;
			for (auto & p : children)
			{
				if (!(p->display))
					continue;

				last = p.get();
			}

			return (div == last);
		}

		static double limit_px(const division* div, double px, unsigned area_px) noexcept
		{
			auto const vert = (div->div_owner && (div->div_owner->kind_of_division == kind::vertical_arrange));

			auto weight_floor = (vert? div->run_.weight_floor.second : div->run_.weight_floor.first);

			if (!div->min_px.empty())
			{
				auto v = div->min_px.get_value(static_cast<int>(area_px));

				if ((weight_floor > 0) && (v < weight_floor))
					v = weight_floor;

				if (px < v)
					return v;
			}
			else if ((weight_floor > 0) && (px < weight_floor))
				return weight_floor;

			if (!div->max_px.empty())
			{
				auto v = div->max_px.get_value(static_cast<int>(area_px));
				if (px > v)
					return v;
			}
			return px;
		}

		bool is_fixed() const
		{
			return (weight.kind_of() == number_t::kind::integer);
		}

		bool is_percent() const
		{
			return (weight.kind_of() == number_t::kind::percent);
		}

		nana::rectangle margin_area() const
		{
			return margin.area(field_area);
		}

		division * previous() const noexcept
		{
			if (div_owner)
			{
				for (auto & child : div_owner->children)
					if (child->div_next == this)
						return child.get();
			}
			return nullptr;
		}
	public:
		double _m_extend_with_margin(std::size_t edge, double & extended)
		{
			auto margin_edge = margin.get_edge(edge);
			if (!margin_edge.empty())
			{
				if (margin_edge.kind_of() != number_t::kind::percent)
					extended += margin_edge.real();
				else
					return margin_edge.real();
			}
			return 0;
		}

		static void _m_visible_for_child(division * div, bool vsb) noexcept
		{
			for (auto & child : div->children)
			{
				if (child->field && (!vsb || child->visible))
					child->field->visible(vsb);

				_m_visible_for_child(child.get(), vsb);
			}
		}
		//Collocate the division and its children divisions,
		//The window parameter is specified for the window which the place object binded.
		virtual void collocate(window) = 0;
	public:
		kind kind_of_division;
		bool display{ true };
		bool visible{ true };
		fit_policy fit{ fit_policy::none };
		repeated_array fit_parameters; //it is ignored when fit is not fit_policy::horz or fit_policy::vert
		::nana::direction dir{::nana::direction::west};
		std::string name;
		std::vector<std::unique_ptr<division>> children;

		::nana::rectangle field_area;
		number_t weight;
		token    weigth_type=token::weight;
		number_t min_px, max_px;

		place_parts::margin	margin;
		place_parts::repeated_array gap;
		field_gather * field{ nullptr };
		division * div_next{ nullptr };
		division * div_owner{ nullptr };

		struct run_data
		{
			std::pair<double, double> weight_floor;
			std::map<window, ::nana::size> fit_extents;
		}run_;
	};//end class division

	class place::implement::div_arrange
		: public division
	{
	public:
		div_arrange(bool vert, std::string&& name, place_parts::repeated_array&& arr) noexcept
			: division((vert ? kind::vertical_arrange : kind::arrange), std::move(name)),
			arrange_(std::move(arr))
		{}

		void collocate(window wd) override
		{
			const bool vert = (kind::arrange != kind_of_division);

			auto area_margined = margin_area();
			rectangle_rotator area(vert, area_margined);
			auto area_px = area.w();

			auto fa = _m_fixed_and_adjustable(kind_of_division, area_px);

			double adjustable_px = _m_revise_adjustable(fa, area_px);

			double position = area.x();
			std::vector<division*> delay_collocates;
			double precise_px = 0;
			for (auto& child_ptr : children)					/// First collocate child div's !!!
			{
				auto child = child_ptr.get();
				if(!child->display)	//Ignore the division if the corresponding field is not displayed.
					continue;

				rectangle_rotator child_area(vert, child->field_area);
				child_area.x_ref() = static_cast<int>(position);
				child_area.y_ref() = area.y();
				child_area.h_ref() = area.h();

				double child_px;				// and calculate this div.
				if (!child->is_fixed())			// with is fixed for fixed div
				{
					if (child->is_percent())			// and calculated for others: if the child div is percent - simple take it full
						child_px = area_px * child->weight.real() + precise_px;
					else
						child_px = adjustable_px;

					child_px = limit_px(child, child_px, area_px);

					auto npx = static_cast<unsigned>(child_px);
					precise_px = child_px - npx;
					child_px = npx;
				}
				else
				{
					child_px = static_cast<unsigned>(child->weight.integer());
				}

				//Use 'endpos' to calc width is to avoid deviation
				int endpos = static_cast<int>(position + child_px);
				if ((!child->is_fixed()) && child->max_px.empty() && is_back(child) && (endpos != area.right()))
					endpos = area.right();

				child_area.w_ref() = static_cast<unsigned>((std::max)(endpos - child_area.x(), 0));

				child->field_area = child_area.result();
				position += child_px;

				if (child->kind_of_division == kind::splitter)
					delay_collocates.emplace_back(child);
				else
					child->collocate(wd);	/// The child div have full position. Now we can collocate  inside it the child fields and child-div.
			}

			for (auto child : delay_collocates)
				child->collocate(wd);

			if (visible && display && field)
			{
				auto element_r = area;
				std::size_t index = 0;
				double precise_px = 0;

				for (auto & el : field->elements)
				{
					element_r.x_ref() = static_cast<int>(position);

					bool moved = false;
					unsigned px = 0;

					auto move_r = element_r.result();
					if (fit_policy::none != this->fit)
					{
						auto i = run_.fit_extents.find(el.handle);
						if (run_.fit_extents.end() != i)
						{
							move_r.dimension(i->second);

							if (vert)
								move_r.x += place_parts::differ(area_margined.width, move_r.width) / 2;
							else
								move_r.y += place_parts::differ(area_margined.height, move_r.height) / 2;

							px = (vert ? move_r.height : move_r.width);
							moved = true;
						}
					}

					if (!moved)
					{
						px = calc_number(arrange_.at(index), area_px, adjustable_px, precise_px);
						element_r.w_ref() = px;
						move_r = element_r.result();
					}

					API::move_window(el.handle, move_r);

					if (index + 1 < field->elements.size())
						position += (px + calc_number(gap.at(index), area_px, 0, precise_px));

					++index;
				}

				for (auto & fsn : field->fastened)
					API::move_window(fsn.handle, area_margined);
			}
		}
	private:
		static std::pair<unsigned, std::size_t> _m_calc_fa(const place_parts::number_t& number, unsigned area_px, double& precise_px)
		{
			std::pair<unsigned, std::size_t> result;
			switch (number.kind_of())
			{
			case number_t::kind::integer:
				result.first = static_cast<unsigned>(number.integer());
				break;
			case number_t::kind::real:
				result.first = static_cast<unsigned>(number.real());
				break;
			case number_t::kind::percent:
				{
					double px = number.real() * area_px + precise_px;
					auto npx = static_cast<unsigned>(px);
					result.first = npx;
					precise_px = px - npx;
				}
				break;
			case number_t::kind::none:
				++result.second;
				break;
			}
			return result;
		}

		//Returns the fixed pixels and the number of adjustable items.
		std::pair<unsigned, std::size_t> _m_fixed_and_adjustable(kind match_kind, unsigned area_px) const noexcept
		{
			std::pair<unsigned, std::size_t> result;
			if (field && (kind_of_division == match_kind))
			{
				auto const vert = (kind_of_division == kind::vertical_arrange);

				//Calculate fixed and adjustable of elements
				double precise_px = 0;
				auto count = field->elements.size();
				for (decltype(count) i = 0; i < count; ++i)
				{
					auto fa = _m_calc_fa(arrange_.at(i), area_px, precise_px);

					//The fit-content element is like a fixed element
					if (fit_policy::none != this->fit)
					{
						auto fi = this->run_.fit_extents.find(field->elements[i].handle);
						if (this->run_.fit_extents.cend() != fi)
						{
							fa.first = (vert ? fi->second.height : fi->second.width);
							fa.second = 0; //This isn't an adjustable element
						}
					}

					result.first += fa.first;
					result.second += fa.second;

					if (i + 1 < count)
					{
						fa = _m_calc_fa(gap.at(i), area_px, precise_px);
						result.first += fa.first;
						//fa.second is ignored for gap, because the it has not the adjustable gap.
					}
				}
			}

			double children_fixed_px = 0;
			for (auto& child : children)
			{
				if (!child->display)	//Ignore the division if the corresponding field is not displayed.
					continue;

				if (!child->weight.empty())
					children_fixed_px += child->weight.get_value(area_px);
				else
					++result.second;
			}
			result.first += static_cast<unsigned>(children_fixed_px);
			return result;
		}

		struct revised_division
		{
			division * div;
			double min_px;
			double max_px;
		};

		static double _m_find_lowest_revised_division(const std::vector<revised_division>& revises, double level_px) noexcept
		{
			double v = (std::numeric_limits<double>::max)();

			for(auto & rev : revises)
			{
				if (rev.min_px >= 0 && rev.min_px < v && rev.min_px > level_px)
					v = rev.min_px;
				else if (rev.max_px >= 0 && rev.max_px < v)
					v = rev.max_px;
			}
			return v;
		}

		static std::size_t _m_remove_revised(std::vector<revised_division>& revises, double value, std::size_t& full_count) noexcept
		{
			full_count = 0;
			std::size_t reached_mins = 0;
			auto i = revises.begin();
			while (i != revises.end())
			{
				if (i->max_px == value)
				{
					++full_count;
					i = revises.erase(i);
				}
				else
				{
					if (i->min_px == value)
						++reached_mins;
					++i;
				}
			}
			return reached_mins;
		}

		double _m_revise_adjustable(std::pair<unsigned, std::size_t>& fa, unsigned area_px)
		{
			if (fa.first >= area_px || 0 == fa.second)
				return 0;

			double var_px = area_px - fa.first;

			std::size_t min_count = 0;
			double sum_min_px = 0;
			std::vector<revised_division> revises;

			for (auto& child : children)
			{
				if ((!child->weight.empty()) || (!child->display))
					continue;

				double min_px = std::numeric_limits<double>::lowest(), max_px = std::numeric_limits<double>::lowest();

				if (!child->min_px.empty())
					min_px = child->min_px.get_value(static_cast<int>(area_px));

				auto weight_floor = (this->kind_of_division == kind::arrange ? child->run_.weight_floor.first : child->run_.weight_floor.second);
				if ((weight_floor > 0) && (min_px < weight_floor))
					min_px = weight_floor;

				if(!child->min_px.empty() || (weight_floor > 0))
				{
					sum_min_px += min_px;
					++min_count;					
				}

				if (!child->max_px.empty())
					max_px = child->max_px.get_value(static_cast<int>(area_px));

				if (min_px >= 0 && max_px >= 0 && min_px > max_px)
				{
					if(weight_floor > 0)
						max_px = min_px;
					else if (child->min_px.kind_of() == number_t::kind::percent)
						min_px = std::numeric_limits<double>::lowest();
					else if (child->max_px.kind_of() == number_t::kind::percent)
						max_px = std::numeric_limits<double>::lowest();
				}

				if (min_px >= 0 || max_px >= 0)
					revises.push_back({ child.get(), min_px, max_px });
			}

			if (revises.empty())
				return var_px / fa.second;

			double block_px = 0;
			double level_px = 0;
			auto rest_px = var_px - sum_min_px;
			std::size_t blocks = fa.second;

			while ((rest_px > 0) && blocks)
			{
				auto lowest = _m_find_lowest_revised_division(revises, level_px);

				double fill_px = 0;
				//blocks may be equal to min_count. E.g, all child divisions have min/max attribute.
				if (blocks > min_count)
				{
					fill_px = rest_px / (blocks - min_count);
					if (fill_px + level_px <= lowest)
					{
						block_px += fill_px;
						break;
					}
				}

				block_px = lowest;
				if (blocks > min_count)
					rest_px -= (lowest-level_px) * (blocks - min_count);

				std::size_t full_count;
				min_count -= _m_remove_revised(revises, lowest, full_count);
				blocks -= full_count;
				level_px = lowest;
			}

			return block_px;
		}
	private:
		place_parts::repeated_array arrange_;
	};

	class place::implement::div_grid
		: public division
	{
	public:
		div_grid(std::string&& name, place_parts::repeated_array&& arrange, std::vector<rectangle>&& collapses) noexcept
			: division(kind::grid, std::move(name)),
			arrange_(std::move(arrange)),
			collapses_(std::move(collapses))
		{
			dimension.first = dimension.second = 0;
		}

		void revise_collapses() noexcept
		{
			if (collapses_.empty())
				return;

			for (auto i = collapses_.begin(); i != collapses_.end();)
			{
				if (i->x >= static_cast<int>(dimension.first) || i->y >= static_cast<int>(dimension.second))
					i = collapses_.erase(i);
				else
					++i;
			}

			//Remove the overlapped collapses
			for (std::size_t i = 0; i < collapses_.size() - 1; ++i)
			{
				auto & col = collapses_[i];
				for (auto u = i + 1; u != collapses_.size();)
				{
					auto & z = collapses_[u];
					if (col.is_hit(z.x, z.y) || col.is_hit(z.right(), z.y) || col.is_hit(z.x, z.bottom()) || col.is_hit(z.right(), z.bottom()))
						collapses_.erase(collapses_.begin() + u);
					else
						++u;
				}
			}

			for (auto & col : collapses_)
			{
				if (col.right() >= static_cast<int>(dimension.first))
					col.width = dimension.first - static_cast<unsigned>(col.x);

				if (col.bottom() >= static_cast<int>(dimension.second))
					col.height = dimension.second - static_cast<unsigned>(col.y);
			}
		}

		void collocate(window) override
		{
			if (!field || !(visible && display))
				return;

			auto const area = margin_area();
			auto const gap_size = static_cast<unsigned>(gap.at(0).get_value(area.width)); //gap_size is 0 if gap isn't specified

			auto i = field->elements.cbegin();
			auto const end = field->elements.cend();

			//The total gaps must not beyond the bound of the area.
			if ((gap_size * dimension.first < area.width) && (gap_size * dimension.second < area.height))
			{
				if (dimension.first <= 1 && dimension.second <= 1)
				{
					auto n_of_wd = field->elements.size();
					std::size_t edge;
					switch (n_of_wd)
					{
					case 0:
					case 1:
						edge = 1;	break;
					case 2: case 3: case 4:
						edge = 2;	break;
					default:
						edge = static_cast<std::size_t>(std::sqrt(n_of_wd));
						if ((edge * edge) < n_of_wd) ++edge;
					}

					double y = area.y;
					const double block_w = area.width / double(edge);
					const double block_h = area.height / double((n_of_wd / edge) + (n_of_wd % edge ? 1 : 0));
					const unsigned uns_block_w = static_cast<unsigned>(block_w);
					const unsigned uns_block_h = static_cast<unsigned>(block_h);
					const unsigned height = (uns_block_h > gap_size ? uns_block_h - gap_size : uns_block_h);

					std::size_t arr_pos = 0;
					for (std::size_t u = 0; (u < edge && i != end); ++u)
					{
						double x = area.x;
						for (std::size_t v = 0; (v < edge && i != end); ++v, ++i)
						{
							unsigned value = 0;
							auto arr = arrange_.at(arr_pos++);

							if (arr.empty())
								value = static_cast<decltype(value)>(block_w);
							else
								value = static_cast<decltype(value)>(arr.get_value(static_cast<int>(area.width)));

							unsigned width = (value > uns_block_w ? uns_block_w : value);
							if (width > gap_size)	width -= gap_size;
							API::move_window(i->handle, rectangle{ static_cast<int>(x), static_cast<int>(y), width, height });
							x += block_w;
						}
						y += block_h;
					}
				}
				else
				{
					const double block_w = int(area.width - gap_size * (dimension.first - 1)) / double(dimension.first);
					const double block_h = int(area.height - gap_size * (dimension.second - 1)) / double(dimension.second);

					std::unique_ptr<char[]> table_ptr{ new char[dimension.first * dimension.second] };
					char *table = table_ptr.get();
					std::memset(table, 0, dimension.first * dimension.second);

					std::size_t lbp = 0;

					double precise_h = 0;
					for (std::size_t u = 0; (u < dimension.second && i != end); ++u)
					{
						auto const block_height_px = static_cast<unsigned>(block_h + precise_h);
						precise_h = (block_h + precise_h) - block_height_px;

						double precise_w = 0;
						for (std::size_t v = 0; (v < dimension.first && i != end); ++v)
						{
							auto const epos = v + lbp;
							if (table[epos])
							{
								precise_w += block_w;
								precise_w -= static_cast<int>(precise_w);
								continue;
							}

							std::pair<unsigned, unsigned> room{ 1, 1 };
							_m_find_collapse(static_cast<int>(v), static_cast<int>(u), room);

							const int pos_x = area.x + static_cast<int>(v * (block_w + gap_size));
							const int pos_y = area.y + static_cast<int>(u * (block_h + gap_size));

							unsigned result_h;
							if (room.first <= 1 && room.second <= 1)
							{
								precise_w += block_w;
								result_h = block_height_px;
								table[epos] = 1;
							}
							else
							{
								precise_w += block_w * room.first + (room.first - 1) * gap_size;
								result_h = static_cast<unsigned>(block_h * room.second + precise_h + (room.second - 1) * gap_size);

								for (unsigned y = 0; y < room.second; ++y)
									for (unsigned x = 0; x < room.first; ++x)
										table[epos + x + y * dimension.first] = 1;
							}

							unsigned result_w = static_cast<unsigned>(precise_w);
							precise_w -= result_w;

							API::move_window(i->handle, rectangle{ pos_x, pos_y, result_w, result_h });
							++i;
						}

						lbp += dimension.first;
					}
				}
			}

			// Empty the size of windows that are out range of grid
			for (; i != end; ++i)
				API::window_size(i->handle, size{});

			for (auto & fsn : field->fastened)
				API::move_window(fsn.handle, area);
		}
	public:
		std::pair<unsigned, unsigned> dimension;
	private:
		void _m_find_collapse(int x, int y, std::pair<unsigned, unsigned>& collapse) const noexcept
		{
			for (auto & col : collapses_)
			{
				if (col.x == x && col.y == y)
				{
					collapse.first = col.width;
					collapse.second = col.height;
					return;
				}
			}
		}
	private:
		place_parts::repeated_array arrange_;
		std::vector<rectangle> collapses_;
	};//end class div_grid

	class place::implement::div_splitter
		: public division
	{
		struct div_block
		{
			division * div;
			int	pixels;
			double		scale;

			div_block(division* d, int px) noexcept
				: div(d), pixels(px)
			{}
		};

		enum{splitter_px = 4};
	public:
		div_splitter(const place_parts::number_t & init_weight, implement* impl) noexcept :
			division(kind::splitter, std::string()),
			impl_(impl),
			init_weight_(init_weight)
		{
			impl->splitters.insert(this);
			this->splitter_.set_renderer(impl_->split_renderer);

			this->weight.assign(splitter_px);
		}

		~div_splitter()
		{
			impl_->splitters.erase(this);
		}

		void set_renderer(const std::function<void(window, paint::graphics&, mouse_action)> & fn, bool update)
		{
			this->splitter_.set_renderer(fn);
			if (update && this->splitter_.handle())
				API::refresh_window(this->splitter_);
		}

		void direction(bool horizontal) noexcept
		{
			splitter_cursor_ = (horizontal ? cursor::size_we : cursor::size_ns);
		}
	private:
		void collocate(window wd) override
		{
			if (API::is_destroying(wd))
				return;

			if (splitter_.empty())
			{
				splitter_.create(wd);
				splitter_.cursor(splitter_cursor_);

				dragger_.trigger(splitter_);

				auto grab_fn = [this](const arg_mouse& arg)
				{
					if ((false == arg.left_button) && (mouse::left_button != arg.button))
						return;

					auto const leaf_left = _m_leaf(true);
					auto const leaf_right = _m_leaf(false);

					if (event_code::mouse_down == arg.evt_code)
					{
						begin_point_ = splitter_.pos();

						auto px_ptr = &nana::rectangle::width;

						//Use field_area of leaf, not margin_area. Otherwise splitter would be at wrong position
						auto const area_left = leaf_left->field_area;
						auto const area_right = leaf_right->field_area;

						if (nana::cursor::size_we != splitter_cursor_)
						{
							left_pos_ = area_left.y;
							right_pos_ = area_right.bottom();
							px_ptr = &nana::rectangle::height;
						}
						else
						{
							left_pos_ = area_left.x;
							right_pos_ = area_right.right();
						}

						left_pixels_ = area_left.*px_ptr;
						right_pixels_ = area_right.*px_ptr;
						
						grabbed_ = true;
					}
					else if(event_code::mouse_up == arg.evt_code)
					{
						grabbed_ = false;
						this->_m_update_div(impl_->div_text);
					}
					else if (event_code::mouse_move == arg.evt_code)
					{
						if(!grabbed_)
							return;
							
						auto const vert = (::nana::cursor::size_we != splitter_cursor_);
						auto const delta = horz_point(vert, splitter_.pos() - begin_point_);

						const auto total_pixels = static_cast<int>(left_pixels_ + right_pixels_);

						auto left_px = std::clamp(static_cast<int>(left_pixels_) + delta, 0, total_pixels);

						auto area_px = rectangle_rotator(vert, div_owner->margin_area()).w();
						double imd_rate = 100.0 / area_px;
						left_px = static_cast<int>(limit_px(leaf_left, left_px, area_px));
						leaf_left->weight.assign_percent(imd_rate * left_px);

						auto right_px = std::clamp(static_cast<int>(right_pixels_) - delta, 0, total_pixels);

						right_px = static_cast<int>(limit_px(leaf_right, right_px, area_px));
						leaf_right->weight.assign_percent(imd_rate * right_px);

						pause_move_collocate_ = true;
						div_owner->collocate(splitter_.parent());

						//After the collocating, the splitter keeps the calculated weight of left division,
						//and clear the weight of right division.
						leaf_right->weight.reset();

						pause_move_collocate_ = false;
					}
				};

				auto & events = splitter_.events();
				events.mouse_down.connect_unignorable(grab_fn);
				events.mouse_up.connect_unignorable(grab_fn);
				events.mouse_move.connect_unignorable(grab_fn);
			}

			auto limited_range = _m_update_splitter_range();

			if (!init_weight_.empty())
			{
				const bool vert = (::nana::cursor::size_we != splitter_cursor_);

				auto leaf_left = _m_leaf(true);
				auto leaf_right = _m_leaf(false);
				rectangle_rotator left(vert, leaf_left->field_area);
				rectangle_rotator right(vert, leaf_right->field_area);
				auto area_px = right.right() - left.x();
				auto right_px = static_cast<int>(limit_px(leaf_right, init_weight_.get_value(area_px), static_cast<unsigned>(area_px)));

				//New position of splitter
				const auto pos = std::clamp(static_cast<int>(area_px - right_px - splitter_px), limited_range.x(), limited_range.right());

				rectangle_rotator sp_r(vert, field_area);
				sp_r.x_ref() = pos;

				left.w_ref() = static_cast<unsigned>(pos - left.x());

				auto right_pos = right.right();
				right.x_ref() = (pos + splitter_px);
				right.w_ref() = static_cast<unsigned>(right_pos - pos - splitter_px);

				field_area = sp_r.result();
				leaf_left->field_area = left.result();
				leaf_right->field_area = right.result();
				leaf_left->collocate(wd);
				leaf_right->collocate(wd);

				//Set the leafs' weight
				rectangle_rotator area(vert, div_owner->field_area);

				double imd_rate = 100.0 / static_cast<int>(area.w());
				leaf_left->weight.assign_percent(imd_rate * static_cast<int>(left.w()));
				leaf_right->weight.assign_percent(imd_rate * static_cast<int>(right.w()));

				splitter_.move(this->field_area);

				init_weight_.reset();
			}

			if (false == pause_move_collocate_)
				splitter_.move(this->field_area);
		}
	private:
		static int _m_search_name(const division* div, std::string& name) noexcept
		{
			if (div->name.size())
			{
				name = div->name;
				return 0;
			}

			for (auto & child : div->children)
			{
				if (child->name.size())
				{
					name = child->name;
					return 1;
				}

				auto depth = _m_search_name(child.get(), name);
				if (depth >= 0)
					return depth + 1;
			}

			return -1;
		}

		static void _m_remove_attr(std::string& div, const char* attr)
		{
			auto attr_pos = div.find(attr);
			if (div.npos == attr_pos)
				return;

			std::size_t off = 0;
			while (true)
			{
				auto pos = div.find('<', off);
				if (div.npos == pos)
					break;

				if (attr_pos < pos)
					break;

				int depth = 0;
				off = pos + 1;
				std::size_t endpos = 0;
				while (true)
				{
					endpos = div.find_first_of("<>", off);
					if (div.npos == endpos)
						return;
					
					if ('<' == div[endpos])
					{
						++depth;
						off = endpos + 1;
						continue;
					}

					if (0 == depth)
						break;

					--depth;
					off = endpos + 1;
				}

				if (attr_pos < endpos)
				{
					attr_pos = div.find(attr, endpos + 1);
					if (div.npos == attr_pos)
						return;
				}

				off = endpos + 1;
			}

			auto len = std::strlen(attr);

			auto endpos = div.find_first_not_of(" ", attr_pos + len);
			if (div.npos != endpos && div[endpos] == '=')
			{
				endpos = div.find_first_not_of(" 0123456789.%", endpos + 1);

				//endpos is a npos if the div doesn't contain the boundary tags
				if (div.npos == endpos)
					endpos = div.length();
			}
			else
				endpos = attr_pos + len;

			div.erase(attr_pos, endpos - attr_pos);
		}

		void _m_update_div(std::string& div)
		{
			auto const leaf_left = _m_leaf(true);
			auto const leaf_right = _m_leaf(false);

			std::string name;
			bool left = true;

			//Search a name recursively from a specified leaf field.
			//It returns the depth from the leaf div to the div which has a name.
			auto depth = _m_search_name(leaf_left, name);
			if (-1 == depth)
			{
				left = false;
				depth = _m_search_name(leaf_right, name);
				if (-1 == depth)
					return;
			}

			//Get the bound of field div-text through reverse recursion.
			auto bound = get_field_boundary(div, name.c_str(), depth);
			if (bound.first == bound.second)
				return;

			auto fieldstr = div.substr(bound.first, bound.second - bound.first);
			_m_remove_attr(fieldstr, "weight");   /// \todo and higth, width ?

			std::string::size_type tag_pos{ left ? div.find('<', bound.second + 2) : div.rfind('>', bound.first - 2) };
			if (div.npos == tag_pos)
				throw place::implement::error("please report an issue: the splitter was unable to update division " + div, name);

			auto other_bound = get_field_boundary(div, tag_pos);

			auto other_fieldstr = div.substr(other_bound.first, other_bound.second - other_bound.first);
			_m_remove_attr(other_fieldstr, "weight");

			const bool vert = (::nana::cursor::size_we != splitter_cursor_);

			rectangle_rotator r_left(vert, leaf_left->field_area);
			rectangle_rotator r_right(vert, leaf_right->field_area);
			rectangle_rotator r_owner(vert, this->div_owner->field_area);

			double percent = double((left ? r_left : r_right).w()) / double(r_owner.w());

			fieldstr += " weight=" + std::to_string(percent * 100) + "%";

			//Replaces the 'right' field before 'left' in order to make the bound consistent
			if (left)
			{
				if (other_fieldstr.length() != (other_bound.second - other_bound.first))
					div.replace(other_bound.first, other_bound.second - other_bound.first, other_fieldstr);

				div.replace(bound.first, bound.second - bound.first, fieldstr);
			}
			else
			{
				div.replace(bound.first, bound.second - bound.first, fieldstr);

				if (other_fieldstr.length() != (other_bound.second - other_bound.first))
					div.replace(other_bound.first, other_bound.second - other_bound.first, other_fieldstr);
			}
		}

		division * _m_leaf(bool left) const noexcept
		{
			return (left ? previous() : div_next);
		}

		rectangle_rotator _m_update_splitter_range()
		{
			const bool vert = (cursor::size_ns == splitter_cursor_);

			rectangle_rotator area(vert, div_owner->margin_area());

			auto leaf_left = _m_leaf(true);
			auto leaf_right = _m_leaf(false);

			rectangle_rotator left(vert, leaf_left->field_area);
			rectangle_rotator right(vert, leaf_right->field_area);

			const int left_base = left.x(), right_base = right.right();
			int pos = left_base;
			int endpos = right_base;

			if (!leaf_left->min_px.empty())
				pos += static_cast<int>(leaf_left->min_px.get_value(area.w()));
	
			if (!leaf_left->max_px.empty())
				endpos = left_base + static_cast<int>(leaf_left->max_px.get_value(area.w()));

			if (!leaf_right->min_px.empty())
				endpos = (std::min)(right_base - static_cast<int>(leaf_right->min_px.get_value(area.w())), endpos);

			if (!leaf_right->max_px.empty())
				pos = (std::max)(right_base - static_cast<int>(leaf_right->max_px.get_value(area.w())), pos);

			area.x_ref() = pos;
			area.w_ref() = unsigned(endpos - pos + splitter_px);

			dragger_.target(splitter_, area.result(), (vert ? nana::arrange::vertical : nana::arrange::horizontal));

			return area;
		}
	private:
		implement* const impl_;
		nana::cursor	splitter_cursor_{nana::cursor::arrow};
		place_parts::splitter	splitter_;
		nana::point	begin_point_;
		int			left_pos_, right_pos_;
		unsigned	left_pixels_, right_pixels_;
		dragger	dragger_;
		bool	grabbed_{ false };
		bool	pause_move_collocate_{ false };	//A flag represents whether do move when collocating.
		place_parts::number_t init_weight_;
	};

	class place::implement::div_dockpane
		: public division, public place_parts::dock_notifier_interface
	{
	public:
		div_dockpane(std::string && name, implement* impl, direction pane_dir) noexcept
			:	division(kind::dockpane, std::move(name)),
				impl_ptr_{impl}
		{
			dir = pane_dir;
			this->display = false;
		}

		~div_dockpane() noexcept
		{
			if (dockable_field)
			{
				dockable_field->dockarea.reset();
				dockable_field->attached = nullptr;
			}
		}

		void collocate(window) override
		{
			if (!dockable_field)
			{
				if (name.empty())
					return;

				auto &dock_ptr = impl_ptr_->docks[name];
				if (!dock_ptr)
					dock_ptr = new field_dock;

				dock_ptr->attached = this;
				dockable_field = dock_ptr;
			}

			auto & dockarea = dockable_field->dockarea;
			if (dockarea && !dockarea->floating())
				dockarea->move(this->field_area);
		}
	private:
		//Implement dock_notifier_interface
		void notify_float() override
		{
			set_display(false);
			impl_ptr_->collocate();
		}

		void notify_dock() override
		{
			indicator_.docker.reset();

			set_display(true);
			impl_ptr_->collocate();
		}

		void notify_move() override
		{
			if (!_m_hit_test(false)) //hit test on indicator
			{
				indicator_.docker.reset();
				return;
			}

			if (!indicator_.docker)
			{
				auto host_size = API::window_size(impl_ptr_->window_handle);
				indicator_.docker.reset(new form(impl_ptr_->window_handle, { static_cast<int>(host_size.width) / 2 - 16, static_cast<int>(host_size.height) / 2 - 16, 32, 32 }, form::appear::bald<>()));
				drawing dw(indicator_.docker->handle());
				dw.draw([](paint::graphics& graph)
				{
					graph.rectangle(false, colors::midnight_blue);
					graph.rectangle({ 1, 1, 30, 30 }, true, colors::light_sky_blue);

					facade<element::arrow> arrow;
					arrow.direction(::nana::direction::south);
					arrow.draw(graph, colors::light_sky_blue, colors::midnight_blue, { 12, 0, 16, 16 }, element_state::normal);

					graph.rectangle({ 4, 16, 24, 11 }, true, colors::midnight_blue);
					graph.rectangle({ 5, 19, 22, 7 }, true, colors::button_face);
				});

				indicator_.docker->z_order(nullptr, ::nana::z_order_action::topmost);
				indicator_.docker->show();

				indicator_.docker->events().destroy.connect([this](const arg_destroy&)
				{
					if (indicator_.dock_area)
					{
						indicator_.dock_area.reset();
						indicator_.graph.release();
					}
				});
			}

			if (_m_hit_test(true)) //hit test on docker
			{
				if (!indicator_.dock_area)
				{
					set_display(true);
					impl_ptr_->collocate();

					indicator_.graph.make(API::window_size(impl_ptr_->window_handle));
					API::window_graphics(impl_ptr_->window_handle, indicator_.graph);

					indicator_.dock_area.reset(new panel<true>(impl_ptr_->window_handle, false));
					indicator_.dock_area->move(this->field_area);

					::nana::drawing dw(indicator_.dock_area->handle());
					dw.draw([this](paint::graphics& graph)
					{
						indicator_.graph.paste(this->field_area, graph, 0, 0);

						const int border_px = 4;
						rectangle r{ graph.size() };
						int right = r.right();
						int bottom = r.bottom();

						graph.blend(r.pare_off(border_px), colors::blue, 0.3);

						::nana::color clr = colors::deep_sky_blue;
						r.y = 0;
						r.height = border_px;
						graph.blend(r, clr, 0.5);
						r.y = bottom - border_px;
						graph.blend(r, clr, 0.5);

						r.x = r.y = 0;
						r.dimension(graph.size());
						r.width = border_px;
						graph.blend(r, clr, 0.5);
						r.x = right - border_px;
						graph.blend(r, clr, 0.5);

					});

					API::bring_top(indicator_.dock_area->handle(), false);
					indicator_.dock_area->show();
				}
			}
			else
			{
				set_display(false);
				impl_ptr_->collocate();
				if (indicator_.dock_area)
				{
					indicator_.dock_area.reset();
					indicator_.graph.release();
				}
			}
		}

		void notify_move_stopped() override
		{
			//hit test on docker
			if (_m_hit_test(true) && dockable_field && dockable_field->dockarea)
				dockable_field->dockarea->dock();

			indicator_.docker.reset();
		}

		void request_close() override
		{
			auto window_handle = dockable_field->dockarea->handle();

			//a workaround for capture
			auto ptr = dockable_field->dockarea.release();
			API::at_safe_place(window_handle, [ptr]
			{
				std::unique_ptr<typename std::remove_pointer<decltype(ptr)>::type> del(ptr);
			});

			this->set_display(false);
			impl_ptr_->collocate();

			API::close_window(window_handle);
		}
	private:
		bool _m_hit_test(bool try_docker) const
		{
			window handle = nullptr;
			if (try_docker)
			{
				if (!indicator_.docker)
					return false;

				handle = indicator_.docker->handle(); //hit test for docker
			}
			else
				handle = impl_ptr_->window_handle;	//hit test for indicator

			point pos;
			API::calc_screen_point(handle, pos);
			return rectangle{ pos, API::window_size(handle) }.is_hit(API::cursor_position());
		}
	public:
		field_dock * dockable_field{ nullptr };

		std::unique_ptr<widget>	splitter;
	private:
		implement * impl_ptr_;

		//
		struct indicator_rep
		{
			paint::graphics graph;
			std::unique_ptr<panel<true>> dock_area;
			std::unique_ptr<form> docker;
		}indicator_;
	};

	class place::implement::div_dock
		: public division
	{
		static const unsigned splitter_px = 5;

		class splitter : public panel<true>
		{
		public:
			splitter(window wd, ::nana::direction dir, division* dock_dv, division* pane_dv)
				: panel<true>(wd, true), dir_(dir), dock_dv_(dock_dv), pane_dv_(pane_dv)
			{
				this->bgcolor(colors::alice_blue);
				this->cursor(is_vert_dir(dir_) ? ::nana::cursor::size_ns : ::nana::cursor::size_we);

				auto grab_fn = [this, wd](const arg_mouse& arg)
				{
					auto const is_vert = is_vert_dir(dir_);

					if (event_code::mouse_down == arg.evt_code)	//press mouse button
					{
						if (arg.button != ::nana::mouse::left_button)
							return;

						this->set_capture(true);

						base_pos_.x = horz_point(is_vert, API::cursor_position());
						base_pos_.y = horz_point(is_vert, this->pos());

						base_px_ = (is_vert ? pane_dv_->field_area.height : pane_dv_->field_area.width);
					}
					else if (event_code::mouse_move == arg.evt_code)	//hover
					{
						if (!arg.is_left_button())
							return;

						auto delta = horz_point(is_vert, API::cursor_position()) - base_pos_.x;
						int new_pos = base_pos_.y + delta;
						if (new_pos < range_.x)
						{
							new_pos = range_.x;
							delta = new_pos - base_pos_.y;
						}
						else if (new_pos >= range_.y)
						{
							new_pos = range_.y - 1;
							delta = new_pos - base_pos_.y;
						}

						auto now_pos = this->pos();
						if (is_vert)
							now_pos.y = new_pos;
						else
							now_pos.x = new_pos;
						this->move(now_pos);

						auto px = base_px_;
						switch (dir_)
						{
						case ::nana::direction::west:
						case ::nana::direction::north:
							if (delta < 0)
								px -= static_cast<unsigned>(-delta);
							else
								px += static_cast<unsigned>(delta);
							break;
						case ::nana::direction::east:
						case ::nana::direction::south:
							if (delta < 0)
								px += static_cast<unsigned>(-delta);
							else
								px -= static_cast<unsigned>(delta);
							break;
						default:
							break;
						}

						auto dock_px = (is_vert ? dock_dv_->field_area.height : dock_dv_->field_area.width);

						pane_dv_->weight.assign_percent(double(px) / double(dock_px) * 100);

						dock_dv_->collocate(wd);
					}
					else
						this->release_capture();
				};

				auto & evt = this->events();
				evt.mouse_down.connect(grab_fn);
				evt.mouse_up.connect(grab_fn);
				evt.mouse_move.connect(grab_fn);
			}

			void range(int begin, int end) noexcept
			{
				range_.x = begin;
				range_.y = end;
			}
		private:
			const ::nana::direction	dir_;
			division* const dock_dv_;
			division* const	pane_dv_;
			::nana::point	range_;
			::nana::point	base_pos_;	//x = mouse pos, y = splitter pos
			unsigned		base_px_;	//weight of div_dockpane when mouse button is been pressing;

		};
	public:
		div_dock(std::string && name, implement* impl) noexcept
			: division(kind::dock, std::move(name)), impl_(impl)
		{}

		division* front() const noexcept
		{
			for (auto & child : children)
			{
				if (child->display)
					return child.get();
			}

			return nullptr;
		}

		void collocate(window wd) override
		{
			auto area = this->margin_area();

			unsigned vert_count = 0, horz_count = 0;

			bool is_first = true;
			bool prev_attr = false;

			for (auto & child : children)
			{
				if (!child->display)
					continue;

				const auto is_vert = is_vert_dir(child->dir);
				if (is_first)
				{
					is_first = false;
					(is_vert ? horz_count : vert_count) = 1;
					prev_attr = is_vert;
				}

				if ((is_vert == prev_attr) == is_vert)
					++vert_count;
				else
					++horz_count;

				prev_attr = is_vert;
			}
			if (0 == vert_count)
				vert_count = 1;
			if (0 == horz_count)
				horz_count = 1;

			//room indicates the size without splitters
			::nana::size room(area.width - splitter_px * (horz_count - 1), area.height - splitter_px * (vert_count - 1));

			double left = area.x;
			double right = area.right();
			double top = area.y;
			double bottom = area.bottom();

			for (auto & child : children)
			{
				if (!child->display)
				{
					auto child_dv = dynamic_cast<div_dockpane*>(child.get());
					if (child_dv)
						child_dv->splitter.reset();

					continue;
				}

				auto child_dv = dynamic_cast<div_dockpane*>(child.get());
				const bool is_vert = is_vert_dir(child->dir);

				auto room_px = (is_vert ? room.height : room.width);

				double weight;
				if (!child->weight.empty())
				{
					weight = child->weight.get_value(is_vert ? room.height : room.width);
					if (weight > room_px)
						weight = room_px;
				}
				else
					weight = room_px / double(is_vert ? vert_count : horz_count);

				splitter* split = nullptr;
				if (_m_right(child_dv))
				{
					//Creates a splitbar if the 'right' leaf is not empty
					if (!child_dv->splitter)
					{
						split = new splitter(impl_->window_handle, child->dir, this, child_dv);
						child_dv->splitter.reset(split);
					}
					else
						split = dynamic_cast<splitter*>(child_dv->splitter.get());
				}
				else
					child_dv->splitter.reset();

				::nana::rectangle child_r;
				double split_range_begin = -1, split_range_end = 0;
				switch (child->dir)
				{
				default:
				case ::nana::direction::west:
					child_r.x = static_cast<int>(left);
					child_r.y = static_cast<int>(top);
					child_r.width = static_cast<unsigned>(weight);
					child_r.height = static_cast<unsigned>(bottom - top);
					left += weight;
					if (split)
					{
						split->move(rectangle{ child_r.right(), child_r.y, splitter_px, child_r.height });
						split_range_begin = left - weight;
						split_range_end = right - static_cast<int>(splitter_px);
						left += splitter_px;
					}
					break;
				case ::nana::direction::east:
					right -= weight;
					child_r.x = static_cast<int>(right);
					child_r.y = static_cast<int>(top);
					child_r.width = static_cast<unsigned>(weight);
					child_r.height = static_cast<unsigned>(bottom - top);
					if (split)
					{
						split->move(rectangle{ child_r.x - static_cast<int>(splitter_px), child_r.y, splitter_px, child_r.height });
						split_range_begin = left;
						split_range_end = right - static_cast<int>(splitter_px)+weight;
						right -= splitter_px;
					}
					break;
				case ::nana::direction::north:
					child_r.x = static_cast<int>(left);
					child_r.y = static_cast<int>(top);
					child_r.width = static_cast<unsigned>(right - left);
					child_r.height = static_cast<unsigned>(weight);
					top += weight;
					if (split)
					{
						split->move(rectangle{ child_r.x, child_r.bottom(), child_r.width, splitter_px });
						split_range_begin = top - weight;
						split_range_end = bottom - static_cast<int>(splitter_px);
						top += splitter_px;
					}
					break;
				case ::nana::direction::south:
					child_r.x = static_cast<int>(left);
					bottom -= weight;
					child_r.y = static_cast<int>(bottom);
					child_r.width = static_cast<unsigned>(right - left);
					child_r.height = static_cast<unsigned>(weight);
					if (split)
					{
						bottom -= splitter_px;
						split->move(rectangle{ child_r.x, child_r.y - static_cast<int>(splitter_px), child_r.width, splitter_px });
						split_range_begin = top;
						split_range_end = bottom + weight;
					}
					break;
				}

				if (split_range_begin > -0.5)
					split->range(static_cast<int>(split_range_begin), static_cast<int>(split_range_end));

				if (is_vert)
				{
					room.height -= child_r.height;
					--vert_count;
				}
				else
				{
					room.width -= child_r.width;
					--horz_count;
				}

				child->field_area = child_r;
				child->collocate(wd);
			}
		}
	private:
		static div_dockpane* _m_right(division* dv)
		{
			dv = dv->div_next;
			while (dv)
			{
				if (dv->display)
					return dynamic_cast<div_dockpane*>(dv);

				dv = dv->div_next;
			}
			return nullptr;
		}
	private:
		implement * const impl_;
	};

	class place::implement::div_switchable
		: public division
	{
	public:
		div_switchable(std::string && name, implement*) noexcept:
			division(kind::switchable, std::move(name))
		{}
	private:
		void collocate(window wd) override
		{
			division * div = nullptr;
			for (auto & child : children)
			{
				if (child->display)
				{
					div = child.get();
					div->field_area = this->margin_area();
					div->collocate(wd);
					break;
				}
			}

			//Hide other child fields.
			rectangle empty_r{ this->margin_area().position() , size{ 0, 0 } };
			for (auto & child : children)
			{
				if (child.get() != div)
				{
					child->field_area = empty_r;
					child->collocate(wd);
				}
			}
		}
	};

	place::implement::~implement()
	{
		API::umake_event(event_size_handle);
		root_division.reset();

		for (auto & pair : fields)
			delete pair.second;

		for (auto & dock : docks)
			delete dock.second;
	}

	void place::implement::collocate()
	{
		if (root_division && window_handle)
		{
			root_division->field_area.dimension(API::window_size(window_handle));

			if (root_division->field_area.empty())
				return;

			root_division->calc_weight_floor();

			root_division->collocate(window_handle);

			for (auto & field : fields)
			{
				bool is_show = false;
				if (field.second->attached && field.second->attached->visible && field.second->attached->display)
				{
					is_show = true;
					auto div = field.second->attached->div_owner;
					while (div)
					{
						if (!div->visible || !div->display)
						{
							is_show = false;
							break;
						}
						div = div->div_owner;
					}
				}

				//Collocate doesn't sync the visibility of fastened windows.
				//This is a feature that allows tabbar panels to be fastened to a same field, the collocate()
				//shouldn't break the visibility of panels that are maintained by tabbar.
				field.second->visible(is_show, false);
			}
		}
	}

	//search_div_name
	//search a division with the specified name.
	place::implement::division * place::implement::search_div_name(division* start, const std::string& name) noexcept
	{
		if (start)
		{
			if (start->name == name)
				return start;

			for (auto& child : start->children)
			{
				auto div = search_div_name(child.get(), name);
				if (div)
					return div;
			}
		}
		return nullptr;
	}

	static int get_parameter(place_parts::tokenizer& tknizer, std::size_t pos)
	{
		auto & arg = tknizer.parameters()[pos];

		if (arg.kind_of() == number_t::kind::integer)
			return arg.integer();
		else if (arg.kind_of() == number_t::kind::real)
			return static_cast<int>(arg.real());

		const char* pos_strs[] = { "1st", "2nd", "3rd", "4th" };
		throw place_parts::tokenizer::error("the type of the " + std::string{pos_strs[pos]} +" parameter for collapse should be integer.", tknizer);
	}

	//implicitly_started indicates whether the field in div-text starts without < mark. 
	//ignore_duplicate A field is allowed to have same name if its has an ancestor which name is same with ignore_duplicate.
	auto place::implement::scan_div(place_parts::tokenizer& tknizer, bool implicitly_started, const std::string& ignore_duplicate) -> std::unique_ptr<division>
	{
		using token = place_parts::tokenizer::token ;

		std::unique_ptr<division> div;
		token div_type = token::eof , weight_type=token::weight;
		auto fit = fit_policy::none;
		place_parts::repeated_array fit_parameters;

		//These variables stand for the new division's attributes
		std::string name;
		number_t weight, min_px, max_px;
		place_parts::repeated_array arrange, gap;
		place_parts::margin margin;
		std::vector<number_t> array;
		std::vector<rectangle> collapses;
		std::vector<std::unique_ptr<division>> children;
		::nana::direction div_dir = ::nana::direction::west;

		bool undisplayed = false;
		bool invisible = false;

		token tk = token::eof;
		try {
			for (tk = tknizer.read(); (tk != token::eof && tk != token::div_end); tk = tknizer.read())
			{
				switch (tk)
				{
				case token::dock:
					if (token::eof != div_type && token::dock != div_type)
						throw std::invalid_argument("conflict of div type -expected dock type-");

					div_type = token::dock;
					break;
				case token::fit:
					fit = fit_policy::both;
					break;
				case token::hfit:
				case token::vfit:
					fit = (token::hfit == tk ? fit_policy::horz : fit_policy::vert);
					fit_parameters = tknizer.reparray();
					break;
				case token::splitter:
					//Ignore the splitter when there is not a division.
					if (!children.empty() && (division::kind::splitter != children.back()->kind_of_division))
					{
						auto splitter = new div_splitter(tknizer.number(), this);
						children.back()->div_next = splitter;

						//Hides the splitter if its left leaf is undisplayed.
						if (!children.back()->display)
							splitter->display = false;

						children.emplace_back(std::unique_ptr<division>{ splitter });
					}
					break;
				case token::div_start:
				{
					auto div = scan_div(tknizer, false, ignore_duplicate);
					if (!children.empty())
					{
						//Hides the splitter if its right leaf is undisplayed.
						if ((children.back()->kind_of_division == division::kind::splitter) && !div->display)
							children.back()->display = false;

						children.back()->div_next = div.get();
					}

					children.emplace_back(std::move(div));
				}
				break;
				case token::switchable:
					div_type = token::switchable;
					break;
				case token::vert:
					div_type = tk;
					break;
				case token::grid:
					div_type = tk;
					switch (tknizer.read())
					{
					case token::number:
						array.push_back(tknizer.number());
						array.push_back(tknizer.number());
						break;
					case token::array:
						tknizer.array().swap(array);
						break;
					case token::reparray:
						array.push_back(tknizer.reparray().at(0));
						array.push_back(tknizer.reparray().at(1));
						break;
					default:
						break;
					}
					break;
				case token::collapse:
				{
					if (tknizer.parameters().size() != 4)
						throw std::invalid_argument("collapse requires 4 parameters");

					::nana::rectangle col{
						get_parameter(tknizer, 0),
						get_parameter(tknizer, 1),
						static_cast<decltype(col.width)>(get_parameter(tknizer, 2)),
						static_cast<decltype(col.width)>(get_parameter(tknizer, 3))
					};

					//Check the collapse area.
					//Ignore this collapse if its area is less than 2(col.width * col.height < 2)
					if (!col.empty() && (col.width > 1 || col.height > 1) && (col.x >= 0 && col.y >= 0))
					{
						//Overwrite if a exist_col in collapses has same position as the col.
						bool use_col = true;
						for (auto& exist_col : collapses)
						{
							if (exist_col.x == col.x && exist_col.y == col.y)
							{
								exist_col = col;
								use_col = false;
								break;
							}
						}
						if (use_col)
							collapses.emplace_back(col);
					}
				}
				break;
				case token::weight: case token::min_px: case token::max_px: case token::width: case token::height:
				{
					auto n = tknizer.number();
					//If n is the type of real, convert it to integer.
					//the integer and percent are allowed for weight/min/max.
					if (n.kind_of() == number_t::kind::real)
						n.assign(static_cast<int>(n.real()));

					switch (tk)
					{
					case token::weight: weight = n; weight_type = token::weight; break;  // we could detect errors here (redefinitions and duplicates)
					case token::width: weight = n; weight_type = token::width; break;
					case token::height: weight = n; weight_type = token::height; break;
					case token::min_px: min_px = n; break;
					case token::max_px: max_px = n; break;
					default: break;	//Useless
					}
				}
				break;
				case token::arrange:
					arrange = tknizer.reparray();
					break;
				case token::gap:
					gap = tknizer.reparray();
					break;
				case token::margin:
					margin.clear();
					switch (tknizer.read())
					{
					case token::number:
						margin.push(tknizer.number(), true);
						break;
					case token::array:
						margin.set_array(tknizer.array());
						break;
					case token::reparray:
						for (std::size_t i = 0; i < 4; ++i)
						{
							auto n = tknizer.reparray().at(i);
							if (n.empty()) n.assign(0);
							margin.push(n);
						}
						break;
					default:
						break;
					}
					break;
				case token::identifier:
					name = tknizer.idstr();
					break;
				case token::left:
					div_dir = ::nana::direction::west; break;
				case token::right:
					div_dir = ::nana::direction::east; break;
				case token::top:
					div_dir = ::nana::direction::north; break;
				case token::bottom:
					div_dir = ::nana::direction::south; break;
				case token::undisplayed:
					undisplayed = true; break;
				case token::invisible:
					invisible = true; break;
				default:	break;
				}
			}

			if (implicitly_started && (tk == token::div_end))
				throw std::invalid_argument("the div-text ends prematurely");

			field_gather * attached_field = nullptr;

			//find the field with specified name.
			//the field may not be created.
			auto i = fields.find(name);
			if (fields.end() != i)
			{
				attached_field = i->second;
				//the field is attached to a division, it means there is another division with same name.
				if (attached_field->attached)
				{
					//The fields are allowed to have a same name. E.g.
					//place.div("A <B><C>");
					//place.modify("A", "<B>");  Here the same name B must be allowed, otherwise it throws std::invalid_argument.

					bool allow_same_name = false;
					if (!ignore_duplicate.empty())
					{
						auto f = attached_field->attached->div_owner;
						while (f)
						{
							if (f->name == ignore_duplicate)
							{
								allow_same_name = true;
								break;
							}

							f = f->div_owner;
						}
					}

					if (!allow_same_name)
						throw std::invalid_argument("redefined field name");
				}
			}

			token unmatch = token::width;
			switch (div_type)
			{
			case token::eof:	// "horizontal" div
			case token::vert:   // "vertical" div
				if (token::eof == div_type)
					unmatch = token::height;

				for (auto& ch : children)
					if (ch->weigth_type == unmatch)
						throw std::invalid_argument("unmatch vertical-height/horizontal-width between division '"
							+ name + "' and children division '" + ch->name);

				div.reset(new div_arrange(token::vert == div_type, std::move(name), std::move(arrange)));
				break;
			case token::grid:
			{
				std::unique_ptr<div_grid> p(new div_grid(std::move(name), std::move(arrange), std::move(collapses)));

				if (array.size())
					p->dimension.first = array[0].integer();

				if (array.size() > 1)
					p->dimension.second = array[1].integer();

				if (0 == p->dimension.first)
					p->dimension.first = 1;

				if (0 == p->dimension.second)
					p->dimension.second = 1;

				p->revise_collapses();
				div = std::move(p);
			}
			break;
			case token::dock:
				div.reset(new div_dock(std::move(name), this));
				break;
			case token::switchable:
				div.reset(new div_switchable(std::move(name), this));
				break;
			default:
				throw std::invalid_argument("invalid division type.");
			}
			div->weigth_type = weight_type;

			//Requirements for min/max
			//1, min and max != negative
			//2, max >= min
			if (min_px.is_negative()) min_px.reset();
			if (max_px.is_negative()) max_px.reset();
			if ((!min_px.empty()) && (!max_px.empty()) && (min_px.get_value(100) > max_px.get_value(100)))
			{
				min_px.reset();
				max_px.reset();
			}

			if (!min_px.empty())
				div->min_px = min_px;

			if (!max_px.empty())
				div->max_px = max_px;

			if ((!min_px.empty()) && (!weight.empty()) && (weight.get_value(100) < min_px.get_value(100)))
				weight.reset();

			if ((!max_px.empty()) && (!weight.empty()) && (weight.get_value(100) > max_px.get_value(100)))
				weight.reset();

			if (!weight.empty())
				div->weight = weight;

			div->gap = std::move(gap);

			//attach the field to the division
			div->field = attached_field;
			if (attached_field)
			{
				//Replaces the previous div with the new div which is allowed to have a same name.

				//Detaches the field from the previous div. 
				if (attached_field->attached)
					attached_field->attached->field = nullptr;

				//Attaches new div
				attached_field->attached = div.get();
			}

			if (children.size())
			{
				if (division::kind::splitter == children.back()->kind_of_division)
				{
					children.pop_back();
					if (children.size())
						children.back()->div_next = nullptr;
				}

				for (auto& child : children)
				{
					child->div_owner = div.get();
					if (division::kind::splitter == child->kind_of_division)
						dynamic_cast<div_splitter&>(*child).direction(div_type != token::vert);
				}

				if (token::dock == div_type)
				{
					//adjust these children for dock division
					std::vector<std::unique_ptr<division>> adjusted_children;
					for (auto& child : children)
					{
						auto dockpn = new div_dockpane(std::move(child->name), this, child->dir);
						dockpn->div_owner = child->div_owner;
						dockpn->weight = child->weight;
						adjusted_children.emplace_back(std::unique_ptr<division>{ dockpn });
					}

					division* next = nullptr;
					for (int i = static_cast<int>(adjusted_children.size()) - 1; i >= 0; --i)
					{
						adjusted_children[i]->div_next = next;
						next = adjusted_children[i].get();
					}

					children.swap(adjusted_children);
				}
			}

			div->children.swap(children);
			div->margin = std::move(margin);
			div->dir = div_dir;

			div->display = !undisplayed;
			div->visible = !(undisplayed || invisible);
			div->fit = fit;
			div->fit_parameters = std::move(fit_parameters);
		}
		catch (place::error& ) { throw; }
		catch (       error& ) { throw; }
		catch (place_parts::tokenizer::error& e)
		{
			throw error(e.what(), name, e.pos);
		}
		catch (std::invalid_argument& e)
		{
			throw error(e.what(), name, tknizer.pos());
		}
		catch (std::exception& e)
		{
			throw error(std::string{"unexpected error: "}+e.what(), name, tknizer.pos());
		}
		catch (...)
		{
			throw error("unknow error", name, tknizer.pos());
		}
		return div;
	}

	void place::implement::check_unique(const division* div) const
	{
		//The second field_impl is useless. Reuse the map type in order to
		//reduce the size of the generated code, because std::set<std::string>
		//will create a new template class.
		std::map<std::string, field_gather*> unique;
		field_gather tmp(nullptr);

		std::function<void(const implement::division* div)> check_fn;
		check_fn = [&check_fn, &unique, &tmp](const implement::division* div)
		{
			if (!div->name.empty())
			{
				auto & ptr = unique[div->name];
				if (ptr)
					throw std::invalid_argument("place, the name '" + div->name + "' is redefined.");
				ptr = &tmp;
			}

			for (auto & child : div->children)
			{
				check_fn(child.get());
			}
		};

		if (div)
			check_fn(div);
	}

	//connect the field/dock with div object,
	void place::implement::connect(division* start)
	{
		if (!start)
			return;

		check_unique(start);	//may throw if there is a redefined name of field.

		this->disconnect();

		std::map<std::string, field_dock*> docks_to_be_closed;
		//disconnect
		for (auto & dk : docks)
		{
			if (dk.second->attached)
				docks_to_be_closed[dk.first] = dk.second;
		}

		std::function<void(division* div)> connect_fn;
		connect_fn = [&connect_fn, this, &docks_to_be_closed](division* div)
		{
			if (div->name.size())
			{
				if (division::kind::dock == div->kind_of_division || division::kind::dockpane == div->kind_of_division)
				{
					auto i = docks.find(div->name);
					if (i != docks.end())
					{
						docks_to_be_closed.erase(div->name);

						auto const pane = dynamic_cast<div_dockpane*>(div);
						auto fd_dock = pane->dockable_field;
						fd_dock = i->second;

						if (fd_dock->attached)
						{
							fd_dock->attached->dockable_field = nullptr;
							div->display = fd_dock->attached->display;
						}

						fd_dock->attached = pane;
						if (fd_dock->dockarea)
							fd_dock->dockarea->set_notifier(pane);
					}
				}
				else
				{
					auto i = fields.find(div->name);
					if (i != fields.end())
					{
						div->field = i->second;
						div->field->attached = div;
					}
				}

			}

			for (auto & child : div->children)
			{
				connect_fn(child.get());
			}
		};

		connect_fn(start);

		for (auto& e : docks_to_be_closed)
		{
			e.second->dockarea.reset();
			e.second->attached->dockable_field = nullptr;
			e.second->attached = nullptr;
		}
	}

	void place::implement::disconnect() noexcept
	{
		for (auto & fd : fields)
		{
			if (fd.second->attached)
			{
				fd.second->attached->field = nullptr;
				fd.second->attached = nullptr;
			}
		}
	}

	//class place
	place::place()
		: impl_(new implement)
	{}

	place::place(window wd)
		: impl_(new implement)
	{
		bind(wd);
	}

	place::~place()
	{
		delete impl_;
	}

	void place::bind(window wd)
	{
		if (impl_->window_handle)
			throw error(" bind('"+ API::window_caption(wd).substr(0, 80)
			                + "'): it was already bound to another window.", *this);

		impl_->window_handle = wd;
		impl_->event_size_handle = API::events(wd).resized.connect_unignorable([this](const arg_resized& arg)
		{
			if (impl_->root_division)
			{
				impl_->root_division->field_area.dimension({ arg.width, arg.height });
				impl_->root_division->calc_weight_floor();
				impl_->root_division->collocate(arg.window_handle);
			}
		});
	}

	window place::window_handle() const
	{
		return impl_->window_handle;
	}

	void place::splitter_renderer(std::function<void(window, paint::graphics&, mouse_action)> fn)
	{
		impl_->split_renderer.swap(fn);

		for (auto sp : impl_->splitters)
			sp->set_renderer(impl_->split_renderer, true);
	}

	void place::div(std::string div_text)
	{
		try
		{
			place_parts::tokenizer tknizer(div_text.c_str());
			impl_->disconnect();
			auto div = impl_->scan_div(tknizer, true);
			impl_->connect(div.get());		//throws if there is a redefined name of field.
			impl_->root_division.reset();	//clear attachments div-fields
			impl_->root_division.swap(div);
			impl_->div_text.swap(div_text);
		}
		catch (place::error & ) { throw; }
		catch (place::implement::error & e)
		{
			throw error("failed to set div('" + div_text + "'): " + e.what(), *this, e.field, e.pos);
		}
		catch (place_parts::tokenizer::error & e)
		{
			throw error("failed to set div('" + div_text + "'): " + e.what(), *this, "", e.pos);
		}
		catch (std::invalid_argument & e)
		{
			throw error("failed to set div('" + div_text + "'): " + e.what(), *this);
		}
		catch (std::exception & e)
		{
			throw error("failed to set div('"+div_text+"'): unexpected error: " +e.what(), *this );
		}
		catch (...)
		{
			throw error("failed to set div('" + div_text + "'): unknonw error", *this);
		}
	}

	const std::string& place::div() const noexcept
	{
		return impl_->div_text;
	}

	//Contributed by dankan1890(PR#156)
	enum class update_operation { erase = 0, insert, replace };

	static void update_div(std::string& div, const char* field, const char* attr, update_operation operation);

	void place::modify(const char* name, const char* div_text)
	{
		if (nullptr == div_text)
			throw error("modify(): invalid div-text (=nullptr)", *this);

		if (! valid_field_name(name) )
			throw badname("modify()", *this, name);

		auto div_ptr = impl_->search_div_name(impl_->root_division.get(), name);
		if (!div_ptr)
		   throw error("modify(): field was not found", *this, name);
		

		std::unique_ptr<implement::division>* replaced = nullptr;

		implement::division * div_owner = div_ptr->div_owner;
		implement::division * div_next = div_ptr->div_next;
		if (div_owner)
		{
			for (auto& child: div_owner->children)
			{
				if (child.get() == div_ptr)
				{
					replaced = &child;
					break;
				}
			}
		}
		else
			replaced = &(impl_->root_division);

		replaced->swap(impl_->tmp_replaced);

		try
		{
			place_parts::tokenizer tknizer(div_text);
			auto modified = impl_->scan_div(tknizer, true, name);
			auto modified_ptr = modified.get();
			modified_ptr->name = name;

			replaced->swap(modified);

			impl_->connect(impl_->root_division.get());	//throws if there is a duplicate name
			impl_->tmp_replaced.reset();
			update_div(impl_->div_text, name, div_text, update_operation::replace);

			modified_ptr->div_owner = div_owner;
			modified_ptr->div_next = div_next;

			if (div_owner)
			{
				implement::division * pv_div = nullptr;
				//Updates the div_next of the div at front of modified one.
				for (auto & div : div_owner->children)
				{
					if (div.get() == modified_ptr)
					{
						if (pv_div)
							pv_div->div_next = modified_ptr;
						break;
					}
					pv_div = div.get();
				}
			}
		}
		catch (place::error & ) 
		{
			replaced->swap(impl_->tmp_replaced);
			throw; 
		}
		catch (place::implement::error & e)
		{
			replaced->swap(impl_->tmp_replaced);
			throw error("failed to modify('"+std::string(name) +", "+ div_text + "'): " + e.what(), *this, e.field, e.pos);
		}
		catch (place_parts::tokenizer::error & e)
		{
			replaced->swap(impl_->tmp_replaced);
			throw error("failed to modify('" + std::string(name) + ", " + div_text + "'): " + e.what(), *this, "", e.pos);
		}
		catch (std::invalid_argument & e)
		{
			replaced->swap(impl_->tmp_replaced);
			throw error("failed to modify('" + std::string(name) + ", " + div_text + "'): " + e.what(), *this);
		}
		catch (std::exception & e)
		{
			replaced->swap(impl_->tmp_replaced);
			throw error("failed to modify('" + std::string(name) + ", " + div_text + "'): unexpected error: " + e.what(), *this);
		}
		catch (...)
		{
			replaced->swap(impl_->tmp_replaced);
			throw error("failed to modify('" + std::string(name) + ", " + div_text + "'): unknonw error", *this);
		}
	}

	place::field_reference place::field(const char* name)
	{
		if (!valid_field_name(name))
			throw badname("field()", *this, name);

		//get the field with the specified name. If no such field with specified name
		//then create one.
		auto & p = impl_->fields[name];
		if (nullptr == p)
			p = new implement::field_gather(this);

		if ((!p->attached) && impl_->root_division)
		{
			//search the division with the specified name,
			//and attaches the division to the field
			auto div = implement::search_div_name(impl_->root_division.get(), name);
			if (div)
			{
				if (div->field && (div->field != p))
					throw error("field(): the division attaches an unexpected field", *this, name);

				div->field = p;
				p->attached = div;
			}
		}
		return *p;
	}

	static void update_div(std::string& div, const char* field, const char* attr, update_operation operation)
	{
		const auto fieldname_pos = find_idstr(div, field);
		if (div.npos == fieldname_pos)
			return;

		auto const bound = get_field_boundary(div, field, 0);

		auto fieldstr = div.substr(bound.first, bound.second - bound.first);
		//Search the attribute
		std::size_t pos = 0;
		int depth = 0;
		for (; true; ++pos)
		{
			pos = find_idstr(fieldstr, attr, pos);
			if (fieldstr.npos == pos)
				break;

			//Check if the attr is belong to this field.
			depth = 0;
			std::size_t off = pos;
			while (true)
			{
				off = fieldstr.find_last_of("<>", off);
				if (fieldstr.npos == off)
					break;

				if ('>' == fieldstr[off])
					++depth;
				else
					--depth;

				if (0 == off)
					break;
				--off;
			}

			if (0 == depth)
				break;
		}

		if (fieldstr.npos == pos)
		{
			//There is not an attribute
			if (operation == update_operation::insert)
				div.insert(fieldname_pos + std::strlen(field), " " + std::string(attr));
			else if (operation == update_operation::replace)
				div.replace(bound.first, bound.second - bound.first, std::string(attr) + " " + std::string(field));
		}
		else
		{
			//There is an attribute
			if (operation == update_operation::erase)
			{
				div.erase(bound.first + pos, std::strlen(attr));

				if (bound.first + pos > 0)
				{
					//erases a whitespace if there are 2 whitespaces after erasing the attr
					if ((div[bound.first + pos] == div[bound.first + pos - 1]) && (' ' == div[bound.first + pos]))
						div.erase(bound.first + pos, 1);
				}
			}
		}
	}

	void place::field_visible(const char* name, bool vsb)
	{
		if (!valid_field_name(name))
			throw badname("field_visible()", *this, name);

		auto div = impl_->search_div_name(impl_->root_division.get(), name);
		if (div)
		{
			div->set_visible(vsb);
			update_div(impl_->div_text, name, "invisible", !vsb ? update_operation::insert : update_operation::erase);
		}
	}

	bool place::field_visible(const char* name) const
	{
		if (!valid_field_name(name))
			throw badname("field_visible()", *this, name);

		auto div = impl_->search_div_name(impl_->root_division.get(), name);
		return (div && div->visible);
	}

	void place::field_display(const char* name, bool dsp)
	{
		if (!valid_field_name(name))
			throw badname("field_display()", *this, name);

		auto div = impl_->search_div_name(impl_->root_division.get(), name);
		if (div)
		{
			update_div(impl_->div_text, name, "invisible", update_operation::erase);
			update_div(impl_->div_text, name, "undisplayed", !dsp ? update_operation::insert : update_operation::erase);
			div->set_display(dsp);
		}
	}

	bool place::field_display(const char* name) const
	{
		if (!valid_field_name(name))
			throw badname("field_display()", *this, name);

		auto div = impl_->search_div_name(impl_->root_division.get(), name);
		return (div && div->display);
	}

	void place::collocate()
	{
		impl_->collocate();
	}

	void place::erase(window handle)
	{
		bool recollocate = false;
		for (auto & fld : impl_->fields)
		{
			auto evt = fld.second->erase_element(fld.second->elements, handle);
			if (evt)
			{
				API::umake_event(evt);
				recollocate |= (nullptr != fld.second->attached);
			}

			API::umake_event(fld.second->erase_element(fld.second->fastened, handle));
		}

		if (recollocate)
			collocate();
	}

	place::field_reference place::operator[](const char* name)
	{
		return field(name);
	}

	place& place::dock(const std::string& name, std::string factory_name, std::function<std::unique_ptr<widget>(window)> factory)
	{
		if (!valid_field_name(name.data()))
			throw badname("dock()", *this, name.data());

		auto & dock_ptr = impl_->docks[name];
		if (!dock_ptr)
			dock_ptr = new implement::field_dock;

		//Register the factory if it has a name
		if (!factory_name.empty())
		{
			if (impl_->dock_factoris.find(factory_name) != impl_->dock_factoris.end())
				throw error("dock() - the specified factory name(" + factory_name + ") already exists", *this, name);

			impl_->dock_factoris[factory_name] = dock_ptr;
			dock_ptr->factories[factory_name] = std::move(factory);
		}

		auto div = dynamic_cast<implement::div_dockpane*>(impl_->search_div_name(impl_->root_division.get(), name));
		if (div)
		{
			dock_ptr->attached = div;
			div->dockable_field = dock_ptr;
		}

		//Create the pane if it has not a name
		if (factory_name.empty() && dock_ptr->attached)
		{
			dock_ptr->attached->set_display(true);
			impl_->collocate();

			if (!dock_ptr->dockarea)
			{
				dock_ptr->dockarea.reset(new ::nana::place_parts::dockarea);
				dock_ptr->dockarea->create(impl_->window_handle);
				dock_ptr->dockarea->set_notifier(dock_ptr->attached);
				dock_ptr->dockarea->move(dock_ptr->attached->field_area);
			}
			dock_ptr->dockarea->add_pane(factory);
		}

		return *this;
	}

	widget* place::dock_create(const std::string& factory)
	{
		auto i = impl_->dock_factoris.find(factory);
		if (i == impl_->dock_factoris.end())
			throw std::invalid_argument("nana::place::dock_create - invalid factory name(" + factory + ") of dockpane");

		auto dock_ptr = i->second;
		if (dock_ptr->attached)
		{
			dock_ptr->attached->set_display(true);
			impl_->collocate();

			if (!dock_ptr->dockarea)
			{
				dock_ptr->dockarea.reset(new ::nana::place_parts::dockarea);
				dock_ptr->dockarea->create(impl_->window_handle);
				dock_ptr->dockarea->set_notifier(dock_ptr->attached);
				dock_ptr->dockarea->move(dock_ptr->attached->field_area);
			}

			return dock_ptr->dockarea->add_pane(dock_ptr->factories[factory]);
		}

		return nullptr;
	}
	
	place::error::error(const std::string& what,
						const place& plc,
						std::string field,
						std::string::size_type pos)

		: std::invalid_argument(  "from widget '" 
		                        + API::window_caption(plc.window_handle()).substr(0,80)
								+ "'; nana::place error "
		                        + what 
		                        + "' in field '" + field
								+ (pos == std::string::npos ? "' " : "' at position " + std::to_string(pos))
								+ " in div_text:\n" + plc.div() ),
		base_what( what ),
		owner_caption( API::window_caption(plc.window_handle()).substr(0,80) ),
		div_text( plc.div() ),
		field( field ),
		pos( pos )
	{}
	//end class place

}//end namespace nana

#include <nana/pop_ignore_diagnostic>
