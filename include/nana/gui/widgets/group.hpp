/**
 *	A group widget implementation
 *	Nana C++ Library(http://www.nanaro.org)
 *	Copyright(C) 2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/group.hpp
 *
 *	@contributors: Stefan Pfeifer (st-321), Jinhao, Ariel Vina-Rodriguez (qPCR4vir)
 *
 *	@brief group is a widget used to visually group and layout other widgets.
 */

#ifndef NANA_GUI_WIDGETS_GROUP_HPP
#define NANA_GUI_WIDGETS_GROUP_HPP

#include <nana/gui/place.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/drawing.hpp>

namespace nana{
    class group
	    : public panel<true>
    {
	    place        plc_outer{*this};
        panel<false> content  {*this};
        label        titel/*  {*this}*/;
        place        plc_inner{content};
	    unsigned int gap;
    public:
        group( window    parent,         ///< 
               string    titel_ ={STR("")},     ///< 
               bool      format =false,  ///< Use a formated label?
               unsigned  gap =2,         ///< betwen the content  and the external limit
               rectangle r ={}           ///<
              );
        place& plc  (){ return plc_inner; }
        window inner(){ return content; }
        label& lbl  (){ return titel; }
    };

}//end namespace nana
#endif
