/**
*	Drag and Drop Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2018 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/dragdrop.cpp
*	@author: Jinhao(cnjinhao@hotmail.com)
*/

#include <nana/gui/dragdrop.hpp>
#include <nana/gui/programming_interface.hpp>

#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/basic_window.hpp>

#include <map>
#include <set>

#ifdef NANA_WINDOWS
#	include <windows.h>
#	include <oleidl.h>
#	include <comdef.h>
#	include <Shlobj.h>
#elif defined(NANA_X11)
#	include "../detail/posix/platform_spec.hpp"
#	include <nana/gui/detail/native_window_interface.hpp>
#	include <X11/Xcursor/Xcursor.h>
#	include <fstream>
#endif

namespace nana
{
	/// drop_association
	/**
	 * This class is used for querying whether tow windows have a connection of drag and drop
	 */
	class drop_association
	{
	public:
		void add(window source, window target)
		{
			assoc_[source].insert(target);
		}

		void erase(window wd)
		{
			assoc_.erase(wd);

			for (auto & assoc : assoc_)
				assoc.second.erase(wd);
		}

		bool has(window source, window target) const
		{
			auto i = assoc_.find(source);
			if (i != assoc_.end())
				return (0 != i->second.count(target));
			
			return false;
		}
	private:
		std::map<window, std::set<window>> assoc_;
	};

#ifdef NANA_WINDOWS
	template<typename Interface, const IID& iid>
	class win32com_iunknown : public Interface
	{
	public:
		//Implements IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
		{
			if (riid == IID_IUnknown || riid == iid) {
				*ppv = static_cast<IUnknown*>(this);
				AddRef();
				return S_OK;
			}
			*ppv = NULL;
			return E_NOINTERFACE;
		}

		STDMETHODIMP_(ULONG) AddRef()
		{
			return InterlockedIncrement(&ref_count_);
		}

		STDMETHODIMP_(ULONG) Release()
		{
			LONG cRef = InterlockedDecrement(&ref_count_);
			if (cRef == 0) delete this;
			return cRef;
		}
	private:
		LONG ref_count_{ 1 };
	};


	class win32com_drop_target : public IDropTarget
	{
	public:
		win32com_drop_target(const drop_association& drop_assoc) :
			drop_assoc_(drop_assoc)
		{}

		void set_source(window wd)
		{
			source_window_ = wd;
		}
	public:
		//Implements IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
		{
			if (riid == IID_IUnknown || riid == IID_IDropTarget) {
				*ppv = static_cast<IUnknown*>(this);
				AddRef();
				return S_OK;
			}
			*ppv = NULL;
			return E_NOINTERFACE;
		}

		STDMETHODIMP_(ULONG) AddRef()
		{
			return InterlockedIncrement(&ref_count_);
		}

		STDMETHODIMP_(ULONG) Release()
		{
			LONG cRef = InterlockedDecrement(&ref_count_);
			if (cRef == 0) delete this;
			return cRef;
		}

	private:
		// IDropTarget
		STDMETHODIMP DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			*pdwEffect &= DROPEFFECT_COPY;
			return S_OK;
		}

		STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			auto hovered_wd = API::find_window(point(pt.x, pt.y));
			if ((hovered_wd && (hovered_wd == source_window_)) || drop_assoc_.has(source_window_, hovered_wd))
				*pdwEffect &= DROPEFFECT_COPY;
			else
				*pdwEffect = DROPEFFECT_NONE;

