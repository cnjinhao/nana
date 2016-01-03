
#include <nana/gui/widgets/slider.hpp>

namespace nana
{
	arg_slider::arg_slider(slider& wdg)
		: widget(wdg)
	{}

	namespace drawerbase
	{
		namespace slider
		{

			class interior_renderer
				: public renderer
			{
			private:
				virtual void background(window wd, graph_reference graph, bool isglass)
				{
					if(!isglass)
						graph.rectangle(true, API::bgcolor(wd));
				}

				virtual void bar(window, graph_reference graph, const bar_t& bi)
				{
					//draw border
					::nana::color lt(0x83, 0x90, 0x97), rb(0x9d,0xae,0xc2);
					graph.frame_rectangle(bi.r, lt, lt, rb, rb);
				}

				virtual void adorn(window, graph_reference graph, const adorn_t& ad)
				{
					auto len = static_cast<const unsigned>(ad.bound.y - ad.bound.x);
					const auto upperblock = ad.block - ad.block / 2;

					::nana::color clr_from(0x84, 0xc5, 0xff), clr_trans(0x0f, 0x41, 0xcd), clr_to(0x6e, 0x96, 0xff);
					if(ad.horizontal)
					{
						graph.gradual_rectangle({ ad.bound.x, ad.fixedpos, len, upperblock }, clr_from, clr_trans, true);
						graph.gradual_rectangle({ ad.bound.x, ad.fixedpos + static_cast<int>(upperblock), len, ad.block - upperblock }, clr_trans, clr_to, true);
					}
					else
					{
						graph.gradual_rectangle({ ad.fixedpos, ad.bound.x, upperblock, len }, clr_from, clr_trans, false);
						graph.gradual_rectangle({ ad.fixedpos + static_cast<int>(upperblock), ad.bound.x, ad.block - upperblock, len }, clr_trans, clr_to, false);
					}
				}

				virtual void adorn_textbox(window, graph_reference graph, const std::string& str, const nana::rectangle & r)
				{
					graph.rectangle(r, false, colors::white);
					graph.string({ r.x + 2, r.y + 1 }, str, colors::white);
				}

				virtual void slider(window, graph_reference graph, const slider_t& s)
				{
					nana::rectangle r{ graph.size() };
					if(s.horizontal)
					{
						r.x = s.pos;
						r.width = s.scale;
					}
					else
					{
						r.y = s.pos;
						r.height = s.scale;
					}
					graph.round_rectangle(r, 3, 3, colors::black, true, static_cast<color_rgb>(0xf0f0f0));
				}
			};

			class controller
			{
			public:
				enum class style{horizontal, vertical};
				enum class parts{none, bar, slider};
				
				typedef drawer_trigger::graph_reference graph_reference;

				controller()
				{
					other_.wd = nullptr;
					other_.widget = nullptr;
					other_.graph = nullptr;

					proto_.renderer = pat::cloneable<renderer>(interior_renderer());

					attr_.skdir = seekdir::bilateral;
					attr_.dir = style::horizontal;
					attr_.vcur = 0;
					attr_.vmax = 10;
					attr_.slider_scale = 8;
					attr_.border = 1;
					attr_.is_draw_adorn = false;
				}

				void seek(seekdir sd)
				{
					attr_.skdir = sd;
				}

				window handle() const
				{
					return other_.wd;
				}

				void attached(nana::slider& wd, graph_reference graph)
				{
					other_.wd = wd.handle();
					other_.widget = &wd;

					other_.graph = &graph;
					_m_mk_slider_pos_by_value();
				}

				void detached()
				{
					other_.graph = nullptr;
				}

				pat::cloneable<renderer>& ext_renderer()
				{
					return proto_.renderer;
				}

				void ext_renderer(const pat::cloneable<renderer>& rd)
				{
					proto_.renderer = rd;
				}

				void ext_provider(const pat::cloneable<provider>& pd)
				{
					proto_.provider = pd;
				}

				void draw()
				{
					if(other_.graph && !other_.graph->size().empty())
					{
						bool is_transparent = (bground_mode::basic == API::effects_bground_mode(other_.wd));
						proto_.renderer->background(other_.wd, *other_.graph, is_transparent);
						_m_draw_objects();
					}
				}

