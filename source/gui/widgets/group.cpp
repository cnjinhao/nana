/**
 *	A group widget implementation
 *	Nana C++ Library(http://www.nanaro.org)
 *	Copyright(C) 2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/group.cpp
 *
 *	@Author: Stefan Pfeifer(st-321), Ariel Vina-Rodriguez (qPCR4vir)
 *
 *	@brief group is a widget used to visually group and layout other widgets.
 */


#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/drawing.hpp>
#include <nana/gui/widgets/checkbox.hpp>

#define _THROW_IF_EMPTY()\
	if(empty())	\
		throw std::logic_error("the group is invalid");

namespace nana{

	static const char* field_title = "__nana_group_title__";
	static const char* field_options = "__nana_group_options__";

	struct group::implement
	{
		label	caption;
		place	place_content;
		unsigned gap{2};
		std::string usr_div_str;

		std::vector<std::unique_ptr<checkbox>> options;
		radio_group * radio_logic{nullptr};

		implement() = default;

		implement(window grp_panel, ::std::string titel, bool vsb, unsigned gap=2)
			: caption (grp_panel, std::move(titel), vsb),
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
			::nana::size sz = caption.measure(1000);

			std::stringstream ss;
			ss << "vert margin=[0," << gap << "," << gap + 5 << "," << gap << "]"
				<< " <weight=" << sz.height << " <weight=5> <" << field_title << " weight=" << sz.width + 1 << "> >"
				<< "<<vert margin=5 " << field_options << ">";

			if (!usr_div_str.empty())
				ss << "<" << usr_div_str << ">>";
			else
				ss << ">";

			place_content.div(ss.str().c_str());

			if (options.empty())
				place_content.field_display(field_options, false);
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

	group::group(window parent, ::std::string titel, bool formatted, unsigned  gap, const rectangle& r, bool vsb)
		: panel(parent, r, vsb),
		  impl_(new implement(*this, std::move(titel), vsb, gap))
	{
		impl_->caption.format(formatted);
		_m_init();
	}

	group::~group()
	{
		delete impl_->radio_logic;
	}

	group& group::add_option(std::string text)
	{
		_THROW_IF_EMPTY()

		impl_->options.emplace_back(new checkbox(handle()));
		auto & opt = impl_->options.back();
		opt->transparent(true);
		opt->caption(std::move(text));
		impl_->place_content[field_options] << *opt;
		impl_->place_content.field_display(field_options, true);
		impl_->place_content.collocate();

		if (impl_->radio_logic)
			impl_->radio_logic->add(*opt);

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

	bool group::option_checked(std::size_t pos) const
	{
		_THROW_IF_EMPTY();
		return impl_->options.at(pos)->checked();
	}

	group& group::enable_format_caption(bool format)
	{
		impl_->caption.format(format);
		return *this;
	}

	group& group::collocate() throw ()
	{
		impl_->place_content.collocate();
		return *this;
	}

	group& group::div(const char* div_str) throw ()
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

		color pbg = API::bgcolor(this->parent());
		impl_->caption.bgcolor(pbg.blend(colors::black, 0.975));
		color bg = pbg.blend(colors::black, 0.950);

		bgcolor(bg);

		drawing dw(*this);

		::nana::size sz = impl_->caption.measure(1000);

		// This drawing function is owner by the onwer of dw (the outer panel of the group widget), not by dw !!
		dw.draw([this, sz, bg, pbg](paint::graphics& graph)
		{
			auto gap_px = impl_->gap - 1;

			graph.rectangle(true, pbg);
			graph.round_rectangle(rectangle(point(gap_px, sz.height / 2),
				nana::size(graph.width() - 2 * gap_px, graph.height() - sz.height / 2 - gap_px)
				),
				3, 3, colors::gray_border, true, bg);
		});
	}

	void group::_m_complete_creation()
	{
		panel::_m_complete_creation();

		impl_->create(handle());


		_m_init();
	}

	auto group::_m_caption() const throw() -> native_string_type
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