			return S_OK;
		}

		STDMETHODIMP DragLeave()
		{
			return E_NOTIMPL;
		}

		STDMETHODIMP Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			return E_NOTIMPL;
		}
	private:
		LONG ref_count_{ 1 };

		window source_window_{ nullptr };
		const drop_association& drop_assoc_;
	};

	class drop_source : public win32com_iunknown<IDropSource, IID_IDropSource>
	{
	public:
		drop_source(window wd) :
			window_handle_(wd)
		{}

		window source() const
		{
			return window_handle_;
		}
	private:
		// IDropSource
		STDMETHODIMP QueryContinueDrag(BOOL esc_pressed, DWORD key_state) override
		{
			if (esc_pressed)
				return DRAGDROP_S_CANCEL;

			//Drop the object if left button is released.
			if (0 == (key_state & (MK_LBUTTON)))
				return DRAGDROP_S_DROP;
			
			return S_OK;
		}

		STDMETHODIMP GiveFeedback(DWORD effect) override
		{
			return DRAGDROP_S_USEDEFAULTCURSORS;
		}
	private:
		window const window_handle_;
	};

	class drop_data : public win32com_iunknown<IDataObject, IID_IDataObject>
	{
		struct medium
		{
			STGMEDIUM * stgmedium;
			FORMATETC * format;
		};
	public:
		STDMETHODIMP GetData(FORMATETC *request_format, STGMEDIUM *pmedium) override
		{
			if (!(request_format && pmedium))
				return E_INVALIDARG;

			pmedium->hGlobal = nullptr;

			for (auto & med : mediums_)
			{
				if ((request_format->tymed & med.format->tymed) &&
					(request_format->dwAspect == med.format->dwAspect) &&
					(request_format->cfFormat == med.format->cfFormat))
				{
					return _m_copy_medium(pmedium, med.stgmedium, med.format);
				}
			}
			
			return DV_E_FORMATETC;
		}

		STDMETHODIMP GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium) override
		{
			return E_NOTIMPL;
		}

		STDMETHODIMP QueryGetData(FORMATETC *pformatetc) override
		{
			if (NULL == pformatetc)
				return E_INVALIDARG;
			
			if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
				return DV_E_DVASPECT;
			
			HRESULT result = DV_E_TYMED;

			for(auto & med : mediums_)
			{
				if (med.format->tymed & pformatetc->tymed)
				{
					if (med.format->cfFormat == pformatetc->cfFormat)
						return S_OK;
					
					result = DV_E_FORMATETC;
				}
			}
			return result;
		}

		STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut) override
		{
			return E_NOTIMPL;
		}

		STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease) override
		{
			if (!(pformatetc && pmedium))
				return E_INVALIDARG;

			if (pformatetc->tymed != pmedium->tymed)
				return E_FAIL;

			medium retain;
			retain.format = new FORMATETC;
			retain.stgmedium = new (std::nothrow) STGMEDIUM;
			if (nullptr == retain.stgmedium)
			{
				delete retain.format;
				return E_FAIL;
			}

			std::memset(retain.format, 0, sizeof(FORMATETC));
			std::memset(retain.stgmedium, 0, sizeof(STGMEDIUM));

			*retain.format = *pformatetc;

			_m_copy_medium(retain.stgmedium, pmedium, pformatetc);

			if (TRUE == fRelease)
				::ReleaseStgMedium(pmedium);

			mediums_.emplace_back(retain);
			return S_OK;
		}

		STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc) override
		{
			if (NULL == ppenumFormatEtc)
				return E_INVALIDARG;

			if (DATADIR_GET != dwDirection)
				return E_NOTIMPL;

			*ppenumFormatEtc = NULL;

			FORMATETC rgfmtetc[] =
			{
				{ CF_UNICODETEXT, NULL, DVASPECT_CONTENT, 0, TYMED_HGLOBAL },
			};
			return ::SHCreateStdEnumFmtEtc(ARRAYSIZE(rgfmtetc), rgfmtetc, ppenumFormatEtc);
		}

		STDMETHODIMP DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) override
		{
			return OLE_E_ADVISENOTSUPPORTED;
		}

		STDMETHODIMP DUnadvise(DWORD dwConnection) override
		{
			return OLE_E_ADVISENOTSUPPORTED;
		}

		STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise) override
		{
			return OLE_E_ADVISENOTSUPPORTED;
		}
	private:
		static HRESULT _m_copy_medium(STGMEDIUM* stgmed_dst, STGMEDIUM* stgmed_src, FORMATETC* fmt_src)
		{
			if (!(stgmed_dst && stgmed_src && fmt_src))
				return E_INVALIDARG;
			
			switch (stgmed_src->tymed)
			{
			case TYMED_HGLOBAL:
				stgmed_dst->hGlobal = (HGLOBAL)OleDuplicateData(stgmed_src->hGlobal, fmt_src->cfFormat, 0);
				break;
			case TYMED_GDI:
				stgmed_dst->hBitmap = (HBITMAP)OleDuplicateData(stgmed_src->hBitmap, fmt_src->cfFormat, 0);
				break;
			case TYMED_MFPICT:
				stgmed_dst->hMetaFilePict = (HMETAFILEPICT)OleDuplicateData(stgmed_src->hMetaFilePict, fmt_src->cfFormat, 0);
				break;
			case TYMED_ENHMF:
				stgmed_dst->hEnhMetaFile = (HENHMETAFILE)OleDuplicateData(stgmed_src->hEnhMetaFile, fmt_src->cfFormat, 0);
				break;
			case TYMED_FILE:
				stgmed_dst->lpszFileName = (LPOLESTR)OleDuplicateData(stgmed_src->lpszFileName, fmt_src->cfFormat, 0);
				break;
			case TYMED_ISTREAM:
				stgmed_dst->pstm = stgmed_src->pstm;
				stgmed_src->pstm->AddRef();
				break;
			case TYMED_ISTORAGE:
				stgmed_dst->pstg = stgmed_src->pstg;
				stgmed_src->pstg->AddRef();
				break;
			case TYMED_NULL:
			default:
				break;
			}
			stgmed_dst->tymed = stgmed_src->tymed;
			stgmed_dst->pUnkForRelease = nullptr;
			if (stgmed_src->pUnkForRelease)
			{
				stgmed_dst->pUnkForRelease = stgmed_src->pUnkForRelease;
				stgmed_src->pUnkForRelease->AddRef();
			}
			return S_OK;
		}
	private:
		std::vector<medium> mediums_;
	};
