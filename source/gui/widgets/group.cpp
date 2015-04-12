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
 *	@contributors: Stefan Pfeifer (st-321), Jinhao, Ariel Vina-Rodriguez (qPCR4vir)
 *
 *	@brief group is a widget used to visually group and layout other widgets.
 */


#include <nana/gui/widgets/group.hpp>

namespace nana{
 group::group( widget   &owner,          ///<
               string    titel_ /*={}*/,     ///<
               bool      format /*=false*/,  ///<
               unsigned  gap /*=2*/,         ///<
               rectangle r /*={} */          ///<
              )
		    : panel (owner, r),
              titel (*this, titel_)
	        {
                titel.format(format);
                ::nana::size sz = titel.measure(1000);
                std::stringstream ft;

                ft << "vertical margin=[0," << gap << "," << gap << "," << gap << "]"
                   << " <weight=" << sz.height << " <weight=5> <titel weight=" << sz.width+1 << "> >"
                   << " <content>";
		        plc_outer.div(ft.str().c_str());

                plc_outer["titel"  ] << titel;
		        plc_outer["content"] << content;
		        plc_outer.collocate();

                color obg = owner.bgcolor();
                titel.bgcolor(obg.blend(colors::black, 0.975) );
                color bg=obg.blend(colors::black, 0.950 );
                bgcolor(bg);

                drawing dw(*this);
		        dw.draw([gap,sz,bg,obg](paint::graphics& graph)
		        {
			        graph.rectangle(true, obg);
                    graph.round_rectangle(rectangle(       point ( gap-1                 ,   sz.height/2                       ), 
                                                     nana::size  (graph.width()-2*(gap-1),   graph.height()-sz.height/2-(gap-1))
                                                    ),
                                          3,3, colors::gray_border,     true, bg);
               });
            }

}//end namespace nana

