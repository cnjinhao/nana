/*
 *	X-Window XDND Protocol Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/detail/posix/xdnd_protocol.hpp
 *
 *  The XDS is not supported.
 */
#ifndef NANA_DETAIL_POSIX_XDND_PROTOCOL_INCLUDED
#define NANA_DETAIL_POSIX_XDND_PROTOCOL_INCLUDED

#include "platform_spec.hpp"
#include <nana/filesystem/filesystem.hpp>
#include <vector>

#include <X11/Xcursor/Xcursor.h>


#include <iostream> //debug

namespace nana{
	namespace detail
	{


	class shared_icons
	{
	public:
		shared_icons()
		{
			path_ = "/usr/share/icons/";
			ifs_.open(path_ + "default/index.theme");
		}

		std::string cursor(const std::string& name)
		{
			auto theme = _m_read("Icon Theme", "Inherits");

			return path_ + theme + "/cursors/" + name;
		}
	private:
		std::string _m_read(const std::string& category, const std::string& key)
		{
			ifs_.seekg(0, std::ios::beg);

			bool found_cat = false;
			while(ifs_.good())
			{
				std::string text;
				std::getline(ifs_, text);

				if(0 == text.find('['))
				{
					if(found_cat)
						break;

					if(text.find(category + "]") != text.npos)
					{
						found_cat = true;
					}
				}
				else if(found_cat && (text.find(key + "=") == 0))
				{
					return text.substr(key.size() + 1);
				}
			}

			return {};
		}
	private:
		std::string path_;
		std::ifstream ifs_;
	};

	struct xdnd_data
	{
		std::vector<std::filesystem::path> files;
	};

	class xdnd_protocol
	{
	public:
		enum class xdnd_status_state
		{
			normal,
			position,
			drop,		//Use the 'accept' flag of XdndStatus when mouse has released(XdndDrop has been sent).
			status_ignore
		};

		xdnd_protocol(Window source):
			spec_(nana::detail::platform_spec::instance()),
			source_(source)
		{
			auto disp = spec_.open_display();
			detail::platform_scope_guard lock;
			::XSetSelectionOwner(disp, spec_.atombase().xdnd_selection, source, CurrentTime);
			std::cout<<"XSetSelectionOwner "<<source<<std::endl;

			shared_icons icons;
			cursor_.dnd_move = ::XcursorFilenameLoadCursor(disp, icons.cursor("dnd-move").c_str());
			cursor_.dnd_none = ::XcursorFilenameLoadCursor(disp, icons.cursor("dnd-none").c_str());	
		}

		~xdnd_protocol()
		{
			auto disp = spec_.open_display();
			::XFreeCursor(disp, cursor_.dnd_move);
			::XFreeCursor(disp, cursor_.dnd_none);
		}

		void mouse_move(Window wd, const nana::point& pos)
		{
			if(wd != target_)
			{
				_m_xdnd_leave();
			
				if(_m_xdnd_enter(wd))
					_m_xdnd_position(pos);
			}
			else
				_m_xdnd_position(pos);
		}

		void mouse_leave()
		{
			_m_xdnd_leave();
		}

		void mouse_release()
		{
			_m_xdnd_drop();
		}

		//Return true to exit xdnd_protocol event handler
		bool client_message(const ::XClientMessageEvent& xclient)
		{
			auto & atombase = spec_.atombase();

			if(atombase.xdnd_status == xclient.message_type)
			{
				std::cout<<"Event: XdndStatus"<<std::endl;

				if(xdnd_status_state::position != xstate_ && xdnd_status_state::drop != xstate_)
					return false;

				Window target_wd = static_cast<Window>(xclient.data.l[0]);
				bool is_accepted_by_target = (xclient.data.l[1] & 1);

				std::cout<<"XdndStatus: Accepted="<<is_accepted_by_target<<", target="<<is_accepted_by_target<<std::endl;

				if(xclient.data.l[1] & 0x2)
				{
					rectangle rct{
						static_cast<int>(xclient.data.l[2] >> 16),
						static_cast<int>(xclient.data.l[2] & 0xFFFF),
						static_cast<unsigned>(xclient.data.l[3] >> 16),
						static_cast<unsigned>(xclient.data.l[3] & 0xFFFF)
					};
					
					if(!rct.empty())
					{
						mvout_table_[target_wd] = rct;
						std::cout<<". rct=("<<rct.x<<","<<rct.y<<","<<rct.width<<","<<rct.height<<")";
					}
				}
				std::cout<<std::endl;

				_m_cursor(is_accepted_by_target);



				if((!is_accepted_by_target) && (xdnd_status_state::drop == xstate_))
				{
					_m_xdnd_leave();
					return true;
				}

				xstate_ = xdnd_status_state::normal;
			}
			else if(atombase.xdnd_finished == xclient.message_type)
				return true;

			return false;
		}

