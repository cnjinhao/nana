/*
 *	A Progress Indicator Implementation
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/progress.cpp
 */

#include <nana/gui/widgets/progress.hpp>

namespace nana
{
	namespace drawerbase
	{
		namespace progress
		{
			scheme::scheme()
			{
				foreground = static_cast<color_rgb>(0x107515);
			}

			class substance
			{
			public:
				static const unsigned border_px = 1;

				void set_widget(widget& wdg)
				{
					widget_ = static_cast<nana::progress*>(&wdg);
				}

				nana::progress* widget_ptr() const
				{
					return widget_;
				}

				unsigned inc()
				{
					auto val = value(nullptr) + 1;
					return value(&val);
				}

				unsigned value(const unsigned* value_ptr)
				{
					//Sets new value if value_ptr is not a nullptr
					if (value_ptr)
					{
						if (unknown_)
							value_ += 5;
						else
							value_ = (std::min)(max_, *value_ptr);

						_m_try_refresh();
					}
					return value_;
				}

				void reset_value()
				{
					value_ = 0;
				}

				unsigned maximum(const unsigned * value_ptr)
				{
					//Sets new maximum if value_ptr is not a nullptr
					if (value_ptr)
					{
						max_ = (*value_ptr > 0 ? *value_ptr : 1);
						_m_try_refresh();
					}
					return max_;
				}

				bool unknown(const bool* state_ptr)
				{
					if (state_ptr)
					{
						unknown_ = *state_ptr;
						if (unknown_)
							value_px_ = 0;
						else
							value_ = (std::min)(value_, max_);
					}
					return unknown_;
				}

				unsigned value_px() const
				{
					return value_px_;
				}

				bool value_px_sync()
				{
					if (widget_)
					{
						auto value_px = (widget_->size().width - border_px * 2);

						//avoid overflow
						if (unknown_ || (value_ < max_))
							value_px = static_cast<unsigned>(value_px * (double(value_) / double(max_)));

						if (value_px != value_px_)
						{
							value_px_ = value_px;
							return true;
						}
					}
					return false;
				}
			private:
				void _m_try_refresh()
				{
					if (value_px_sync())
						API::refresh_window(*widget_);
				}
			private:
				nana::progress * widget_{ nullptr };
				unsigned max_{ 100 };
				unsigned value_{ 0 };
				unsigned value_px_{ 0 };
				bool unknown_{ false };
			};

			trigger::trigger()
				: progress_(new substance)
			{}

			trigger::~trigger()
			{
				delete progress_;
			}

			substance* trigger::progress() const
			{
				return progress_;
			}
			
			void trigger::attached(widget_reference wdg, graph_reference)
			{
				progress_->set_widget(wdg);
			}

			void trigger::refresh(graph_reference graph)
			{	
				const unsigned border_px = substance::border_px;

				rectangle rt_val{ graph.size() };
				auto const width = rt_val.width - border_px * 2;

				rt_val.pare_off(static_cast<int>(border_px));

				auto rt_bground = rt_val;
				if (false == progress_->unknown(nullptr))
				{
					//Sync the value_px otherwise the progress is incorrect when it is resized.
					progress_->value_px_sync();

					rt_bground.x = static_cast<int>(progress_->value_px()) + static_cast<int>(border_px);
					rt_bground.width -= progress_->value_px();

					rt_val.width = progress_->value_px();
				}
				else
				{
					auto const block = width / 3;

					auto const value = progress_->value(nullptr);

					auto left = (std::max)(0, static_cast<int>(value - block)) + static_cast<int>(border_px);
					auto right = static_cast<int>((std::min)(value, width + border_px -1));

					if (right > left)
					{
						rt_val.x = left;
						rt_val.width = static_cast<unsigned>(right - left + 1);
					}
					else
						rt_val.width = 0;

					if (value >= width + block)
						progress_->reset_value();
				}

				auto & sch = progress_->widget_ptr()->scheme();

				//Draw the gradient background if gradient_bgcolor is available.

				auto bgcolor = sch.background.get_color();
				if (bgcolor.invisible())
					bgcolor = colors::button_face;

				if (sch.gradient_bgcolor.get_color().invisible())
					graph.rectangle(rt_bground, true, bgcolor);
				else
					graph.gradual_rectangle(rt_bground, bgcolor, sch.gradient_bgcolor.get_color(), true);

				//Draw the gradient fgcolor if gradient_fgcolor is available.

				auto fgcolor = sch.foreground.get_color();
				if (fgcolor.invisible())
					fgcolor = static_cast<color_rgb>(0x107515);

				if (sch.gradient_fgcolor.get_color().invisible())
					graph.rectangle(rt_val, true, fgcolor);
				else
					graph.gradual_rectangle(rt_val, sch.gradient_fgcolor.get_color(), fgcolor, true);

				graph.frame_rectangle(rectangle{ graph.size() }, colors::gray, colors::gray, colors::white, colors::white);
			}
		}//end namespace progress
	}//end namespace drawerbase

	//class progress
		progress::progress(){}

		progress::progress(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		progress::progress(window wd, const rectangle & r, bool visible)
		{
			create(wd, r, visible);
		}

		unsigned progress::value() const
		{
			return get_drawer_trigger().progress()->value(nullptr);
		}

		unsigned progress::value(unsigned val)
		{
			internal_scope_guard lock;
			if(API::empty_window(this->handle()) == false)
				return get_drawer_trigger().progress()->value(&val);
			return 0;
		}

		unsigned progress::inc()
		{
			internal_scope_guard lock;
			return get_drawer_trigger().progress()->inc();
		}

		unsigned progress::amount() const
		{
			return get_drawer_trigger().progress()->maximum(nullptr);
		}

		unsigned progress::amount(unsigned value)
		{
			return get_drawer_trigger().progress()->maximum(&value);
		}

		void progress::unknown(bool enb)
		{
			internal_scope_guard lock;
			get_drawer_trigger().progress()->unknown(&enb);
		}

		bool progress::unknown() const
		{
			return get_drawer_trigger().progress()->unknown(nullptr);
		}
	//end class progress
}//end namespace nana