				void vertical(bool v)
				{
					auto dir = (v ? style::vertical : style::horizontal);

					if(dir != attr_.dir)
					{
						attr_.dir = dir;
						_m_mk_slider_pos_by_value();
						this->draw();
					}
				}

				bool vertical() const
				{
					return (style::vertical == attr_.dir);
				}

				void vmax(unsigned m)
				{
					if(m == 0) m = 1;

					if(attr_.vmax != m)
					{
						attr_.vmax = m;
						if(attr_.vcur > m)
						{
							attr_.vcur = m;
							_m_emit_value_changed();
						}

						_m_mk_slider_pos_by_value();
						draw();
					}
				}

				unsigned vmax() const
				{
					return attr_.vmax;
				}

				void vcur(unsigned v)
				{
					if(attr_.vmax < v)
						v = attr_.vmax;

					if(attr_.vcur != v)
					{
						attr_.vcur = v;
						this->_m_mk_slider_pos_by_value();
						draw();
					}
				}

				unsigned vcur() const
				{
					return static_cast<unsigned>(attr_.vcur);
				}

				void resize()
				{
					_m_mk_slider_pos_by_value();
					attr_.adorn_pos = attr_.pos;
				}

				parts seek_where(::nana::point pos) const
				{
					nana::rectangle r = _m_bar_area();
					if(style::vertical == attr_.dir)
					{
						std::swap(pos.x, pos.y);
						std::swap(r.width, r.height);
					}

					int sdpos = _m_slider_pos();
					if (sdpos <= pos.x && pos.x < sdpos + static_cast<int>(attr_.slider_scale))
						return parts::slider;

					sdpos = static_cast<int>(attr_.slider_scale) / 2;
					
					if (sdpos <= pos.x && pos.x < sdpos + static_cast<int>(r.width))
					{
						if(pos.y < r.y + static_cast<int>(r.height))
							return parts::bar;
					}
					return parts::none;
				}

				//set_slider_pos
				//move the slider to a position where a mouse click on WhereBar.
				bool set_slider_pos(::nana::point pos)
				{
					if(style::vertical == attr_.dir)
						std::swap(pos.x, pos.y);

					pos.x -= _m_slider_refpos();
					if(pos.x < 0)
						return false;

					if(pos.x > static_cast<int>(_m_scale()))
						pos.x = static_cast<int>(_m_scale());

					double attr_pos = attr_.pos;
					double dx = _m_evaluate_by_seekdir(pos.x);

					attr_.pos = dx;
					attr_.adorn_pos = dx;
					_m_mk_slider_value_by_pos();

					return (attr_.pos != attr_pos);
				}

				void set_slider_refpos(::nana::point pos)
				{
					if(style::vertical == attr_.dir)
						std::swap(pos.x, pos.y);

					slider_state_.trace = slider_state_.TraceCapture;
					slider_state_.snap_pos = static_cast<int>(attr_.pos);
					slider_state_.refpos = pos;
					API::capture_window(other_.wd, true);
				}

				bool release_slider()
				{
					if(slider_state_.trace == slider_state_.TraceCapture)
					{
						API::capture_window(other_.wd, false);
						if(other_.wd != API::find_window(API::cursor_position()))
						{
							slider_state_.trace = slider_state_.TraceNone;
							attr_.is_draw_adorn = false;
						}
						else
							slider_state_.trace = slider_state_.TraceOver;

						_m_mk_slider_value_by_pos();
						_m_mk_slider_pos_by_value();
						return true;
					}
					return false;
				}

				bool if_trace_slider() const
				{
					return (slider_state_.trace == slider_state_.TraceCapture);
				}

				bool move_slider(const ::nana::point& pos)
				{
					int mpos = (style::horizontal == attr_.dir ? pos.x : pos.y);
					int adorn_pos = slider_state_.snap_pos + (mpos - slider_state_.refpos.x);
					
					if (adorn_pos > 0)
					{
						int scale = static_cast<int>(_m_scale());
						if (adorn_pos > scale)
							adorn_pos = scale;
					}
					else
						adorn_pos = 0;

					double dstpos = _m_evaluate_by_seekdir(adorn_pos);
					attr_.is_draw_adorn = true;

					if(dstpos != attr_.pos)
					{
						attr_.pos = dstpos;
						attr_.adorn_pos = dstpos;
						return true;
					}
					return false;
				}

