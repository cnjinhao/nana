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
		label			caption;
		panel<false>	content;
		place place_outter;
		place place_content;

		implement(group* host):
			caption(*host),
			content(*host),
			place_outter(*host),
			place_content(content)
		{}
	};

	group::group(	window    parent,              ///<
					std::wstring    titel_ /*={}*/,     ///<
					bool      format /*=false*/,  ///<
					unsigned  gap /*=2*/,         ///<
					rectangle r /*={} */          ///<
              )
			  :	panel (parent, r),
				impl_(new implement(this))
	{
		impl_->caption.format(format);
		::nana::size sz = impl_->caption.measure(1000);
		std::stringstream ft;
		
		ft << "vert margin=[0," << gap << ","<<gap<<","<<gap<<"]"
		   << " <weight=" << sz.height << " <weight=5> <titel weight=" << sz.width+1 << "> >"
		   << " <content>";

		auto & outter = impl_->place_outter;

		outter.div(ft.str().c_str());
		
		outter["titel"] << impl_->caption;
		outter["content"] << impl_->content;
		outter.collocate();
		
		color pbg =  API::bgcolor( parent);
		impl_->caption.bgcolor(pbg.blend(colors::black, 0.975) );
		color bg=pbg.blend(colors::black, 0.950 );
		
		bgcolor(pbg);
		impl_->content.bgcolor(bg);
		
		drawing dw(*this);
		
		// This drawing function is owner by the onwer of dw (the outer panel of the group widget), not by dw !!
		dw.draw([gap, sz, bg, pbg](paint::graphics& graph)
		{
			graph.rectangle(true, pbg);
			graph.round_rectangle(rectangle(point(gap - 1, sz.height / 2),
				nana::size(graph.width() - 2 * (gap - 1), graph.height() - sz.height / 2 - (gap - 1))
				),
				3, 3, colors::gray_border, true, bg);
		});
	}

	group::~group()
	{
	}

	place& group::get_place()
	{
		return impl_->place_content;
	}

	window group::inner()
	{
		return impl_->content;
	}

	void group::_m_add_child(const char* field, widget* wdg)
	{
		impl_->place_content[field] << wdg->handle();
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

