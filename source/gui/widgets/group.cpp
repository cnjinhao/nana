/**
 *	A group widget implementation
 *	Nana C++ Library(http://www.nanaro.org)
 *	Copyright(C) 2015-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/widgets/group.cpp
 *
 *	@author Stefan Pfeifer(st-321), Ariel Vina-Rodriguez (qPCR4vir)
 *
 *	@brief group is a widget used to visually group and layout other widgets.
 *
 * 	@contributor
 *		dankan1890(https://github.com/dankan1890)
 */


#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/drawing.hpp>
#include <nana/gui/widgets/checkbox.hpp>

#define _THROW_IF_EMPTY()\
	if(empty())	\
		throw std::logic_error("the group is invalid");

namespace nana
{

static const char* field_title = "__nana_group_title__";
static const char* field_options = "__nana_group_options__";

	struct group::implement
	{
		label	caption;
		align	caption_align{ align::left };
		background_mode caption_mode{ background_mode::blending };
		place	place_content;
		unsigned gap{2};
		std::string usr_div_str;

    nana::size caption_dimension;

    std::vector<std::unique_ptr<checkbox>> options;
    radio_group * radio_logic{nullptr};

    implement() = default;

    implement(window grp_panel, ::std::string title, bool vsb, unsigned gap=2)
        : caption (grp_panel, std::move(title), vsb),
          place_content{grp_panel},
          gap{gap}
    {
    }

    void create(window pnl)
    {
        caption.create(pnl);
        caption.caption("");
        place_content.bind(pnl);

        if (!radio_logic)
            radio_logic = new radio_group;
    }

    void update_div()
    {
        const std::size_t padding = 10;
        caption_dimension = caption.measure(1000);
        caption_dimension.width += 1;

        std::string div = "vert margin=[0," + std::to_string(gap) + "," + std::to_string(gap + 5) + "," + std::to_string(gap) + "]";

        div += "<weight=" + std::to_string(caption_dimension.height) + " ";

        if (align::left == caption_align)
            div += "<weight=" + std::to_string(padding) + ">";
        else
            div += "<>";	//right or center

        div += "<" + std::string{ field_title } + " weight=" + std::to_string(caption_dimension.width) + ">";

        if (align::right == caption_align)
            div += "<weight=" + std::to_string(padding) + ">";
        else if (align::center == caption_align)
            div += "<>";

        div += "><<vert margin=5 " + std::string(field_options) + ">";

        if (!usr_div_str.empty())
            div += "<" + usr_div_str + ">>";
        else
            div += ">";

        place_content.div(div.c_str());

        if (options.empty())
            place_content.field_display(field_options, false);

        if (caption.caption().empty())
            place_content.field_display(field_title, false);
    }
};

group::group()
    : impl_(new implement)
{
}

group::group(window parent, const rectangle& r, bool vsb)
    : group()
{
    create(parent, r, vsb);
}

using groupbase_type = widget_object<category::widget_tag, drawerbase::panel::drawer, general_events, drawerbase::group::scheme>;

group::group(window parent, ::std::string title, bool formatted, unsigned  gap, const rectangle& r, bool vsb)
    : group(parent, r, vsb)
{
    this->bgcolor(API::bgcolor(parent));

    impl_.reset(new implement(*this, std::move(title), vsb, gap));

    impl_->caption.format(formatted);
    _m_init();
}

group::~group()
{
    delete impl_->radio_logic;
}

checkbox& group::add_option(std::string text)
{
    _THROW_IF_EMPTY()

#ifdef _nana_std_has_emplace_return_type
    auto & opt = impl_->options.emplace_back(new checkbox { handle() });
#else
    impl_->options.emplace_back(new checkbox(handle()));
    auto & opt = impl_->options.back();
#endif

		opt->transparent(true);
		opt->caption(std::move(text));
		impl_->place_content[field_options] << *opt;
		impl_->place_content.field_display(field_options, true);
		impl_->place_content.collocate();

		if (impl_->radio_logic)
			impl_->radio_logic->add(*opt);

		return *impl_->options.back();
	}

	group& group::caption_align(align position)
	{
		if (position != impl_->caption_align)
		{
			impl_->caption_align = position;
			impl_->update_div();
			impl_->place_content.collocate();
			API::refresh_window(*this);
		}
		return *this;
	}

	group&  group::caption_background_mode(background_mode mode)
	{
		if (mode != impl_->caption_mode)
		{
			impl_->caption_mode = mode;
			switch (mode)
			{
			case background_mode::none:
				impl_->caption.bgcolor(this->bgcolor());
				impl_->caption.transparent(false);
				break;
			case background_mode::blending:
				impl_->caption.transparent(true);
				impl_->caption.bgcolor(API::bgcolor(this->parent()).blend(colors::black, 0.025));
				break;
			case background_mode::transparent:
				impl_->caption.transparent(true);
				impl_->caption.bgcolor(API::bgcolor(this->parent()).blend(colors::black, 0.025));
				break;
			}
			API::refresh_window(*this);
		}
		return *this;
	}

