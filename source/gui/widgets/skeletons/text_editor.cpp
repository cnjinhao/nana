/*
*	A text editor implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/widgets/skeletons/text_editor.cpp
*	@contributors: Ariel Vina-Rodriguez, Oleg Smolsky
*/
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <nana/gui/widgets/skeletons/textbase_export_interface.hpp>
#include <nana/gui/element.hpp>
#include <nana/system/dataexch.hpp>
#include <nana/unicode_bidi.hpp>
#include <nana/gui/widgets/widget.hpp>
#include "content_view.hpp"

#include <deque>
#include <numeric>
#include <cwctype>
#include <cstring>
#include <algorithm>
#include <map>

namespace nana {
	namespace widgets
	{
		namespace skeletons
		{

			template<typename EnumCommand>
			class undoable_command_interface
			{
			public:
				virtual ~undoable_command_interface() = default;

				virtual EnumCommand get() const = 0;
				virtual bool merge(const undoable_command_interface&) = 0;
				virtual void execute(bool redo) = 0;
			};

			template<typename EnumCommand>
			class undoable
			{
			public:
				using command = EnumCommand;
				using container = std::deque<std::unique_ptr<undoable_command_interface<command>>>;

				void clear() noexcept
				{
					commands_.clear();
					pos_ = 0;
				}

				void max_steps(std::size_t maxs)
				{
					max_steps_ = maxs;
					if (maxs && (commands_.size() >= maxs))
						commands_.erase(commands_.begin(), commands_.begin() + (commands_.size() - maxs + 1));
				}

				std::size_t max_steps() const
				{
					return max_steps_;
				}

				void push(std::unique_ptr<undoable_command_interface<command>> && ptr)
				{
					if (!ptr)
						return;

					if (pos_ < commands_.size())
						commands_.erase(commands_.begin() + pos_, commands_.end());
					else if (max_steps_ && (commands_.size() >= max_steps_))
						commands_.erase(commands_.begin(), commands_.begin() + (commands_.size() - max_steps_ + 1));

					pos_ = commands_.size();
					if (!commands_.empty())
					{
						if (commands_.back().get()->merge(*ptr))
							return;
					}

					commands_.emplace_back(std::move(ptr));
					++pos_;
				}

				std::size_t count(bool is_undo) const noexcept
				{
					return (is_undo ? pos_ : commands_.size() - pos_);
				}

				void undo()
				{
					if (pos_ > 0)
						commands_[--pos_].get()->execute(false);
				}

				void redo()
				{
					if (pos_ != commands_.size())
						commands_[pos_++].get()->execute(true);
				}

			private:
				container commands_;
				std::size_t max_steps_{ 30 };
				std::size_t pos_{ 0 };
			};

			template<typename T>
			using undo_command_ptr = std::unique_ptr <undoable_command_interface<T>>;

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
					//sel_a_ and sel_b_ are not sorted, sel_b_ keeps the caret position.
					sel_a_ = editor_.select_.a;
					sel_b_ = editor_.select_.b;

					if (sel_a_ != sel_b_)
					{
						selected_text_ = editor_._m_make_select_string();
					}
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

				virtual bool merge(const undoable_command_interface<EnumCommand>&) override
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
					: basic_undoable<command>(editor, command::backspace)
				{
				}

				void set_removed(std::wstring str)
				{
					selected_text_ = std::move(str);
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
							editor_._m_erase_select(false);
							editor_.select_.a = editor_.select_.b;
							editor_.points_.caret = sel_a_;
						}
						else
						{
							if (is_enter)
							{
								editor_.points_.caret = nana::upoint(0, pos_.y + 1);
								editor_.backspace(false, false);
							}
							else
								editor_.textbase().erase(pos_.y, pos_.x, selected_text_.size());
						}
					}
					else
					{
						if (is_enter)
						{
							editor_.enter(false, false);
						}
						else
						{
							editor_._m_put(selected_text_, false);
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

					editor_.textbase().text_changed();

					editor_.reset_caret();
				}
			};

			class text_editor::undo_input_text
				: public basic_undoable <command>
			{
			public:
				undo_input_text(text_editor & editor, const std::wstring& text)
					: basic_undoable<command>(editor, command::input_text),
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
							editor_.enter(false, false);
						}
						else
						{
							if (!selected_text_.empty())
							{
								editor_.select_.a = sel_a_;
								editor_.select_.b = sel_b_;
								editor_._m_erase_select(false);
							}
							editor_.points_.caret = editor_._m_put(text_, false); //redo
						}
					}
					else
					{
						if (is_enter)
						{
							editor_.points_.caret.x = 0;
							++editor_.points_.caret.y;
							editor_.backspace(false, false);
						}
						else
						{
							std::vector<std::pair<std::size_t, std::size_t>> lines;
							if (editor_._m_resolve_text(text_, lines))
							{
								editor_.select_.a = pos_;
								editor_.select_.b = upoint(static_cast<unsigned>(lines.back().second - lines.back().first), static_cast<unsigned>(pos_.y + lines.size() - 1));
								editor_.backspace(false, false);
								editor_.select_.a = editor_.select_.b;
							}
							else
								editor_.textbase().erase(pos_.y, pos_.x, text_.size());	//undo
						}

						if (!selected_text_.empty())
						{
							editor_.points_.caret = (std::min)(sel_a_, sel_b_);
							editor_._m_put(selected_text_, false);
							editor_.points_.caret = sel_b_;
							editor_.select_.a = sel_a_;	//Reset the selected text
							editor_.select_.b = sel_b_;
						}
					}