#elif defined(NANA_X11)
	class x11_dragdrop: public detail::dragdrop_interface
	{

	};

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
#endif

	class dragdrop_service
	{
		dragdrop_service() = default;
	public:
		static dragdrop_service& instance()
		{
			static dragdrop_service serv;
			return serv;
		}

		void create_dragdrop(window wd)
		{
			auto native_wd = API::root(wd);
			if (nullptr == native_wd)
				return;

#ifdef NANA_WINDOWS
			if(table_.empty())
				::OleInitialize(nullptr);

			win32com_drop_target* drop_target = nullptr;

			auto i = table_.find(native_wd);
			if (i == table_.end())
			{
				drop_target = new win32com_drop_target{drop_assoc_};
				::RegisterDragDrop(reinterpret_cast<HWND>(native_wd), drop_target);

				table_[native_wd] = drop_target;
			}
			else
			{
				drop_target = i->second;
				drop_target->AddRef();
			}
#elif defined(NANA_X11)
			auto ddrop = new x11_dragdrop;
			if(!_m_spec().register_dragdrop(native_wd, ddrop))
				delete ddrop;
#endif
		}

		void remove(window wd)
		{
#ifdef NANA_WINDOWS
			auto i = table_.find(API::root(wd));
			if (i != table_.end())
			{
				if (0 == i->second->Release())
					table_.erase(i);
			}
#elif defined(NANA_X11)
			auto ddrop = _m_spec().remove_dragdrop(API::root(wd));
			delete ddrop;
#endif
			drop_assoc_.erase(wd);

		}

		bool dragdrop(window drag_wd)
		{
#ifdef NANA_WINDOWS
			auto i = table_.find(API::root(drag_wd));
			if (table_.end() == i)
				return false;

			auto drop_src = new drop_source{ drag_wd };
			auto drop_dat = new (std::nothrow) drop_data;
			if (!drop_dat)
			{
				delete drop_src;
				return false;
			}

			i->second->set_source(drag_wd);


			DWORD eff;
			auto status = ::DoDragDrop(drop_dat, drop_src, DROPEFFECT_COPY, &eff);

			i->second->set_source(nullptr);

			return true;
#elif defined(NANA_X11)
			auto const native_wd = reinterpret_cast<Window>(API::root(drag_wd));

			{
				detail::platform_scope_guard lock;
				::XSetSelectionOwner(_m_spec().open_display(), _m_spec().atombase().xdnd_selection, native_wd, CurrentTime);
			}


			hovered_.window_handle = nullptr;
			hovered_.native_wd = 0;
			window target_wd = 0;
			auto& atombase = _m_spec().atombase();
			//while(true)
			{

				_m_spec().msg_dispatch([this, drag_wd, native_wd, &target_wd, &atombase](const detail::msg_packet_tag& msg_pkt) mutable{
					if(detail::msg_packet_tag::pkt_family::xevent == msg_pkt.kind)
					{
						auto const disp = _m_spec().open_display();
						if (MotionNotify == msg_pkt.u.xevent.type)
						{
							auto pos = API::cursor_position();
							auto native_cur_wd = reinterpret_cast<Window>(detail::native_interface::find_window(pos.x, pos.y));

							if(hovered_.native_wd != native_cur_wd)
							{
								if(hovered_.native_wd)
								{
									_m_free_cursor();
									::XUndefineCursor(disp, hovered_.native_wd);
								}

								_m_client_msg(native_cur_wd, native_wd, atombase.xdnd_enter, atombase.text_uri_list, XA_STRING);
								hovered_.native_wd = native_cur_wd;
							}

							auto cur_wd = API::find_window(API::cursor_position());

							if(hovered_.window_handle != cur_wd)
							{
								_m_free_cursor();

								hovered_.window_handle = cur_wd;

								if((drag_wd == cur_wd) || drop_assoc_.has(drag_wd, cur_wd))
									hovered_.cursor = ::XcursorFilenameLoadCursor(disp, icons_.cursor("dnd-move").c_str());
								else
									hovered_.cursor = ::XcursorFilenameLoadCursor(disp, icons_.cursor("dnd-none").c_str());
								::XDefineCursor(disp, native_cur_wd, hovered_.cursor);								
							}
						}
						else if(msg_pkt.u.xevent.type == ButtonRelease)
						{
							target_wd = API::find_window(API::cursor_position());
							::XUndefineCursor(disp, hovered_.native_wd);
							_m_free_cursor();
							return true;
						}
						
					}
					return false;
				});
			}

			return (nullptr != target_wd);
#endif
			return false;
		}

		drop_association& drop_assoc()
		{
			return drop_assoc_;
		}
#ifdef NANA_X11
	private:
		static nana::detail::platform_spec & _m_spec()
		{
			return nana::detail::platform_spec::instance();
		}

		//dndversion<<24, fl_XdndURIList, XA_STRING, 0
		static void _m_client_msg(Window wd_target, Window wd_src, Atom xdnd_atom, Atom data, Atom data_type)
		{
			auto const display = _m_spec().open_display();
			XEvent evt;
			::memset(&evt, 0, sizeof evt);
			evt.xany.type = ClientMessage;
			evt.xany.display = display;
			evt.xclient.window = wd_target;
			evt.xclient.message_type = xdnd_atom;
			evt.xclient.format = 32;

			//Target window
			evt.xclient.data.l[0] = wd_src;
			//Accept set
			evt.xclient.data.l[1] = 1;
			evt.xclient.data.l[2] = data;
			evt.xclient.data.l[3] = data_type;
			evt.xclient.data.l[4] = 0;

			::XSendEvent(display, wd_target, True, NoEventMask, &evt);

		}

		static int _m_xdnd_aware(Window wd)
		{
			Atom actual; int format; unsigned long count, remaining;
			unsigned char *data = 0;
			XGetWindowProperty(_m_spec().open_display(), wd, _m_spec().atombase().xdnd_aware, 
				0, 4, False, XA_ATOM, &actual, &format, &count, &remaining, &data);

			int version = 0;
			if ((actual == XA_ATOM) && (format==32) && count && data)
				version = int(*(Atom*)data);

			if (data)
				::XFree(data);
			return version;
		}

		void _m_free_cursor()
		{
			if(hovered_.cursor)
			{
				::XFreeCursor(_m_spec().open_display(), hovered_.cursor);
				hovered_.cursor = 0;
			}

		}
#endif
	private:
		drop_association drop_assoc_;
#ifdef NANA_WINDOWS
		std::map<native_window_type, win32com_drop_target*> table_;
#elif defined (NANA_X11)
		shared_icons icons_;
		struct hovered_status
		{
			Window native_wd{0};
			window window_handle{nullptr};

			unsigned shape{0};
			Cursor cursor{0};
		}hovered_;
#endif
	};



	struct simple_dragdrop::implementation
	{
		window window_handle;
		std::function<bool()> predicate;
		std::map<window, std::function<void()>> targets;

		bool dragging{ false };

#ifdef NANA_X11
		bool cancel()
		{
			if (!dragging)
				return false;

			if (API::is_window(window_handle))
			{
				dragging = true;
				using basic_window = ::nana::detail::basic_window;
				auto real_wd = reinterpret_cast<::nana::detail::basic_window*>(window_handle);
				real_wd->other.dnd_state = dragdrop_status::not_ready;
			}

			API::release_capture(window_handle);
			dragging = false;

			return true;
		}
#endif
	};



	simple_dragdrop::simple_dragdrop(window drag_wd) :
		impl_(new implementation)
	{
		dragdrop_service::instance().create_dragdrop(drag_wd);

		if (!API::is_window(drag_wd))
		{
			delete impl_;
			throw std::invalid_argument("simple_dragdrop: invalid window handle");
		}

		impl_->window_handle = drag_wd;
		API::dev::window_draggable(drag_wd, true);

		auto & events = API::events<>(drag_wd);

#if 1 //#ifdef NANA_WINDOWS
		events.mouse_down.connect_unignorable([this](const arg_mouse& arg){
			if (arg.is_left_button() && API::is_window(impl_->window_handle))
			{
				impl_->dragging = ((!impl_->predicate) || impl_->predicate());

				using basic_window = ::nana::detail::basic_window;
				auto real_wd = reinterpret_cast<::nana::detail::basic_window*>(impl_->window_handle);
				real_wd->other.dnd_state = dragdrop_status::ready;
			}
		});

		events.mouse_move.connect_unignorable([this](const arg_mouse& arg) {
			if (!(arg.is_left_button() && impl_->dragging && API::is_window(arg.window_handle)))
				return;

			using basic_window = ::nana::detail::basic_window;
			auto real_wd = reinterpret_cast<::nana::detail::basic_window*>(arg.window_handle);
			real_wd->other.dnd_state = dragdrop_status::in_progress;

			auto has_dropped = dragdrop_service::instance().dragdrop(arg.window_handle);

			real_wd->other.dnd_state = dragdrop_status::not_ready;
			impl_->dragging = false;

			if (has_dropped)
			{
				auto drop_wd = API::find_window(API::cursor_position());
				auto i = impl_->targets.find(drop_wd);
				if ((impl_->targets.end() != i) && i->second)
					i->second();
			}
		});
#elif 1
		events.mouse_down.connect_unignorable([drag_wd](const arg_mouse& arg){
			if (arg.is_left_button() && API::is_window(drag_wd))
			{
				//API::set_capture(drag_wd, true);

				using basic_window = ::nana::detail::basic_window;
				auto real_wd = reinterpret_cast<::nana::detail::basic_window*>(drag_wd);
				real_wd->other.dnd_state = dragdrop_status::ready;
			}
		});

		events.mouse_move.connect_unignorable([this](const arg_mouse& arg){
			if (!arg.is_left_button())
				return;

			if (impl_->dragging)
			{
				auto drop_wd = API::find_window(API::cursor_position());
				auto i = impl_->targets.find(drop_wd);

			}
			else
			{
				if ((!impl_->predicate) || impl_->predicate())
				{
					if (API::is_window(arg.window_handle))
					{
						impl_->dragging = true;
						using basic_window = ::nana::detail::basic_window;
						auto real_wd = reinterpret_cast<::nana::detail::basic_window*>(arg.window_handle);
						real_wd->other.dnd_state = dragdrop_status::in_progress;

						dragdrop_service::instance().dragdrop(arg.window_handle);
						return;
					}
				}

				//API::release_capture(impl_->window_handle);
			}
			
		});

		events.mouse_up.connect_unignorable([this]{
			if (impl_->cancel())
			{
				auto drop_wd = API::find_window(API::cursor_position());
				auto i = impl_->targets.find(drop_wd);
				if (impl_->targets.end() == i || !i->second)
					return;

				i->second();
			}
		});

		events.key_press.connect_unignorable([this]{
			impl_->cancel();
		});
#endif
	}

	simple_dragdrop::~simple_dragdrop()
	{
		dragdrop_service::instance().remove(impl_->window_handle);
		API::dev::window_draggable(impl_->window_handle, false);
		delete impl_;
	}

	void simple_dragdrop::condition(std::function<bool()> predicate_fn)
	{
		impl_->predicate.swap(predicate_fn);
	}

	void simple_dragdrop::make_drop(window target, std::function<void()> drop_fn)
	{
		dragdrop_service::instance().drop_assoc().add(impl_->window_handle, target);
		impl_->targets[target].swap(drop_fn);
	}
}