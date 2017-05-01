#ifndef NANA_DETAIL_MSG_PACKET_HPP
#define NANA_DETAIL_MSG_PACKET_HPP
#include <X11/Xlib.h>
#include <vector>
#include <nana/deploy.hpp>

namespace nana
{
namespace detail
{
	struct msg_packet_tag
	{
		enum kind_t{kind_xevent, kind_mouse_drop, kind_cleanup};
		kind_t kind;
		union
		{
			XEvent xevent;

			Window packet_window; //Avaiable if the packet is not kind_xevent
			struct mouse_drop_tag
			{
				Window window;
				int x;
				int y;
				std::vector<std::string> * files;
			}mouse_drop;
		}u;
	};
}//end namespace detail
}//end namespace nana
#endif

