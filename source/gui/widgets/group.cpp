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

namespace nana{

	struct group::implement
	{
		label	caption;
		place	place_content;

		unsigned gap{2};

		void create(window pnl)
		{
			caption.create(pnl);
			caption.caption(STR(""));
			place_content.bind(pnl);
		}

		implement() = default;
		implement(window grp_panel, string titel, bool format, unsigned gap=2)
			: caption (grp_panel, std::move(titel), format),
			  place_content{grp_panel},
			  gap{gap}
		{
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

	group::group(	window    parent,              ///<
					string    titel_   /*={}*/,     ///<
					bool      format /*=false*/,  ///<
					unsigned  gap /*=2*/,         ///<
					rectangle r /*={} */,          ///<
		            bool      vsb /*= true */        ///<
					)
		: panel(parent, r, vsb),
		  impl_(new implement(*this, std::move(titel_), vsb))
	{
		impl_->caption.format(format);
		init();
	}


	group::~group()
	{
	}

	group& group::enable_format_caption(bool format)
	{
		impl_->caption.format(format);
		return *this;
	}

	place& group::get_place()
	{
		return impl_->place_content;
	}


	void group::collocate()
	{
		impl_->place_content.collocate();
	}

	void group::div(const char* div_str)
	{
		::nana::size sz = impl_->caption.measure(1000);

		std::stringstream ss;
		ss << "vert margin=[0," << impl_->gap << "," << impl_->gap << "," << impl_->gap << "]"
			<< " <weight=" << sz.height << " <weight=5> <nanaGroupTitle2015 weight=" << sz.width + 1 << "> >"
			<< " <"<<(div_str ? div_str : "")<<">";

		impl_->place_content.div(ss.str().c_str());

	}

	group::field_reference group::operator[](const char* field)
	{
		return impl_->place_content.field(field);
	}

	void group::_m_add_child(const char* field, widget* wdg)
	{
		impl_->place_content[field] << wdg->handle();
	}

	void group::init()
	{
		this->div(nullptr);

		auto & outter = impl_->place_content;

		outter["nanaGroupTitle2015"] << impl_->caption;
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


		init();
	}

	::nana::string group::_m_caption() const
	{
		return impl_->caption.caption();
	}

	void group::_m_caption(::nana::string&& str)
	{
		return impl_->caption.caption(std::move(str));
	}
}//end namespace nana

