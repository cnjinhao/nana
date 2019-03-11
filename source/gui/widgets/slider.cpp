
#include <nana/gui/widgets/slider.hpp>
#include <nana/paint/pixel_buffer.hpp>
#include <cstring>	//memcpy

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
				: public renderer_interface
			{
			private:
				void background(window, graph_reference graph, bool transparent, const scheme& schm) override
				{
					if (!transparent)
						graph.rectangle(true, schm.background);
				}

				void bar(window, graph_reference graph, const data_bar& data, const scheme& schm) override
				{
					auto area = data.area;

					if (data.vert)
					{
						area.x = area.width / 2 - 2;
						area.width = 4;
					}
					else
					{
						area.y = area.height / 2 - 2;
						area.height = 4;
					}

					graph.rectangle(area, false, schm.color_bar);
				}

				void adorn(window, graph_reference graph, const data_adorn& data, const scheme& schm) override
				{
					rectangle area{
						data.bound.x, data.fixedpos + static_cast<int>(data.block / 2) - 1,
						static_cast<unsigned>(data.bound.y - data.bound.x) , 2
					};

					if (data.vert)
						area.shift();

					graph.rectangle(area, true, schm.color_adorn);

				}

				void vernier(window, graph_reference graph, const data_vernier& data, const scheme& schm) override
				{
					if (data.vert)
						_m_draw_vernier_vert(graph, data, schm);
					else
						_m_draw_vernier_horz(graph, data, schm);
				}

				void slider(window, graph_reference graph, mouse_action mouse_act, const data_slider& data, const scheme& schm) override
				{
					nana::rectangle area{ graph.size() };

					if (data.vert)
					{
						area.y = static_cast<int>(data.pos);
						area.height = data.weight;
					}
					else
					{
						area.x = static_cast<int>(data.pos);
						area.width = data.weight;
					}

					color rgb = schm.color_slider;
					if (mouse_action::normal != mouse_act && mouse_action::normal_captured != mouse_act)
						rgb = schm.color_slider_highlighted;

					graph.frame_rectangle(area, rgb + static_cast<color_rgb>(0x0d0d0d), 1);
					graph.rectangle(area.pare_off(1), true, rgb);

					area.height /= 2;
					graph.rectangle(area, true, rgb + static_cast<color_rgb>(0x101010));
				}
			private:
				void _m_draw_vernier_horz(graph_reference graph, const data_vernier& data, const scheme& schm)
				{
					const unsigned arrow_weight = 5;

					unsigned arrow_pxbuf[] = {
						0x7F, 0x00, 0x00, 0x00, 0x00,
						0x7F, 0x7F, 0x00, 0x00, 0x00,
						0x7F, 0x7F, 0x7F, 0x00, 0x00,
						0x7F, 0x7F, 0x7F, 0x7F, 0x00,
						0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
						0x7F, 0x7F, 0x7F, 0x7F, 0x00,
						0x7F, 0x7F, 0x7F, 0x00, 0x00,
						0x7F, 0x7F, 0x00, 0x00, 0x00,
						0x7F, 0x00, 0x00, 0x00, 0x00
					};

					const size arrow_size{ arrow_weight, 9 };

					const auto label_size = graph.text_extent_size(data.text) + size{ schm.vernier_text_margin * 2, 0 };

					paint::graphics graph_vern{ label_size };
					graph_vern.rectangle(true, schm.color_vernier);

					int arrow_pos;

					point label_pos{ data.position, static_cast<int>(graph.height() - label_size.height) / 2 };

					if (static_cast<int>(label_size.width + arrow_weight) > data.position)
					{
						label_pos.x += arrow_weight;
						arrow_pos = data.position;
					}
					else
					{
						label_pos.x -= label_size.width + arrow_weight;
						arrow_pos = data.position - arrow_weight;
					}

					graph.blend(rectangle{ label_pos, label_size }, graph_vern, {}, 0.5);


					unsigned arrow_color = 0x7F | schm.color_vernier.get_color().argb().value;
					for (auto & color : arrow_pxbuf)
					{
						if (color == 0x7F)
							color = arrow_color;
					}

					if (label_pos.x > data.position)
					{
						for (::nana::size::value_type l = 0; l < arrow_size.height; ++l)
						{
							auto ptr = arrow_pxbuf + l * arrow_size.width;

							for (::nana::size::value_type x = 0; x < arrow_size.width / 2; ++x)
								std::swap(ptr[x], ptr[(arrow_size.width - 1) - x]);
						}
					}

					paint::pixel_buffer pxbuf{ arrow_size.width, arrow_size.height };
					pxbuf.alpha_channel(true);
					pxbuf.put(reinterpret_cast<unsigned char*>(arrow_pxbuf), arrow_size.width, arrow_size.height, 32, arrow_size.width * 4, false);

					pxbuf.paste(rectangle{ arrow_size }, graph.handle(), { arrow_pos, label_pos.y + static_cast<int>(label_size.height - arrow_size.height) / 2 });

					label_pos.x += static_cast<int>(schm.vernier_text_margin);
					graph.string(label_pos, data.text, schm.color_vernier_text);
				}

				void _m_draw_vernier_vert(graph_reference graph, const data_vernier& data, const scheme& schm)
				{
					const unsigned arrow_weight = 5;

					unsigned arrow_pxbuf[] = {
						0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
						0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00,
						0x00, 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x7f, 0x7f, 0x7f, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00,
					};

					const size arrow_size{ 9, arrow_weight};

					const size label_size = (graph.text_extent_size(data.text) + size{ schm.vernier_text_margin * 2, 0 }).shift();

					paint::graphics graph_vern{ label_size };

					paint::graphics graph_horz{ size(label_size).shift() };
					graph_horz.rectangle(true, schm.color_vernier);
					graph_horz.string({ static_cast<int>(schm.vernier_text_margin), static_cast<int>(graph_horz.height() - label_size.width) / 2 }, data.text, schm.color_vernier_text);

					paint::pixel_buffer{ graph_horz.handle(), 0, graph_horz.height() }.rotate(90, colors::white).paste(graph_vern.handle(), {});

					int arrow_pos;

					point label_pos{ static_cast<int>(graph.width() - label_size.width) / 2, data.position };

					if (static_cast<int>(label_size.height + arrow_weight) > (data.end_position - data.position))
					{
						label_pos.y -= arrow_weight + label_size.height;
						arrow_pos = data.position - arrow_weight;

						const unsigned line_bytes = arrow_size.width * sizeof(unsigned);
						for (size::value_type l = 0; l < arrow_size.height / 2; ++l)
						{
							auto swap_x = arrow_pxbuf + l* arrow_size.width;
							auto swap_y = arrow_pxbuf + (arrow_size.height - 1 - l) * arrow_size.width;

							unsigned tmp[9];
							std::memcpy(tmp, swap_x, line_bytes);
							std::memcpy(swap_x, swap_y, line_bytes);
							std::memcpy(swap_y, tmp, line_bytes);
						}
					}
					else
					{
						label_pos.y += arrow_weight;
						arrow_pos = data.position;
					}

					graph.blend(rectangle{ label_pos, label_size }, graph_vern, {}, 0.5);

					unsigned arrow_color = 0x7F | schm.color_vernier.get_color().argb().value;
					for (auto & color : arrow_pxbuf)
					{
						if (color == 0x7F)
							color = arrow_color;
					}


					paint::pixel_buffer pxbuf{ arrow_size.width, arrow_size.height };
					pxbuf.alpha_channel(true);
					pxbuf.put(reinterpret_cast<unsigned char*>(arrow_pxbuf), arrow_size.width, arrow_size.height, 32, arrow_size.width * 4, false);

					pxbuf.paste(rectangle{ arrow_size }, graph.handle(), { label_pos.x + static_cast<int>(label_size.width - arrow_size.width) / 2, arrow_pos });

					label_pos.y += static_cast<int>(schm.vernier_text_margin);
				}
			};

			class trigger::model
			{
				struct attrib_rep
				{
					seekdir		seek_dir;
					bool		is_draw_adorn;

					unsigned	vmax;
					double		vcur;
					double		adorn_pos;

					renderer_interface::data_slider slider;
				};
			public:
				enum class parts{none, bar, slider};

				using graph_reference = drawer_trigger::graph_reference;

				model()
				{
					other_.wd = nullptr;
					other_.widget = nullptr;

					proto_.renderer = pat::cloneable<renderer_interface>{interior_renderer{}};

					attr_.seek_dir = seekdir::bilateral;

					attr_.is_draw_adorn = false;
					attr_.vcur = 0;
					attr_.vmax = 10;

					attr_.slider.vert = false;
					attr_.slider.border_weight = 1;
					attr_.slider.pos = 0;
					attr_.slider.weight = 8;
				}

				void seek_direction(seekdir sd)
				{
					attr_.seek_dir = sd;
				}

				window handle() const
				{
					return other_.wd;
				}

				void attached(nana::slider& wdg, graph_reference)
				{
					other_.wd = wdg.handle();
					other_.widget = &wdg;

					_m_mk_slider_pos_by_value();
				}

				pat::cloneable<renderer_interface>& renderer()
				{
					return proto_.renderer;
				}

				void vernier(std::function<std::string(unsigned maximum, unsigned cursor_value)> vernier_string)
				{
					proto_.vernier = vernier_string;
				}

				void draw(graph_reference graph)
				{
					if(!graph.size().empty())
					{
						proto_.renderer->background(other_.wd, graph, API::dev::copy_transparent_background(other_.wd, graph), other_.widget->scheme());
						_m_draw_elements(graph);
					}
				}

				const attrib_rep & attribute() const
				{
					return attr_;
				}

				bool vertical(bool vert)
				{
					if (vert != attr_.slider.vert)
					{
						attr_.slider.vert = vert;
						_m_mk_slider_pos_by_value();
						return true;
					}
					return false;
				}

				void maximum(unsigned m)
				{
					if(m == 0) m = 1;

					if (attr_.vmax == m)
						return;

					attr_.vmax = m;
					if(attr_.vcur > m)
					{
						attr_.vcur = m;
						_m_emit_value_changed();
					}

					_m_mk_slider_pos_by_value();
					API::refresh_window(other_.wd);
				}

				bool vcur(unsigned v)
				{
					if(attr_.vmax < v)
						v = attr_.vmax;

					if(attr_.vcur != v)
					{
						attr_.vcur = v;
						this->_m_mk_slider_pos_by_value();
						return true;
					}
					return false;
				}

				void resize()
				{
					_m_mk_slider_pos_by_value();
					attr_.adorn_pos = attr_.slider.pos;
				}

				parts seek_where(::nana::point pos) const
				{
					nana::rectangle r = _m_bar_area();

					if (attr_.slider.vert)
					{
						std::swap(pos.x, pos.y);
						std::swap(r.width, r.height);
					}

					int sdpos = _m_slider_pos();
					if (sdpos <= pos.x && pos.x < sdpos + static_cast<int>(attr_.slider.weight))
						return parts::slider;

					sdpos = static_cast<int>(attr_.slider.weight) / 2;

					if (sdpos <= pos.x && pos.x < sdpos + static_cast<int>(r.width))
					{
						if(pos.y < r.bottom())
							return parts::bar;
					}
					return parts::none;
				}

				//set_slider_pos
				//move the slider to a position where a mouse click on WhereBar.
				bool set_slider_pos(::nana::point pos)
				{
					if(attr_.slider.vert)
						std::swap(pos.x, pos.y);

					pos.x -= _m_slider_refpos();
					if(pos.x < 0)
						return false;

					if(pos.x > static_cast<int>(_m_range()))
						pos.x = static_cast<int>(_m_range());

					auto attr_pos = attr_.slider.pos;
					double dx = _m_evaluate_by_seekdir(pos.x);

					attr_.slider.pos = dx;
					attr_.adorn_pos = dx;
					_m_mk_slider_value_by_pos();

					return (attr_.slider.pos != attr_pos);
				}

				void set_slider_refpos(::nana::point pos)
				{
					if (attr_.slider.vert)
						std::swap(pos.x, pos.y);

					slider_state_.mouse_state = ::nana::mouse_action::pressed;
					slider_state_.snap_pos = static_cast<int>(attr_.slider.pos);
					slider_state_.refpos = pos;
					API::set_capture(other_.wd, true);
				}

				bool release_slider()
				{
					if(::nana::mouse_action::pressed == slider_state_.mouse_state)
					{
						API::release_capture(other_.wd);

						if (other_.wd != API::find_window(API::cursor_position()))
						{
							slider_state_.mouse_state = ::nana::mouse_action::normal;
							attr_.is_draw_adorn = false;
						}
						else
							slider_state_.mouse_state = ::nana::mouse_action::hovered;

						_m_mk_slider_value_by_pos();
						_m_mk_slider_pos_by_value();
						return true;
					}
					return false;
				}

				bool if_trace_slider() const
				{
					return (::nana::mouse_action::pressed == slider_state_.mouse_state);
				}

				bool move_slider(const ::nana::point& pos)
				{
					int adorn_pos = slider_state_.snap_pos + (attr_.slider.vert ? pos.y : pos.x) - slider_state_.refpos.x;

					if (adorn_pos > 0)
					{
						int range = static_cast<int>(_m_range());
						if (adorn_pos > range)
							adorn_pos = range;
					}
					else
						adorn_pos = 0;

					double dstpos = _m_evaluate_by_seekdir(adorn_pos);
					attr_.is_draw_adorn = true;

					if(dstpos != attr_.slider.pos)
					{
						attr_.slider.pos = dstpos;
						attr_.adorn_pos = dstpos;
						return true;
					}
					return false;
				}

				bool move_adorn(const ::nana::point& pos)
				{
					double xpos = (attr_.slider.vert ? pos.y : pos.x) - _m_slider_refpos();

					auto range = static_cast<int>(_m_range());
					if (xpos > range)
						xpos = range;

					int adorn_pos = static_cast<int>(attr_.adorn_pos);
					xpos = _m_evaluate_by_seekdir(xpos);

					attr_.adorn_pos = xpos;
					attr_.is_draw_adorn = true;

					if (mouse_action::normal == slider_state_.mouse_state || mouse_action::normal_captured == slider_state_.mouse_state)
						slider_state_.mouse_state = ::nana::mouse_action::hovered;

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
						API::refresh_window(other_.wd);

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
					if((::nana::mouse_action::pressed == slider_state_.mouse_state) && (API::capture_window() == this->other_.wd))
						return false;

					auto state_changed = ((slider_state_.mouse_state != ::nana::mouse_action::normal)
										|| (attr_.adorn_pos != attr_.slider.pos));

					slider_state_.mouse_state = ::nana::mouse_action::normal;
					attr_.is_draw_adorn = false;

					attr_.adorn_pos = attr_.slider.pos;
					slider_state_.mouse_state = ::nana::mouse_action::normal;

					return state_changed;
				}

			private:
				void _m_emit_value_changed() const
				{
					other_.widget->events().value_changed.emit(::nana::arg_slider{ *other_.widget }, other_.widget->handle());
				}

				::nana::rectangle _m_bar_area() const
				{
					auto sz = other_.widget->size();

					nana::rectangle area{ sz };
					if (attr_.slider.vert)
					{
						area.y = attr_.slider.weight / 2 - attr_.slider.border_weight;
						area.height = (static_cast<int>(sz.height) > (area.y << 1) ? sz.height - (area.y << 1) : 0);
					}
					else
					{
						area.x = attr_.slider.weight / 2 - attr_.slider.border_weight;
						area.width = (static_cast<int>(sz.width) > (area.x << 1) ? sz.width - (area.x << 1) : 0);
					}
					return area;
				}

				unsigned _m_range() const
				{
					nana::rectangle r = _m_bar_area();
					return (attr_.slider.vert ? r.height : r.width) - attr_.slider.border_weight * 2;
				}

				double _m_evaluate_by_seekdir(double pos) const
				{
					if (seekdir::bilateral != attr_.seek_dir)
					{
						if ((seekdir::backward == attr_.seek_dir) == (pos < attr_.slider.pos))
							pos = attr_.slider.pos;
					}
					return (pos < 0 ? 0 : pos);
				}

				int _m_slider_refpos() const
				{
					return static_cast<int>(attr_.slider.weight / 2);
				}

				int _m_slider_pos() const
				{
					return static_cast<int>(_m_range() * attr_.vcur / attr_.vmax);
				}

				void _m_mk_slider_value_by_pos()
				{
					auto range = _m_range();
					if (range)
					{
						auto cmpvalue = static_cast<int>(attr_.vcur);
						if (attr_.slider.vert)
							attr_.vcur = (range - attr_.slider.pos) * attr_.vmax;
						else
							attr_.vcur = (attr_.slider.pos * attr_.vmax);

						attr_.vcur /= range;
						if (cmpvalue != static_cast<int>(attr_.vcur))
							_m_emit_value_changed();
					}
				}

				void _m_mk_slider_pos_by_value()
				{
					const auto range = _m_range();
					attr_.slider.pos = double(range) * attr_.vcur / attr_.vmax;

					if (attr_.slider.vert)
						attr_.slider.pos = range - attr_.slider.pos;

					if(mouse_action::normal == slider_state_.mouse_state || mouse_action::normal_captured == slider_state_.mouse_state)
						attr_.adorn_pos = attr_.slider.pos;
				}

				unsigned _m_value_by_pos(double pos) const
				{
					const auto range = _m_range();

					if (0 == range)
						return 0;

					return static_cast<unsigned>((attr_.slider.vert ? range - pos : pos) * attr_.vmax / range);
				}

				void _m_draw_elements(graph_reference graph)
				{
					auto & scheme = other_.widget->scheme();

					renderer_interface::data_bar bar;

					bar.vert = attr_.slider.vert;
					bar.border_weight = attr_.slider.border_weight;
					bar.area = _m_bar_area();

					if (bar.area.empty())
						return;

					proto_.renderer->bar(other_.wd, graph, bar, scheme);

					//adorn
					renderer_interface::data_adorn adorn;
					adorn.vert = bar.vert;
					if (adorn.vert)
					{
						adorn.bound.x = static_cast<int>(attr_.adorn_pos + attr_.slider.border_weight + bar.area.y);
						adorn.bound.y = static_cast<int>(graph.height()) - static_cast<int>(attr_.slider.border_weight + bar.area.y);
					}
					else
					{
						adorn.bound.x = bar.area.x + attr_.slider.border_weight;
						adorn.bound.y = adorn.bound.x + static_cast<int>(attr_.adorn_pos);
					}

					adorn.vcur_scale = static_cast<unsigned>(attr_.slider.pos);
					adorn.block = (bar.vert ? bar.area.width : bar.area.height) - attr_.slider.border_weight * 2;
					adorn.fixedpos = static_cast<int>((bar.vert ? bar.area.x : bar.area.y) + attr_.slider.border_weight);

					proto_.renderer->adorn(other_.wd, graph, adorn, scheme);

					//Draw slider
					proto_.renderer->slider(other_.wd, graph, slider_state_.mouse_state, attr_.slider, scheme);

					//adorn textbox
					if (proto_.vernier && attr_.is_draw_adorn)
					{
						renderer_interface::data_vernier vern;
						vern.vert = attr_.slider.vert;
						vern.knob_weight = attr_.slider.weight;

						auto vadorn = _m_value_by_pos(attr_.adorn_pos);
						proto_.vernier(attr_.vmax, vadorn).swap(vern.text);
						if(vern.text.size())
						{
							vern.position = adorn.bound.x;
							if (!adorn.vert)
								vern.position += static_cast<int>(attr_.adorn_pos);

							vern.end_position = adorn.bound.y;
							proto_.renderer->vernier(other_.wd, graph, vern, scheme);
						}
					}
				}
			private:
				attrib_rep attr_;

				struct other_tag
				{
					window wd;
					nana::slider * widget;
				}other_;

				struct prototype_tag
				{
					pat::cloneable<slider::renderer_interface> renderer;
					std::function<std::string(unsigned maximum, unsigned vernier_value)> vernier;
				}proto_;

				struct slider_state_tag
				{
					int		snap_pos;
					::nana::point refpos; //a point for slider when the mouse was clicking on slider.
					::nana::mouse_action mouse_state{ ::nana::mouse_action::normal };
				}slider_state_;
			};

			//class trigger
				trigger::trigger()
					: model_ptr_(new model)
				{}

				trigger::~trigger()
				{
					delete model_ptr_;
				}

				auto trigger::get_model() const -> model*
				{
					return model_ptr_;
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					model_ptr_->attached(static_cast< ::nana::slider&>(widget), graph);
				}

				void trigger::refresh(graph_reference graph)
				{
					model_ptr_->draw(graph);
				}

				void trigger::mouse_down(graph_reference graph, const arg_mouse& arg)
				{
					using parts = model::parts;
					auto what = model_ptr_->seek_where(arg.pos);
					if(parts::bar == what || parts::slider == what)
					{
						bool updated = model_ptr_->set_slider_pos(arg.pos);
						model_ptr_->set_slider_refpos(arg.pos);
						if (updated)
						{
							model_ptr_->draw(graph);
							API::dev::lazy_refresh();
						}
					}
				}

				void trigger::mouse_up(graph_reference graph, const arg_mouse&)
				{
					if (model_ptr_->release_slider())
					{
						model_ptr_->draw(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					// check if slider is disabled
					if(!API::get_widget(arg.window_handle)->enabled())
						return;		// do nothing

					bool updated = false;
					if (model_ptr_->if_trace_slider())
					{
						updated = model_ptr_->move_slider(arg.pos);
						updated |= model_ptr_->set_slider_pos(arg.pos);
					}
					else
					{
						if (model::parts::none != model_ptr_->seek_where(arg.pos))
							updated = model_ptr_->move_adorn(arg.pos);
						else
							updated = model_ptr_->reset_adorn();
					}

					if (updated)
					{
						model_ptr_->draw(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference graph, const arg_mouse&)
				{
					if (model_ptr_->reset_adorn())
					{
						model_ptr_->draw(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::resized(graph_reference graph, const arg_resized&)
				{
					model_ptr_->resize();
					model_ptr_->draw(graph);
					API::dev::lazy_refresh();
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

		void slider::seek(seekdir sd)
		{
			get_drawer_trigger().get_model()->seek_direction(sd);
		}

		void slider::vertical(bool v)
		{
			if(get_drawer_trigger().get_model()->vertical(v))
				API::refresh_window(this->handle());
		}

		bool slider::vertical() const
		{
			return get_drawer_trigger().get_model()->attribute().slider.vert;
		}

		void slider::maximum(unsigned m)
		{
			get_drawer_trigger().get_model()->maximum(m);
		}

		unsigned slider::maximum() const
		{
			if (empty())
				return 0;

			return get_drawer_trigger().get_model()->attribute().vmax;
		}

		void slider::value(int v)
		{
			if(handle())
			{
			    // limit to positive values, vcur expects unsigned
			    if( v < 0 )
                    v = 0;

				if(get_drawer_trigger().get_model()->vcur(v))
					API::refresh_window(handle());
			}
		}

		unsigned slider::value() const
		{
			if (empty())
				return 0;

			return static_cast<unsigned>(get_drawer_trigger().get_model()->attribute().vcur);
		}

		unsigned slider::move_step(bool forward)
		{
			if (empty())
				return 0;

			return this->get_drawer_trigger().get_model()->move_step(forward);
		}

		unsigned slider::adorn() const
		{
			if(empty())
				return 0;

			return get_drawer_trigger().get_model()->adorn();
		}

		const pat::cloneable<slider::renderer_interface>& slider::renderer()
		{
			return get_drawer_trigger().get_model()->renderer();
		}

		void slider::renderer(const pat::cloneable<slider::renderer_interface>& rd)
		{
			get_drawer_trigger().get_model()->renderer() = rd;
		}

		void slider::vernier(std::function<std::string(unsigned maximum, unsigned cursor_value)> vernier_string)
		{
			get_drawer_trigger().get_model()->vernier(vernier_string);
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
			return API::is_transparent_background(*this);
		}
	//end class slider
}//end namespace nana