				bool move_adorn(const ::nana::point& pos)
				{
					double xpos = (style::horizontal == attr_.dir ? pos.x : pos.y);

					xpos -= _m_slider_refpos();
					if(xpos > static_cast<int>(_m_scale()))
						xpos = static_cast<int>(_m_scale());

					int adorn_pos = static_cast<int>(attr_.adorn_pos);
					xpos = _m_evaluate_by_seekdir(xpos);

					attr_.adorn_pos = xpos;
					attr_.is_draw_adorn = true;

					if(slider_state_.trace == slider_state_.TraceNone)
						slider_state_.trace = slider_state_.TraceOver;

					return (adorn_pos != static_cast<int>(xpos));
				}

				unsigned move_step(bool forward)
				{
					unsigned cmpvalue = static_cast<unsigned>(attr_.vcur);
					auto value = cmpvalue;
					if(forward)
					{
						if (value)
							--value;
					}
					else if (value < attr_.vmax)
						++value;

					attr_.vcur = value;
					if (cmpvalue != value)
					{
						_m_mk_slider_pos_by_value();
						draw();
						_m_emit_value_changed();
					}

					return cmpvalue;
				}

				unsigned adorn() const
				{
					return _m_value_by_pos(attr_.adorn_pos);
				}

				bool reset_adorn()
				{
					//Test if the slider is captured, the operation should be ignored. Because the mouse_leave always be generated even through
					//the slider is captured.
					if(slider_state_.trace == slider_state_.TraceCapture && (nana::API::capture_window() == this->other_.wd))
						return false;

					slider_state_.trace = slider_state_.TraceNone;
					attr_.is_draw_adorn = false;
					if(attr_.adorn_pos != attr_.pos)
					{
						attr_.adorn_pos = attr_.pos;
						return true;
					}
					return false;
				}

			private:
				void _m_emit_value_changed() const
				{
					other_.widget->events().value_changed.emit(::nana::arg_slider{ *other_.widget });
				}

				nana::rectangle _m_bar_area() const
				{
					auto sz = other_.graph->size();
					nana::rectangle r{ sz };
					if(style::horizontal == attr_.dir)
					{
						r.x = attr_.slider_scale / 2 - attr_.border;
						r.width = (static_cast<int>(sz.width) > (r.x << 1) ? sz.width - (r.x << 1) : 0);
					}
					else
					{
						r.y = attr_.slider_scale / 2 - attr_.border;
						r.height = (static_cast<int>(sz.height) > (r.y << 1) ? sz.height - (r.y << 1) : 0);
					}
					return r;
				}

				unsigned _m_scale() const
				{
					nana::rectangle r = _m_bar_area();
					return ((style::horizontal == attr_.dir ? r.width : r.height) - attr_.border * 2);
				}

				double _m_evaluate_by_seekdir(double pos) const
				{
					switch(attr_.skdir)
					{
					case seekdir::backward:
						if(pos < attr_.pos)
							pos = attr_.pos;
						break;
					case seekdir::forward:
						if(pos > attr_.pos)
							pos = attr_.pos;
						break;
					default:
						break;
					}
					return (pos < 0 ? 0 : pos);
				}

				int _m_slider_refpos() const
				{
					return static_cast<int>(attr_.slider_scale / 2);
				}

				int _m_slider_pos() const
				{
					return static_cast<int>(_m_scale() * attr_.vcur / attr_.vmax);
				}

				void _m_mk_slider_value_by_pos()
				{
					if(_m_scale())
					{
						auto cmpvalue = static_cast<int>(attr_.vcur);
						if (style::vertical == attr_.dir)
						{
							double scl = _m_scale();
							attr_.vcur = (scl - attr_.pos) * attr_.vmax / scl;
						}
						else
							attr_.vcur = (attr_.pos * attr_.vmax / _m_scale());
						if (cmpvalue != static_cast<int>(attr_.vcur))
							_m_emit_value_changed();
					}
				}

