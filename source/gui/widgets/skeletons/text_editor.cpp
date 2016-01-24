/*
 *	A text editor implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/skeletons/text_editor.cpp
 *	@contributors: Ariel Vina-Rodriguez
 */
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <nana/gui/widgets/skeletons/textbase_export_interface.hpp>
#include <nana/gui/element.hpp>
#include <nana/system/dataexch.hpp>
#include <nana/unicode_bidi.hpp>
#include <numeric>
#include <cwctype>
#include <cstring>
#include <set>
#include <algorithm>

namespace nana{	namespace widgets
{
	namespace skeletons
	{
		template<typename T>
		using undo_command_ptr = std::unique_ptr <undoable_command_interface<T>> ;

		template<typename EnumCommand>
		class text_editor::basic_undoable
			: public undoable_command_interface<EnumCommand>
		{
		public:
			basic_undoable(text_editor& te, EnumCommand cmd)
				: editor_(te), cmd_(cmd)
			{}

			void set_selected_text()
			{
				if (editor_._m_get_sort_select_points(sel_a_, sel_b_))
					editor_._m_make_select_string(selected_text_);
			}

			void set_caret_pos()
			{
				pos_ = editor_.caret();
			}
		protected:
			EnumCommand get() const override
			{
				return cmd_;
			}

			virtual bool merge(const undoable_command_interface<EnumCommand>& rhs) override
			{
				return false;
			}
		protected:
			text_editor & editor_;
			upoint			pos_;
			upoint			sel_a_, sel_b_;
			std::wstring	selected_text_;
		private:
			const EnumCommand cmd_;
		};

		class text_editor::undo_backspace
			: public basic_undoable < command >
		{
		public:
			undo_backspace(text_editor& editor)
				:	basic_undoable<command>(editor, command::backspace)
			{
			}

			void set_removed(std::wstring str)
			{
				//use selected_text_ as removed text
				selected_text_ = str;
			}

			void execute(bool redo) override
			{
				editor_._m_cancel_select(0);
				editor_.points_.caret = pos_;

				bool is_enter = ((selected_text_.size() == 1) && ('\n' == selected_text_[0]));
				if (redo)
				{
					if (sel_a_ != sel_b_)
					{
						editor_.select_.a = sel_a_;
						editor_.select_.b = sel_b_;
						editor_._m_erase_select();
						editor_.select_.a = editor_.select_.b;
						editor_.points_.caret = sel_a_;
					}
					else
					{
						if (is_enter)
						{
							editor_.points_.caret = nana::upoint(0, pos_.y + 1);
							editor_.backspace(false);
						}
						else
							editor_.textbase_.erase(pos_.y, pos_.x, selected_text_.size());
					}
				}
				else
				{
					if (is_enter)
					{
						editor_.enter(false);
					}
					else
					{
						editor_._m_put(selected_text_);
						if (sel_a_ != sel_b_)
						{
							editor_.select_.a = sel_a_;
							editor_.select_.b = sel_b_;
							editor_.points_.caret = sel_b_;
						}
						else
							++editor_.points_.caret.x;
					}
				}

				editor_.move_caret(editor_.points_.caret);
			}
		};

		class text_editor::undo_input_text
			: public basic_undoable <command>
		{
		public:
			undo_input_text(text_editor & editor, const std::wstring& text)
				:	basic_undoable<command>(editor, command::input_text),
					text_(text)
			{
			}

			void execute(bool redo) override
			{
				bool is_enter = (text_.size() == 1 && '\n' == text_[0]);
				editor_._m_cancel_select(0);
				editor_.points_.caret = pos_;	//The pos_ specifies the caret position before input

				if (redo)
				{
					if (is_enter)
					{
						editor_.enter(false);
					}
					else
					{
						if (!selected_text_.empty())
						{
							editor_.select_.a = sel_a_;
							editor_.select_.b = sel_b_;
							editor_._m_erase_select();
						}
						editor_.points_.caret = editor_._m_put(text_); //redo
					}
				}
				else
				{
					if (is_enter)
					{
						editor_.points_.caret.x = 0;
						++editor_.points_.caret.y;
						editor_.backspace(false);
					}
					else
					{
						std::vector<std::pair<std::size_t, std::size_t>> lines;
						if (editor_._m_resolve_text(text_, lines))
						{
							editor_.select_.a = pos_;
							editor_.select_.b = upoint(static_cast<unsigned>(lines.back().second - lines.back().first), static_cast<unsigned>(pos_.y + lines.size() - 1));
							editor_.backspace(false);
							editor_.select_.a = editor_.select_.b;
						}
						else
							editor_.textbase_.erase(pos_.y, pos_.x, text_.size());	//undo

						if (!selected_text_.empty())
						{
							editor_.points_.caret = sel_a_;
							editor_._m_put(selected_text_);
							editor_.points_.caret = sel_b_;
							editor_.select_.a = sel_a_;	//Reset the selected text
							editor_.select_.b = sel_b_;
						}
					}
				}
				editor_.move_caret(editor_.points_.caret);
			}
		private:
			std::wstring text_;
		};

		class text_editor::undo_move_text
			: public basic_undoable <command>
		{
		public:
			undo_move_text(text_editor& editor)
				:	basic_undoable<command>(editor, command::move_text)
			{}

			void execute(bool redo) override
			{
				if (redo)
				{
					editor_.select_.a = sel_a_;
					editor_.select_.b = sel_b_;
					editor_.points_.caret = pos_;
				}
				else
				{
					editor_.select_.a = dest_a_;
					editor_.select_.b = dest_b_;
					editor_.points_.caret = sel_a_;
				}
				editor_._m_move_select(false);
			}

			void set_destination(const nana::upoint& dest_a, const nana::upoint& dest_b)
			{
				dest_a_ = dest_a;
				dest_b_ = dest_b;
			}
		private:
			nana::upoint dest_a_, dest_b_;
		};

		class text_editor::editor_behavior_interface
		{
		public:
			virtual ~editor_behavior_interface(){}

			/// Deletes lines between first and second, and then, second line will be merged into first line.
			virtual void merge_lines(std::size_t first, std::size_t second) = 0;
			//Calculates how many lines the specified line of text takes with a specified pixels of width.
			virtual void add_lines(std::size_t pos, std::size_t lines) = 0;
			virtual void pre_calc_line(std::size_t line, unsigned pixels) = 0;
			virtual void pre_calc_lines(unsigned pixels) = 0;
			virtual std::size_t take_lines() const = 0;
			/// Returns the number of lines that the line of text specified by pos takes.
			virtual std::size_t take_lines(std::size_t pos) const = 0;

			virtual void update_line(std::size_t textline, std::size_t secondary_before) = 0;
			virtual std::vector<::nana::upoint> render(const ::nana::color& fgcolor) = 0;
			virtual	nana::point		caret_to_screen(upoint) = 0;
			virtual nana::upoint	screen_to_caret(point scrpos) = 0;
			virtual bool move_caret_ns(bool to_north) = 0;
			virtual bool adjust_caret_into_screen() = 0;
		};

		inline bool is_right_text(const unicode_bidi::entity& e)
		{
			return ((e.bidi_char_type != unicode_bidi::bidi_char::L) && (e.level & 1));
		}


		class text_editor::behavior_normal
			: public editor_behavior_interface
		{
		public:
			behavior_normal(text_editor& editor)
				: editor_(editor)
			{}

			void merge_lines(std::size_t first, std::size_t second) override{}
			void add_lines(std::size_t pos, std::size_t lines) override{}
			void pre_calc_line(std::size_t, unsigned) override{}
			void pre_calc_lines(unsigned) override{}

			std::size_t take_lines() const override
			{
				return editor_.textbase_.lines();
			}

			std::size_t take_lines(std::size_t pos) const override
			{
				return 1;
			}

			void update_line(std::size_t textline, std::size_t secondary_before) override
			{
				int top = editor_._m_text_top_base() + static_cast<int>(editor_.line_height() * (textline - editor_.points_.offset.y));
				editor_.graph_.rectangle({ editor_.text_area_.area.x, top, editor_.text_area_.area.width, editor_.line_height() }, true, API::bgcolor(editor_.window_));
				editor_._m_draw_string(top, API::fgcolor(editor_.window_), nana::upoint(0, editor_.points_.caret.y), editor_.textbase_.getline(textline), true);
			}

			std::vector<upoint> render(const ::nana::color& fgcolor) override
			{
				std::vector<upoint> line_index;

				::nana::upoint str_pos(0, static_cast<unsigned>(editor_.points_.offset.y));

				std::size_t scrlines = editor_.screen_lines() + str_pos.y;
				if (scrlines > editor_.textbase_.lines())
					scrlines = editor_.textbase_.lines();

				int top = editor_._m_text_top_base();
				const unsigned pixels = editor_.line_height();

				while( str_pos.y < scrlines)
				{
					editor_._m_draw_string(top, fgcolor, str_pos, editor_.textbase_.getline(str_pos.y), true);
					line_index.push_back(str_pos);
					++str_pos.y;
					top += pixels;
				}

				return line_index;
			}

			nana::point	caret_to_screen(nana::upoint pos) override
			{
				auto & textbase = editor_.textbase_;
				if (pos.y > static_cast<unsigned>(textbase.lines()))
					pos.y = static_cast<unsigned>(textbase.lines());

				std::unique_ptr<std::wstring> mask_str;
				if (editor_.mask_char_)
					mask_str.reset(new std::wstring(textbase.getline(pos.y).size(), editor_.mask_char_));
				
				auto & lnstr = editor_.mask_char_ ? *mask_str : textbase.getline(pos.y);

				pos.x = editor_._m_pixels_by_char(lnstr, pos.x) + editor_.text_area_.area.x;

				int pos_y = static_cast<int>((pos.y - editor_.points_.offset.y) * editor_.line_height() + editor_._m_text_top_base());

				return{ static_cast<int>(pos.x - editor_.points_.offset.x), pos_y };
			}

			nana::upoint screen_to_caret(point scrpos) override
			{
				nana::upoint res{ 0, static_cast<unsigned>(_m_textline_from_screen(scrpos.y)) };

				//Convert the screen point to text caret point
				const string_type& real_str = editor_.textbase_.getline(res.y);

				std::unique_ptr<std::wstring> mask_str;
				if (editor_.mask_char_)
					mask_str.reset(new std::wstring(real_str.size(), editor_.mask_char_));
				
				auto & lnstr = (editor_.mask_char_ ? *mask_str : real_str);
				if (lnstr.size() > 0)
				{
					scrpos.x += (editor_.points_.offset.x - editor_.text_area_.area.x);
					if (scrpos.x > 0)
					{
						unicode_bidi bidi;
						std::vector<unicode_bidi::entity> reordered;
						bidi.linestr(lnstr.data(), lnstr.size(), reordered);

						for (auto & ent : reordered)
						{
							std::size_t len = ent.end - ent.begin;
							auto str_px = static_cast<int>(editor_._m_text_extent_size(ent.begin, len).width);
							if (scrpos.x < str_px)
							{
								std::unique_ptr<unsigned[]> pxbuf(new unsigned[len]);

								res.x = editor_._m_char_by_pixels(ent.begin, len, pxbuf.get(), str_px, scrpos.x, is_right_text(ent));
								res.x += static_cast<unsigned>(ent.begin - lnstr.data());
								return res;
							}
							scrpos.x -= str_px;
						}

						res.x = static_cast<int>(lnstr.size());
					}
				}

				return res;
			}

			bool move_caret_ns(bool to_north) override
			{
				auto & points = editor_.points_;

				if (to_north)	//North
				{
					if (points.caret.y)
					{
						points.caret.x = static_cast<unsigned>(editor_.textbase_.getline(--points.caret.y).size());

						if (points.xpos < points.caret.x)
							points.caret.x = points.xpos;

						bool out_of_screen = (static_cast<int>(points.caret.y) < points.offset.y);
						if (out_of_screen)
							editor_._m_offset_y(static_cast<int>(points.caret.y));

						return (adjust_caret_into_screen() || out_of_screen);
					}
				}
				else //South
				{
					if (points.caret.y + 1 < editor_.textbase_.lines())
					{
						points.caret.x = static_cast<unsigned>(editor_.textbase_.getline(++points.caret.y).size());

						if (points.xpos < points.caret.x)
							points.caret.x = points.xpos;

						return adjust_caret_into_screen();
					}
				}
				return false;
			}

			//adjust_caret_into_screen
			//@brief:	Adjust the text offset in order to moving caret into visible area if it is out of the visible area
			//@note:	the function assumes the points_.caret is correct
			bool adjust_caret_into_screen() override
			{
				const auto scrlines = editor_.screen_lines();
				if (0 == scrlines)
					return false;

				auto& points = editor_.points_;
				auto& textbase = editor_.textbase_;

				editor_._m_get_scrollbar_size();

				auto	x = points.caret.x;
				auto&	lnstr = textbase.getline(points.caret.y);

				if (x > lnstr.size()) x = static_cast<unsigned>(lnstr.size());

				auto const text_w = editor_._m_pixels_by_char(lnstr, x);

				unsigned area_w = editor_._m_text_area().width;

				bool adjusted_cond = true;
				if (static_cast<int>(text_w) < points.offset.x)
				{
					auto delta_pixels = editor_._m_text_extent_size(L"    ", 4).width;
					points.offset.x = (text_w > delta_pixels ? text_w - delta_pixels : 0);
				}
				else if (area_w && (text_w >= points.offset.x + area_w))
					points.offset.x = text_w - area_w + 2;
				else
					adjusted_cond = false;

				bool adjusted_cond2 = true;
				int value = points.offset.y;
				if (scrlines && (points.caret.y >= points.offset.y + scrlines))
				{
					value = static_cast<int>(points.caret.y - scrlines) + 1;
				}
				else if (static_cast<int>(points.caret.y) < points.offset.y)
				{
					if (scrlines >= static_cast<unsigned>(points.offset.y))
						value = 0;
					else
						value = static_cast<int>(points.offset.y - scrlines);
				}
				else if (points.offset.y && (textbase.lines() <= scrlines))
					value = 0;
				else
					adjusted_cond2 = false;

				editor_._m_offset_y(value);
				editor_._m_scrollbar();
				return (adjusted_cond || adjusted_cond2);
			}

		private:
			std::size_t	_m_textline_from_screen(int y) const
			{
				const std::size_t textlines = editor_.textbase_.lines();
				if (0 == textlines)
					return 0;

				const int offset_top = editor_.points_.offset.y;
				const int text_area_top = editor_.text_area_.area.y;

				if (y < text_area_top)
					y = offset_top ? offset_top - 1 : 0;
				else
					y = (y - text_area_top) / static_cast<int>(editor_.line_height()) + offset_top;

				return (textlines <= static_cast<std::size_t>(y) ? textlines - 1 : static_cast<std::size_t>(y));
			}
		private:
			text_editor& editor_;
		}; //end class behavior_normal