					editor_.textbase().text_changed();
					editor_.reset_caret();
				}
			private:
				std::wstring text_;
			};

			class text_editor::undo_move_text
				: public basic_undoable <command>
			{
			public:
				undo_move_text(text_editor& editor)
					: basic_undoable<command>(editor, command::move_text)
				{}

				void execute(bool redo) override
				{
					if (redo)
					{
						editor_.select_.a = sel_a_;
						editor_.select_.b = sel_b_;
						editor_.points_.caret = pos_;
						editor_._m_move_select(false);
					}
					else
					{
						editor_.select_.a = dest_a_;
						editor_.select_.b = dest_b_;
						editor_.points_.caret = (sel_a_ < sel_b_ ? sel_a_ : sel_b_);

						const auto text = editor_._m_make_select_string();

						editor_._m_erase_select(false);
						editor_._m_put(text, false);

						editor_.select_.a = sel_a_;
						editor_.select_.b = sel_b_;

						editor_.points_.caret = sel_b_;
						editor_.reset_caret();
					}
					editor_.textbase().text_changed();
				}

				void set_destination(const nana::upoint& dest_a, const nana::upoint& dest_b)
				{
					dest_a_ = dest_a;
					dest_b_ = dest_b;
				}
			private:
				nana::upoint dest_a_, dest_b_;
			};

			struct text_editor::text_section
			{
				const wchar_t* begin{ nullptr };
				const wchar_t* end{ nullptr };
				unsigned pixels{ 0 };

				text_section() = default;
				text_section(const wchar_t* ptr, const wchar_t* endptr, unsigned px)
					: begin(ptr), end(endptr), pixels(px)
				{}
			};


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

			struct entity
			{
				const wchar_t* begin;
				const wchar_t* end;
				const keyword_scheme * scheme;
			};

			enum class sync_graph
			{
				none,
				refresh,
				lazy_refresh
			};

			colored_area_access_interface::~colored_area_access_interface() {}

			class colored_area_access
				: public colored_area_access_interface
			{
			public:
				void set_window(window handle)
				{
					window_handle_ = handle;
				}

				std::shared_ptr<colored_area_type> find(std::size_t pos) const
				{
					for (auto & sp : colored_areas_)
					{
						if (sp->begin <= pos && pos < sp->begin + sp->count)
							return sp;
						else if (sp->begin > pos)
							break;
					}
					return{};
				}
			public:
				//Overrides methods of colored_area_access_interface
				std::shared_ptr<colored_area_type> get(std::size_t line_pos) override
				{
#ifdef _MSC_VER
					auto i = colored_areas_.cbegin();
					for (; i != colored_areas_.cend(); ++i)
#else
					auto i = colored_areas_.begin();
					for (; i != colored_areas_.end(); ++i)
#endif
					{
						auto & area = *(i->get());
						if (area.begin <= line_pos && line_pos < area.begin + area.count)
							return *i;

						if (area.begin > line_pos)
							break;
					}

					return *colored_areas_.emplace(i,
						std::make_shared<colored_area_type>(colored_area_type{ line_pos, 1, color{}, color{} })
					);
				}

				bool clear() override
				{
					if (colored_areas_.empty())
						return false;

					colored_areas_.clear();
					API::refresh_window(window_handle_);
					return true;
				}

				bool remove(std::size_t pos) override
				{
					bool changed = false;
#ifdef _MSC_VER
					for (auto i = colored_areas_.cbegin(); i != colored_areas_.cend();)
#else
					for (auto i = colored_areas_.begin(); i != colored_areas_.end();)
#endif
					{
						if (i->get()->begin <= pos && pos < i->get()->begin + i->get()->count)
						{
							i = colored_areas_.erase(i);
							changed = true;
						}
						else if (i->get()->begin > pos)
							break;
					}
					if (changed)
						API::refresh_window(window_handle_);

					return changed;
				}

				std::size_t size() const override
				{
					return colored_areas_.size();
				}

				std::shared_ptr<colored_area_type> at(std::size_t index) override
				{
					return colored_areas_.at(index);
				}
			private:
				window window_handle_;
				std::vector<std::shared_ptr<colored_area_type>> colored_areas_;
			};

			struct text_editor::implementation
			{
				undoable<command>	undo;			//undo command
				renderers			customized_renderers;
				std::vector<upoint> text_position;	//positions of text since last rendering.
				int text_position_origin{ -1 };	//origin when last text_exposed

				skeletons::textbase<wchar_t> textbase;

				sync_graph try_refresh{ sync_graph::none };

				colored_area_access colored_area;

				struct inner_capacities
				{
					editor_behavior_interface * behavior;

					accepts acceptive{ accepts::no_restrict };
					std::function<bool(char_type)> pred_acceptive;
				}capacities;

				struct inner_counterpart
				{
					bool enabled{ false };
					paint::graphics buffer;	//A offscreen buffer which keeps the background that painted by external part.
				}counterpart;

				struct indent_rep
				{
					bool enabled{ false };
					std::function<std::string()> generator;
				}indent;

				struct inner_keywords
				{
					std::map<std::string, std::shared_ptr<keyword_scheme>> schemes;
					std::deque<keyword_desc> base;
				}keywords;

				std::unique_ptr<content_view> cview;
			};


			class text_editor::editor_behavior_interface
			{
			public:
				using row_coordinate = std::pair<std::size_t, std::size_t>; ///< A coordinate type for line position. first: the absolute line position of text. second: the secondary line position of a part of line.

				virtual ~editor_behavior_interface() = default;

				/// Returns the text sections of a specified line
				/**
				* @param pos The absolute line number.
				* @return The text sections of this line.
				*/
				virtual std::vector<text_section> line(std::size_t pos) const = 0;
				virtual row_coordinate text_position_from_screen(int top) const = 0;

				virtual unsigned max_pixels() const = 0;

				/// Deletes lines between first and second, and then, second line will be merged into first line.
				virtual void merge_lines(std::size_t first, std::size_t second) = 0;
				//Calculates how many lines the specified line of text takes with a specified pixels of width.
				virtual void add_lines(std::size_t pos, std::size_t lines) = 0;
				virtual void prepare() = 0;
				virtual void pre_calc_line(std::size_t line, unsigned pixels) = 0;
				virtual void pre_calc_lines(unsigned pixels) = 0;
				virtual std::size_t take_lines() const = 0;
				/// Returns the number of lines that the line of text specified by pos takes.
				virtual std::size_t take_lines(std::size_t pos) const = 0;
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

				std::vector<text_section> line(std::size_t pos) const override
				{
					//Every line of normal behavior only has one text_section
					std::vector<text_section> sections;
					sections.emplace_back(this->sections_[pos]);
					return sections;
				}

				row_coordinate text_position_from_screen(int top) const override
				{
					const std::size_t textlines = editor_.textbase().lines();
					const auto line_px = static_cast<int>(editor_.line_height());
					if ((0 == textlines) || (0 == line_px))
						return{};

					if (top < editor_.text_area_.area.y)
						top = (std::max)(editor_._m_text_topline() - 1, 0);
					else
						top = (top - editor_.text_area_.area.y + editor_.impl_->cview->origin().y) / line_px;

					return{ (textlines <= static_cast<std::size_t>(top) ? textlines - 1 : static_cast<std::size_t>(top)),
						0 };
				}

				unsigned max_pixels() const override
				{
					unsigned px = editor_.width_pixels();
					for (auto & sct : sections_)
					{
						if (sct.pixels > px)
							px = sct.pixels;
					}
					return px;
				}

				void merge_lines(std::size_t first, std::size_t second) override
				{
					if (first > second)
						std::swap(first, second);

					if (second < this->sections_.size())
#ifdef _MSC_VER
						this->sections_.erase(this->sections_.cbegin() + (first + 1), this->sections_.cbegin() + second);
#else
						this->sections_.erase(this->sections_.begin() + (first + 1), this->sections_.begin() + second);
#endif
					pre_calc_line(first, 0);

					//textbase is implement by using deque, and the linemtr holds the text pointers
					//If the textbase is changed, it will check the text pointers.
					std::size_t line = 0;

					auto const & const_sections = sections_;
					for (auto & sct : const_sections)
					{
						auto const& text = editor_.textbase().getline(line);
						if (sct.begin < text.c_str() || (text.c_str() + text.size() < sct.begin))
							pre_calc_line(line, 0);

						++line;
					}
				}

				void add_lines(std::size_t pos, std::size_t line_size) override
				{
					if (pos < this->sections_.size())
					{
						for (std::size_t i = 0; i < line_size; ++i)
#ifdef _MSC_VER
							this->sections_.emplace(this->sections_.cbegin() + (pos + i));
#else
							this->sections_.emplace(this->sections_.begin() + (pos + i));
#endif
						//textbase is implement by using deque, and the linemtr holds the text pointers
						//If the textbase is changed, it will check the text pointers.
						std::size_t line = 0;

						auto const & const_sections = sections_;
						for (auto & sct : const_sections)
						{
							if (line < pos || (pos + line_size) <= line)
							{
								auto const & text = editor_.textbase().getline(line);
								if (sct.begin < text.c_str() || (text.c_str() + text.size() < sct.begin))
									pre_calc_line(line, 0);
							}
							++line;
						}
					}
				}

				void prepare() override
				{
					auto const line_count = editor_.textbase().lines();
					this->sections_.resize(line_count);
				}

				void pre_calc_line(std::size_t pos, unsigned) override
				{
					auto const & text = editor_.textbase().getline(pos);
					auto& txt_section = this->sections_[pos];
					txt_section.begin = text.c_str();
					txt_section.end = txt_section.begin + text.size();
					txt_section.pixels = editor_._m_text_extent_size(txt_section.begin, text.size()).width;
				}

				void pre_calc_lines(unsigned) override
				{
					auto const line_count = editor_.textbase().lines();
					this->sections_.resize(line_count);
					for (std::size_t i = 0; i < line_count; ++i)
						pre_calc_line(i, 0);
				}

				std::size_t take_lines() const override
				{
					return editor_.textbase().lines();
				}

				std::size_t take_lines(std::size_t) const override
				{
					return 1;
				}
			private:
				text_editor& editor_;
				std::vector<text_section> sections_;
			}; //end class behavior_normal


			class text_editor::behavior_linewrapped
				: public text_editor::editor_behavior_interface
			{
				struct line_metrics
				{
					std::size_t		take_lines;	//The number of lines that text of this line takes.
					std::vector<text_section>	line_sections;
				};
			public:
				behavior_linewrapped(text_editor& editor)
					: editor_(editor)
				{}

				std::vector<text_section> line(std::size_t pos) const override
				{
					return linemtr_[pos].line_sections;
				}

				row_coordinate text_position_from_screen(int top) const override
				{
					row_coordinate coord;
					const auto line_px = static_cast<int>(editor_.line_height());

					if ((0 == editor_.textbase().lines()) || (0 == line_px))
						return coord;

					auto text_row = (std::max)(0, (top - editor_.text_area_.area.y + editor_.impl_->cview->origin().y) / line_px);

					coord = _m_textline(static_cast<std::size_t>(text_row));
					if (linemtr_.size() <= coord.first)
					{
						coord.first = linemtr_.size() - 1;
						coord.second = linemtr_.back().line_sections.size() - 1;
					}
					return coord;
				}

				unsigned max_pixels() const override
				{
					return editor_.width_pixels();
				}

				void merge_lines(std::size_t first, std::size_t second) override
				{
					if (first > second)
						std::swap(first, second);

					if (second < linemtr_.size())
						linemtr_.erase(linemtr_.begin() + first + 1, linemtr_.begin() + second + 1);

					pre_calc_line(first, editor_.width_pixels());
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

						auto const & const_linemtr = linemtr_;
						for (auto & mtr : const_linemtr)
						{
							if (line < pos || (pos + lines) <= line)
							{
								auto & linestr = editor_.textbase().getline(line);
								auto p = mtr.line_sections.front().begin;
								if (p < linestr.c_str() || (linestr.c_str() + linestr.size() < p))
									pre_calc_line(line, editor_.width_pixels());
							}
							++line;
						}
					}
				}

				void prepare() override
				{
					auto const lines = editor_.textbase().lines();
					linemtr_.resize(lines);
				}

				void pre_calc_line(std::size_t line, unsigned pixels) override
				{
					const string_type& lnstr = editor_.textbase().getline(line);
					if (lnstr.empty())
					{
						auto & mtr = linemtr_[line];
						mtr.line_sections.clear();

						mtr.line_sections.emplace_back(lnstr.c_str(), lnstr.c_str(), unsigned{});
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
						if (text_px >= pixels)
						{
							if (text_px != str_w)
							{
								line_sections.emplace_back(secondary_begin, ts.begin, unsigned{ text_px - str_w });
								text_px = str_w;
								secondary_begin = ts.begin;
							}

							if (str_w > pixels)	//Indicates the splitting of ts string
							{
								std::size_t len = ts.end - ts.begin;
#ifdef _nana_std_has_string_view
								auto pxbuf = editor_.graph_.glyph_pixels({ ts.begin, len });
#else
								std::unique_ptr<unsigned[]> pxbuf(new unsigned[len]);
								editor_.graph_.glyph_pixels(ts.begin, len, pxbuf.get());
#endif

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
									line_sections.emplace_back(secondary_begin, endptr, unsigned{ text_px - (text_px == pixels ? 0 : *pxi) });
									secondary_begin = endptr;

									text_px = (text_px == pixels ? 0 : *pxi);
								}
							}
							continue;
						}
						else if (text_px == pixels)
						{
							line_sections.emplace_back(secondary_begin, ts.begin, unsigned{ text_px - str_w });
							secondary_begin = ts.begin;
							text_px = str_w;
						}
					}

					auto & mtr = linemtr_[line];

					mtr.take_lines = line_sections.size();
					mtr.line_sections.swap(line_sections);

					if (secondary_begin)
					{
						mtr.line_sections.emplace_back(secondary_begin, sections.back().end, unsigned{ text_px });
						++mtr.take_lines;
					}
				}

				void pre_calc_lines(unsigned pixels) override
				{
					auto const lines = editor_.textbase().lines();
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
			private:
				/// Split a text into multiple sections, a section indicates an english word or a CKJ character
				void _m_text_section(const std::wstring& str, std::vector<text_section>& tsec)
				{
					if (str.empty())
					{
						tsec.emplace_back(str.c_str(), str.c_str(), unsigned{});
						return;
					}
					const auto end = str.c_str() + str.size();

					const wchar_t * word = nullptr;
					for (auto i = str.c_str(); i != end; ++i)
					{
						wchar_t const ch = *i;

						//CKJ characters and whitespace
						if (' ' == ch || '\t' == ch || (0x4E00 <= ch && ch <= 0x9FCF))
						{
							if (word)	//Record the word.
							{
								tsec.emplace_back(word, i, unsigned{});
								word = nullptr;
							}

							tsec.emplace_back(i, i + 1, unsigned{});
							continue;
						}

						if (nullptr == word)
							word = i;
					}

					if (word)
						tsec.emplace_back(word, end, unsigned{});
				}

				row_coordinate _m_textline(std::size_t scrline) const
				{
					row_coordinate coord;
					for (auto & mtr : linemtr_)
					{
						if (mtr.take_lines > scrline)
						{
							coord.second = scrline;
							return coord;
						}
						else
							scrline -= mtr.take_lines;

						++coord.first;
					}
					return coord;
				}
			private:
				text_editor& editor_;
				std::vector<line_metrics> linemtr_;
			}; //end class behavior_linewrapped

			class text_editor::keyword_parser
			{
			public:
				void parse(const wchar_t* c_str, std::size_t len, implementation::inner_keywords& keywords) //need string_view
				{
					if (keywords.base.empty() || (0 == len) || (*c_str == 0))
						return;

					std::wstring text{ c_str, len };

					using index = std::wstring::size_type;

					std::vector<entity> entities;

					::nana::ciwstring cistr;
					for (auto & ds : keywords.base)
					{
						index pos{ 0 };
						for (index rest{ text.size() }; rest >= ds.text.size(); ++pos, rest = text.size() - pos)
						{
							if (ds.case_sensitive)
							{
								pos = text.find(ds.text, pos);
								if (pos == text.npos)
									break;
							}
							else
							{
								if (cistr.empty())
									cistr.append(text.c_str(), text.size());

								pos = cistr.find(ds.text.c_str(), pos);
								if (pos == cistr.npos)
									break;
							}

							if (ds.whole_word_matched && (!_m_whole_word(text, pos, ds.text.size())))
								continue;

							auto ki = keywords.schemes.find(ds.scheme);
							if ((ki != keywords.schemes.end()) && ki->second)
							{
#ifdef _nana_std_has_emplace_return_type
								auto & last = entities.emplace_back();
#else
								entities.emplace_back();
								auto & last = entities.back();
#endif
								last.begin = c_str + pos;
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
								i = entities.erase(i);  // erase overlapping. Left only the first.
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
				static bool _m_whole_word(const std::wstring& text, std::wstring::size_type pos, std::size_t len)
				{
					if (pos)
					{
						auto chr = text[pos - 1];
						if ((std::iswalpha(chr) && !std::iswspace(chr)) || chr == '_')
							return false;
					}

					if (pos + len < text.size())
					{
						auto chr = text[pos + len];
						if ((std::iswalpha(chr) && !std::iswspace(chr)) || chr == '_')
							return false;
					}

					return true;
				}
			private:
				std::vector<entity> entities_;
			};

			//class text_editor

			text_editor::text_editor(window wd, graph_reference graph, const text_editor_scheme* schm):
				impl_(new implementation),
				window_(wd),
				graph_(graph),
				scheme_(schm)
			{
				impl_->capacities.behavior = new behavior_normal(*this);

				text_area_.area.dimension(graph.size());

				impl_->cview.reset(new content_view{ wd });
				impl_->cview->disp_area(text_area_.area, {}, {}, {});
				impl_->cview->events().scrolled = [this] {
					this->reset_caret();
				};

				impl_->cview->events().hover_outside = [this](const point& pos) {
					mouse_caret(pos, false);
					if (selection::mode::mouse_selected == select_.mode_selection || selection::mode::method_selected == select_.mode_selection)
						set_end_caret(false);
				};

				API::create_caret(wd, { 1, line_height() });
				API::bgcolor(wd, colors::white);
				API::fgcolor(wd, colors::black);
			}

			text_editor::~text_editor()
			{
				//For instance of unique_ptr pimpl idiom.

				delete impl_->capacities.behavior;
				delete impl_;
			}

			size text_editor::caret_size() const
			{
				return { 1, line_height() };
			}

			const point& text_editor::content_origin() const
			{
				return impl_->cview->origin();
			}

			void text_editor::set_highlight(const std::string& name, const ::nana::color& fgcolor, const ::nana::color& bgcolor)
			{
				if (fgcolor.invisible() && bgcolor.invisible())
				{
					impl_->keywords.schemes.erase(name);
					return;
				}

				auto sp = std::make_shared<keyword_scheme>();
				sp->fgcolor = fgcolor;
				sp->bgcolor = bgcolor;
				impl_->keywords.schemes[name].swap(sp);
			}

			void text_editor::erase_highlight(const std::string& name)
			{
				impl_->keywords.schemes.erase(name);
			}

			void text_editor::set_keyword(const ::std::wstring& kw, const std::string& name, bool case_sensitive, bool whole_word_matched)
			{
				for (auto & ds : impl_->keywords.base)
				{
					if (ds.text == kw)
					{
						ds.scheme = name;
						ds.case_sensitive = case_sensitive;
						ds.whole_word_matched = whole_word_matched;
						return;
					}
				}

				impl_->keywords.base.emplace_back(kw, name, case_sensitive, whole_word_matched);
			}

			void text_editor::erase_keyword(const ::std::wstring& kw)
			{
				for (auto i = impl_->keywords.base.begin(); i != impl_->keywords.base.end(); ++i)
				{
					if (kw == i->text)
					{
						impl_->keywords.base.erase(i);
						return;
					}
				}
			}

			colored_area_access_interface& text_editor::colored_area()
			{
				return impl_->colored_area;
			}

			void text_editor::set_accept(std::function<bool(char_type)> pred)
			{
				impl_->capacities.pred_acceptive = std::move(pred);
			}

			void text_editor::set_accept(accepts acceptive)
			{
				impl_->capacities.acceptive = acceptive;
			}

			bool text_editor::respond_char(const arg_keyboard& arg)	//key is a character of ASCII code
			{
				if (!API::window_enabled(window_))
					return false;

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

				if (attributes_.editable && (!impl_->capacities.pred_acceptive || impl_->capacities.pred_acceptive(key)))
				{
					switch (key)
					{
					case '\b':
						backspace(true, true);	break;
					case '\n': case '\r':
						enter(true, true);	break;
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
					impl_->try_refresh = sync_graph::refresh;
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
					_m_handle_move_key(arg);
					break;
				case keyboard::del:
					// send delete to set_accept function
					if (this->attr().editable && (!impl_->capacities.pred_acceptive || impl_->capacities.pred_acceptive(key)))
						del();
					break;
				default:
					return false;
				}
				impl_->try_refresh = sync_graph::refresh;
				return true;
			}

			void text_editor::typeface_changed()
			{
				_m_reset_content_size(true);
			}

			void text_editor::indent(bool enb, std::function<std::string()> generator)
			{
				impl_->indent.enabled = enb;
				impl_->indent.generator = std::move(generator);
			}

			void text_editor::set_event(event_interface* ptr)
			{
				event_handler_ = ptr;
			}

			bool text_editor::load(const path_type& fs)
			{
				if (!impl_->textbase.load(fs))
					return false;

				_m_reset();

				impl_->try_refresh = sync_graph::refresh;
				_m_reset_content_size(true);
				return true;
			}

			void text_editor::text_align(::nana::align alignment)
			{
				this->attributes_.alignment = alignment;
				this->reset_caret();
				impl_->try_refresh = sync_graph::refresh;
				_m_reset_content_size();
			}

			bool text_editor::text_area(const nana::rectangle& r)
			{
				if (text_area_.area == r)
					return false;

				text_area_.area = r;

				if (impl_->counterpart.enabled)
					impl_->counterpart.buffer.make(r.dimension());

				impl_->cview->disp_area(r, { -1, 1 }, { 1, -1 }, { 2, 2 });
				if (impl_->cview->content_size().empty() || this->attributes_.line_wrapped)
					_m_reset_content_size(true);

				reset_caret();
				return true;
			}

			rectangle text_editor::text_area(bool including_scroll) const
			{
				return (including_scroll ? impl_->cview->view_area() : text_area_.area);
			}

			bool text_editor::tip_string(::std::string&& str)
			{
				if (attributes_.tip_string == str)
					return false;

				attributes_.tip_string = std::move(str);
				return true;
			}

			const text_editor::attributes& text_editor::attr() const noexcept
			{
				return attributes_;
			}

			bool text_editor::line_wrapped(bool autl)
			{
				if (autl != attributes_.line_wrapped)
				{
					attributes_.line_wrapped = autl;

					delete impl_->capacities.behavior;
					if (autl)
						impl_->capacities.behavior = new behavior_linewrapped(*this);
					else
						impl_->capacities.behavior = new behavior_normal(*this);

					_m_reset_content_size(true);

					impl_->cview->move_origin(point{} -impl_->cview->origin());
					move_caret(upoint{});

					impl_->try_refresh = sync_graph::refresh;
					return true;
				}
				return false;
			}

			bool text_editor::multi_lines(bool ml)
			{
				if ((ml == false) && attributes_.multi_lines)
				{
					//retain the first line and remove the extra lines
					if (impl_->textbase.erase(1, impl_->textbase.lines() - 1))
						_m_reset();
				}

				if (attributes_.multi_lines == ml)
					return false;

				attributes_.multi_lines = ml;

				if (!ml)
					line_wrapped(false);

				_m_reset_content_size();
				impl_->cview->enable_scrolls(ml ? content_view::scrolls::both : content_view::scrolls::none);
				impl_->cview->move_origin(point{} -impl_->cview->origin());

				impl_->try_refresh = sync_graph::refresh;
				return true;
			}

			void text_editor::editable(bool enable, bool enable_caret)
			{
				attributes_.editable = enable;
				attributes_.enable_caret = (enable || enable_caret);
			}

			void text_editor::enable_background(bool enb)
			{
				attributes_.enable_background = enb;
			}

			void text_editor::enable_background_counterpart(bool enb)
			{
				impl_->counterpart.enabled = enb;
				if (enb)
					impl_->counterpart.buffer.make(text_area_.area.dimension());
				else
					impl_->counterpart.buffer.release();
			}

			void text_editor::undo_clear()
			{
				auto size = this->undo_max_steps();
				impl_->undo.max_steps(0);
				impl_->undo.max_steps(size);
			}

			void text_editor::undo_max_steps(std::size_t maxs)
			{
				impl_->undo.max_steps(maxs);
			}

			std::size_t text_editor::undo_max_steps() const
			{
				return impl_->undo.max_steps();
			}

			auto text_editor::customized_renderers() -> renderers&
			{
				return impl_->customized_renderers;
			}

			unsigned text_editor::line_height() const
			{
				unsigned ascent, descent, internal_leading;
				if (!graph_.text_metrics(ascent, descent, internal_leading))
					return 0;

				impl_->cview->step(ascent + descent, false);
				return ascent + descent;
			}

			unsigned text_editor::screen_lines(bool completed_line) const
			{
				auto const line_px = line_height();
				if (line_px)
				{
					auto h = impl_->cview->view_area().height;
					return (h / line_px) + ((completed_line || !(h % line_px)) ? 0 : 1);
				}
				return 0;
			}

			bool text_editor::focus_changed(const arg_focus& arg)
			{
				bool renderred = false;

				if (arg.getting && (select_.a == select_.b)) //Do not change the selected text
				{
					bool select_all = false;
					switch (select_.behavior)
					{
					case text_focus_behavior::select:
						select_all = true;
						break;
					case text_focus_behavior::select_if_click:
						select_all = (arg_focus::reason::mouse_press == arg.focus_reason);
						break;
					case text_focus_behavior::select_if_tabstop:
						select_all = (arg_focus::reason::tabstop == arg.focus_reason);
						break;
					case text_focus_behavior::select_if_tabstop_or_click:
						select_all = (arg_focus::reason::tabstop == arg.focus_reason || arg_focus::reason::mouse_press == arg.focus_reason);
					default:
						break;
					}

					if (select_all)
					{
						select(true);
						move_caret_end(false);
						renderred = true;

						//If the text widget is focused by clicking mouse button, the selected text will be cancelled
						//by the subsequent mouse down event. In this situation, the subsequent mouse down event should
						//be ignored.
						select_.ignore_press = (arg_focus::reason::mouse_press == arg.focus_reason);
					}
				}
				show_caret(arg.getting);
				reset_caret();
				return renderred;
			}

			bool text_editor::mouse_enter(bool entering)
			{
				if ((false == entering) && (false == text_area_.captured))
					API::window_cursor(window_, nana::cursor::arrow);

				return false;
			}

			bool text_editor::mouse_move(bool left_button, const point& scrpos)
			{
				cursor cur = cursor::iterm;
				if (((!hit_text_area(scrpos)) && (!text_area_.captured)) || !attributes_.enable_caret || !API::window_enabled(window_))
					cur = cursor::arrow;

				API::window_cursor(window_, cur);

				if (!attributes_.enable_caret)
					return false;

				if (left_button)
				{
					mouse_caret(scrpos, false);

					if (selection::mode::mouse_selected == select_.mode_selection || selection::mode::method_selected == select_.mode_selection)
						set_end_caret(false);
					else if (selection::mode::move_selected == select_.mode_selection)
						select_.mode_selection = selection::mode::move_selected_take_effect;

					impl_->try_refresh = sync_graph::refresh;
					return true;
				}
				return false;
			}

			void text_editor::mouse_pressed(const arg_mouse& arg)
			{
				if (!attributes_.enable_caret)
					return;

				if (event_code::mouse_down == arg.evt_code)
				{
					if (select_.ignore_press || (!hit_text_area(arg.pos)))
					{
						select_.ignore_press = false;
						return;
					}

					if (::nana::mouse::left_button == arg.button)
					{
						API::set_capture(window_, true);
						text_area_.captured = true;

						if (this->hit_select_area(_m_coordinate_to_caret(arg.pos), true))
						{
							//The selected of text can be moved only if it is editable
							if (attributes_.editable)
								select_.mode_selection = selection::mode::move_selected;
						}
						else
						{
							//Set caret pos by screen point and get the caret pos.
							mouse_caret(arg.pos, true);
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
									set_end_caret(true);
								}
								points_.shift_begin_caret = points_.caret;
							}
							select_.mode_selection = selection::mode::mouse_selected;
						}
					}

					impl_->try_refresh = sync_graph::refresh;
				}
				else if (event_code::mouse_up == arg.evt_code)
				{
					select_.ignore_press = false;

					if (select_.mode_selection == selection::mode::mouse_selected)
					{
						select_.mode_selection = selection::mode::no_selected;
						set_end_caret(true);
					}
					else if (selection::mode::move_selected == select_.mode_selection || selection::mode::move_selected_take_effect == select_.mode_selection)
					{
						//move_selected indicates the mouse is pressed on the selected text, but the mouse has not moved. So the text_editor should cancel the selection.
						//move_selected_take_effect indicates the text_editor should try to move the selection.

						if ((selection::mode::move_selected == select_.mode_selection) || !move_select())
						{
							//no move occurs
							select(false);
							move_caret(_m_coordinate_to_caret(arg.pos));
						}

						select_.mode_selection = selection::mode::no_selected;
						impl_->try_refresh = sync_graph::refresh;
					}

					API::release_capture(window_);

					text_area_.captured = false;
					if (hit_text_area(arg.pos) == false)
						API::window_cursor(window_, nana::cursor::arrow);
				}
			}

			//Added Windows-style mouse double-click to the textbox(https://github.com/cnjinhao/nana/pull/229)
			//Oleg Smolsky
			bool text_editor::select_word(const arg_mouse& arg)
			{
				if (!attributes_.enable_caret)
					return false;

				// Set caret pos by screen point and get the caret pos.
				mouse_caret(arg.pos, true);

				// Set the initial selection: it is an empty range.
				select_.a = select_.b = points_.caret;
				const auto& line = impl_->textbase.getline(select_.b.y);



				if (select_.a.x < line.size() && !std::isalnum(line[select_.a.x]) && line[select_.a.x] != '_') {
					++select_.b.x;
				}
				else {
					// Expand the selection forward to the word's end.
					while (select_.b.x < line.size() && !std::iswspace(line[select_.b.x]) && (std::isalnum(line[select_.b.x]) || line[select_.b.x] == '_'))
						++select_.b.x;

					// Expand the selection backward to the word's start.
					while (select_.a.x > 0 && !std::iswspace(line[select_.a.x - 1]) && (std::isalnum(line[select_.a.x - 1]) || line[select_.a.x - 1] == '_'))
						--select_.a.x;
				}
				select_.mode_selection = selection::mode::method_selected;
				impl_->try_refresh = sync_graph::refresh;
				return true;
			}

			textbase<text_editor::char_type> & text_editor::textbase() noexcept
			{
				return impl_->textbase;
			}

			const textbase<text_editor::char_type> & text_editor::textbase() const noexcept
			{
				return impl_->textbase;
			}

			bool text_editor::try_refresh()
			{
				if (sync_graph::none != impl_->try_refresh)
				{
					if (sync_graph::refresh == impl_->try_refresh)
						render(API::is_focus_ready(window_));

					impl_->try_refresh = sync_graph::none;
					return true;
				}
				return false;
			}

			bool text_editor::getline(std::size_t pos, std::wstring& text) const
			{
				if (impl_->textbase.lines() <= pos)
					return false;

				text = impl_->textbase.getline(pos);
				return true;
			}

			void text_editor::text(std::wstring str, bool end_caret)
			{
				impl_->undo.clear();

				impl_->textbase.erase_all();
				_m_reset();
				_m_reset_content_size(true);

				if (!end_caret)
				{
					auto undo_ptr = std::unique_ptr<undo_input_text>{ new undo_input_text(*this, str) };
					undo_ptr->set_caret_pos();

					_m_put(std::move(str), false);

					impl_->undo.push(std::move(undo_ptr));

					if (graph_)
					{
						this->_m_adjust_view();

						reset_caret();
						impl_->try_refresh = sync_graph::refresh;

						//_m_put calcs the lines
						_m_reset_content_size(true);
						impl_->cview->sync(false);
					}
				}
				else
					put(std::move(str), false);

				textbase().text_changed();
			}

			std::wstring text_editor::text() const
			{
				std::wstring str;
				std::size_t lines = impl_->textbase.lines();
				if (lines > 0)
				{
					str = impl_->textbase.getline(0);
					for (std::size_t i = 1; i < lines; ++i)
					{
						str += L"\r\n";
						str += impl_->textbase.getline(i);
					}
				}
				return str;
			}

			bool text_editor::move_caret(upoint crtpos, bool stay_in_view)
			{
				const unsigned line_pixels = line_height();

				if (crtpos != points_.caret)
				{
					//Check and make the crtpos available
					if (crtpos.y < impl_->textbase.lines())
					{
						crtpos.x = (std::min)(static_cast<decltype(crtpos.x)>(impl_->textbase.getline(crtpos.y).size()), crtpos.x);
					}
					else
					{
						crtpos.y = static_cast<unsigned>(impl_->textbase.lines());
						if (crtpos.y > 0)
							--crtpos.y;

						crtpos.x = static_cast<unsigned>(impl_->textbase.getline(crtpos.y).size());
					}

					points_.caret = crtpos;
				}

				//The coordinate of caret
				auto coord = _m_caret_to_coordinate(crtpos);

				const int line_bottom = coord.y + static_cast<int>(line_pixels);

				if (!API::is_focus_ready(window_))
					return false;

				auto caret = API::open_caret(window_, true);

				bool visible = false;
				auto text_area = impl_->cview->view_area();

				if (text_area.is_hit(coord) && (line_bottom > text_area.y))
				{
					visible = true;
					if (line_bottom > text_area.bottom())
						caret->dimension(nana::size(1, line_pixels - (line_bottom - text_area.bottom())));
					else if (caret->dimension().height != line_pixels)
						reset_caret_pixels();
				}

				if (!attributes_.enable_caret)
					visible = false;

				caret->visible(visible);
				if (visible)
					caret->position(coord);

				//Adjust the caret into screen when the caret position is modified by this function
				if (stay_in_view && (!hit_text_area(coord)))
				{
					if (_m_adjust_view())
						impl_->cview->sync(false);

					impl_->try_refresh = sync_graph::refresh;
					caret->visible(true);
					return true;
				}
				return false;
			}

			void text_editor::move_caret_end(bool update)
			{
				points_.caret.y = static_cast<unsigned>(impl_->textbase.lines());
				if (points_.caret.y) --points_.caret.y;
				points_.caret.x = static_cast<unsigned>(impl_->textbase.getline(points_.caret.y).size());

				if (update)
					this->reset_caret();
			}

			void text_editor::reset_caret_pixels() const
			{
				API::open_caret(window_, true).get()->dimension({ 1, line_height() });
			}

			void text_editor::reset_caret(bool stay_in_view)
			{
				move_caret(points_.caret, stay_in_view);
			}

			void text_editor::show_caret(bool isshow)
			{
				if (isshow == false || API::is_focus_ready(window_))
					API::open_caret(window_, true).get()->visible(isshow);
			}

			bool text_editor::selected() const
			{
				return (select_.a != select_.b);
			}

			bool text_editor::get_selected_points(nana::upoint &a, nana::upoint &b) const
			{
				if (select_.a == select_.b)
					return false;

				if (select_.a < select_.b)
				{
					a = select_.a;
					b = select_.b;
				}
				else
				{
					a = select_.b;
					b = select_.a;
				}

				return true;
			}

			bool text_editor::select(bool yes)
			{
				if (yes)
				{
					select_.a.x = select_.a.y = 0;
					select_.b.y = static_cast<unsigned>(impl_->textbase.lines());
					if (select_.b.y) --select_.b.y;
					select_.b.x = static_cast<unsigned>(impl_->textbase.getline(select_.b.y).size());
					select_.mode_selection = selection::mode::method_selected;
					impl_->try_refresh = sync_graph::refresh;
					return true;
				}

				select_.mode_selection = selection::mode::no_selected;
				if (_m_cancel_select(0))
				{
					impl_->try_refresh = sync_graph::refresh;
					return true;
				}
				return false;
			}

			bool text_editor::select_points(nana::upoint arg_a, nana::upoint arg_b)
			{
				select_.a = arg_a;
				select_.b = arg_b;
				select_.mode_selection = selection::mode::method_selected;
				impl_->try_refresh = sync_graph::refresh;
				return true;
			}

			void text_editor::set_end_caret(bool stay_in_view)
			{
				bool new_sel_end = (select_.b != points_.caret);
				select_.b = points_.caret;

				if (new_sel_end || (stay_in_view && this->_m_adjust_view()))
					impl_->try_refresh = sync_graph::refresh;
			}

			bool text_editor::hit_text_area(const point& pos) const
			{
				return impl_->cview->view_area().is_hit(pos);
			}

			bool text_editor::hit_select_area(nana::upoint pos, bool ignore_when_select_all) const
			{
				nana::upoint a, b;
				if (get_selected_points(a, b))
				{
					if (ignore_when_select_all)
					{
						if (a.x == 0 && a.y == 0 && (b.y + 1) == static_cast<unsigned>(textbase().lines()))
						{
							//is select all
							if (b.x == static_cast<unsigned>(textbase().getline(b.y).size()))
								return false;
						}
					}

					if ((pos.y > a.y || (pos.y == a.y && pos.x >= a.x)) && ((pos.y < b.y) || (pos.y == b.y && pos.x < b.x)))
						return true;
				}
				return false;
			}

			bool text_editor::move_select()
			{
				if (hit_select_area(points_.caret, true) || (select_.b == points_.caret))
				{
					points_.caret = select_.b;

					if (this->_m_adjust_view())
						impl_->try_refresh = sync_graph::refresh;

					reset_caret();
					return true;
				}

				if (_m_move_select(true))
				{
					textbase().text_changed();
					this->_m_adjust_view();
					impl_->try_refresh = sync_graph::refresh;
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
				unsigned exclude_px = API::open_caret(window_, true).get()->dimension().width;

				if (attributes_.line_wrapped)
					exclude_px += impl_->cview->extra_space(false);

				return (text_area_.area.width > exclude_px ? text_area_.area.width - exclude_px : 0);
			}

			window text_editor::window_handle() const
			{
				return window_;
			}

			const std::vector<upoint>& text_editor::text_position() const
			{
				return impl_->text_position;
			}

			void text_editor::focus_behavior(text_focus_behavior behavior)
			{
				select_.behavior = behavior;
			}

			void text_editor::select_behavior(bool move_to_end)
			{
				select_.move_to_end = move_to_end;
			}

			std::size_t text_editor::line_count(bool text_lines) const
			{
				if (text_lines)
					return textbase().lines();

				return impl_->capacities.behavior->take_lines();
			}

			std::shared_ptr<scroll_operation_interface> text_editor::scroll_operation() const
			{
				return impl_->cview->scroll_operation();
			}

			void text_editor::draw_corner()
			{
				impl_->cview->draw_corner(graph_);
			}

			void text_editor::render(bool has_focus)
			{
				const auto bgcolor = _m_bgcolor();

				auto fgcolor = scheme_->foreground.get_color();
				if (!API::window_enabled(window_))
					fgcolor = fgcolor.blend(bgcolor, 0.5); //Thank to besh81 for getting the fgcolor to be changed

				if (API::widget_borderless(window_))
					graph_.rectangle(false, bgcolor);

				//Draw background
				if (!API::dev::copy_transparent_background(window_, graph_))
				{
					if (attributes_.enable_background)
						graph_.rectangle(text_area_.area, true, bgcolor);
				}

				if (impl_->customized_renderers.background)
					impl_->customized_renderers.background(graph_, text_area_.area, bgcolor);

				if (impl_->counterpart.buffer && !text_area_.area.empty())
					impl_->counterpart.buffer.bitblt(rectangle{ text_area_.area.dimension() }, graph_, text_area_.area.position());

				//Render the content when the text isn't empty or the window has got focus,
				//otherwise draw the tip string.
				if ((false == textbase().empty()) || has_focus)
				{
					auto text_pos = _m_render_text(fgcolor);

					if (text_pos.empty())
						text_pos.emplace_back(upoint{});

					if ((impl_->text_position_origin != impl_->cview->origin().y) || (text_pos != impl_->text_position))
					{
						impl_->text_position_origin = impl_->cview->origin().y;
						impl_->text_position.swap(text_pos);
						if (event_handler_)
							event_handler_->text_exposed(impl_->text_position);
					}
				}
				else //Draw tip string
				{
					graph_.string({ text_area_.area.x - impl_->cview->origin().x, text_area_.area.y }, attributes_.tip_string, static_cast<color_rgb>(0x787878));
				}

				if (impl_->text_position.empty())
					impl_->text_position.emplace_back(upoint{});

				_m_draw_border();
				impl_->try_refresh = sync_graph::none;
			}
			//public:
			void text_editor::put(std::wstring text, bool perform_event)
			{
				if (text.empty())
					return;

				auto undo_ptr = std::unique_ptr<undo_input_text>{ new undo_input_text(*this, text) };

				undo_ptr->set_selected_text();

				//Do not forget to assign the _m_erase_select() to caret
				//because _m_put() will insert the text at the position where the caret is.
				points_.caret = _m_erase_select(false);

				undo_ptr->set_caret_pos();
				points_.caret = _m_put(std::move(text), false);

				impl_->undo.push(std::move(undo_ptr));

				_m_reset_content_size(true);
				if (perform_event)
					textbase().text_changed();

				if (graph_)
				{
					if (this->_m_adjust_view())
						impl_->cview->sync(false);

					reset_caret();
					impl_->try_refresh = sync_graph::refresh;
				}
			}

			void text_editor::put(wchar_t ch)
			{
				std::wstring ch_str(1, ch);

				auto undo_ptr = std::unique_ptr<undo_input_text>{ new undo_input_text(*this, ch_str) };
				bool refresh = (select_.a != select_.b);

				undo_ptr->set_selected_text();
				if (refresh)
					points_.caret = _m_erase_select(false);

				undo_ptr->set_caret_pos();

				impl_->undo.push(std::move(undo_ptr));

				auto secondary_before = impl_->capacities.behavior->take_lines(points_.caret.y);
				textbase().insert(points_.caret, std::move(ch_str));
				_m_pre_calc_lines(points_.caret.y, 1);

				textbase().text_changed();

				points_.caret.x++;

				_m_reset_content_size();

				if (!refresh)
				{
					if (!_m_update_caret_line(secondary_before))
						draw_corner();
				}
				else
					impl_->try_refresh = sync_graph::refresh;

			}

			void text_editor::copy() const
			{
				//Disallows copying text if the text_editor is masked.
				if (mask_char_)
					return;

				auto text = _m_make_select_string();
				if (!text.empty())
					nana::system::dataexch().set(text, API::root(this->window_));
			}

			void text_editor::cut()
			{
				copy();
				del();
			}

			void text_editor::paste()
			{
				auto text = system::dataexch{}.wget();

				if ((accepts::no_restrict == impl_->capacities.acceptive) || !impl_->capacities.pred_acceptive)
				{
					put(move(text), true);
					return;
				}

				//Check if the input is acceptable
				for (auto i = text.begin(); i != text.end(); ++i)
				{
					if (_m_accepts(*i))
					{
						if (accepts::no_restrict == impl_->capacities.acceptive)
							put(*i);

						continue;
					}

					if (accepts::no_restrict != impl_->capacities.acceptive)
					{
						text.erase(i, text.end());
						put(move(text), true);
					}
					break;
				}
			}

			void text_editor::enter(bool record_undo, bool perform_event)
			{
				if (false == attributes_.multi_lines)
					return;

				auto undo_ptr = std::unique_ptr<undo_input_text>(new undo_input_text(*this, std::wstring(1, '\n')));

				undo_ptr->set_selected_text();
				points_.caret = _m_erase_select(false);

				undo_ptr->set_caret_pos();

				auto & textbase = this->textbase();

				const string_type& lnstr = textbase.getline(points_.caret.y);
				++points_.caret.y;

				if (lnstr.size() > points_.caret.x)
				{
					//Breaks the line and moves the rest part to a new line
					auto rest_part_len = lnstr.size() - points_.caret.x;	//Firstly get the length of rest part, because lnstr may be invalid after insertln
					textbase.insertln(points_.caret.y, lnstr.substr(points_.caret.x));
					textbase.erase(points_.caret.y - 1, points_.caret.x, rest_part_len);
				}
				else
				{
					if (textbase.lines() == 0)
						textbase.insertln(0, std::wstring{});
					textbase.insertln(points_.caret.y, std::wstring{});
				}

				if (record_undo)
					impl_->undo.push(std::move(undo_ptr));

				impl_->capacities.behavior->add_lines(points_.caret.y - 1, 1);
				_m_pre_calc_lines(points_.caret.y - 1, 2);

				points_.caret.x = 0;

				auto origin = impl_->cview->origin();
				origin.x = 0;

				if (impl_->indent.enabled)
				{
					if (impl_->indent.generator)
					{
						put(nana::to_wstring(impl_->indent.generator()), false);
					}
					else
					{
						auto & text = textbase.getline(points_.caret.y - 1);
						auto indent_pos = text.find_first_not_of(L"\t ");
						if (indent_pos != std::wstring::npos)
							put(text.substr(0, indent_pos), false);
						else
							put(text, false);
					}
				}
				else
					_m_reset_content_size();

				if (perform_event)
					textbase.text_changed();

				auto origin_moved = impl_->cview->move_origin(origin - impl_->cview->origin());

				if (this->_m_adjust_view() || origin_moved)
					impl_->cview->sync(true);
			}

			void text_editor::del()
			{
				if (select_.a == select_.b)
				{
					if (textbase().getline(points_.caret.y).size() > points_.caret.x)
					{
						++points_.caret.x;
					}
					else if (points_.caret.y + 1 < textbase().lines())
					{	//Move to next line
						points_.caret.x = 0;
						++points_.caret.y;
					}
					else
						return;	//No characters behind the caret
				}

				backspace(true, true);
			}

			void text_editor::backspace(bool record_undo, bool perform_event)
			{
				auto undo_ptr = std::unique_ptr<undo_backspace>(new undo_backspace(*this));
				bool has_to_redraw = true;
				if (select_.a == select_.b)
				{
					auto & textbase = this->textbase();
					if (points_.caret.x)
					{
						unsigned erase_number = 1;
						--points_.caret.x;

						auto& lnstr = textbase.getline(points_.caret.y);

						undo_ptr->set_caret_pos();
						undo_ptr->set_removed(lnstr.substr(points_.caret.x, erase_number));
						auto secondary = impl_->capacities.behavior->take_lines(points_.caret.y);
						textbase.erase(points_.caret.y, points_.caret.x, erase_number);
						_m_pre_calc_lines(points_.caret.y, 1);

						if (!this->_m_adjust_view())
						{
							_m_update_line(points_.caret.y, secondary);
							has_to_redraw = false;
						}
					}
					else if (points_.caret.y)
					{
						points_.caret.x = static_cast<unsigned>(textbase.getline(--points_.caret.y).size());
						textbase.merge(points_.caret.y);
						impl_->capacities.behavior->merge_lines(points_.caret.y, points_.caret.y + 1);
						undo_ptr->set_caret_pos();
						undo_ptr->set_removed(std::wstring(1, '\n'));
					}
					else
						undo_ptr.reset();
				}
				else
				{
					undo_ptr->set_selected_text();
					points_.caret = _m_erase_select(false);
					undo_ptr->set_caret_pos();
				}

				if (record_undo)
					impl_->undo.push(std::move(undo_ptr));

				_m_reset_content_size(false);

				if (perform_event)
					textbase().text_changed();

				textbase().text_changed();

				if (has_to_redraw)
				{
					this->_m_adjust_view();
					impl_->try_refresh = sync_graph::refresh;
				}
			}

			void text_editor::undo(bool reverse)
			{
				if (reverse)
					impl_->undo.redo();
				else
					impl_->undo.undo();

				_m_reset_content_size(true);

				this->_m_adjust_view();
				impl_->try_refresh = sync_graph::refresh;
			}

			void text_editor::move_ns(bool to_north)
			{
				const bool redraw_required = _m_cancel_select(0);
				if (_m_move_caret_ns(to_north) || redraw_required)
					impl_->try_refresh = sync_graph::refresh;
			}

			void text_editor::move_left()
			{
				bool pending = true;
				if (_m_cancel_select(1) == false)
				{
					if (points_.caret.x)
					{
						--points_.caret.x;
						pending = false;
						if (this->_m_adjust_view())
							impl_->try_refresh = sync_graph::refresh;
					}
					else if (points_.caret.y) //Move to previous line
						points_.caret.x = static_cast<unsigned>(textbase().getline(--points_.caret.y).size());
					else
						pending = false;
				}

				if (pending && this->_m_adjust_view())
					impl_->try_refresh = sync_graph::refresh;
			}

			void text_editor::move_right()
			{
				bool do_render = false;
				if (_m_cancel_select(2) == false)
				{
					auto lnstr = textbase().getline(points_.caret.y);
					if (lnstr.size() > points_.caret.x)
					{
						++points_.caret.x;
						do_render = this->_m_adjust_view();
					}
					else if (points_.caret.y + 1 < textbase().lines())
					{	//Move to next line
						points_.caret.x = 0;
						++points_.caret.y;
						do_render = this->_m_adjust_view();
					}
				}
				else
					do_render = this->_m_adjust_view();

				if (do_render)
					impl_->try_refresh = sync_graph::refresh;
			}

			void text_editor::_m_handle_move_key(const arg_keyboard& arg)
			{
				if (arg.shift && (select_.a == select_.b))
					select_.a = select_.b = points_.caret;

				auto origin = impl_->cview->origin();
				auto pos = points_.caret;
				auto coord = _m_caret_to_coordinate(points_.caret, false);
				auto coord_org = coord;

				wchar_t key = arg.key;

				auto const line_px = this->line_height();

				//The number of text lines
				auto const line_count = textbase().lines();

				//The number of characters in the line of caret
				auto const text_length = textbase().getline(points_.caret.y).size();

				switch (key) {
				case keyboard::os_arrow_left:
					if (select_.move_to_end && (select_.a != select_.b) && (!arg.shift))
					{
						pos = select_.a;
					}
					else if (pos.x != 0)
					{
						--pos.x;
					}
					else if (pos.y != 0) {
						--pos.y;
						pos.x = static_cast<decltype(pos.x)>(textbase().getline(pos.y).size());
					}
					break;
				case keyboard::os_arrow_right:
					if (select_.move_to_end && (select_.a != select_.b) && (!arg.shift))
					{
						pos = select_.b;
					}
					else if (pos.x < text_length)
					{
						++pos.x;
					}
					else if (pos.y != line_count - 1)
					{
						++pos.y;
						pos.x = 0;
					}
					break;
				case keyboard::os_arrow_up:
					coord.y -= static_cast<int>(line_px);
					break;
				case keyboard::os_arrow_down:
					coord.y += static_cast<int>(line_px);
					break;
				case keyboard::os_home:
					//move the caret to the beginning of the line
					pos.x = 0;

					//move the caret to the beginning of the text if Ctrl is pressed
					if (arg.ctrl)
						pos.y = 0;
					break;
				case keyboard::os_end:

					//move the caret to the end of the text if Ctrl is pressed
					if (arg.ctrl) {
						coord.y = static_cast<unsigned>((line_count - 1) * line_px);
						//The number of characters of the bottom line
						auto const text_length = textbase().getline(std::max<size_t>(0, line_count - 1)).size();
						//move the caret to the end of the line
						pos.x = static_cast<decltype(pos.x)>(text_length);
					}
					else {
						//move the caret to the end of the line
						pos.x = static_cast<decltype(pos.x)>(text_length);
					}
					break;
				case keyboard::os_pageup:
					if (origin.y > 0)
					{
						auto off = coord - origin;
						origin.y -= (std::min)(origin.y, static_cast<int>(impl_->cview->view_area().height));
						coord = off + origin;
					}
					break;
				case keyboard::os_pagedown:
					if (impl_->cview->content_size().height > impl_->cview->view_area().height)
					{
						auto off = coord - origin;
						origin.y = (std::min)(origin.y + static_cast<int>(impl_->cview->view_area().height), static_cast<int>(impl_->cview->content_size().height - impl_->cview->view_area().height));
						coord = off + origin;
					}
					break;
				}

				if (coord != coord_org)
				{
					auto pos_x = pos.x;
					impl_->cview->move_origin(origin - impl_->cview->origin());
					pos = _m_coordinate_to_caret(coord, false);
					pos.x = pos_x;
				}

				if (pos != points_.caret) {
					if (arg.shift) {
						switch (key) {
						case keyboard::os_arrow_left:
						case keyboard::os_arrow_up:
						case keyboard::os_home:
						case keyboard::os_pageup:
							select_.b = pos;
							break;
						case keyboard::os_arrow_right:
						case keyboard::os_arrow_down:
						case keyboard::os_end:
						case keyboard::os_pagedown:
							select_.b = pos;
							break;
						}
					}
					else {
						select_.b = pos;
						select_.a = pos;
					}
					points_.caret = pos;
					impl_->try_refresh = sync_graph::refresh;
					this->_m_adjust_view();
					impl_->cview->sync(true);
					this->reset_caret();
				}
			}

			unsigned text_editor::_m_width_px(bool include_vs) const
			{
				unsigned exclude_px = API::open_caret(window_, true).get()->dimension().width;

				if (!include_vs)
					exclude_px += impl_->cview->space();

				return (text_area_.area.width > exclude_px ? text_area_.area.width - exclude_px : 0);
			}

			void text_editor::_m_draw_border()
			{
				if (!API::widget_borderless(this->window_))
				{
					if (impl_->customized_renderers.border)
					{
						impl_->customized_renderers.border(graph_, _m_bgcolor());
					}
					else
					{
						::nana::facade<element::border> facade;
						facade.draw(graph_, _m_bgcolor(), API::fgcolor(this->window_), ::nana::rectangle{ API::window_size(this->window_) }, API::element_state(this->window_));
					}

					if (!attributes_.line_wrapped)
					{
						auto exclude_px = API::open_caret(window_, true).get()->dimension().width;
						int x = this->text_area_.area.x + static_cast<int>(width_pixels());
						graph_.rectangle(rectangle{ x, this->text_area_.area.y, exclude_px, text_area_.area.height }, true, _m_bgcolor());
					}
				}

				draw_corner();
			}

			const upoint& text_editor::mouse_caret(const point& scrpos, bool stay_in_view)	//From screen position
			{
				points_.caret = _m_coordinate_to_caret(scrpos);

				if (stay_in_view && this->_m_adjust_view())
					impl_->try_refresh = sync_graph::refresh;

				reset_caret();
				return points_.caret;
			}

			const upoint& text_editor::caret() const noexcept
			{
				return points_.caret;
			}

			point text_editor::caret_screen_pos() const
			{
				return _m_caret_to_coordinate(points_.caret);
			}

			bool text_editor::scroll(bool upwards, bool vert)
			{
				impl_->cview->scroll(!upwards, !vert);
				return false;
			}

			color text_editor::_m_draw_colored_area(paint::graphics& graph, const std::pair<std::size_t, std::size_t>& row, bool whole_line)
			{
				auto area = impl_->colored_area.find(row.first);
				if (area)
				{
					if (!area->bgcolor.invisible())
					{
						auto const height = line_height();

						auto top = _m_caret_to_coordinate(upoint{ 0, static_cast<unsigned>(row.first) }).y;
						std::size_t lines = 1;

						if (whole_line)
							lines = impl_->capacities.behavior->take_lines(row.first);
						else
							top += static_cast<int>(height * row.second);

						const rectangle area_r = { text_area_.area.x, top, width_pixels(), static_cast<unsigned>(height * lines) };

						if (API::is_transparent_background(this->window_))
							graph.blend(area_r, area->bgcolor, 1);
						else
							graph.rectangle(area_r, true, area->bgcolor);
					}

					return area->fgcolor;
				}
				return{};
			}

			std::vector<upoint> text_editor::_m_render_text(const color& text_color)
			{
				std::vector<upoint> line_indexes;
				auto const behavior = this->impl_->capacities.behavior;
				auto const line_count = textbase().lines();

				auto row = behavior->text_position_from_screen(impl_->cview->view_area().y);

				if (row.first >= line_count || graph_.empty())
					return line_indexes;

				auto sections = behavior->line(row.first);
				if (row.second < sections.size())
				{
					nana::upoint str_pos(0, static_cast<unsigned>(row.first));
					str_pos.x = static_cast<unsigned>(sections[row.second].begin - textbase().getline(row.first).c_str());

					int top = _m_text_top_base() - (impl_->cview->origin().y % line_height());
					const unsigned pixels = line_height();

					const std::size_t scrlines = screen_lines() + 1;
					for (std::size_t pos = 0; pos < scrlines; ++pos, top += pixels)
					{
						if (row.first >= line_count)
							break;

						auto fgcolor = _m_draw_colored_area(graph_, row, false);
						if (fgcolor.invisible())
							fgcolor = text_color;

						sections = behavior->line(row.first);
						if (row.second < sections.size())
						{
							auto const & sct = sections[row.second];

							_m_draw_string(top, fgcolor, str_pos, sct, true);
							line_indexes.emplace_back(str_pos);
							++row.second;
							if (row.second >= sections.size())
							{
								++row.first;
								row.second = 0;
								str_pos.x = 0;
								++str_pos.y;
							}
							else
								str_pos.x += static_cast<unsigned>(sct.end - sct.begin);
						}
						else
							break;
					}
				}

				return line_indexes;
			}

			void text_editor::_m_pre_calc_lines(std::size_t line_off, std::size_t lines)
			{
				unsigned width_px = width_pixels();
				for (auto pos = line_off, end = line_off + lines; pos != end; ++pos)
					this->impl_->capacities.behavior->pre_calc_line(pos, width_px);
			}

			nana::point	text_editor::_m_caret_to_coordinate(nana::upoint pos, bool to_screen_coordinate) const
			{
				auto const behavior = impl_->capacities.behavior;
				auto const sections = behavior->line(pos.y);

				std::size_t lines = 0;	//lines before the caret line;
				for (std::size_t i = 0; i < pos.y; ++i)
				{
					lines += behavior->take_lines(i);
				}

				const text_section * sct_ptr = nullptr;
				nana::point scrpos;
				if (0 != pos.x)
				{
					std::wstring str;

					std::size_t sct_pos = 0;
					for (auto & sct : sections)
					{
						std::size_t chsize = sct.end - sct.begin;
						str.clear();
						if (mask_char_)
							str.append(chsize, mask_char_);
						else
							str.append(sct.begin, sct.end);

						//In line-wrapped mode. If the caret is at the end of a line which is not the last section,
						//the caret should be moved to the beginning of next section line.
						if ((sct_pos + 1 < sections.size()) ? (pos.x < chsize) : (pos.x <= chsize))
						{
							sct_ptr = &sct;
							if (pos.x == chsize)
								scrpos.x = _m_text_extent_size(str.c_str(), sct.end - sct.begin).width;
							else
								scrpos.x = _m_pixels_by_char(str, pos.x);
							break;
						}
						else
						{
							pos.x -= static_cast<unsigned>(chsize);
							++lines;
						}

						++sct_pos;
					}
				}

				if (!sct_ptr)
				{
					if (sections.empty())
						scrpos.x += _m_text_x({});
					else
						scrpos.x += _m_text_x(sections.front());
				}
				else
					scrpos.x += _m_text_x(*sct_ptr);

				scrpos.y = static_cast<int>(lines * line_height());

				if (!to_screen_coordinate)
				{
					//_m_text_x includes origin x and text_area x. remove these factors
					scrpos.x += (impl_->cview->origin().x - text_area_.area.x);
				}
				else
					scrpos.y += this->_m_text_top_base() - impl_->cview->origin().y;

				return scrpos;
			}

			upoint text_editor::_m_coordinate_to_caret(point scrpos, bool from_screen_coordinate) const
			{
				if (!from_screen_coordinate)
					scrpos -= (impl_->cview->origin() - point{ text_area_.area.x, this->_m_text_top_base() });

				auto const behavior = impl_->capacities.behavior;

				auto const row = behavior->text_position_from_screen(scrpos.y);

				auto sections = behavior->line(row.first);
				if (sections.empty())
					return{ 0, static_cast<unsigned>(row.first) };

				//First of all, find the text of secondary.
				auto real_str = sections[row.second];

				auto text_ptr = real_str.begin;
				const auto text_size = real_str.end - real_str.begin;

				std::wstring mask_str;
				if (mask_char_)
				{
					mask_str.resize(text_size, mask_char_);
					text_ptr = mask_str.c_str();
				}

				auto const reordered = unicode_reorder(text_ptr, text_size);

				nana::upoint res(static_cast<unsigned>(real_str.begin - sections.front().begin), static_cast<unsigned>(row.first));

				scrpos.x = (std::max)(0, (scrpos.x - _m_text_x(sections[row.second])));

				for (auto & ent : reordered)
				{
					auto str_px = static_cast<int>(_m_text_extent_size(ent.begin, ent.end - ent.begin).width);
					if (scrpos.x <= str_px)
					{
						res.x += _m_char_by_pixels(ent, scrpos.x) + static_cast<unsigned>(ent.begin - text_ptr);
						return res;
					}
					scrpos.x -= str_px;
				}

				//move the caret to the end of this section.
				res.x = static_cast<unsigned>(text_size);
				for (std::size_t i = 0; i < row.second; ++i)
					res.x += static_cast<int>(sections[i].end - sections[i].begin);

				return res;
			}

			bool text_editor::_m_pos_from_secondary(std::size_t textline, const nana::upoint& secondary, unsigned & pos)
			{
				if (textline >= textbase().lines())
					return false;

				auto sections = impl_->capacities.behavior->line(textline);

				if (secondary.y >= sections.size())
					return false;

				auto const & sct = sections[secondary.y];

				auto chptr = sct.begin + (std::min)(secondary.x, static_cast<unsigned>(sct.end - sct.begin));
				pos = static_cast<unsigned>(chptr - textbase().getline(textline).c_str());
				return true;
			}


			bool text_editor::_m_pos_secondary(const nana::upoint& charpos, nana::upoint& secondary_pos) const
			{
				if (charpos.y >= textbase().lines())
					return false;

				secondary_pos.x = charpos.x;
				secondary_pos.y = 0;

				auto sections = impl_->capacities.behavior->line(charpos.y);
				unsigned len = 0;
				for (auto & sct : sections)
				{
					len = static_cast<unsigned>(sct.end - sct.begin);
					if (len >= secondary_pos.x)
						return true;

					++secondary_pos.y;
					secondary_pos.x -= len;
				}
				--secondary_pos.y;
				secondary_pos.x = len;
				return true;
			}

			bool text_editor::_m_move_caret_ns(bool to_north)
			{
				auto const behavior = impl_->capacities.behavior;

				nana::upoint secondary_pos;
				_m_pos_secondary(points_.caret, secondary_pos);

				if (to_north)	//North
				{
					if (0 == secondary_pos.y)
					{
						if (0 == points_.caret.y)
							return false;

						--points_.caret.y;
						secondary_pos.y = static_cast<unsigned>(behavior->take_lines(points_.caret.y)) - 1;
					}
					else
					{
						//Test if out of screen
						if (static_cast<int>(points_.caret.y) < _m_text_topline())
						{
							auto origin = impl_->cview->origin();
							origin.y = static_cast<int>(points_.caret.y) * line_height();
							impl_->cview->move_origin(origin - impl_->cview->origin());
						}

						--secondary_pos.y;
					}
				}
				else //South
				{
					++secondary_pos.y;
					if (secondary_pos.y >= behavior->take_lines(points_.caret.y))
					{
						secondary_pos.y = 0;
						if (points_.caret.y + 1 >= textbase().lines())
							return false;

						++points_.caret.y;
					}
				}

				_m_pos_from_secondary(points_.caret.y, secondary_pos, points_.caret.x);
				return this->_m_adjust_view();
			}

			void text_editor::_m_update_line(std::size_t pos, std::size_t secondary_count_before)
			{
				auto behavior = impl_->capacities.behavior;
				if (behavior->take_lines(pos) == secondary_count_before)
				{
					auto top = _m_caret_to_coordinate(upoint{ 0, static_cast<unsigned>(pos) }).y;

					const unsigned pixels = line_height();
					const rectangle update_area = { text_area_.area.x, top, width_pixels(), static_cast<unsigned>(pixels * secondary_count_before) };

					if (!API::dev::copy_transparent_background(window_, update_area, graph_, update_area.position()))
					{
						_m_draw_colored_area(graph_, { pos, 0 }, true);
						graph_.rectangle(update_area, true, API::bgcolor(window_));
					}
					else
						_m_draw_colored_area(graph_, { pos, 0 }, true);

					auto fgcolor = API::fgcolor(window_);
					auto text_ptr = textbase().getline(pos).c_str();

					auto sections = behavior->line(pos);
					for (auto & sct : sections)
					{
						_m_draw_string(top, fgcolor, nana::upoint(static_cast<unsigned>(sct.begin - text_ptr), points_.caret.y), sct, true);
						top += pixels;
					}

					_m_draw_border();
					impl_->try_refresh = sync_graph::lazy_refresh;
				}
				else
					impl_->try_refresh = sync_graph::refresh;
			}

			bool text_editor::_m_accepts(char_type ch) const
			{
				if (accepts::no_restrict == impl_->capacities.acceptive)
				{
					if (impl_->capacities.pred_acceptive)
						return impl_->capacities.pred_acceptive(ch);
					return true;
				}

				//Checks the input whether it meets the requirement for a numeric.
				auto str = text();

				if ('+' == ch || '-' == ch)
					return str.empty();

				if ((accepts::real == impl_->capacities.acceptive) && ('.' == ch))
					return (str.find(L'.') == str.npos);

				return ('0' <= ch && ch <= '9');
			}

			::nana::color text_editor::_m_bgcolor() const
			{
				return (!API::window_enabled(window_) ? static_cast<color_rgb>(0xE0E0E0) : API::bgcolor(window_));
			}

			void text_editor::_m_reset_content_size(bool calc_lines)
			{
				size csize;

				if (this->attributes_.line_wrapped)
				{
					//detect if vertical scrollbar is required
					auto const max_lines = screen_lines(true);

					if (calc_lines)
					{
						auto text_lines = textbase().lines();
						if (text_lines <= max_lines)
						{
							impl_->capacities.behavior->prepare();

							auto const width_px = _m_width_px(true);

							std::size_t lines = 0;
							for (std::size_t i = 0; i < text_lines; ++i)
							{
								impl_->capacities.behavior->pre_calc_line(i, width_px);
								lines += impl_->capacities.behavior->take_lines(i);

								if (lines > max_lines)
								{
									text_lines = lines;
									break;
								}
							}
						}

						//enable vertical scrollbar when text_lines > max_lines
						csize.width = _m_width_px(text_lines <= max_lines);
						impl_->capacities.behavior->pre_calc_lines(csize.width);
					}
					else
					{
						csize.width = impl_->cview->content_size().width;
					}
				}
				else
				{
					if (calc_lines)
						impl_->capacities.behavior->pre_calc_lines(width_pixels());

					auto maxline = textbase().max_line();
					csize.width = _m_text_extent_size(textbase().getline(maxline.first).c_str(), maxline.second).width + caret_size().width;
				}

				csize.height = static_cast<unsigned>(impl_->capacities.behavior->take_lines() * line_height());

				impl_->cview->content_size(csize);
			}

			void text_editor::_m_reset()
			{
				points_.caret.x = points_.caret.y = 0;
				impl_->cview->move_origin(point{} -impl_->cview->origin());
				select_.a = select_.b;
			}

			nana::upoint text_editor::_m_put(std::wstring text, bool perform_event)
			{
				auto & textbase = this->textbase();
				auto crtpos = points_.caret;
				std::vector<std::pair<std::size_t, std::size_t>> lines;
				if (_m_resolve_text(text, lines) && attributes_.multi_lines)
				{
					auto str_orig = textbase.getline(crtpos.y);

					auto const subpos = lines.front();
					auto substr = text.substr(subpos.first, subpos.second - subpos.first);

					if (str_orig.size() == crtpos.x)
						textbase.insert(crtpos, std::move(substr));
					else
						textbase.replace(crtpos.y, str_orig.substr(0, crtpos.x) + substr);

					//There are at least 2 elements in lines
					for (auto i = lines.begin() + 1, end = lines.end() - 1; i != end; ++i)
					{
						textbase.insertln(++crtpos.y, text.substr(i->first, i->second - i->first));
					}

					auto backpos = lines.back();
					textbase.insertln(++crtpos.y, text.substr(backpos.first, backpos.second - backpos.first) + str_orig.substr(crtpos.x));
					crtpos.x = static_cast<decltype(crtpos.x)>(backpos.second - backpos.first);

					impl_->capacities.behavior->add_lines(points_.caret.y, lines.size() - 1);
					_m_pre_calc_lines(points_.caret.y, lines.size());
				}
				else
				{
					//Just insert the first line of text if the text is multilines.
					if (lines.size() > 1)
						text = text.substr(lines.front().first, lines.front().second - lines.front().first);

					crtpos.x += static_cast<unsigned>(text.size());
					textbase.insert(points_.caret, std::move(text));

					_m_pre_calc_lines(crtpos.y, 1);
				}

				if (perform_event)
					textbase.text_changed();

				return crtpos;
			}

			nana::upoint text_editor::_m_erase_select(bool perform_event)
			{
				nana::upoint a, b;
				if (get_selected_points(a, b))
				{
					auto & textbase = this->textbase();
					if (a.y != b.y)
					{
						textbase.erase(a.y, a.x, std::wstring::npos);
						textbase.erase(a.y + 1, b.y - a.y - 1);

						textbase.erase(a.y + 1, 0, b.x);
						textbase.merge(a.y);

						impl_->capacities.behavior->merge_lines(a.y, b.y);
					}
					else
					{
						textbase.erase(a.y, a.x, b.x - a.x);
						_m_pre_calc_lines(a.y, 1);
					}

					if (perform_event)
						textbase.text_changed();

					select_.a = select_.b;
					return a;
				}

				return points_.caret;
			}

			std::wstring text_editor::_m_make_select_string() const
			{
				std::wstring text;

				nana::upoint a, b;
				if (get_selected_points(a, b))
				{
					auto & textbase = this->textbase();

					if (a.y != b.y)
					{
						text = textbase.getline(a.y).substr(a.x);
						text += L"\r\n";
						for (unsigned i = a.y + 1; i < b.y; ++i)
						{
							text += textbase.getline(i);
							text += L"\r\n";
						}
						text += textbase.getline(b.y).substr(0, b.x);
					}
					else
						return textbase.getline(a.y).substr(a.x, b.x - a.x);
				}

				return text;
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
				auto const text_str = text.c_str();
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
							lines.emplace_back();
							chp += (eats - 1);
						}
					}

					if (text.npos == begin)
					{
						lines.emplace_back();
						break;
					}
				}
				return !lines.empty();
			}

			bool text_editor::_m_cancel_select(int align)
			{
				upoint a, b;
				if (get_selected_points(a, b))
				{
					//domain of algin = [0, 2]
					if (align)
					{
						this->points_.caret = (1 == align ? a : b);
						this->_m_adjust_view();
					}

					select_.a = select_.b = points_.caret;
					reset_caret();
					return true;
				}
				return false;
			}

			nana::size text_editor::_m_text_extent_size(const char_type* str, size_type n) const
			{
				if (mask_char_)
				{
					std::wstring maskstr;
					maskstr.append(n, mask_char_);
					return graph_.text_extent_size(maskstr);
				}
#ifdef _nana_std_has_string_view
				return graph_.text_extent_size(std::basic_string_view<char_type>(str, n));
#else
				return graph_.text_extent_size(str, static_cast<unsigned>(n));
#endif
			}

			bool text_editor::_m_adjust_view()
			{
				auto const view_area = impl_->cview->view_area();

				auto const line_px = static_cast<int>(this->line_height());
				auto coord = _m_caret_to_coordinate(points_.caret, true);

				if (view_area.is_hit(coord) && view_area.is_hit({ coord.x, coord.y + line_px }))
					return false;

				unsigned extra_count_horz = 4;
				unsigned extra_count_vert = 0;

				auto const origin = impl_->cview->origin();
				coord = _m_caret_to_coordinate(points_.caret, false);

				point moved_origin;

				//adjust x-axis if it isn't line-wrapped mode
				if (!attributes_.line_wrapped)
				{
					auto extra = points_.caret;

					if (coord.x < origin.x)
					{
						extra.x -= (std::min)(extra_count_horz, points_.caret.x);
						moved_origin.x = _m_caret_to_coordinate(extra, false).x - origin.x;
					}
					else if (coord.x + static_cast<int>(caret_size().width) >= origin.x + static_cast<int>(view_area.width))
					{
						extra.x = (std::min)(static_cast<unsigned>(textbase().getline(points_.caret.y).size()), points_.caret.x + extra_count_horz);
						auto new_origin = _m_caret_to_coordinate(extra, false).x + static_cast<int>(caret_size().width) - static_cast<int>(view_area.width);
						moved_origin.x = new_origin - origin.x;
					}
				}

				auto extra_px = static_cast<int>(line_px * extra_count_vert);

				if (coord.y < origin.y)
				{
					//Top of caret is less than the top of view

					moved_origin.y = (std::max)(0, coord.y - extra_px) - origin.y;
				}
				else if (coord.y + line_px >= origin.y + static_cast<int>(view_area.height))
				{
					//Bottom of caret is greater than the bottom of view

					auto const bottom = static_cast<int>(impl_->capacities.behavior->take_lines() * line_px);
					auto new_origin = (std::min)(coord.y + line_px + extra_px, bottom) - static_cast<int>(view_area.height);
					moved_origin.y = new_origin - origin.y;
				}

				return impl_->cview->move_origin(moved_origin);
			}

			bool text_editor::_m_move_select(bool record_undo)
			{
				if (!attributes_.editable)
					return false;

				nana::upoint caret = points_.caret;
				const auto text = _m_make_select_string();
				if (!text.empty())
				{
					auto undo_ptr = std::unique_ptr<undo_move_text>(new undo_move_text(*this));
					undo_ptr->set_selected_text();

					//Determines whether the caret is at left or at right. The select_.b indicates the caret position when finish selection
					const bool at_left = (select_.b < select_.a);

					nana::upoint a, b;
					get_selected_points(a, b);
					if (caret.y < a.y || (caret.y == a.y && caret.x < a.x))
					{//forward
						undo_ptr->set_caret_pos();

						_m_erase_select(false);
						_m_put(text, false);

						select_.a = caret;
						select_.b.y = b.y + (caret.y - a.y);
					}
					else if (b.y < caret.y || (caret.y == b.y && b.x < caret.x))
					{
						undo_ptr->set_caret_pos();

						_m_put(text, false);
						_m_erase_select(false);

						select_.b.y = caret.y;
						select_.a.y = caret.y - (b.y - a.y);
						select_.a.x = caret.x - (caret.y == b.y ? (b.x - a.x) : 0);
					}
					select_.b.x = b.x + (a.y == b.y ? (select_.a.x - a.x) : 0);

					//restores the caret at the proper end.
					if ((select_.b < select_.a) != at_left)
						std::swap(select_.a, select_.b);

					if (record_undo)
					{
						undo_ptr->set_destination(select_.a, select_.b);
						impl_->undo.push(std::move(undo_ptr));
					}

					points_.caret = select_.b;
					reset_caret();
					return true;
				}
				return false;
			}

			int text_editor::_m_text_top_base() const
			{
				if (false == attributes_.multi_lines)
				{
					unsigned px = line_height();
					if (text_area_.area.height > px)
						return text_area_.area.y + static_cast<int>((text_area_.area.height - px) >> 1);
				}
				return text_area_.area.y;
			}

			int text_editor::_m_text_topline() const
			{
				auto px = static_cast<int>(line_height());
				return (px ? (impl_->cview->origin().y / px) : 0);
			}

			int text_editor::_m_text_x(const text_section& sct) const
			{
				auto const behavior = impl_->capacities.behavior;
				int left = this->text_area_.area.x;

				if (::nana::align::left != this->attributes_.alignment)
				{
					auto blank_px = behavior->max_pixels() - sct.pixels;

					if (::nana::align::center == this->attributes_.alignment)
						left += static_cast<int>(blank_px) / 2;
					else
						left += static_cast<int>(blank_px);
				}

				return left - impl_->cview->origin().x;
			}


			void text_editor::_m_draw_parse_string(const keyword_parser& parser, bool rtl, ::nana::point pos, const ::nana::color& fgcolor, const wchar_t* str, std::size_t len) const
			{
#ifdef _nana_std_has_string_view
				graph_.string(pos, { str, len }, fgcolor);
#else
				graph_.palette(true, fgcolor);
				graph_.string(pos, str, len);
#endif
				if (parser.entities().empty())
					return;

#ifdef _nana_std_has_string_view
				auto glyph_px = graph_.glyph_pixels({ str, len });
#else
				std::unique_ptr<unsigned[]> glyph_px(new unsigned[len]);
				graph_.glyph_pixels(str, len, glyph_px.get());
#endif
				auto glyphs = glyph_px.get();

				auto px_h = line_height();
				auto px_w = std::accumulate(glyphs, glyphs + len, unsigned{});

				::nana::paint::graphics canvas;
				canvas.make({ px_w, px_h });
				canvas.typeface(graph_.typeface());

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
						ent_off = static_cast<int>(std::accumulate(glyphs, glyphs + (ent.begin - str), unsigned{}));
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

						ent_pos.x = pos.x + ent_off;

#ifdef _nana_std_has_string_view
						std::wstring_view ent_sv;
						if (rtl)
						{
							//draw the whole text if it is a RTL text, because Arabic language is transformable.
							ent_sv = { str, len };
						}
						else
						{
							ent_sv = { ent_begin, static_cast<std::wstring_view::size_type>(ent_end - ent_begin) };
							ent_off = 0;
						}
						canvas.string({}, ent_sv);
#else
						if (rtl)
						{
							//draw the whole text if it is a RTL text, because Arabic language is transformable.
							canvas.string({}, str, len);
						}
						else
						{
							canvas.string({}, ent_begin, ent_end - ent_begin);
							ent_off = 0;
						}
#endif
						graph_.bitblt(rectangle{ ent_pos, size{ ent_pixels, canvas.height() } }, canvas, point{ ent_off, 0 });
					}
				}
			}

			class text_editor::helper_pencil
			{
			public:
				helper_pencil(paint::graphics& graph, const text_editor& editor, keyword_parser& parser) :
					graph_(graph),
					editor_(editor),
					parser_(parser),
					line_px_(editor.line_height())
				{}

				color selection_color(bool fgcolor, bool focused) const
				{
					if (fgcolor)
						return (focused ? editor_.scheme_->selection_text : editor_.scheme_->foreground).get_color();

					return (focused ? editor_.scheme_->selection : editor_.scheme_->selection_unfocused).get_color();
				}

				void write_selection(const point& text_pos, unsigned text_px, const wchar_t* text, std::size_t len, bool has_focused)
				{
#ifdef _nana_std_has_string_view
					graph_.rectangle(::nana::rectangle{ text_pos,{ text_px, line_px_ } }, true,
						selection_color(false, has_focused));

					graph_.string(text_pos, { text, len }, selection_color(true, has_focused));
#else
					graph_.palette(true, selection_color(true, has_focused));

					graph_.rectangle(::nana::rectangle{ text_pos,{ text_px, line_px_ } }, true,
						selection_color(false, has_focused));

					graph_.string(text_pos, text, len);
#endif
				}

				void rtl_string(point strpos, const wchar_t* str, std::size_t len, std::size_t str_px, unsigned glyph_front, unsigned glyph_selected, bool has_focused)
				{
					editor_._m_draw_parse_string(parser_, true, strpos, selection_color(true, has_focused), str, len);

					//Draw selected part
					paint::graphics graph({ glyph_selected, line_px_ });
					graph.typeface(this->graph_.typeface());
					graph.rectangle(true, selection_color(false, has_focused));

					int sel_xpos = static_cast<int>(str_px - (glyph_front + glyph_selected));

#ifdef _nana_std_has_string_view
					graph.string({ -sel_xpos, 0 }, { str, len }, selection_color(true, has_focused));
#else
					graph.palette(true, selection_color(true, has_focused));
					graph.string({ -sel_xpos, 0 }, str, len);
#endif
					graph_.bitblt(nana::rectangle(strpos.x + sel_xpos, strpos.y, glyph_selected, line_px_), graph);
				};
			private:
				paint::graphics& graph_;
				const text_editor& editor_;
				keyword_parser & parser_;
				unsigned line_px_;
			};

			void text_editor::_m_draw_string(int top, const ::nana::color& clr, const nana::upoint& text_coord, const text_section& sct, bool if_mask) const
			{
				point text_draw_pos{ _m_text_x(sct), top };

				const int text_right = text_area_.area.right();
				auto const text_len = static_cast<unsigned>(sct.end - sct.begin);
				auto text_ptr = sct.begin;

				std::wstring mask_str;
				if (if_mask && mask_char_)
				{
					mask_str.resize(text_len, mask_char_);
					text_ptr = mask_str.c_str();
				}

				const auto focused = API::is_focus_ready(window_);

				auto const reordered = unicode_reorder(text_ptr, text_len);

				//Parse highlight keywords
				keyword_parser parser;
				parser.parse(text_ptr, text_len, impl_->keywords);

				const auto line_h_pixels = line_height();

				helper_pencil pencil(graph_, *this, parser);

				graph_.palette(true, clr);
				graph_.palette(false, scheme_->selection.get_color());


				//Get the selection begin and end position of the current text.
				const wchar_t *sbegin = nullptr, *send = nullptr;

				nana::upoint a, b;
				if (get_selected_points(a, b))
				{
					if (a.y < text_coord.y && text_coord.y < b.y)
					{
						sbegin = sct.begin;
						send = sct.end;
					}
					else if ((a.y == b.y) && a.y == text_coord.y)
					{
						auto sbegin_pos = (std::max)(a.x, text_coord.x);
						auto send_pos = (std::min)(text_coord.x + text_len, b.x);

						if (sbegin_pos < send_pos)
						{
							sbegin = text_ptr + (sbegin_pos - text_coord.x);
							send = text_ptr + (send_pos - text_coord.x);
						}
					}
					else if (a.y == text_coord.y)
					{
						if (a.x < text_coord.x + text_len)
						{
							sbegin = text_ptr;
							if (text_coord.x < a.x)
								sbegin += (a.x - text_coord.x);

							send = text_ptr + text_len;
						}
					}
					else if (b.y == text_coord.y)
					{
						if (text_coord.x < b.x)
						{
							sbegin = text_ptr;
							send = text_ptr + (std::min)(b.x - text_coord.x, text_len);
						}
					}
				}

				//A text editor feature, it draws an extra block at end of line if the end of line is in range of selection.
				bool extra_space = false;

				//Create a flag for indicating whether the whole line is selected
				const bool text_selected = (sbegin == text_ptr && send == text_ptr + text_len);

				//The text is not selected or the whole line text is selected
				if ((!sbegin || !send) || text_selected)
				{
					for (auto & ent : reordered)
					{
						std::size_t len = ent.end - ent.begin;

#ifdef _nana_std_has_string_view
						unsigned str_w = graph_.text_extent_size(std::wstring_view{ ent.begin, len }).width;
#else
						unsigned str_w = graph_.text_extent_size(ent.begin, len).width;
#endif

						if ((text_draw_pos.x + static_cast<int>(str_w) > text_area_.area.x) && (text_draw_pos.x < text_right))
						{
							if (text_selected)
								pencil.write_selection(text_draw_pos, str_w, ent.begin, len, focused);
							else
								_m_draw_parse_string(parser, is_right_text(ent), text_draw_pos, clr, ent.begin, len);
						}
						text_draw_pos.x += static_cast<int>(str_w);
					}

					extra_space = text_selected;
				}
				else
				{
					for (auto & ent : reordered)
					{
						const auto len = ent.end - ent.begin;
#ifdef _nana_std_has_string_view
						auto ent_px = graph_.text_extent_size(std::wstring_view(ent.begin, len)).width;
#else
						auto ent_px = graph_.text_extent_size(ent.begin, len).width;
#endif

						extra_space = false;

						//Only draw the text which is in the visual rectangle.
						if ((text_draw_pos.x + static_cast<int>(ent_px) > text_area_.area.x) && (text_draw_pos.x < text_right))
						{
							if (send <= ent.begin || ent.end <= sbegin)
							{
								//this string is not selected
								_m_draw_parse_string(parser, false, text_draw_pos, clr, ent.begin, len);
							}
							else if (sbegin <= ent.begin && ent.end <= send)
							{
								//this string is completed selected
								pencil.write_selection(text_draw_pos, ent_px, ent.begin, len, focused);
								extra_space = true;
							}
							else
							{
								//a part of string is selected

								//get the selected range of this string.
								auto ent_sbegin = (std::max)(sbegin, ent.begin);
								auto ent_send = (std::min)(send, ent.end);

								unsigned select_pos = static_cast<unsigned>(ent_sbegin != ent.begin ? ent_sbegin - ent.begin : 0);
								unsigned select_len = static_cast<unsigned>(ent_send - ent_sbegin);

#ifdef _nana_std_has_string_view
								auto pxbuf = graph_.glyph_pixels({ ent.begin, static_cast<std::size_t>(len) });
#else
								std::unique_ptr<unsigned[]> pxbuf{ new unsigned[len] };
								graph_.glyph_pixels(ent.begin, len, pxbuf.get());
#endif

								auto head_px = std::accumulate(pxbuf.get(), pxbuf.get() + select_pos, unsigned{});
								auto select_px = std::accumulate(pxbuf.get() + select_pos, pxbuf.get() + select_pos + select_len, unsigned{});

								graph_.palette(true, clr);
								if (is_right_text(ent))
								{	//RTL
									pencil.rtl_string(text_draw_pos, ent.begin, len, ent_px, head_px, select_px, focused);
								}
								else
								{	//LTR
									_m_draw_parse_string(parser, false, text_draw_pos, clr, ent.begin, select_pos);

									auto part_pos = text_draw_pos;
									part_pos.x += static_cast<int>(head_px);

									pencil.write_selection(part_pos, select_px, ent.begin + select_pos, select_len, focused);

									if (ent_send < ent.end)
									{
										part_pos.x += static_cast<int>(select_px);
										_m_draw_parse_string(parser, false, part_pos, clr, ent_send, ent.end - ent_send);
									}
								}

								extra_space = (select_pos + select_len == text_len);
							}
						}
						text_draw_pos.x += static_cast<int>(ent_px);
					}//end for
				}

				//extra_space is true if the end of line is selected
				if (extra_space)
				{
					//draw the extra space if end of line is not equal to the second selection position.
					auto pos = text_coord.x + text_len;
					if (b.x != pos || text_coord.y != b.y)
					{
#ifdef _nana_std_has_string_view
						auto whitespace_w = graph_.text_extent_size(std::wstring_view{ L" ", 1 }).width;
#else
						auto whitespace_w = graph_.text_extent_size(L" ", 1).width;
#endif
						graph_.rectangle(::nana::rectangle{ text_draw_pos,{ whitespace_w, line_h_pixels } }, true);
					}
				}
			}

			bool text_editor::_m_update_caret_line(std::size_t secondary_before)
			{
				if (false == this->_m_adjust_view())
				{
					if (_m_caret_to_coordinate(points_.caret).x < impl_->cview->view_area().right())
					{
						_m_update_line(points_.caret.y, secondary_before);
						return false;
					}
				}
				else
				{
					//The content view is adjusted, now syncs it with active mode to avoid updating.
					impl_->cview->sync(false);
				}
				impl_->try_refresh = sync_graph::refresh;
				return true;
			}

			unsigned text_editor::_m_char_by_pixels(const unicode_bidi::entity& ent, unsigned pos) const
			{
				auto len = static_cast<std::size_t>(ent.end - ent.begin);
#ifdef _nana_std_has_string_view
				auto pxbuf = graph_.glyph_pixels({ ent.begin, len });
				if (pxbuf)
#else
				std::unique_ptr<unsigned[]> pxbuf(new unsigned[len]);
				if (graph_.glyph_pixels(ent.begin, len, pxbuf.get()))
#endif
				{
					const auto px_end = pxbuf.get() + len;

					if (is_right_text(ent))
					{
						auto total_px = std::accumulate(pxbuf.get(), px_end, unsigned{});

						for (auto p = pxbuf.get(); p != px_end; ++p)
						{
							auto chpos = total_px - *p;
							if ((chpos <= pos) && (pos < total_px))
							{
								if ((*p < 2) || (pos <= chpos + (*p >> 1)))
									return static_cast<unsigned>(p - pxbuf.get()) + 1;

								return static_cast<unsigned>(p - pxbuf.get());
							}
							total_px = chpos;
						}
					}
					else
					{
						for (auto p = pxbuf.get(); p != px_end; ++p)
						{
							if (pos <= *p)
							{
								if ((*p > 1) && (pos >(*p >> 1)))
									return static_cast<unsigned>(p - pxbuf.get()) + 1;
								return static_cast<unsigned>(p - pxbuf.get());
							}
							pos -= *p;
						}
					}
				}

				return 0;
			}

			unsigned text_editor::_m_pixels_by_char(const std::wstring& lnstr, std::size_t pos) const
			{
				if (pos > lnstr.size())
					return 0;

				auto const reordered = unicode_reorder(lnstr.c_str(), lnstr.size());

				auto target = lnstr.c_str() + pos;

				unsigned text_w = 0;
				for (auto & ent : reordered)
				{
					std::size_t len = ent.end - ent.begin;
					if (ent.begin <= target && target <= ent.end)
					{
						if (is_right_text(ent))
						{
							//Characters of some bidi languages may transform in a word.
							//RTL
#ifdef _nana_std_has_string_view
							auto pxbuf = graph_.glyph_pixels({ ent.begin, len });
#else
							std::unique_ptr<unsigned[]> pxbuf(new unsigned[len]);
							graph_.glyph_pixels(ent.begin, len, pxbuf.get());
#endif
							return std::accumulate(pxbuf.get() + (target - ent.begin), pxbuf.get() + len, text_w);
						}
						//LTR
						return text_w + _m_text_extent_size(ent.begin, target - ent.begin).width;
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