				void _m_mk_slider_pos_by_value()
				{
					attr_.pos = double(_m_scale()) * attr_.vcur / attr_.vmax;

					if (style::vertical == attr_.dir)
						attr_.pos = _m_scale() - attr_.pos;

					if(slider_state_.trace == slider_state_.TraceNone)
						attr_.adorn_pos = attr_.pos;
				}

				unsigned _m_value_by_pos(double pos) const
				{
					if(_m_scale())
						return static_cast<unsigned>(pos * attr_.vmax / _m_scale());
					return 0;
				}

				void _m_draw_objects()
				{
					renderer::bar_t bar;

					bar.horizontal = (style::horizontal == attr_.dir);
					bar.border_size = attr_.border;
					bar.r = _m_bar_area();

					if (bar.r.empty())
						return;

					proto_.renderer->bar(other_.wd, *other_.graph, bar);

					//adorn
					renderer::adorn_t adorn;
					adorn.horizontal = bar.horizontal;
					if (adorn.horizontal)
					{
						adorn.bound.x = bar.r.x + attr_.border;
						adorn.bound.y = adorn.bound.x + static_cast<int>(attr_.adorn_pos);
					}
					else
					{
						adorn.bound.y = static_cast<int>(other_.graph->height()) - static_cast<int>(attr_.border + bar.r.y);
						adorn.bound.x = static_cast<int>(attr_.adorn_pos + attr_.border + bar.r.y);
					}
					adorn.vcur_scale = static_cast<unsigned>(attr_.pos);
					adorn.block = (bar.horizontal ? bar.r.height : bar.r.width) - attr_.border * 2;
					adorn.fixedpos = static_cast<int>((bar.horizontal ? bar.r.y : bar.r.x) + attr_.border);

					proto_.renderer->adorn(other_.wd, *other_.graph, adorn);

					_m_draw_slider();

					//adorn textbox
					if(proto_.provider && attr_.is_draw_adorn)
					{
						unsigned vadorn = _m_value_by_pos(attr_.adorn_pos);
						auto str = proto_.provider->adorn_trace(attr_.vmax, vadorn);
						if(str.size())
						{
							nana::size ts = other_.graph->text_extent_size(str);
							ts.width += 6;
							ts.height += 2;

							int x, y;
							const int room = static_cast<int>(attr_.adorn_pos);
							if(bar.horizontal)
							{
								y = adorn.fixedpos + static_cast<int>(adorn.block - ts.height) / 2;
								x = (room > static_cast<int>(ts.width + 2) ? room - static_cast<int>(ts.width + 2) : room + 2) + _m_slider_refpos();
							}
							else
							{
								x = (other_.graph->width() - ts.width) / 2;
								y = (room > static_cast<int>(ts.height + 2) ? room - static_cast<int>(ts.height + 2) : room + 2) + _m_slider_refpos();
							}
							proto_.renderer->adorn_textbox(other_.wd, *other_.graph, str, {x, y, ts.width, ts.height});
						}
					}
				}

				void _m_draw_slider()
				{
					renderer::slider_t s;
					s.pos = static_cast<int>(attr_.pos);
					s.horizontal = (style::horizontal == attr_.dir);
					s.scale = attr_.slider_scale;
					s.border = attr_.border;
					proto_.renderer->slider(other_.wd, *other_.graph, s);
				}
			private:
				struct other_tag
				{
					window wd;
					nana::slider * widget;
					paint::graphics * graph;
				}other_;
				
				struct prototype_tag
				{
					pat::cloneable<slider::renderer> renderer;
					pat::cloneable<slider::provider> provider;
				}proto_;

				struct attr_tag
				{
					seekdir skdir;
					style dir;
					unsigned border;
					unsigned vmax;
					double vcur;
					double		pos;
					bool		is_draw_adorn;
					double		adorn_pos;
					unsigned slider_scale;
				}attr_;

				struct slider_state_tag
				{
					enum t{TraceNone, TraceOver, TraceCapture};

					t trace;	//true if the mouse press on slider.
					int		snap_pos;
					nana::point refpos; //a point for slider when the mouse was clicking on slider.

					slider_state_tag(): trace(TraceNone){}
				}slider_state_;
			};

			//class trigger
				trigger::trigger()
					: impl_(new controller_t)
				{}