		class text_editor::behavior_linewrapped
			: public text_editor::editor_behavior_interface
		{
			struct text_section
			{
				const wchar_t* begin;
				const wchar_t* end;
				unsigned pixels;

				text_section()
				{
					throw std::runtime_error("text_section default construction is forbidden.");
				}

				text_section(const wchar_t* ptr, const wchar_t* endptr)
					: begin(ptr), end(endptr)
				{}
			};

			struct line_metrics
			{
				std::size_t		take_lines;	//The number of lines that text of this line takes.
				std::vector<text_section>	line_sections;
			};
		public:
			behavior_linewrapped(text_editor& editor)
				: editor_(editor)
			{}

			void merge_lines(std::size_t first, std::size_t second) override
			{
				if (first > second)
					std::swap(first, second);

				if (second < linemtr_.size())
					linemtr_.erase(linemtr_.begin() + first + 1, linemtr_.begin() + second);

				pre_calc_line(first, editor_.width_pixels());

				//textbase is implement by using deque, and the linemtr holds the text pointers
				//If the textbase is changed, it will check the text pointers.
				std::size_t line = 0;
				for (auto & mtr: linemtr_)
				{
					auto& linestr = editor_.textbase_.getline(line);
					auto p = mtr.line_sections.front().begin;
					if (p < linestr.data() || (linestr.data() + linestr.size() < p))
						pre_calc_line(line, editor_.width_pixels());

					++line;
				}
			}

			void add_lines(std::size_t pos, std::size_t lines) override
			{
				if (pos < linemtr_.size())
				{
					for (std::size_t i = 0; i < lines; ++i)
						linemtr_.emplace(linemtr_.begin() + pos + i);

					//textbase is implement by using deque, and the linemtr holds the text pointers
					//If the textbase is changed, it will check the text pointers.
					std::size_t line = 0;
					for (auto & mtr : linemtr_)
					{
						if (line < pos || (pos + lines) <= line)
						{
							auto & linestr = editor_.textbase_.getline(line);
							auto p = mtr.line_sections.front().begin;
							if (p < linestr.data() || (linestr.data() + linestr.size() < p))
								pre_calc_line(line, editor_.width_pixels());
						}
						++line;
					}
				}
			}

			void pre_calc_line(std::size_t line, unsigned pixels) override
			{
				const string_type& lnstr = editor_.textbase_.getline(line);
				if (lnstr.empty())
				{
					auto & mtr = linemtr_[line];
					mtr.line_sections.clear();
					mtr.line_sections.emplace_back(lnstr.data(), lnstr.data());
					mtr.line_sections.back().pixels = 0;
					mtr.take_lines = 1;
					return;
				}

				std::vector<text_section> sections;
				_m_text_section(lnstr, sections);

				std::vector<text_section> line_sections;

				unsigned text_px = 0;
				const wchar_t * secondary_begin = nullptr;
				for (auto & ts : sections)
				{
					if (!secondary_begin)
						secondary_begin = ts.begin;

					const unsigned str_w = editor_._m_text_extent_size(ts.begin, ts.end - ts.begin).width;

					text_px += str_w;
					if (text_px > pixels)
					{
						if (text_px != str_w)
						{
							line_sections.emplace_back(secondary_begin, ts.begin);
							line_sections.back().pixels = text_px - str_w;
							text_px = str_w;
							secondary_begin = ts.begin;
						}

						if (str_w > pixels)	//Indicates the splitting of ts string
						{
							std::size_t len = ts.end - ts.begin;
							std::unique_ptr<unsigned[]> pxbuf(new unsigned[len]);
							editor_.graph_.glyph_pixels(ts.begin, len, pxbuf.get());

							auto pxptr = pxbuf.get();
							auto pxend = pxptr + len;

							secondary_begin = ts.begin;
							text_px = 0;
							for (auto pxi = pxptr; pxi != pxend; ++pxi)
							{
								text_px += *pxi;
								if (text_px < pixels)
									continue;

								const wchar_t * endptr = ts.begin + (pxi - pxptr) + (text_px == pixels ? 1 : 0);
								line_sections.emplace_back(secondary_begin, endptr);
								line_sections.back().pixels = text_px - (text_px == pixels ? 0 : *pxi);
								secondary_begin = endptr;

								text_px = (text_px == pixels ? 0 : *pxi);
							}
						}
						continue;
					}
					else if (text_px == pixels)
					{
						line_sections.emplace_back(secondary_begin, ts.begin);
						line_sections.back().pixels = text_px - str_w;
						secondary_begin = ts.begin;
						text_px = str_w;
					}
				}

				auto & mtr = linemtr_[line];

				mtr.take_lines = line_sections.size();
				mtr.line_sections.swap(line_sections);

				if (secondary_begin)
				{
					mtr.line_sections.emplace_back(secondary_begin, sections.back().end);
					mtr.line_sections.back().pixels = text_px;
					++mtr.take_lines;
				}
			}

			void pre_calc_lines(unsigned pixels) override
			{
				const auto lines = editor_.textbase_.lines();
				linemtr_.resize(lines);

				for (std::size_t i = 0; i < lines; ++i)
					pre_calc_line(i, pixels);
			}

			std::size_t take_lines() const override
			{
				std::size_t lines = 0;
				for (auto & mtr : linemtr_)
					lines += mtr.take_lines;

				return lines;
			}

			std::size_t take_lines(std::size_t pos) const override
			{
				return (pos < linemtr_.size() ? linemtr_[pos].take_lines : 0);
			}

			void update_line(std::size_t textline, std::size_t secondary_before) override
			{
				if (take_lines(textline) == secondary_before)
				{
					int top = caret_to_screen(upoint{ 0, static_cast<unsigned>(textline) }).y;

					const unsigned pixels = editor_.line_height();
					editor_.graph_.rectangle({ editor_.text_area_.area.x, top, editor_.width_pixels(), static_cast<unsigned>(pixels * secondary_before) }, true, API::bgcolor(editor_.window_));

					auto fgcolor = API::fgcolor(editor_.window_);
					auto text_ptr = editor_.textbase_.getline(textline).data();

					for (std::size_t pos = 0; pos < secondary_before; ++pos, top+=pixels)
					{
						auto & sct = linemtr_[textline].line_sections[pos];
						editor_._m_draw_string(top, fgcolor, nana::upoint(static_cast<unsigned>(sct.begin - text_ptr), editor_.points_.caret.y), std::wstring(sct.begin, sct.end), true);
					}
				}
				else
					editor_.render(API::is_focus_ready(editor_.window_));
			}

			std::vector<upoint> render(const ::nana::color& fgcolor) override
			{
				std::vector<upoint> line_index;

				std::size_t secondary;
				auto primary = _m_textline_from_screen(0, secondary);
				if (primary >= linemtr_.size() || secondary >= linemtr_[primary].line_sections.size())
					return line_index;

				nana::upoint str_pos(0, static_cast<unsigned>(primary));
				str_pos.x = static_cast<unsigned>(linemtr_[primary].line_sections[secondary].begin - editor_.textbase_.getline(primary).data());

				int top = editor_._m_text_top_base();
				const unsigned pixels = editor_.line_height();

				const std::size_t scrlines = editor_.screen_lines();
				for (std::size_t pos = 0; pos < scrlines; ++pos, top += pixels)
				{
					if ((primary < linemtr_.size()) && (secondary < linemtr_[primary].line_sections.size()))
					{
						auto & mtr = linemtr_[primary];
						auto & section = mtr.line_sections[secondary];

						std::wstring text(section.begin, section.end);
						editor_._m_draw_string(top, fgcolor, str_pos, text, true);
						line_index.push_back(str_pos);
						++secondary;
						if (secondary >= mtr.line_sections.size())
						{
							++primary;
							secondary = 0;
							str_pos.x = 0;
							++str_pos.y;
						}
						else
							str_pos.x += static_cast<unsigned>(text.size());
					}
					else
						break;
				}

				return line_index;
			}

			nana::point	caret_to_screen(upoint pos) override
			{
				const auto & mtr = linemtr_[pos.y];

				std::size_t lines = 0;	//lines before the caret line;
				for (auto & v : linemtr_)
				{
					if (pos.y)
					{
						lines += v.take_lines;
						--pos.y;
					}
					else
						break;
				}

				nana::point scrpos;
				if (0 != pos.x)
				{
					std::wstring str;
					for (auto & sec : mtr.line_sections)
					{
						std::size_t chsize = sec.end - sec.begin;
						str.clear();
						if (editor_.mask_char_)
							str.append(chsize, editor_.mask_char_);
						else
							str.append(sec.begin, sec.end);

						if (pos.x < chsize)
						{
							scrpos.x = editor_._m_pixels_by_char(str, pos.x);
							break;
						}
						else if (pos.x == chsize)
						{
							scrpos.x = editor_._m_text_extent_size(str.data(), sec.end - sec.begin).width;
							break;
						}
						else
						{
							pos.x -= static_cast<unsigned>(chsize);
							++lines;
						}
					}
				}

				scrpos.x += editor_.text_area_.area.x;
				scrpos.y = editor_.text_area_.area.y + static_cast<int>((lines - editor_.points_.offset.y) * editor_.line_height());
				return scrpos;
			}

			nana::upoint screen_to_caret(point scrpos) override
			{
				std::size_t secondary;
				std::size_t primary = _m_textline_from_screen(scrpos.y, secondary);

				auto & mtr = linemtr_[primary];
				if (mtr.line_sections.empty())
					return{ 0, static_cast<unsigned>(primary) };

				//First of all, find the text of secondary.
				auto real_str = mtr.line_sections[secondary];

				std::unique_ptr<std::wstring> mask_str;
				if (editor_.mask_char_)
					mask_str.reset(new std::wstring(real_str.end - real_str.begin, editor_.mask_char_));

				const wchar_t * str = (editor_.mask_char_ ? mask_str->data() : real_str.begin);

				std::vector<unicode_bidi::entity> reordered;
				unicode_bidi bidi;
				bidi.linestr(str, real_str.end - real_str.begin, reordered);

				nana::upoint res(static_cast<unsigned>(real_str.begin - mtr.line_sections.front().begin), static_cast<unsigned>(primary));
				scrpos.x -= editor_.text_area_.area.x;
				if (scrpos.x < 0)
					scrpos.x = 0;

				for (auto & ent : reordered)
				{
					std::size_t len = ent.end - ent.begin;
					auto str_px = static_cast<int>(editor_._m_text_extent_size(ent.begin, len).width);
					if (scrpos.x < str_px)
					{
						std::unique_ptr<unsigned[]> pxbuf(new unsigned[len]);

						res.x += editor_._m_char_by_pixels(ent.begin, len, pxbuf.get(), str_px, scrpos.x, is_right_text(ent));
						res.x += static_cast<unsigned>(ent.begin - str);
						return res;
					}
					scrpos.x -= str_px;
				}
				res.x = static_cast<unsigned>(editor_.textbase_.getline(res.y).size());
				return res;
			}

			bool move_caret_ns(bool to_north) override
			{
				auto & points = editor_.points_;

				nana::upoint secondary_pos;
				_m_pos_secondary(points.caret, secondary_pos);

				if (to_north)	//North
				{
					if (0 == secondary_pos.y)
					{
						if (0 == points.caret.y)
							return false;

						--points.caret.y;
						secondary_pos.y = static_cast<unsigned>(take_lines(points.caret.y)) - 1;
					}
					else
						--secondary_pos.y;
				}
				else //South
				{
					++secondary_pos.y;
					if (secondary_pos.y >= take_lines(points.caret.y))
					{
						secondary_pos.y = 0;
						if (points.caret.y + 1 >= editor_.textbase_.lines())
							return false;

						++points.caret.y;
					}
				}

				_m_pos_from_secondary(points.caret.y, secondary_pos, points.caret.x);
				return adjust_caret_into_screen();
			}

			bool adjust_caret_into_screen() override
			{
				const auto scrlines = editor_.screen_lines();
				if (0 == scrlines)
					return false;

				auto & points = editor_.points_;
				editor_._m_get_scrollbar_size();

				std::size_t off_secondary;
				auto off_primary = _m_textline(points.offset.y, off_secondary);

				unsigned caret_secondary;
				{
					nana::upoint secondpos;
					_m_pos_secondary(points.caret, secondpos);
					caret_secondary = secondpos.y;
				}

				//Use the caret line for the offset line when caret is in front of current offset line.
				if (off_primary > points.caret.y || ((off_primary == points.caret.y) && (off_secondary > caret_secondary)))
				{
					//Use the line which was specified by points.caret for the first line.
					_m_set_offset_by_secondary(points.caret.y, caret_secondary);
					return true;
				}

				//Find the last screen line. If the text does not reach the bottom of screen,
				//do not adjust the offset line.
				nana::upoint bottom;	//x=primary, y=secondary
				if (false == _m_advance_secondary(off_primary, off_secondary, static_cast<int>(scrlines - 1), bottom))
					return false;

				//Do not adjust the offset line if the caret line does not reach the bottom line.
				if (points.caret.y < bottom.x || ((points.caret.y == bottom.x) && (caret_secondary <= bottom.y)))
					return false;

				_m_advance_secondary(points.caret.y, caret_secondary, -static_cast<int>(scrlines - 1), bottom);

				_m_set_offset_by_secondary(bottom.x, bottom.y);
				return true;
			}
		private:
			void _m_text_section(const std::wstring& str, std::vector<text_section>& tsec)
			{
				if (str.empty())
				{
					tsec.emplace_back(str.data(), str.data());
					return;
				}
				const auto end = str.data() + str.size();

				const wchar_t * word = nullptr;
				for (auto i = str.data(); i != end; ++i)
				{
					wchar_t const ch = *i;

					//CKJ characters and whitespace
					if (' ' == ch || '\t' == ch || (0x4E00 <= ch && ch <= 0x9FCF))
					{
						if (word)	//Record the word.
						{
							tsec.emplace_back(word, i);
							word = nullptr;
						}

						tsec.emplace_back(i, i + 1);
						continue;
					}

					if (nullptr == word)
						word = i;
				}

				if(word)
					tsec.emplace_back(word, end);
			}

			void _m_set_offset_by_secondary(std::size_t primary, std::size_t secondary)
			{
				for (auto i = linemtr_.begin(), end = linemtr_.begin() + primary; i != end; ++i)
					secondary += i->take_lines;

				editor_.points_.offset.y = static_cast<int>(secondary);
			}

			bool _m_advance_secondary(std::size_t primary, std::size_t secondary, int distance, nana::upoint& new_sec)
			{
				if ((primary >= linemtr_.size()) || (secondary >= linemtr_[primary].take_lines))
					return false;

				if (0 == distance)
				{
					new_sec.x = static_cast<unsigned>(primary);
					new_sec.y = static_cast<unsigned>(secondary);
					return true;
				}

				if (distance < 0)
				{
					std::size_t n = static_cast<std::size_t>(-distance);

					if (secondary > n)
					{
						new_sec.x = static_cast<unsigned>(primary);
						new_sec.y = static_cast<unsigned>(secondary - n);
						return true;
					}

					if (0 == primary)
						return false;

					--primary;
					n -= (secondary + 1);

					while (true)
					{
						auto lines = linemtr_[primary].take_lines;
						if (lines >= n)
						{
							new_sec.x = static_cast<unsigned>(primary);
							new_sec.y = static_cast<unsigned>(lines - n);
							return true;
						}
						if (0 == primary)
							return false;

						--primary;
						n -= lines;
					}
				}
				else
				{
					std::size_t n = static_cast<std::size_t>(distance);

					if (linemtr_[primary].take_lines - (secondary + 1) >= n)
					{
						new_sec.x = static_cast<unsigned>(primary);
						new_sec.y = static_cast<unsigned>(secondary + n);
						return true;
					}

					n -= (linemtr_[primary].take_lines - (secondary + 1));

					while (++primary < linemtr_.size())
					{
						auto & mtr = linemtr_[primary];
						if (mtr.take_lines >= n)
						{
							new_sec.x = static_cast<unsigned>(primary);
							new_sec.y = static_cast<unsigned>(n - 1);
							return true;
						}
						n -= mtr.take_lines;
					}
				}
				return false;
			}

			bool _m_pos_from_secondary(std::size_t textline, const nana::upoint& secondary, unsigned & pos)
			{
				if (textline >= linemtr_.size())
					return false;

				auto & mtr = linemtr_[textline];
				if (secondary.y >= mtr.line_sections.size() || mtr.line_sections.size() != mtr.take_lines)
					return false;

				auto & section = mtr.line_sections[secondary.y];
				unsigned len = static_cast<unsigned>(section.end - section.begin);

				auto chptr = section.begin + (secondary.x > len ? len : secondary.x);
				pos = static_cast<unsigned>(chptr - editor_.textbase_.getline(textline).data());
				return true;
			}

			bool _m_pos_secondary(const nana::upoint& charpos, nana::upoint& secondary_pos) const
			{
				if (charpos.y >= linemtr_.size())
					return false;

				secondary_pos.x = charpos.x;
				secondary_pos.y = 0;

				unsigned len = 0;
				for (auto & ts : linemtr_[charpos.y].line_sections)
				{
					len = static_cast<unsigned>(ts.end - ts.begin);
					if (len >= secondary_pos.x)
						return true;

					++secondary_pos.y;
					secondary_pos.x -= len;
				}
				--secondary_pos.y;
				secondary_pos.x = len;
				return true;
			}


			std::size_t _m_textline(std::size_t scrline, std::size_t & secondary) const
			{
				secondary = 0;
				std::size_t primary = 0;

				for (auto & mtr : linemtr_)
				{
					if (mtr.take_lines > scrline)
					{
						secondary = scrline;
						return primary;
					}
					else
						scrline -= mtr.take_lines;

					++primary;
				}

				return primary;
			}

			//secondary, index of line that the text was splitted into multilines.
			std::size_t _m_textline_from_screen(int y, std::size_t & secondary) const
			{
				const int text_area_top = editor_.text_area_.area.y;
				const int offset_top = editor_.points_.offset.y;

				if (0 == editor_.textbase_.lines())
				{
					secondary = 0;
					return 0;
				}

				std::size_t screen_line;
				if (y < text_area_top)
				{
					screen_line = (text_area_top - y) / static_cast<int>(editor_.line_height());
					if (screen_line > static_cast<std::size_t>(offset_top))
						screen_line = 0;
					else
						screen_line = static_cast<std::size_t>(offset_top)-screen_line;
				}
				else
					screen_line = static_cast<std::size_t>((y - text_area_top) / static_cast<int>(editor_.line_height()) + offset_top);

				auto primary = _m_textline(screen_line, secondary);
				if (primary < linemtr_.size())
					return primary;

				secondary = linemtr_.back().line_sections.size() - 1;
				return linemtr_.size() - 1;
			}
		private:
			text_editor& editor_;
			std::vector<line_metrics> linemtr_;
		}; //end class behavior_linewrapped


