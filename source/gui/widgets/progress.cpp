/*
 *	A Progress Indicator Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
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
			//class trigger
			trigger::trigger()
				:   graph_(nullptr), draw_width_(static_cast<unsigned>(-1)), has_value_(true),
                    unknown_(false), max_(100), value_(0)
			{}

			void trigger::attached(widget_reference wd, graph_reference graph)
			{
				widget_	= &wd;
				graph_	= &graph;
			}

			unsigned trigger::value() const
			{
				return value_;
			}

			unsigned trigger::value(unsigned v)
			{
				internal_scope_guard isg;
				if(false == unknown_)
				{
					if(value_ != v)
						value_ = v > max_?max_:v;
				}
				else
					value_ += (v?10:v);

				if(_m_check_changing(value_))
				{
					_m_draw();
					API::update_window(widget_->handle());
				}
				return v;
			}

			unsigned trigger::inc()
			{
				internal_scope_guard isg;
				if(false == unknown_)
				{
					if(value_ < max_)
						++value_;
				}
				else
					value_ += 5;

				if(_m_check_changing(value_))
					API::refresh_window(widget_->handle());
				return value_;
			}

			unsigned trigger::Max() const
			{
				return max_;
			}

			unsigned trigger::Max(unsigned value)
			{
				max_ = value;
				if(max_ == 0)	++max_;

				API::refresh_window(widget_->handle());
				return max_;
			}

			void trigger::unknown(bool enb)
			{
				unknown_ = enb;
				if(enb)
					draw_width_ = static_cast<unsigned>(-1);
			}

			bool trigger::unknown() const
			{
				return unknown_;
			}

			void trigger::refresh(graph_reference)
			{
				_m_draw();
			}

			void trigger::_m_draw()
			{
				if(false == unknown_)
					draw_width_ = static_cast<unsigned>((graph_->width() - border * 2) * (double(value_) / max_));

				_m_draw_box(*graph_);
				_m_draw_progress(*graph_);
			}

			void trigger::_m_draw_box(graph_reference graph)
			{
				rectangle r = graph.size();
				graph.shadow_rectangle(r, color::button_face_shadow_end, color::button_face_shadow_start, true);
				graph.rectangle_line(r, 0x808080, 0x808080, 0xFFFFFF, 0xFFFFFF);
			}

			void trigger::_m_draw_progress(graph_reference graph)
			{
				unsigned width = graph.width() - border * 2;
				unsigned height = graph.height() - border * 2;

				if(false == unknown_)
				{
					if(draw_width_)
						graph.shadow_rectangle(border, border, draw_width_, height, 0x6FFFA8, 0x107515, true);
				}
				else
				{
					unsigned block = width / 3;

					int left = (value_ < block ? 0 : value_ - block) + border;
					int right = (value_ >= width - 1 + border? width - 1 + border: value_);

					if(right >= left)
						graph.shadow_rectangle(left, border, right - left + 1, height, 0x6FFFA8, 0x107515, true);

					if(value_ >= width + block)	value_ = 0;
				}
			}

			bool trigger::_m_check_changing(unsigned newvalue) const
			{
				if(graph_)
					return (((graph_->width() - border * 2) * newvalue / max_) != draw_width_);
				return false;
			}
			//end class drawer
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
			return get_drawer_trigger().value();
		}

		unsigned progress::value(unsigned val)
		{
			internal_scope_guard isg;
			if(API::empty_window(this->handle()) == false)
				return get_drawer_trigger().value(val);
			return 0;
		}

		unsigned progress::inc()
		{
			internal_scope_guard isg;
			return get_drawer_trigger().inc();
		}

		unsigned progress::amount() const
		{
			return get_drawer_trigger().Max();
		}

		unsigned progress::amount(unsigned value)
		{
			return get_drawer_trigger().Max(value);
		}

		void progress::unknown(bool enb)
		{
			get_drawer_trigger().unknown(enb);
		}

		bool progress::unknown() const
		{
			return get_drawer_trigger().unknown();
		}
	//end class progress
}//end namespace nana