		void selection_request(const ::XSelectionRequestEvent& xselectionrequest, const xdnd_data& data)
		{
			if(spec_.atombase().xdnd_selection == xselectionrequest.selection)
			{
				std::cout<<"Event SelectionRequest: XdndSelection"<<std::endl;

			    ::XEvent evt;
			    evt.xselection.type = SelectionNotify;
			    evt.xselection.display = xselectionrequest.display;
			    evt.xselection.requestor = xselectionrequest.requestor;
			    evt.xselection.selection = xselectionrequest.selection;
			    evt.xselection.target = 0;
			    evt.xselection.property = 0;
			    evt.xselection.time = xselectionrequest.time;

			    auto property_name = ::XGetAtomName(spec_.open_display(), xselectionrequest.property); //debug

			    std::cout<<"SelectionRequest"<<std::endl;
			    std::cout<<"    xdnd_selection:"<<spec_.atombase().xdnd_selection<<std::endl;
			    std::cout<<"    text_uri_list :"<<spec_.atombase().text_uri_list<<std::endl;
			    std::cout<<"    selection="<<xselectionrequest.selection<<std::endl;
			    std::cout<<"    property ="<<xselectionrequest.property<<", name="<<property_name<<std::endl;
			    std::cout<<"    target   ="<<xselectionrequest.target<<std::endl;
			    ::XFree(property_name);

			    if(xselectionrequest.target == spec_.atombase().text_uri_list)
			    {
			    	std::cout<<"SelectionRequest target = text_uri_list";

				    if(data.files.size())
				    {
				    	std::string uri_list;
				    	for(auto& file : data.files)
				    	{
				    		uri_list += "file://";
				    		uri_list += file.u8string();
				    		uri_list += "\r\n";
				    	}

				    	std::cout<<". URIs="<<uri_list<<std::endl;

				    	::XChangeProperty (spec_.open_display(),
				    			xselectionrequest.requestor,
				    			xselectionrequest.property, 
				    			xselectionrequest.target,
				    			8, PropModeReplace,
	                            reinterpret_cast<unsigned char*>(&uri_list.front()), uri_list.size() + 1);
	            	
	            		evt.xselection.property = xselectionrequest.property;
	            		evt.xselection.target = xselectionrequest.target;

				    }
			    }
			    else
			    	std::cout<<"SelectionRequest target = "<<xselectionrequest.target<<std::endl;

			    platform_scope_guard lock;
			    ::XSendEvent(spec_.open_display(), xselectionrequest.requestor, False, 0, &evt);
			    ::XFlush(spec_.open_display());

			    if(0 == evt.xselection.target)
			    	_m_xdnd_leave();
			}
		}
	private:

		bool _m_xdnd_enter(Window wd)
		{
			//xdnd version of the window
			auto xdnd_ver = _m_xdnd_aware(wd);
			if(0 == xdnd_ver)
				return false;

			target_ = wd;
			std::cout<<"Send XdndEnter, text/uri-list="<<spec_.atombase().text_uri_list<<std::endl;
			_m_client_msg(spec_.atombase().xdnd_enter, (xdnd_ver << 24), spec_.atombase().text_uri_list, XA_STRING);

			return true;
		}