		struct keyword_scheme
		{
			::nana::color fgcolor;
			::nana::color bgcolor;
		};

		struct keyword_desc
		{
			std::wstring text;
			std::string scheme;
			bool case_sensitive;
			bool whole_word_matched;

			keyword_desc(const std::wstring& txt, const std::string& schm, bool cs, bool wwm)
				: text(txt), scheme(schm), case_sensitive(cs), whole_word_matched(wwm)
			{}
		};

		struct text_editor::keywords
		{
			std::map<std::string, std::shared_ptr<keyword_scheme>> schemes;
			std::deque<keyword_desc> kwbase;
		};

		struct entity
		{
			const wchar_t* begin;
			const wchar_t* end;
			const keyword_scheme * scheme;
		};

		class text_editor::keyword_parser
		{
		public:
			void parse(const std::wstring& text, const keywords* kwptr)
			{
				if ( kwptr->kwbase.empty() || text.empty() )
					return;

				using index = std::wstring::size_type;

				std::vector<entity> entities;

				auto test_whole_word = [&text](index pos, index len)
				{
					if (pos)
					{
						auto chr = text[pos - 1];
						if ((std::iswalpha(chr) && !std::isspace(chr)) || chr == '_')
							return false;
					}

					if (pos + len < text.size())
					{
						auto chr = text[pos + len];
						if ((std::iswalpha(chr) && !std::isspace(chr)) || chr == '_')
							return false;
					}

					return true;
				};

				::nana::ciwstring cistr;
				for (auto & ds : kwptr->kwbase)
				{
                    index pos{0} ;
                    for (index rest{text.size()}; rest >= ds.text.size() ; ++pos, rest = text.size() - pos)
                    {
					    if (ds.case_sensitive)
					    {
						    pos = text.find(ds.text, pos);
						    if (pos == text.npos)
							    break;

						    if (ds.whole_word_matched)
						    {
							    if (!test_whole_word(pos, ds.text.size()))
								    continue;
						    }
					    }
					    else
					    {
						    if (cistr.empty())
							    cistr.append(text.data(), text.size());

						    pos = cistr.find(ds.text.data(), pos);
						    if (pos == cistr.npos)
							    break;

						    if (ds.whole_word_matched)
						    {
							    if (!test_whole_word(pos, ds.text.size()))
								    continue;
						    }
					    }

					    auto ki = kwptr->schemes.find(ds.scheme);
					    if (ki != kwptr->schemes.end() && ki->second)
					    {
							entities.emplace_back();
							auto & last = entities.back();
							last.begin = text.data() + pos;
							last.end = last.begin + ds.text.size();
							last.scheme = ki->second.get();
					    }
                    }
				}

				if (!entities.empty())
				{
					std::sort(entities.begin(), entities.end(), [](const entity& a, const entity& b)
					{
						return (a.begin < b.begin);
					});

					auto i = entities.begin();
					auto bound = i->end;

					for (++i; i != entities.end(); )
					{
						if (bound > i->begin)
							i = entities.erase(i);  // erase overlaping. Left only the first.
						else
							++i;
					}
				}

				entities_.swap(entities);
			}

			const std::vector<entity>& entities() const
			{
				return entities_;
			}
		private:
			std::vector<entity> entities_;
		};