	group& group::radio_mode(bool enable)
	{
		_THROW_IF_EMPTY()

		if (enable)
		{
			//Create radio_group if it is null
			if (!impl_->radio_logic)
				impl_->radio_logic = new ::nana::radio_group;

			//add all options into the radio_group
			for (auto & opt : impl_->options)
				impl_->radio_logic->add(*opt);
		}
		else
		{
			delete impl_->radio_logic;
			impl_->radio_logic = nullptr;
		}
		return *this;
	}

	std::size_t group::option() const
	{
		_THROW_IF_EMPTY();

		if (impl_->radio_logic)
			return impl_->radio_logic->checked();

		throw std::logic_error("the radio_mode of the group is disabled");
	}

    void group::option_check( std::size_t pos, bool check )
    {
 		_THROW_IF_EMPTY();
		return impl_->options.at(pos)->check( check );
    }

	bool group::option_checked(std::size_t pos) const
	{
		_THROW_IF_EMPTY();
		return impl_->options.at(pos)->checked();
	}

	group& group::enable_format_caption(bool format)
	{
		impl_->caption.format(format);

		// if the caption is already set, make sure the layout is updated
		if(!caption().empty())
		{
			impl_->update_div();
			impl_->place_content.collocate();
			API::refresh_window(*this);
		}
		return *this;
	}

	group& group::collocate() noexcept
	{
		impl_->place_content.collocate();
		return *this;
	}

	group& group::div(const char* div_str) noexcept
	{
		if (div_str)
			impl_->usr_div_str = div_str;
		else
			impl_->usr_div_str.clear();

		impl_->update_div();
		return *this;
	}

	group::field_reference group::operator[](const char* field)
	{
		return impl_->place_content.field(field);
	}

	void group::field_display(const char* field_name, bool display)
	{
		impl_->place_content.field_display(field_name, display);
	}

	bool group::field_display(const char* field_name) const
	{
		return impl_->place_content.field_display(field_name);
	}

	void group::erase(window handle)
	{
		impl_->place_content.erase(handle);
	}

	void group::_m_add_child(const char* field, widget* wdg)
	{
		impl_->place_content[field] << wdg->handle();
	}

	void group::_m_init()
	{
		this->div(nullptr);

		auto & outter = impl_->place_content;

		outter[field_title] << impl_->caption;
		outter.collocate();

		impl_->caption.transparent(true);
		color pbg = API::bgcolor(this->parent());
		impl_->caption.bgcolor(pbg.blend(colors::black, 0.025));

		this->bgcolor(pbg.blend(colors::black, 0.05));

		drawing dw(*this);

		//When the group is resized, the drawing is called before moving the caption, but
		//the drawing of group requires the latest position of caption for gradual rectangle.
		//For the requirement, a move event handler is required for listening the change of caption's position.
		impl_->caption.events().move([this](const arg_move&){
			if (align::left != impl_->caption_align)
				API::refresh_window(*this);
		});

		// This drawing function is owner by the owner of dw (the outer panel of the group widget), not by dw !!
		dw.draw([this](paint::graphics& graph)
		{
			auto gap_px = impl_->gap - 1;

			auto const top_round_line = static_cast<int>(impl_->caption_dimension.height) / 2;

			graph.rectangle(true, API::bgcolor(this->parent()));
			graph.round_rectangle(rectangle(point(gap_px, top_round_line),
				nana::size(graph.width() - 2 * gap_px, graph.height() - top_round_line - gap_px)
				),
				3, 3, this->scheme().border, true, this->bgcolor());

			if (background_mode::blending == impl_->caption_mode)
			{
				auto opt_r = API::window_rectangle(impl_->caption);
				if (opt_r)
				{
					rectangle grad_r{ opt_r->position(), nana::size{ opt_r->width + 4, static_cast<unsigned>(top_round_line - opt_r->y) } };

					grad_r.y += top_round_line * 2 / 3;
					grad_r.x -= 2;

					graph.gradual_rectangle(grad_r,
						API::bgcolor(this->parent()), this->bgcolor(), true
						);
				}
			}
		});
	}

	void group::_m_complete_creation()
	{
		widget::_m_complete_creation();
		impl_->create(handle());
		_m_init();
	}

	auto group::_m_caption() const noexcept -> native_string_type
	{
		return impl_->caption.caption_native();
	}

	void group::_m_caption(native_string_type&& str)
	{
		impl_->caption.caption(std::move(str));
		impl_->update_div();
		impl_->place_content.collocate();
	}

}//end namespace nana