		void _m_xdnd_position(const nana::point& pos)
		{
			if(xdnd_status_state::normal != xstate_)
				return;

			auto i = mvout_table_.find(target_);
			if(i != mvout_table_.end() && i->second.is_hit(pos))
				return;

			std::cout<<"Send: XdndPosition"<<std::endl;

			xstate_ = xdnd_status_state::position;
			//Send XdndPosition
			long position = (pos.x << 16 | pos.y);
			_m_client_msg(spec_.atombase().xdnd_position, 0, position, CurrentTime, spec_.atombase().xdnd_action_copy);
		}

		void _m_xdnd_leave()
		{
			::XUndefineCursor(spec_.open_display(), source_);

			if(target_)
			{
				std::cout<<"Send: XdndLeave"<<std::endl;
				_m_client_msg(spec_.atombase().xdnd_leave, 0, 0, 0);
				target_ = 0;
			}
		}

		void _m_xdnd_drop()
		{
			::XUndefineCursor(spec_.open_display(), source_);
			xstate_ = xdnd_status_state::drop;
			std::cout<<"Send: XdndDrop"<<std::endl;
			_m_client_msg(spec_.atombase().xdnd_drop, 0, CurrentTime, 0);
			target_ = 0;
		}
	private:
		//dndversion<<24, fl_XdndURIList, XA_STRING, 0
		void _m_client_msg(Atom xdnd_atom, long data1, long data2, long data3, long data4 = 0)
		{
			auto const display = spec_.open_display();
			XEvent evt;
			::memset(&evt, 0, sizeof evt);
			evt.xany.type = ClientMessage;
			evt.xany.display = display;
			evt.xclient.window = target_;
			evt.xclient.message_type = xdnd_atom;
			evt.xclient.format = 32;

			//Target window
			evt.xclient.data.l[0] = source_;
			
			evt.xclient.data.l[1] = data1;
			evt.xclient.data.l[2] = data2;
			evt.xclient.data.l[3] = data3;
			evt.xclient.data.l[4] = data4;

			::XSendEvent(display, target_, True, NoEventMask, &evt);
		}

		// Returns the XDND version of specified window
		//@return the XDND version. If the specified window does not have property XdndAware, it returns 0
		int _m_xdnd_aware(Window wd)
		{
			Atom actual; int format; unsigned long count, remaining;
			unsigned char *data = 0;
			::XGetWindowProperty(spec_.open_display(), wd, spec_.atombase().xdnd_aware, 
				0, 4, False, XA_ATOM, &actual, &format, &count, &remaining, &data);

			int version = 0;
			if ((actual == XA_ATOM) && (format==32) && count && data)
			{
				version = int(*(Atom*)data);
				std::cout<<"Get:XdndAware version:"<<version<<std::endl;
			}

			if (data)
				::XFree(data);
			return version;
		}

		void _m_cursor(bool accepted)
		{
			::XDefineCursor(spec_.open_display(), source_, (accepted ? cursor_.dnd_move : cursor_.dnd_none));
		}

#if 0 //deprecated
		//Check if window has a property
		static bool _m_has_property(Window wd, Atom atom, unsigned char** data)
		{
	        Atom type = 0;
	        int f;
	        unsigned long n, a;

	        unsigned char * data_back = nullptr;
	        if(nullptr == data)
	        	data = &data_back;

	        if (::XGetWindowProperty(spec_.open_display(), wd, atom, 0, 0, False, AnyPropertyType, &type, &f,&n,&a,data) == Success)
	        {
	        	//release the *data if failed to get the property or unspecified output buffer
	        	if((0 == type) || data_back)
	        		::XFree(*data);

			    return (0 != type);
	        }
	        return false;
		}
#endif
	private:
		nana::detail::platform_spec& spec_;
		Window const source_;
		Window target_{ 0 };
		xdnd_status_state xstate_{xdnd_status_state::normal};
		std::map<Window, nana::rectangle> mvout_table_;

		struct cursor_rep
		{
			Cursor dnd_move{ 0 };
			Cursor dnd_none{ 0 };
		}cursor_;
	}; //end class xdnd_protocol
	}
}

#endif