		//class text_editor
		text_editor::text_editor(window wd, graph_reference graph, const text_editor_scheme* schm)
			:	behavior_(new behavior_normal(*this)),
				window_(wd), graph_(graph),
				scheme_(schm), keywords_(new keywords)
		{
			text_area_.area = graph.size();
			text_area_.captured = false;
			text_area_.tab_space = 4;
			text_area_.scroll_pixels = 16;
			text_area_.hscroll = text_area_.vscroll = 0;
			select_.mode_selection = selection::mode_no_selected;
			select_.dragged = false;

			API::create_caret(wd, 1, line_height());
			API::bgcolor(wd, colors::white);
			API::fgcolor(wd, colors::black);

			text_area_.border_renderer = [this](graph_reference graph, const ::nana::color& bgcolor)
			{
				if (!API::widget_borderless(this->window_))
				{
					::nana::facade<element::border> facade;
					facade.draw(graph, bgcolor, API::fgcolor(this->window_), ::nana::rectangle{ API::window_size(this->window_) }, API::element_state(this->window_));
				}
			};
		}

		text_editor::~text_editor()
		{
			//For instance of unique_ptr pimpl idiom.
		}

		void text_editor::set_highlight(const std::string& name, const ::nana::color& fgcolor, const ::nana::color& bgcolor)
		{
			if (fgcolor.invisible() && fgcolor.invisible())
			{
				keywords_->schemes.erase(name);
				return;
			}

			auto sp = std::make_shared<keyword_scheme>();
			sp->fgcolor = fgcolor;
			sp->bgcolor = bgcolor;
			keywords_->schemes[name].swap(sp);
		}

		void text_editor::erase_highlight(const std::string& name)
		{
			keywords_->schemes.erase(name);
		}

		void text_editor::set_keyword(const ::std::wstring& kw, const std::string& name, bool case_sensitive, bool whole_word_matched)
		{
			for (auto & ds : keywords_->kwbase)
			{
				if (ds.text == kw)
				{
					ds.scheme = name;
					ds.case_sensitive = case_sensitive;
					ds.whole_word_matched = whole_word_matched;
					return;
				}
			}
			keywords_->kwbase.emplace_back(kw, name, case_sensitive, whole_word_matched);
		}

		void text_editor::erase_keyword(const ::std::wstring& kw)
		{
			for (auto i = keywords_->kwbase.begin(); i != keywords_->kwbase.end(); ++i)
				if (i->text == kw)
				{
					keywords_->kwbase.erase(i);
					return;
				}
		}

		void text_editor::set_accept(std::function<bool(char_type)> pred)
		{
			attributes_.pred_acceptive = std::move(pred);
		}

		void text_editor::set_accept(accepts acceptive)
		{
			attributes_.acceptive = acceptive;
		}

		bool text_editor::respond_char(const arg_keyboard& arg)	//key is a character of ASCII code
		{
			char_type key = arg.key;
			switch (key)
			{
			case keyboard::end_of_text:
				copy();
				return false;
			case keyboard::select_all:
				select(true);
				return true;
			}

			if (attributes_.editable && API::window_enabled(window_) && (!attributes_.pred_acceptive || attributes_.pred_acceptive(key)))
			{
				switch (key)
				{
				case '\b':
					backspace();	break;
				case '\n': case '\r':
					enter();	break;
				case keyboard::sync_idel:
					paste();	break;
				case keyboard::tab:
					put(static_cast<wchar_t>(keyboard::tab)); break;
				case keyboard::cancel:
					cut();
					break;
				case keyboard::end_of_medium:
					undo(true);
					break;
				case keyboard::substitute:
					undo(false);
					break;
				default:
					if (!_m_accepts(key))
						return false;

					if (key > 0x7F || (32 <= key && key <= 126))
						put(key);
				}
				reset_caret();
				return true;
			}
			return false;
		}

		bool text_editor::respond_key(const arg_keyboard& arg)
		{
			char_type key = arg.key;
			switch (key)
			{
			case keyboard::os_arrow_left:
			case keyboard::os_arrow_right:
			case keyboard::os_arrow_up:
			case keyboard::os_arrow_down:
			case keyboard::os_home:
			case keyboard::os_end:
			case keyboard::os_pageup:
			case keyboard::os_pagedown:
				_handle_move_key(arg);
				break;
			case keyboard::os_del:
				if (this->attr().editable)
					del();
				break;
			default:
				return false;
			}
			return true;
		}

		void text_editor::typeface_changed()
		{
			behavior_->pre_calc_lines(width_pixels());
		}

		void text_editor::indent(bool enb, std::function<std::string()> generator)
		{
			indent_.enabled = enb;
			indent_.generator.swap(generator);
		}

		void text_editor::set_event(event_interface* ptr)
		{
			event_handler_ = ptr;
		}

		bool text_editor::line_wrapped() const
		{
			return attributes_.line_wrapped;
		}

		bool text_editor::line_wrapped(bool autl)
		{
			if (autl != attributes_.line_wrapped)
			{
				attributes_.line_wrapped = autl;
				if (autl)
				{
					behavior_.reset(new behavior_linewrapped(*this));
					text_area_.vscroll = text_area_.scroll_pixels;
					text_area_.hscroll = 0;
					behavior_->pre_calc_lines(width_pixels());
				}
				else
					behavior_.reset(new behavior_normal(*this));

				points_.offset.x = 0;
				_m_offset_y(0);
				move_caret(upoint{});

				_m_scrollbar();
				render(API::is_focus_ready(window_));
				return true;
			}
			return false;
		}

		void text_editor::border_renderer(std::function<void(nana::paint::graphics&, const ::nana::color&)> f)
		{
			text_area_.border_renderer = f;
		}

		bool text_editor::load(const char* fs)
		{
			if (!textbase_.load(fs))
				return false;

			_m_reset();
			behavior_->pre_calc_lines(width_pixels());

			render(API::is_focus_ready(window_));
			_m_scrollbar();
			return true;
		}

		bool text_editor::text_area(const nana::rectangle& r)
		{
			if(text_area_.area == r)
				return false;

			text_area_.area = r;
			if(attributes_.enable_counterpart)
				attributes_.counterpart.make({ r.width, r.height });

			behavior_->pre_calc_lines(width_pixels());
			_m_scrollbar();

			move_caret(points_.caret);
			return true;
		}

		rectangle text_editor::text_area(bool including_scroll) const
		{
			if (including_scroll)
				return text_area_.area;

			auto r = text_area_.area;

			r.width = text_area_.area.width > text_area_.vscroll ? text_area_.area.width - text_area_.vscroll : 0;
			r.height = text_area_.area.height > text_area_.hscroll ? text_area_.area.height - text_area_.hscroll : 0;
			return r;
		}

		bool text_editor::tip_string(::std::string&& str)
		{
			if(attributes_.tip_string == str)
				return false;

			attributes_.tip_string = std::move(str);
			return true;
		}

		const text_editor::attributes& text_editor::attr() const
		{
			return attributes_;
		}

		bool text_editor::multi_lines(bool ml)
		{
			if((ml == false) && attributes_.multi_lines)
			{
				//retain the first line and remove the extra lines
				if (textbase_.erase(1, textbase_.lines() - 1))
					_m_reset();
			}

			if (attributes_.multi_lines == ml)
				return false;

			attributes_.multi_lines = ml;

			if (!ml)
				line_wrapped(false);

			_m_scrollbar();
			return true;
		}

		void text_editor::editable(bool v)
		{
			attributes_.editable = v;
		}

		void text_editor::enable_background(bool enb)
		{
			attributes_.enable_background = enb;
		}

		void text_editor::enable_background_counterpart(bool enb)
		{
			attributes_.enable_counterpart = enb;
			if(enb)
				attributes_.counterpart.make({ text_area_.area.width, text_area_.area.height });
			else
				attributes_.counterpart.release();
		}

		void text_editor::undo_enabled(bool enb)
		{
			undo_.enable(enb);
		}

		bool text_editor::undo_enabled() const
		{
			return undo_.enabled();
		}

		void text_editor::undo_max_steps(std::size_t maxs)
		{
			undo_.max_steps(maxs);
		}

		std::size_t text_editor::undo_max_steps() const
		{
			return undo_.max_steps();
		}

		text_editor::ext_renderer_tag& text_editor::ext_renderer() const
		{
			return ext_renderer_;
		}

		unsigned text_editor::line_height() const
		{
			return (graph_ ? (graph_.text_extent_size(L"jH{").height) : 0);
		}

		unsigned text_editor::screen_lines() const
		{
			if(graph_ && (text_area_.area.height > text_area_.hscroll))
			{
				auto lines = (text_area_.area.height - text_area_.hscroll) / line_height();
				return (lines ? lines : 1);
			}
			return 0;
		}

		bool text_editor::mouse_enter(bool enter)
		{
			if((false == enter) && (false == text_area_.captured))
				API::window_cursor(window_, nana::cursor::arrow);

			if(API::focus_window() == window_)
				return false;
			
			render(false);
			return true;
		}

		bool text_editor::mouse_move(bool left_button, const point& scrpos)
		{
			cursor cur = cursor::iterm;
			if ((!hit_text_area(scrpos)) && (!text_area_.captured))
				cur = cursor::arrow;

			API::window_cursor(window_, cur);

			if(left_button)
			{
				auto caret_pos_before = caret();
				mouse_caret(scrpos);

				if(select_.mode_selection != selection::mode_no_selected)
					set_end_caret();
				else if ((!select_.dragged) && (caret_pos_before != caret()))
					select_.dragged = true;

				text_area_.border_renderer(graph_, _m_bgcolor());
				return true;
			}
			return false;
		}

		bool text_editor::mouse_pressed(const arg_mouse& arg)
		{
			if (event_code::mouse_down == arg.evt_code)
			{
				if (!hit_text_area(arg.pos))
					return false;

				if (::nana::mouse::left_button == arg.button)
				{
					API::capture_window(window_, true);
					text_area_.captured = true;

					//Set caret pos by screen point and get the caret pos.
					mouse_caret(arg.pos);
					if (arg.shift)
					{
						if (points_.shift_begin_caret != points_.caret)
						{
							select_.a = points_.shift_begin_caret;
							select_.b = points_.caret;
						}
					}
					else
					{
						if (!select(false))
						{
							select_.a = points_.caret;	//Set begin caret
							set_end_caret();
						}
						points_.shift_begin_caret = points_.caret;
					}
					select_.mode_selection = selection::mode_mouse_selected;
				}

				text_area_.border_renderer(graph_, _m_bgcolor());
				return true;
			}
			else if (event_code::mouse_up == arg.evt_code)
			{
				auto is_prev_no_selected = (select_.mode_selection == selection::mode_no_selected);

				if (select_.mode_selection == selection::mode_mouse_selected)
				{
					select_.mode_selection = selection::mode_no_selected;
					set_end_caret();
				}
				else if (is_prev_no_selected)
				{
					if ((!select_.dragged) || (!move_select()))
						select(false);
				}
				select_.dragged = false;

				API::capture_window(window_, false);
				text_area_.captured = false;
				if (hit_text_area(arg.pos) == false)
					API::window_cursor(window_, nana::cursor::arrow);

				text_area_.border_renderer(graph_, _m_bgcolor());

				//Redraw if is_prev_no_selected is true
				return is_prev_no_selected;
			}
			return false;
		}

		textbase<wchar_t> & text_editor::textbase()
		{
			return textbase_;
		}

		const textbase<wchar_t> & text_editor::textbase() const
		{
			return textbase_;
		}

		bool text_editor::getline(std::size_t pos, std::wstring& text) const
		{
			if (textbase_.lines() <= pos)
				return false;

			text = textbase_.getline(pos);
			return true;
		}

		void text_editor::text(std::wstring str)
		{
			textbase_.erase_all();
			_m_reset();
			behavior_->pre_calc_lines(width_pixels());
			put(std::move(str));
		}

		std::wstring text_editor::text() const
		{
			std::wstring str;
			std::size_t lines = textbase_.lines();
			if(lines > 0)
			{
				str = textbase_.getline(0);
				for(std::size_t i = 1; i < lines; ++i)
				{
					str += L"\n\r";
					str += textbase_.getline(i);
				}
			}
			return str;
		}