				trigger::~trigger()
				{
					delete impl_;
				}

				trigger::controller_t* trigger::ctrl() const
				{
					return impl_;
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					impl_->attached(static_cast<nana::slider&>(widget), graph);
				}

				void trigger::detached()
				{
					impl_->detached();
				}

				void trigger::refresh(graph_reference)
				{
					impl_->draw();
				}

				void trigger::mouse_down(graph_reference, const arg_mouse& arg)
				{
					using parts = controller_t::parts;
					auto what = impl_->seek_where(arg.pos);
					if(parts::bar == what || parts::slider == what)
					{
						bool mkdir = impl_->set_slider_pos(arg.pos);
						impl_->set_slider_refpos(arg.pos);
						if(mkdir)
						{
							impl_->draw();
							API::lazy_refresh();
						}
					}
				}

				void trigger::mouse_up(graph_reference, const arg_mouse&)
				{
					bool mkdraw = impl_->release_slider();
					if(mkdraw)
					{
						impl_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_move(graph_reference, const arg_mouse& arg)
				{
					bool mkdraw = false;
					if(impl_->if_trace_slider())
					{
						mkdraw = impl_->move_slider(arg.pos);
					}
					else
					{
						auto what = impl_->seek_where(arg.pos);
						if(controller_t::parts::none != what)
							mkdraw = impl_->move_adorn(arg.pos);
						else
							mkdraw = impl_->reset_adorn();
					}

					if(mkdraw)
					{
						impl_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference, const arg_mouse&)
				{
					if(impl_->reset_adorn())
					{
						impl_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::resized(graph_reference, const arg_resized&)
				{
					impl_->resize();
					impl_->draw();
					API::lazy_refresh();
				}
			//end class trigger
		}//end namespace slider
	}//end namespace drawerbase

	//class slider
		slider::slider(){}
		
		slider::slider(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		slider::slider(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void slider::seek(slider::seekdir sd)
		{
			get_drawer_trigger().ctrl()->seek(sd);
		}

		void slider::vertical(bool v)
		{
			get_drawer_trigger().ctrl()->vertical(v);
			API::update_window(this->handle());
		}

		bool slider::vertical() const
		{
			return get_drawer_trigger().ctrl()->vertical();
		}

		void slider::vmax(unsigned m)
		{
			if(this->handle())
			{
				get_drawer_trigger().ctrl()->vmax(m);
				API::update_window(handle());
			}
		}

		unsigned slider::vmax() const
		{
			if(handle())
				return get_drawer_trigger().ctrl()->vmax();
			return 0;
		}

		void slider::value(unsigned v)
		{
			if(handle())
			{
				get_drawer_trigger().ctrl()->vcur(v);
				API::update_window(handle());
			}
		}

		unsigned slider::value() const
		{
			if(handle())
				return get_drawer_trigger().ctrl()->vcur();
			return 0;
		}

		unsigned slider::move_step(bool forward)
		{
			if(handle())
			{
				drawerbase::slider::controller* ctrl = this->get_drawer_trigger().ctrl();
				unsigned val = ctrl->move_step(forward);
				if(val != ctrl->vcur())
					API::update_window(handle());
				return val;
			}
			return 0;
		}

		unsigned slider::adorn() const
		{
			if(empty())	return 0;
			return get_drawer_trigger().ctrl()->adorn();
		}

		pat::cloneable<slider::renderer>& slider::ext_renderer()
		{
			return get_drawer_trigger().ctrl()->ext_renderer();
		}

		void slider::ext_renderer(const pat::cloneable<slider::renderer>& di)
		{
			get_drawer_trigger().ctrl()->ext_renderer(di);
		}

		void slider::ext_provider(const pat::cloneable<slider::provider>& pi)
		{
			get_drawer_trigger().ctrl()->ext_provider(pi);
		}

		void slider::transparent(bool enabled)
		{
			if(enabled)
				API::effects_bground(*this, effects::bground_transparent(0), 0.0);
			else
				API::effects_bground_remove(*this);
		}

		bool slider::transparent() const
		{
			return (bground_mode::basic == API::effects_bground_mode(*this));
		}
	//end class slider
}//end namespace nana
