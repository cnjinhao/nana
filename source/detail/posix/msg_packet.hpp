#ifndef NANA_DETAIL_MSG_PACKET_HPP
#define NANA_DETAIL_MSG_PACKET_HPP
#include <X11/Xlib.h>
#include <vector>
#include <nana/deploy.hpp>
#include <nana/filesystem/filesystem.hpp>

namespace nana
{
namespace detail
{
	enum class propagation_chain
	{
		exit,	//Exit the chain 
		stop,	//Stop propagating
		pass 	//propagate
	};

	struct msg_packet_tag
	{
		enum class pkt_family{xevent, mouse_drop, cleanup};
		pkt_family kind;
		union
		{
			XEvent xevent;

			Window packet_window; //Avaiable if the packet is not kind_xevent
			struct mouse_drop_tag
			{
				Window window;
				int x;
				int y;
				std::vector<std::filesystem::path> * files;
			}mouse_drop;
		}u;
	};
}//end namespace detail
}//end namespace nana
#endif