		//move_caret
		//Set caret position through text coordinate
		void text_editor::move_caret(const upoint& crtpos)
		{
			if (!API::is_focus_ready(window_))
				return;
			
			const unsigned line_pixels = line_height();
			auto pos = this->behavior_->caret_to_screen(crtpos);
			const int line_bottom = pos.y + static_cast<int>(line_pixels);

			bool visible = false;
			if (hit_text_area(pos) && (line_bottom > text_area_.area.y))
			{
				visible = true;
				if (line_bottom > _m_end_pos(false))
					API::caret_size(window_, nana::size(1, line_pixels - (line_bottom - _m_end_pos(false))));
				else if (API::caret_size(window_).height != line_pixels)
					reset_caret_pixels();
			}

			API::caret_visible(window_, visible);

			if(visible)
				API::caret_pos(window_, pos);
		}

		void text_editor::move_caret_end()
		{
			points_.caret.y = static_cast<unsigned>(textbase_.lines());
			if(points_.caret.y) --points_.caret.y;
			points_.caret.x = static_cast<unsigned>(textbase_.getline(points_.caret.y).size());
		}

		void text_editor::reset_caret_pixels() const
		{
			API::caret_size(window_, nana::size(1, line_height()));
		}

		void text_editor::reset_caret()
		{
			move_caret(points_.caret);
		}

		void text_editor::show_caret(bool isshow)
		{
			if(isshow == false || API::is_focus_ready(window_))
				API::caret_visible(window_, isshow);
		}

		bool text_editor::selected() const
		{
			return (select_.a != select_.b);
		}

		void text_editor::set_end_caret()
		{
			bool new_sel_end = (select_.b != points_.caret);
			select_.b = points_.caret;
			points_.xpos = points_.caret.x;

			if (new_sel_end || behavior_->adjust_caret_into_screen())
				render(true);
		}

		bool text_editor::select(bool yes)
		{
			if(yes)
			{
				select_.a.x = select_.a.y = 0;
				select_.b.y = static_cast<unsigned>(textbase_.lines());
				if(select_.b.y) --select_.b.y;
				select_.b.x = static_cast<unsigned>(textbase_.getline(select_.b.y).size());
				select_.mode_selection = selection::mode_method_selected;
				render(true);
				return true;
			}

			select_.mode_selection = selection::mode_no_selected;
			if (_m_cancel_select(0))
			{
				render(true);
				return true;
			}
			return false;
		}

		bool text_editor::hit_text_area(const point& pos) const
		{
			return ((text_area_.area.x <= pos.x && pos.x < _m_end_pos(true)) && (text_area_.area.y <= pos.y && pos.y < _m_end_pos(false)));
		}

		bool text_editor::hit_select_area(nana::upoint pos) const
		{
			nana::upoint a, b;
			if(_m_get_sort_select_points(a, b))
			{
				if((pos.y > a.y || (pos.y == a.y && pos.x >= a.x)) && ((pos.y < b.y) || (pos.y == b.y && pos.x < b.x)))
					return true;
			}
			return false;
		}

		bool text_editor::move_select()
		{
			if(hit_select_area(points_.caret) || (select_.b == points_.caret))
			{
				points_.caret = select_.b;

				if (behavior_->adjust_caret_into_screen())
					render(true);

				reset_caret();
				return true;
			}

			if (_m_move_select(true))
			{
				behavior_->adjust_caret_into_screen();
				render(true);
				return true;
			}
			return false;
		}

		bool text_editor::mask(wchar_t ch)
		{
			if (mask_char_ == ch)
				return false;

			mask_char_ = ch;
			return true;
		}

		unsigned text_editor::width_pixels() const
		{
			unsigned exclude_px;
			if (attributes_.line_wrapped)
				exclude_px = text_area_.vscroll;
			else
				exclude_px = API::caret_size(window_).width;

			return (text_area_.area.width > exclude_px ? text_area_.area.width - exclude_px : 0);
		}

		window text_editor::window_handle() const
		{
			return window_;
		}

		const std::vector<upoint>& text_editor::text_position() const
		{
			return text_position_;
		}

		void text_editor::draw_corner()
		{
			if(text_area_.vscroll && text_area_.hscroll)
			{
				graph_.rectangle({ text_area_.area.right() - static_cast<int>(text_area_.vscroll), text_area_.area.bottom() - static_cast<int>(text_area_.hscroll), text_area_.vscroll, text_area_.hscroll },
								true, colors::button_face);
			}
		}

		void text_editor::render(bool has_focus)
		{
			const auto bgcolor = _m_bgcolor();

			auto fgcolor = scheme_->foreground.get_color();
			if (!API::window_enabled(window_))
				fgcolor.blend(bgcolor, 0.5);

			if (API::widget_borderless(window_))
				graph_.rectangle(false, bgcolor);

			//Draw background
			if(attributes_.enable_background)
				graph_.rectangle(text_area_.area, true, bgcolor);

			if(ext_renderer_.background)
				ext_renderer_.background(graph_, text_area_.area, bgcolor);

			if(attributes_.counterpart && !text_area_.area.empty())
				attributes_.counterpart.bitblt(nana::rectangle(0, 0, text_area_.area.width, text_area_.area.height), graph_, nana::point(text_area_.area.x, text_area_.area.y));

			//Render the content when the text isn't empty or the window has got focus,
			//otherwise draw the tip string.
			if ((false == textbase_.empty()) || has_focus)
			{
				auto && text_pos = behavior_->render(fgcolor);
				

				if (text_pos.empty())
					text_pos.push_back({ 0, 0 });
				
				if (text_pos != text_position_)
				{
					text_position_.swap(text_pos);
					if (event_handler_)
						event_handler_->text_exposed(text_position_);
				}
			}
			else //Draw tip string
				graph_.string({ text_area_.area.x - points_.offset.x, text_area_.area.y }, attributes_.tip_string, static_cast<color_rgb>(0x787878));

			if (text_position_.empty())
				text_position_.push_back({ 0, 0 });

			draw_corner();

			text_area_.border_renderer(graph_, bgcolor);
		}
	//public:
		void text_editor::put(std::wstring text)
		{
			auto undo_ptr = std::unique_ptr<undo_input_text>{ new undo_input_text(*this, text) };

			undo_ptr->set_selected_text();

			//Do not forget to assign the _m_erase_select() to caret
			//because _m_put() will insert the text at the position where the caret is.
			points_.caret = _m_erase_select();

			undo_ptr->set_caret_pos();
			points_.caret = _m_put(std::move(text));

			undo_.push(std::move(undo_ptr));

			if(graph_)
			{
				behavior_->adjust_caret_into_screen();
				reset_caret();
				render(API::is_focus_ready(window_));
				_m_scrollbar();

				points_.xpos = points_.caret.x;
			}
		}

		void text_editor::put(wchar_t ch)
		{
			std::wstring ch_str(1, ch);

			auto undo_ptr = std::unique_ptr < undo_input_text > {new undo_input_text(*this, ch_str)};
			bool refresh = (select_.a != select_.b);

			undo_ptr->set_selected_text();
			if(refresh)
				points_.caret = _m_erase_select();

			undo_ptr->set_caret_pos();
			undo_.push(std::move(undo_ptr));

			auto secondary_before = behavior_->take_lines(points_.caret.y);
			textbase_.insert(points_.caret, std::move(ch_str));
			behavior_->pre_calc_line(points_.caret.y, width_pixels());
			points_.caret.x ++;

			if (refresh || _m_update_caret_line(secondary_before))
				render(true);
			else
				draw_corner();

			_m_scrollbar();

			points_.xpos = points_.caret.x;
		}

		void text_editor::copy() const
		{
			std::wstring str;
			if(_m_make_select_string(str))
				nana::system::dataexch().set(str);
		}

		void text_editor::cut()
		{
			copy();
			del();
		}

		void text_editor::paste()
		{
			std::wstring text;
			nana::system::dataexch().get(text);

			//If it is required check the acceptable
			if (accepts::no_restrict != attributes_.acceptive)
			{
				for (auto i = text.begin(); i != text.end(); ++i)
				{
					if (!_m_accepts(*i))
					{
						text.erase(i, text.end());
						break;
					}
				}
			}

			if (!text.empty())
				put(std::move(text));
		}

		void text_editor::enter(bool record_undo)
		{
			if(false == attributes_.multi_lines)
				return;

			auto undo_ptr = std::unique_ptr<undo_input_text>(new undo_input_text(*this, std::wstring(1, '\n')));
			bool need_refresh = (select_.a != select_.b);

			undo_ptr->set_selected_text();
			if(need_refresh)
				points_.caret = _m_erase_select();

			undo_ptr->set_caret_pos();
			const string_type& lnstr = textbase_.getline(points_.caret.y);
			++points_.caret.y;

			if(lnstr.size() > points_.caret.x)
			{
				//Breaks the line and moves the rest part to a new line
				auto rest_part_len = lnstr.size() - points_.caret.x;	//Firstly get the length of rest part, because lnstr may be invalid after insertln
				textbase_.insertln(points_.caret.y, lnstr.substr(points_.caret.x));
				textbase_.erase(points_.caret.y - 1, points_.caret.x, rest_part_len);
			}
			else
			{
				if (textbase_.lines() == 0)
					textbase_.insertln(0, std::wstring{});
				textbase_.insertln(points_.caret.y, std::wstring{});
			}

			if (record_undo)
				undo_.push(std::move(undo_ptr));

			const auto width_px = width_pixels();
			behavior_->add_lines(points_.caret.y - 1, 1);
			behavior_->pre_calc_line(points_.caret.y, width_px);
			behavior_->pre_calc_line(points_.caret.y - 1, width_px);

			points_.caret.x = 0;

			if(points_.offset.x || (points_.caret.y < textbase_.lines()) || textbase_.getline(points_.caret.y).size())
			{
				points_.offset.x = 0;
				need_refresh = true;
			}

			if (indent_.enabled)
			{
				std::wstring indent_text;
				if (indent_.generator)
				{
					indent_text = to_wstring(indent_.generator());
				}
				else
				{
					auto & text = textbase_.getline(points_.caret.y - 1);
					auto indent_pos = text.find_first_not_of(L"\t ");
					if (indent_pos != std::wstring::npos)
						indent_text = text.substr(0, indent_pos);
					else
						indent_text = text;
				}

				if (indent_text.size())
					put(indent_text);
				
			}


			if (behavior_->adjust_caret_into_screen() || need_refresh)
				render(true);

			_m_scrollbar();
		}

		void text_editor::del()
		{
			bool has_erase = true;

			if(select_.a == select_.b)
			{
				if(textbase_.getline(points_.caret.y).size() > points_.caret.x)
				{
					++points_.caret.x;
				}
				else if(points_.caret.y + 1 < textbase_.lines())
				{	//Move to next line
					points_.caret.x = 0;
					++ points_.caret.y;
				}
				else
					has_erase = false;	//No characters behind the caret
			}

			if(has_erase)	backspace();

			_m_scrollbar();
			points_.xpos = points_.caret.x;
		}

		void text_editor::backspace(bool record_undo)
		{
			auto undo_ptr = std::unique_ptr<undo_backspace>(new undo_backspace(*this));
			bool has_to_redraw = true;
			if(select_.a == select_.b)
			{
				if(points_.caret.x)
				{
					unsigned erase_number = 1;
					--points_.caret.x;

					auto& lnstr = textbase_.getline(points_.caret.y);

					undo_ptr->set_caret_pos();
					undo_ptr->set_removed(lnstr.substr(points_.caret.x, erase_number));
					auto secondary = behavior_->take_lines(points_.caret.y);
					textbase_.erase(points_.caret.y, points_.caret.x, erase_number);
					behavior_->pre_calc_line(points_.caret.y, width_pixels());
					if(_m_move_offset_x_while_over_border(-2) == false)
					{
						behavior_->update_line(points_.caret.y, secondary);
						draw_corner();
						has_to_redraw = false;
					}
				}
				else if (points_.caret.y)
				{
					points_.caret.x = static_cast<unsigned>(textbase_.getline(--points_.caret.y).size());
					textbase_.merge(points_.caret.y);
					behavior_->merge_lines(points_.caret.y, points_.caret.y + 1);
					undo_ptr->set_caret_pos();
					undo_ptr->set_removed(std::wstring(1, '\n'));
				}
				else
					undo_ptr.reset();
			}
			else
			{
				undo_ptr->set_selected_text();
				points_.caret = _m_erase_select();
				undo_ptr->set_caret_pos();
			}

			if (record_undo)
				undo_.push(std::move(undo_ptr));

			if(has_to_redraw)
			{
				behavior_->pre_calc_lines(width_pixels());
				behavior_->adjust_caret_into_screen();
				render(true);
			}
			_m_scrollbar();
		}

		void text_editor::undo(bool reverse)
		{
			if (reverse)
				undo_.redo();
			else
				undo_.undo();

			behavior_->pre_calc_lines(width_pixels());
			behavior_->adjust_caret_into_screen();
			render(true);
			_m_scrollbar();

		}

		void text_editor::move_ns(bool to_north)
		{
			const bool redraw_required = _m_cancel_select(0);
			if (behavior_->move_caret_ns(to_north) || redraw_required)
				render(true);
			_m_scrollbar();
		}

		void text_editor::move_left()
		{
			bool pending = true;
			if(_m_cancel_select(1) == false)
			{
				if(points_.caret.x)
				{
					--points_.caret.x;

					pending = false;
					bool adjust_y = (attributes_.line_wrapped && behavior_->adjust_caret_into_screen());
					if (_m_move_offset_x_while_over_border(-2) || adjust_y)
						render(true);
				}
				else if (points_.caret.y) //Move to previous line
					points_.caret.x = static_cast<unsigned>(textbase_.getline(--points_.caret.y).size());
				else
					pending = false;
			}

			if (pending && behavior_->adjust_caret_into_screen())
				render(true);

			_m_scrollbar();
			points_.xpos = points_.caret.x;
		}

		void text_editor::move_right()
		{
			bool do_render = false;
			if(_m_cancel_select(2) == false)
			{
				auto lnstr = textbase_.getline(points_.caret.y);
				if(lnstr.size() > points_.caret.x)
				{
					++points_.caret.x;

					bool adjust_y = (attributes_.line_wrapped && behavior_->adjust_caret_into_screen());
					do_render = (_m_move_offset_x_while_over_border(2) || adjust_y);
				}
				else if(textbase_.lines() && (points_.caret.y < textbase_.lines() - 1))
				{	//Move to next line
					points_.caret.x = 0;
					++ points_.caret.y;
					do_render = behavior_->adjust_caret_into_screen();
				}
			}
			else
				do_render = behavior_->adjust_caret_into_screen();

			if (do_render)
				render(true);

			_m_scrollbar();
			points_.xpos = points_.caret.x;
		}

		void text_editor::_handle_move_key(const arg_keyboard& arg)
		{
			if (arg.shift && (select_.a == select_.b))
				select_.a = select_.b = points_.caret;

			bool changed = false;
			nana::upoint caret = points_.caret;
			wchar_t key = arg.key;
			size_t nlines = textbase_.lines();
			if (arg.ctrl) {
				switch (key) {
				case keyboard::os_arrow_left:
				case keyboard::os_arrow_right:
					// TODO: move the caret word by word
					break;
				case keyboard::os_home:
					if (caret.y != 0) {
						caret.y = 0;
						points_.offset.y = 0;
						changed = true;
					}
					break;
				case keyboard::os_end:
					if (caret.y != nlines - 1) {
						caret.y = static_cast<decltype(caret.y)>(nlines - 1);
						changed = true;
					}
					break;
				}
			}
			size_t lnsz = textbase_.getline(caret.y).size();
			switch (key) {
			case keyboard::os_arrow_left:
				if (caret.x != 0) {
					--caret.x;
					changed = true;
				}else {
					if (caret.y != 0) {
						--caret.y;
						caret.x = static_cast<decltype(caret.x)>(textbase_.getline(caret.y).size());
						changed = true;
					}
				}
				break;
			case keyboard::os_arrow_right:
				if (caret.x < lnsz) {
					++caret.x;
					changed = true;
				}else {
					if (caret.y != nlines - 1) {
						++caret.y;
						caret.x = 0;
						changed = true;
					}
				}
				break;
			case keyboard::os_arrow_up:
			case keyboard::os_arrow_down:
				{
					auto screen_pt = behavior_->caret_to_screen(caret);
					int offset = line_height();
					if (key == keyboard::os_arrow_up) {
						offset = -offset;
					}
					screen_pt.y += offset;
					auto new_caret = behavior_->screen_to_caret(screen_pt);
					if (new_caret != caret) {
						caret = new_caret;
						if (screen_pt.y < 0) {
							scroll(true, true);
						}
						changed = true;
					}
				}
				break;
			case keyboard::os_home:
				if (caret.x != 0) {
					caret.x = 0;
					changed = true;
				}
				break;
			case keyboard::os_end:
				if (caret.x < lnsz) {
					caret.x = static_cast<decltype(caret.x)>(lnsz);
					changed = true;
				}
				break;
			case keyboard::os_pageup:
				if (caret.y >= screen_lines() && points_.offset.y >= static_cast<int>(screen_lines())) {
					points_.offset.y -= screen_lines();
					caret.y -= screen_lines();
					changed = true;
				}
				break;
			case keyboard::os_pagedown:
				if (caret.y + screen_lines() <= behavior_->take_lines()) {
					points_.offset.y += screen_lines();
					caret.y += screen_lines();
					changed = true;
				}
				break;
			}
			if (select_.a != caret || select_.b != caret) {
				changed = true;
			}
			if (changed) {
				if (arg.shift) {
					switch (key) {
					case keyboard::os_arrow_left:
					case keyboard::os_arrow_up:
					case keyboard::os_home:
					case keyboard::os_pageup:
						select_.b = caret;
						break;
					case keyboard::os_arrow_right:
					case keyboard::os_arrow_down:
					case keyboard::os_end:
					case keyboard::os_pagedown:
						select_.b = caret;
						break;
					}
				}else {
					select_.b = caret;
					select_.a = caret;
				}
				points_.caret = caret;
				behavior_->adjust_caret_into_screen();
				render(true);
				_m_scrollbar();
				points_.xpos = points_.caret.x;
			}
		}

		const upoint& text_editor::mouse_caret(const point& scrpos)	//From screen position
		{
			points_.caret = behavior_->screen_to_caret(scrpos);

			if (behavior_->adjust_caret_into_screen())
				render(true);

			move_caret(points_.caret);
			return points_.caret;
		}

		const upoint& text_editor::caret() const
		{
			return points_.caret;
		}

		point text_editor::caret_screen_pos() const
		{
			return behavior_->caret_to_screen(points_.caret);
		}

		bool text_editor::scroll(bool upwards, bool vert)
		{
			if(vert && attributes_.vscroll)
			{
				attributes_.vscroll->make_step(!upwards);
				if(_m_scroll_text(true))
				{
					render(true);
					return true;
				}
			}
			return false;
		}

		bool text_editor::_m_accepts(char_type ch) const
		{
			if (accepts::no_restrict == attributes_.acceptive)
				return true;

			//Checks the input whether it meets the requirement for a numeric.
			auto str = text();

			if ('+' == ch || '-' == ch)
				return str.empty();

			if ((accepts::real == attributes_.acceptive) && ('.' == ch))
				return (str.find(L'.') == str.npos);

			return ('0' <= ch && ch <= '9');
		}

		::nana::color text_editor::_m_bgcolor() const
		{
			return (!API::window_enabled(window_) ? static_cast<color_rgb>(0xE0E0E0) : API::bgcolor(window_));
		}

		bool text_editor::_m_scroll_text(bool vert)
		{
			if (vert)
			{
				if (attributes_.vscroll)
				{
					auto sv = static_cast<int>(attributes_.vscroll->value());
					if (sv != points_.offset.y)
					{
						_m_offset_y(sv);
						return true;
					}
				}
			}
			else if(attributes_.hscroll)
			{
				auto sv = static_cast<int>(attributes_.hscroll->value());
				if(sv != points_.offset.x)
				{
					points_.offset.x = sv;
					return true;
				}
			}
			return false;
		}

		void text_editor::_m_on_scroll(const arg_mouse& arg)
		{
			if((arg.evt_code == event_code::mouse_move) && (arg.left_button == false))
				return;

			bool vert = (attributes_.vscroll && (attributes_.vscroll->handle() == arg.window_handle));
			if(_m_scroll_text(vert))
			{
				render(true);
				reset_caret();
				API::update_window(window_);
			}
		}

		void text_editor::_m_scrollbar()
		{
			_m_get_scrollbar_size();
			nana::size tx_area = _m_text_area();

			auto scroll_fn = [this](const arg_mouse& arg) {
				_m_on_scroll(arg);
			};

			if (text_area_.vscroll)
			{
				const int x = text_area_.area.x + static_cast<int>(tx_area.width);
				auto wdptr = attributes_.vscroll.get();
				if (!wdptr)
				{
					attributes_.vscroll.reset(new nana::scroll<true>);
					wdptr = attributes_.vscroll.get();
					wdptr->create(window_, nana::rectangle(x, text_area_.area.y, text_area_.vscroll, tx_area.height));

					auto & evts = wdptr->events();
					evts.mouse_down(scroll_fn);
					evts.mouse_move(scroll_fn);
					evts.mouse_wheel(scroll_fn);

					API::take_active(wdptr->handle(), false, window_);
				}

				if (behavior_->take_lines() != wdptr->amount())
					wdptr->amount(static_cast<int>(behavior_->take_lines()));

				if (screen_lines() != wdptr->range())
					wdptr->range(screen_lines());

				if (points_.offset.y != static_cast<int>(wdptr->value()))
					wdptr->value(points_.offset.y);

				wdptr->move(rectangle{ x, text_area_.area.y, text_area_.vscroll, tx_area.height });
			}
			else
				attributes_.vscroll.reset();

			//HScroll
			if(text_area_.hscroll)
			{
				auto wdptr = attributes_.hscroll.get();
				int y = text_area_.area.y + static_cast<int>(tx_area.height);
				if(nullptr == wdptr)
				{
					attributes_.hscroll.reset(new nana::scroll<false>);
					wdptr = attributes_.hscroll.get();
					wdptr->create(window_, nana::rectangle(text_area_.area.x, y, tx_area.width, text_area_.hscroll));

					auto & evts = wdptr->events();
					evts.mouse_down(scroll_fn);
					evts.mouse_move(scroll_fn);
					evts.mouse_wheel(scroll_fn);

					wdptr->step(20);
					API::take_active(wdptr->handle(), false, window_);
				}
				auto maxline = textbase_.max_line();
				nana::size text_size = _m_text_extent_size(textbase_.getline(maxline.first).c_str(), maxline.second);

				text_size.width += 1;
				if(text_size.width > wdptr->amount())
					wdptr->amount(text_size.width);

				if(tx_area.width != wdptr->range())	wdptr->range(tx_area.width);
				if(points_.offset.x != static_cast<int>(wdptr->value()))
					wdptr->value(points_.offset.x);

				wdptr->move(rectangle{ text_area_.area.x, y, tx_area.width, text_area_.hscroll });
			}
			else
				attributes_.hscroll.reset();
		}

		nana::size text_editor::_m_text_area() const
		{
			return nana::size((text_area_.area.width > text_area_.vscroll ? text_area_.area.width - text_area_.vscroll : 0),
				(text_area_.area.height > text_area_.hscroll ? text_area_.area.height - text_area_.hscroll : 0));
		}

		void text_editor::_m_get_scrollbar_size()
		{
			text_area_.hscroll = 0;

			if (attributes_.line_wrapped)
			{
				text_area_.vscroll = text_area_.scroll_pixels;
				return;
			}

			//No scrollbar if it is not multi-line
			if (!attributes_.multi_lines)
			{
				text_area_.vscroll = 0;
				return;
			}

			text_area_.vscroll = (textbase_.lines() > screen_lines() ? text_area_.scroll_pixels : 0);

			auto max_line = textbase_.max_line();
			if(max_line.second)
			{
				if(points_.offset.x || _m_text_extent_size(textbase_.getline(max_line.first).c_str(), max_line.second).width > _m_text_area().width)
				{
					text_area_.hscroll = text_area_.scroll_pixels;
					if((text_area_.vscroll == 0) && (textbase_.lines() > screen_lines()))
						text_area_.vscroll = text_area_.scroll_pixels;
				}
			}
		}

		void text_editor::_m_reset()
		{
			points_.caret.x = points_.caret.y = 0;
			points_.offset.x = 0;
			_m_offset_y(0);
			select_.a = select_.b;
		}

		nana::upoint text_editor::_m_put(std::wstring text)
		{
			auto crtpos = points_.caret;
			std::vector<std::pair<std::size_t, std::size_t>> lines;
			if (_m_resolve_text(text, lines) && attributes_.multi_lines)
			{
				auto str_orig = textbase_.getline(crtpos.y);
				auto x_orig = crtpos.x;

				auto subpos = lines.front();
				auto substr = text.substr(subpos.first, subpos.second - subpos.first);

				if (str_orig.size() == x_orig)
					textbase_.insert(crtpos, std::move(substr));
				else
					textbase_.replace(crtpos.y, str_orig.substr(0, x_orig) + substr);

				//There are at least 2 elements in lines
				for (auto i = lines.begin() + 1, end = lines.end() - 1; i != end; ++i)
				{
					textbase_.insertln(++crtpos.y, text.substr(i->first, i->second - i->first));
				}

				auto backpos = lines.back();
				textbase_.insertln(++crtpos.y, text.substr(backpos.first, backpos.second - backpos.first) + str_orig.substr(x_orig));
				crtpos.x = static_cast<decltype(crtpos.x)>(backpos.second - backpos.first);

				const auto width_px = width_pixels();
				behavior_->add_lines(points_.caret.y, lines.size() - 1);
				const auto endline = points_.caret.y + lines.size();
				for (auto i = points_.caret.y; i < endline; ++i)
					behavior_->pre_calc_line(i, width_px);
			}
			else
			{
				//Just insert the first line of text if the text is multilines.
				if (lines.size() > 1)
					text = text.substr(lines.front().first, lines.front().second - lines.front().first);

				auto length = text.size();
				textbase_.insert(crtpos, std::move(text));

				crtpos.x += static_cast<unsigned>(length);
				behavior_->pre_calc_line(crtpos.y, width_pixels());
			}
			return crtpos;
		}

		nana::upoint text_editor::_m_erase_select()
		{
			nana::upoint a, b;
			if (_m_get_sort_select_points(a, b))
			{
				if(a.y != b.y)
				{
					textbase_.erase(a.y, a.x, std::wstring::npos);
					textbase_.erase(a.y + 1, b.y - a.y - 1);

					textbase_.erase(a.y + 1, 0, b.x);
					textbase_.merge(a.y);

					behavior_->merge_lines(a.y, b.y);
				}
				else
				{
					textbase_.erase(a.y, a.x, b.x - a.x);
					behavior_->pre_calc_line(a.y, width_pixels());
				}

				select_.a = select_.b;
				return a;
			}

			return points_.caret;
		}

		bool text_editor::_m_make_select_string(std::wstring& text) const
		{
			nana::upoint a, b;
			if (!_m_get_sort_select_points(a, b))
				return false;

			if(a.y != b.y)
			{
				text = textbase_.getline(a.y).substr(a.x);
				text += L"\r\n";
				for(unsigned i = a.y + 1; i < b.y; ++i)
				{
					text += textbase_.getline(i);
					text += L"\r\n";
				}
				text += textbase_.getline(b.y).substr(0, b.x);
			}
			else
				text = textbase_.getline(a.y).substr(a.x, b.x - a.x);

			return true;
		}

		std::size_t eat_endl(const wchar_t* str, std::size_t pos)
		{
			auto ch = str[pos];
			if (0 == ch)
				return pos;

			const wchar_t * endlstr;
			switch (ch)
			{
			case L'\n':
				endlstr = L"\n\r";
				break;
			case L'\r':
				endlstr = L"\r\n";
				break;
			default:
				return pos;
			}

			if (std::memcmp(str + pos, endlstr, sizeof(wchar_t) * 2) == 0)
				return pos + 2;

			return pos + 1;
		}

		bool text_editor::_m_resolve_text(const std::wstring& text, std::vector<std::pair<std::size_t, std::size_t>> & lines)
		{
			auto const text_str = text.data();
			std::size_t begin = 0;
			while (true)
			{
				auto pos = text.find_first_of(L"\r\n", begin);
				if (text.npos == pos)
				{
					if (!lines.empty())
						lines.emplace_back(begin, text.size());
					break;
				}

				lines.emplace_back(begin, pos);

				pos = eat_endl(text_str, pos);

				begin = text.find_first_not_of(L"\r\n", pos);

				//The number of new lines minus one
				const auto chp_end = text_str + (begin == text.npos ? text.size() : begin);

				for (auto chp = text_str + pos; chp != chp_end; ++chp)
				{
					auto eats = eat_endl(chp, 0);
					if (eats)
					{
						lines.emplace_back(0, 0);
						chp += (eats - 1);
					}
				}

				if (text.npos == begin)
				{
					lines.emplace_back(0, 0);
					break;
				}
			}
			return !lines.empty();
		}

		bool text_editor::_m_cancel_select(int align)
		{
			nana::upoint a, b;
			if(_m_get_sort_select_points(a, b))
			{
				switch(align)
				{
				case 1:
					points_.caret = a;
					_m_move_offset_x_while_over_border(-2);
					break;
				case 2:
					points_.caret = b;
					_m_move_offset_x_while_over_border(2);
					break;
				}
				select_.a = select_.b = points_.caret;
				reset_caret();
				return true;
			}
			return false;
		}

		unsigned text_editor::_m_tabs_pixels(size_type tabs) const
		{
			if(0 == tabs) return 0;

			wchar_t ws[2] = {};
			ws[0] = mask_char_ ? mask_char_ : ' ';
			return static_cast<unsigned>(tabs * graph_.text_extent_size(ws).width * text_area_.tab_space);
		}

		nana::size text_editor::_m_text_extent_size(const char_type* str, size_type n) const
		{
			if (!graph_)
				return{};

			if(mask_char_)
			{
				std::wstring maskstr;
				maskstr.append(n, mask_char_);
				return graph_.text_extent_size(maskstr);
			}
			return graph_.text_extent_size(str, static_cast<unsigned>(n));
		}

		//_m_move_offset_x_while_over_border
		//@brief: Move the view window
		bool text_editor::_m_move_offset_x_while_over_border(int many)
		{
			//x never beyonds border in line-wrapped mode.
			if (attributes_.line_wrapped || (0 == many))
				return false;

			const string_type& lnstr = textbase_.getline(points_.caret.y);
			unsigned width = _m_text_extent_size(lnstr.c_str(), points_.caret.x).width;

			const auto count = static_cast<unsigned>(std::abs(many));
			if(many < 0)
			{
				if(points_.offset.x && (points_.offset.x >= static_cast<int>(width)))
				{	//Out of screen text area
					if(points_.caret.x > count)
						points_.offset.x = static_cast<int>(width - _m_text_extent_size(lnstr.c_str() + points_.caret.x - count, count).width);
					else
						points_.offset.x = 0;
					return true;
				}
			}
			else
			{
				width += text_area_.area.x;
				if(static_cast<int>(width) - points_.offset.x >= _m_end_pos(true))
				{	//Out of screen text area
					points_.offset.x = static_cast<int>(width) -_m_end_pos(false) + 1;
					auto rest_size = lnstr.size() - points_.caret.x;
					points_.offset.x += static_cast<int>(_m_text_extent_size(lnstr.c_str() + points_.caret.x, (rest_size >= static_cast<unsigned>(many) ? static_cast<unsigned>(many) : rest_size)).width);
					return true;
				}
			}
			return false;
		}

		bool text_editor::_m_move_select(bool record_undo)
		{
			nana::upoint caret = points_.caret;
			std::wstring text;
			if (_m_make_select_string(text))
			{
				auto undo_ptr = std::unique_ptr<undo_move_text>(new undo_move_text(*this));
				undo_ptr->set_selected_text();

				nana::upoint a, b;
				_m_get_sort_select_points(a, b);
				if (caret.y < a.y || (caret.y == a.y && caret.x < a.x))
				{//forward
					_m_erase_select();

					undo_ptr->set_caret_pos();
					_m_put(text);

					select_.a = caret;
					select_.b.y = b.y + (caret.y - a.y);
				}
				else if (b.y < caret.y || (caret.y == b.y && b.x < caret.x))
				{
					undo_ptr->set_caret_pos();
					_m_put(text);
					_m_erase_select();

					select_.b.y = caret.y;
					select_.a.y = caret.y - (b.y - a.y);
					select_.a.x = caret.x - (caret.y == b.y ? (b.x - a.x) : 0);
				}
				select_.b.x = b.x + (a.y == b.y ? (select_.a.x - a.x) : 0);

				if (record_undo)
				{
					undo_ptr->set_destination(select_.a, select_.b);
					undo_.push(std::move(undo_ptr));
				}

				points_.caret = select_.a;
				reset_caret();
				return true;
			}
			return false;
		}

		int text_editor::_m_text_top_base() const
		{
			if(false == attributes_.multi_lines)
			{
				unsigned px = line_height();
				if(text_area_.area.height > px)
					return text_area_.area.y + static_cast<int>((text_area_.area.height - px) >> 1);
			}
			return text_area_.area.y;
		}

		int text_editor::_m_end_pos(bool right) const
		{
			if(right)
				return static_cast<int>(text_area_.area.x + text_area_.area.width - text_area_.vscroll);

			return static_cast<int>(text_area_.area.y + text_area_.area.height - text_area_.hscroll);
		}

		void text_editor::_m_draw_parse_string(const keyword_parser& parser, bool rtl, ::nana::point pos, const ::nana::color& fgcolor, const wchar_t* str, std::size_t len) const
		{
			graph_.palette(true, fgcolor);
			graph_.string(pos, str, len);
			if (parser.entities().empty())
				return;

			std::unique_ptr<unsigned[]> glyph_px(new unsigned[len]);
			graph_.glyph_pixels(str, len, glyph_px.get());
			auto glyphs = glyph_px.get();

			auto px_h = line_height();
			auto px_w = std::accumulate(glyphs, glyphs + len, unsigned{});

			::nana::paint::graphics canvas;
			canvas.make({ px_w, px_h });
			canvas.typeface(graph_.typeface());
			::nana::point canvas_text_pos;

			auto ent_pos = pos;
			const auto str_end = str + len;
			auto & entities = parser.entities();

			for (auto & ent : entities)
			{
				const wchar_t* ent_begin = nullptr;

				int ent_off = 0;
				if (str <= ent.begin && ent.begin < str_end)
				{
					ent_begin = ent.begin;
					ent_off = std::accumulate(glyphs, glyphs + (ent.begin - str), 0);
				}
				else if (ent.begin <= str && str < ent.end)
					ent_begin = str;

				if (ent_begin)
				{
					auto ent_end = (ent.end < str_end ? ent.end : str_end);
					auto ent_pixels = std::accumulate(glyphs + (ent_begin - str), glyphs + (ent_end - str), unsigned{});

					canvas.palette(false, ent.scheme->bgcolor.invisible() ? _m_bgcolor() : ent.scheme->bgcolor);
					canvas.palette(true, ent.scheme->fgcolor.invisible() ? fgcolor : ent.scheme->fgcolor);

					canvas.rectangle(true);

					ent_pos.x += ent_off;
					if (rtl)
					{
						//draw the whole text if it is a RTL text, because Arbic language is transformable.
						canvas.string({}, str, len);
						graph_.bitblt(::nana::rectangle{ ent_pos, ::nana::size{ ent_pixels, canvas.height() } }, canvas, ::nana::point{ ent_off, 0 });
					}
					else
					{
						canvas.string({}, ent_begin, ent_end - ent_begin);
						graph_.bitblt(::nana::rectangle{ ent_pos, ::nana::size{ ent_pixels, canvas.height() } }, canvas);
					}
				}
			}
		}

		void text_editor::_m_draw_string(int top, const ::nana::color& clr, const nana::upoint& str_pos, const std::wstring& str, bool if_mask) const
		{
			::nana::point text_pos{ text_area_.area.x - points_.offset.x, top };
			const int text_right = text_area_.area.right();

			std::unique_ptr<std::wstring> mask_str;
			if (if_mask && mask_char_)
				mask_str.reset(new std::wstring(str.size(), mask_char_));

			bool focused = API::is_focus_ready(window_);

			auto & linestr = (if_mask && mask_char_ ? *mask_str : str);

			unicode_bidi bidi;
			std::vector<unicode_bidi::entity> reordered;
			bidi.linestr(linestr.c_str(), linestr.size(), reordered);

			//Parse highlight keywords
			keyword_parser parser;
			parser.parse(linestr, keywords_.get());

			auto whitespace_w = graph_.text_extent_size(L" ", 1).width;

			const auto line_h_pixels = line_height();

			//The line of text is in the range of selection
			nana::upoint a, b;
			graph_.palette(true, clr);
			graph_.palette(false, scheme_->selection.get_color());

			//The text is not selected or the whole line text is selected
			if (!focused || (!_m_get_sort_select_points(a, b)) || (select_.a.y != str_pos.y && select_.b.y != str_pos.y))
			{
				bool selected = (a.y < str_pos.y && str_pos.y < b.y);
				for (auto & ent : reordered)
				{
					std::size_t len = ent.end - ent.begin;
					unsigned str_w = graph_.text_extent_size(ent.begin, len).width;

					if ((text_pos.x + static_cast<int>(str_w) > text_area_.area.x) && (text_pos.x < text_right))
					{
						if (selected && focused)
						{
							graph_.palette(true, scheme_->selection_text.get_color());
							graph_.rectangle(::nana::rectangle{ text_pos, { str_w, line_h_pixels } }, true);
							graph_.string(text_pos, ent.begin, len);
						}
						else
							_m_draw_parse_string(parser, is_right_text(ent), text_pos, clr, ent.begin, len);
					}
					text_pos.x += static_cast<int>(str_w);
				}
				if (selected)
					graph_.rectangle(::nana::rectangle{ text_pos, { whitespace_w, line_h_pixels } }, true);
			}
			else
			{
				auto rtl_string = [this, line_h_pixels, &parser, &clr](point strpos, const wchar_t* str, std::size_t len, std::size_t str_px, unsigned glyph_front, unsigned glyph_selected){
					this->_m_draw_parse_string(parser, true, strpos, clr, str, len);

					//Draw selected part
					paint::graphics graph({ glyph_selected, line_h_pixels });
					graph.typeface(this->graph_.typeface());
					graph.rectangle(true, scheme_->selection.get_color());

					int sel_xpos = static_cast<int>(str_px - (glyph_front + glyph_selected));

					graph.palette(true, scheme_->selection_text.get_color());
					graph.string({-sel_xpos, 0}, str, len);
					graph_.bitblt(nana::rectangle(strpos.x + sel_xpos, strpos.y, glyph_selected, line_h_pixels), graph);
				};

				const wchar_t * strbeg = linestr.c_str();
				if (a.y == b.y)
				{
					for (auto & ent : reordered)
					{
						std::size_t len = ent.end - ent.begin;
						unsigned str_w = graph_.text_extent_size(ent.begin, len).width;
						if ((text_pos.x + static_cast<int>(str_w) > text_area_.area.x) && (text_pos.x < text_right))
						{
							std::size_t pos = ent.begin - strbeg + str_pos.x;
							const auto str_end = pos + len;

							//NOT selected or seleceted all
							if ((str_end <= a.x || pos >= b.x) || (a.x <= pos && str_end <= b.x))
							{
								//selected all
								if (a.x <= pos && str_end <= b.x)
								{
									graph_.rectangle(::nana::rectangle{ text_pos, { str_w, line_h_pixels } }, true);
									graph_.palette(true, scheme_->selection_text.get_color());
									graph_.string(text_pos, ent.begin, len);
								}
								else
									_m_draw_parse_string(parser, false, text_pos, clr, ent.begin, len);
							}
							else if (pos <= a.x && a.x < str_end)
							{	//Partial selected
								int endpos = static_cast<int>(b.x < str_end ? b.x : str_end);
								std::unique_ptr<unsigned> pxbuf_ptr(new unsigned[len]);
								unsigned * pxbuf = pxbuf_ptr.get();
								if (graph_.glyph_pixels(ent.begin, len, pxbuf))
								{
									auto head_w = std::accumulate(pxbuf, pxbuf + (a.x - pos), unsigned());
									auto sel_w = std::accumulate(pxbuf + (a.x - pos), pxbuf + (endpos - pos), unsigned());

									graph_.palette(true, clr);
									if (is_right_text(ent))
									{	//RTL
										rtl_string(text_pos, ent.begin, len, str_w, head_w, sel_w);
									}
									else
									{	//LTR
										_m_draw_parse_string(parser, false, text_pos, clr, ent.begin, a.x - pos);

										auto part_pos = text_pos;
										part_pos.x += static_cast<int>(head_w);

										//Draw selected part
										graph_.rectangle(::nana::rectangle{ part_pos, { sel_w, line_h_pixels } }, true);
										graph_.palette(true, scheme_->selection_text.get_color());
										graph_.string(part_pos, ent.begin + (a.x - pos), endpos - a.x);

										if (static_cast<size_t>(endpos) < str_end)
										{
											part_pos.x += static_cast<int>(sel_w);
											_m_draw_parse_string(parser, false, part_pos, clr, ent.begin + (endpos - pos), str_end - endpos);
										}
									}
								}
							}
							else if (pos <= b.x && b.x < str_end)
							{	//Partial selected
								int endpos = b.x;
								unsigned sel_w = graph_.glyph_extent_size(ent.begin, len, 0, endpos - pos).width;

								if (is_right_text(ent))
								{	//RTL
									graph_.palette(true, clr);
									rtl_string(text_pos, ent.begin, len, str_w, 0, sel_w);
								}
								else
								{	//LTR
									//Draw selected part
									graph_.rectangle(::nana::rectangle{ text_pos, { sel_w, line_h_pixels } }, true);
									graph_.palette(true, scheme_->selection_text.get_color());
									graph_.string(text_pos, ent.begin, endpos - pos);

									_m_draw_parse_string(parser, false, text_pos + ::nana::point(static_cast<int>(sel_w), 0), clr, ent.begin + (endpos - pos), str_end - endpos);
								}
							}
						}
						text_pos.x += static_cast<int>(str_w);
					}//end for
				}
				else if (a.y == str_pos.y)
				{
					for (auto & ent : reordered)
					{
						std::size_t len = ent.end - ent.begin;
						unsigned str_w = graph_.text_extent_size(ent.begin, len).width;
						if ((text_pos.x + static_cast<int>(str_w) > text_area_.area.x) && (text_pos.x < text_right))
						{
							graph_.palette(true, clr);
							std::size_t pos = ent.begin - strbeg + str_pos.x;
							if ((pos + len <= a.x) || (a.x < pos))	//Not selected or selected all
							{
								if (a.x < pos)
								{
									//Draw selected all
									graph_.rectangle(::nana::rectangle{ text_pos, { str_w, line_h_pixels } }, true, static_cast<color_rgb>(0x3399FF));
									graph_.palette(true, scheme_->selection_text.get_color());
									graph_.string(text_pos, ent.begin, len);
								}
								else
									_m_draw_parse_string(parser, false, text_pos, clr, ent.begin, len);
							}
							else
							{
								unsigned head_w = graph_.glyph_extent_size(ent.begin, len, 0, a.x - pos).width;
								if (is_right_text(ent))
								{	//RTL
									rtl_string(text_pos, ent.begin, len, str_w, head_w, str_w - head_w);
								}
								else
								{	//LTR
									graph_.string(text_pos, ent.begin, a.x - pos);

									::nana::point part_pos{ text_pos.x + static_cast<int>(head_w), text_pos.y };

									//Draw selected part
									graph_.rectangle(::nana::rectangle{ part_pos, {str_w - head_w, line_h_pixels } }, true);
									graph_.palette(true, scheme_->selection_text.get_color());
									graph_.string(part_pos, ent.begin + a.x - pos, len - (a.x - pos));
								}
							}
						}

						text_pos.x += static_cast<int>(str_w);
					}

					if (str_pos.y < b.y)
					{
						if (a.y < str_pos.y || ((a.y == str_pos.y) && (a.x <= str_pos.x )))
							graph_.rectangle(::nana::rectangle{ text_pos, { whitespace_w, line_h_pixels } }, true);
					}
				}
				else if (b.y == str_pos.y)
				{
					for (auto & ent : reordered)
					{
						std::size_t len = ent.end - ent.begin;
						unsigned str_w = graph_.text_extent_size(ent.begin, len).width;
						if ((text_pos.x + static_cast<int>(str_w) > text_area_.area.x) && (text_pos.x < text_right))
						{
							std::size_t pos = ent.begin - strbeg + str_pos.x;
							graph_.palette(true, clr);
							if (pos + len <= b.x)
							{
								//Draw selected part
								graph_.rectangle(::nana::rectangle{ text_pos, { str_w, line_h_pixels } }, true);
								graph_.palette(true, scheme_->selection_text.get_color());
								graph_.string(text_pos, ent.begin, len);
							}
							else if (pos <= b.x && b.x < pos + len)
							{
								unsigned sel_w = graph_.glyph_extent_size(ent.begin, len, 0, b.x - pos).width;
								if (is_right_text(ent))
								{	//RTL
									rtl_string(text_pos, ent.begin, len, str_w, 0, sel_w);
								}
								else
								{
									//draw selected part
									graph_.rectangle(::nana::rectangle{ text_pos, { sel_w, line_h_pixels } }, true);
									graph_.palette(true, scheme_->selection_text.get_color());
									graph_.string(text_pos, ent.begin, b.x - pos);

									_m_draw_parse_string(parser, false, text_pos + ::nana::point(static_cast<int>(sel_w), 0), clr, ent.begin + b.x - pos, len - (b.x - pos));
								}
							}
							else
								_m_draw_parse_string(parser, false, text_pos, clr, ent.begin, len);
						}
						text_pos.x += static_cast<int>(str_w);
					}
				}
			}
		}

		bool text_editor::_m_update_caret_line(std::size_t secondary_before)
		{
			if (false == behavior_->adjust_caret_into_screen())
			{
				if (behavior_->caret_to_screen(points_.caret).x < _m_end_pos(true))
				{
					behavior_->update_line(points_.caret.y, secondary_before);
					return false;
				}
			}
			return true;
		}

		bool text_editor::_m_get_sort_select_points(nana::upoint& a, nana::upoint& b) const
		{
			if (select_.a == select_.b)
				return false;

			if((select_.a.y > select_.b.y) || ((select_.a.y == select_.b.y) && (select_.a.x > select_.b.x)))
			{
				a = select_.b;
				b = select_.a;
			}
			else
			{
				a = select_.a;
				b = select_.b;
			}
			return true;
		}

		void text_editor::_m_offset_y(int y)
		{
			points_.offset.y = y;
		}

		unsigned text_editor::_m_char_by_pixels(const wchar_t* str, std::size_t len, unsigned * pxbuf, int str_px, int pixels, bool is_rtl)
		{
			if (graph_.glyph_pixels(str, len, pxbuf))
			{
				if (is_rtl)
				{	//RTL
					for (std::size_t u = 0; u < len; ++u)
					{
						auto px = static_cast<int>(pxbuf[u]);
						auto chbeg = str_px - px;
						if (chbeg <= pixels && pixels < str_px)
						{
							if ((px < 2) || (pixels <= chbeg + (px >> 1)))
								return static_cast<unsigned>(u + 1);

							return static_cast<unsigned>(u);
						}
						str_px = chbeg;
					}
				}
				else
				{
					//LTR
					for (std::size_t u = 0; u < len; ++u)
					{
						auto px = static_cast<int>(pxbuf[u]);
						if (pixels < px)
						{
							if ((px > 1) && (pixels > (px >> 1)))
								return static_cast<unsigned>(u + 1);
							return static_cast<unsigned>(u);
						}
						pixels -= px;
					}
				}
			}
			return 0;
		}

		unsigned text_editor::_m_pixels_by_char(const std::wstring& lnstr, std::size_t pos) const
		{
			if (pos > lnstr.size())
				return 0;

			unicode_bidi bidi;
			std::vector<unicode_bidi::entity> reordered;
			bidi.linestr(lnstr.data(), lnstr.size(), reordered);

			auto ch = lnstr.data() + pos;

			unsigned text_w = 0;
			for (auto & ent : reordered)
			{
				std::size_t len = ent.end - ent.begin;
				if (ent.begin <= ch && ch <= ent.end)
				{
					if (is_right_text(ent))
					{
						//Characters of some bidi languages may transform in a word.
						//RTL
						std::unique_ptr<unsigned[]> pxbuf(new unsigned[len]);
						graph_.glyph_pixels(ent.begin, len, pxbuf.get());
						return std::accumulate(pxbuf.get() + (ch - ent.begin), pxbuf.get() + len, text_w);
					}
					//LTR
					return text_w + _m_text_extent_size(ent.begin, ch - ent.begin).width;
				}
				else
					text_w += _m_text_extent_size(ent.begin, len).width;
			}
			return text_w;
		}

		//end class text_editor
	}//end namespace skeletons
}//end namespace widgets
}//end namespace nana

