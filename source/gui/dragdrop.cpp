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
	class dragdrop_session
	{
	public:
		virtual ~dragdrop_session() = default;

		void insert(window source, window target)
		{
			table_[source].insert(target);
		}

		void erase(window source, window target)
		{
			auto i = table_.find(source);
			if (table_.end() == i)
				return;

			i->second.erase(target);

			if ((nullptr == target) || i->second.empty())
				table_.erase(i);
		}

		bool has(window source, window target) const
		{
			auto i = table_.find(source);
			if (i != table_.end())
				return (0 != i->second.count(target));

			return false;
		}

		bool has_source(window source) const
		{
			return (table_.count(source) != 0);
		}

		bool empty() const
		{
			return table_.empty();
		}

		void set_current_source(window source)
		{
			if (table_.count(source))
				current_source_ = source;
			else
				current_source_ = nullptr;
		}

		window current_source() const
		{
			return current_source_;
		}
	private:
		std::map<window, std::set<window>> table_;
		window current_source_{ nullptr };
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

	class win32com_drop_target : public IDropTarget, public dragdrop_session
	{
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

			if ((hovered_wd && (hovered_wd == this->current_source())) || this->has(this->current_source(), hovered_wd))
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
		~drop_data()
		{
			if(hglobal_)
				::GlobalFree(hglobal_);
		}

		void assign(const std::vector<std::wstring>& files)
		{
			std::size_t bytes = 0;
			for (auto & f : files)
				bytes += (f.size() + 1) * sizeof(f.front());

			hglobal_ = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(DROPFILES) + bytes);

			auto dropfiles = reinterpret_cast<DROPFILES*>(::GlobalLock(hglobal_));
			dropfiles->pFiles = sizeof(DROPFILES);
			dropfiles->fWide = true;

			auto file_buf = reinterpret_cast<char*>(dropfiles) + sizeof(DROPFILES);

			for (auto & f : files)
			{
				std::memcpy(file_buf, f.data(), (f.size() + 1) * sizeof(f.front()));
				file_buf += f.size() + 1;
			}

			::GlobalUnlock(hglobal_);
		}
	public:
		// Implement IDataObject
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
		HGLOBAL hglobal_{nullptr};
		std::vector<medium> mediums_;
	};


#elif defined(NANA_X11)
	class x11_dragdrop: public detail::x11_dragdrop_interface, public dragdrop_session
	{
	public:
		//Implement x11_dragdrop_interface
		void add_ref() override
		{
			++ref_count_;
		}
		
		std::size_t release() override
		{
			std::size_t val = --ref_count_;
			if(0 == val)
				delete this;

			return val;
		}
	private:
		std::atomic<std::size_t> ref_count_{ 1 };
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
#ifdef NANA_WINDOWS
			using dragdrop_target = win32com_drop_target;
#else
			using dragdrop_target = x11_dragdrop;
#endif

		static dragdrop_service& instance()
		{
			static dragdrop_service serv;
			return serv;
		}

		dragdrop_session* create_dragdrop(window wd)
		{
			auto native_wd = API::root(wd);
			if (nullptr == native_wd)
				return nullptr;

			dragdrop_target * ddrop = nullptr;

			auto i = table_.find(native_wd);
			if(table_.end() == i)
			{
				ddrop = new dragdrop_target;
#ifdef NANA_WINDOWS
				if (table_.empty())
					::OleInitialize(nullptr);

				::RegisterDragDrop(reinterpret_cast<HWND>(native_wd), ddrop);
#else
				if(!_m_spec().register_dragdrop(native_wd, ddrop))
				{
					delete ddrop;
					return nullptr;
				}
#endif
				table_[native_wd] = ddrop;
			}
			else
			{
				ddrop = dynamic_cast<dragdrop_target*>(i->second);

#ifdef NANA_WINDOWS
				ddrop->AddRef();
#else
				ddrop->add_ref();
#endif
			}

			return ddrop;
		}

		void remove(window source)
		{
			auto native_src = API::root(source);

			auto i = table_.find(API::root(source));
			if (i == table_.end())
				return;

			i->second->erase(source, nullptr);

			if (i->second->empty())
			{
				auto ddrop = dynamic_cast<dragdrop_target*>(i->second);
				table_.erase(i);
#ifdef NANA_WINDOWS
				::RevokeDragDrop(reinterpret_cast<HWND>(native_src));
				ddrop->Release();
#elif defined(NANA_X11)
				_m_spec().remove_dragdrop(native_src);
				ddrop->release();
#endif
			}
		}

		bool dragdrop(window drag_wd)
		{
			auto i = table_.find(API::root(drag_wd));
			if (table_.end() == i)
				return false;

#ifdef NANA_WINDOWS
			auto drop_src = new drop_source{ drag_wd };
			auto drop_dat = new (std::nothrow) drop_data;
			if (!drop_dat)
			{
				delete drop_src;
				return false;
			}

			i->second->set_current_source(drag_wd);

			DWORD eff;
			auto status = ::DoDragDrop(drop_dat, drop_src, DROPEFFECT_COPY, &eff);

			i->second->set_current_source(nullptr);

			delete drop_src;
			delete drop_dat;

			return true;
#elif defined(NANA_X11)
			auto ddrop = dynamic_cast<x11_dragdrop*>(i->second);
			
			auto const native_source = reinterpret_cast<Window>(API::root(drag_wd));

			{
				detail::platform_scope_guard lock;
				::XSetSelectionOwner(_m_spec().open_display(), _m_spec().atombase().xdnd_selection, native_source, CurrentTime);
			}


			hovered_.window_handle = nullptr;
			hovered_.native_wd = 0;
			window target_wd = 0;
			auto& atombase = _m_spec().atombase();
			//while(true)
			{

				_m_spec().msg_dispatch([this, ddrop, drag_wd, native_source, &target_wd, &atombase](const detail::msg_packet_tag& msg_pkt) mutable{
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

								_m_client_msg(native_cur_wd, native_source, atombase.xdnd_enter, atombase.text_uri_list, XA_STRING);
								hovered_.native_wd = native_cur_wd;
							}



							auto cur_wd = API::find_window(API::cursor_position());
							if(hovered_.window_handle != cur_wd)
							{
								_m_free_cursor();

								hovered_.window_handle = cur_wd;

								const char* icon = (((drag_wd == cur_wd) || ddrop->has(drag_wd, cur_wd)) ? "dnd-move" : "dnd-none");
								hovered_.cursor = ::XcursorFilenameLoadCursor(disp, icons_.cursor(icon).c_str());							
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
		std::map<native_window_type, dragdrop_session*> table_;

#ifdef NANA_WINDOWS
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
		window window_handle{ nullptr };
		dragdrop_session * ddrop{ nullptr };
		std::function<bool()> predicate;
		std::map<window, std::function<void()>> targets;

		bool dragging{ false };

		struct event_handlers
		{
			nana::event_handle destroy;
			nana::event_handle mouse_move;
			nana::event_handle mouse_down;
		}events;

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
		if (!API::is_window(drag_wd))
		{
			delete impl_;
			throw std::invalid_argument("simple_dragdrop: invalid window handle");
		}

		impl_->ddrop = dragdrop_service::instance().create_dragdrop(drag_wd);

		impl_->window_handle = drag_wd;
		API::dev::window_draggable(drag_wd, true);

		auto & events = API::events<>(drag_wd);

#if 1 //#ifdef NANA_WINDOWS
		impl_->events.destroy = events.destroy.connect_unignorable([this](const arg_destroy&) {
			dragdrop_service::instance().remove(impl_->window_handle);
			API::dev::window_draggable(impl_->window_handle, false);
		});

		impl_->events.mouse_down = events.mouse_down.connect_unignorable([this](const arg_mouse& arg){
			if (arg.is_left_button() && API::is_window(impl_->window_handle))
			{
				impl_->dragging = ((!impl_->predicate) || impl_->predicate());

				using basic_window = ::nana::detail::basic_window;
				auto real_wd = reinterpret_cast<::nana::detail::basic_window*>(impl_->window_handle);
				real_wd->other.dnd_state = dragdrop_status::ready;
			}
		});

		impl_->events.mouse_move = events.mouse_move.connect_unignorable([this](const arg_mouse& arg) {
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
		if (impl_->window_handle)
		{
			dragdrop_service::instance().remove(impl_->window_handle);
			API::dev::window_draggable(impl_->window_handle, false);

			API::umake_event(impl_->events.destroy);
			API::umake_event(impl_->events.mouse_down);
			API::umake_event(impl_->events.mouse_move);
		}

		delete impl_;
	}

	void simple_dragdrop::condition(std::function<bool()> predicate_fn)
	{
		if (nullptr == impl_)
			throw std::logic_error("simple_dragdrop is empty");

		impl_->predicate.swap(predicate_fn);
	}

	void simple_dragdrop::make_drop(window target, std::function<void()> drop_fn)
	{
		if (nullptr == impl_)
			throw std::logic_error("simple_dragdrop is empty");

		impl_->ddrop->insert(impl_->window_handle, target);
		impl_->targets[target].swap(drop_fn);
	}







	//This is class dragdrop
	struct dragdrop::implementation
	{
		window source_handle;
		bool dragging{ false };
		std::function<bool()> predicate;

		std::function<void()> generator;


		struct event_handlers
		{
			nana::event_handle destroy;
			nana::event_handle mouse_move;
			nana::event_handle mouse_down;
		}events;

		void make_data()
		{
			//https://www.codeproject.com/Articles/840/How-to-Implement-Drag-and-Drop-Between-Your-Progra

			std::vector<std::wstring> files;
			files.push_back(L"D:\\universal_access");
			files.push_back(L"D:\\examplexxxx.cpp");

			std::size_t bytes = 0;
			for (auto & f : files)
				bytes += (f.size() + 1) * sizeof(f.front());

			auto data_handle = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(DROPFILES) + bytes);

			auto dropfiles = reinterpret_cast<DROPFILES*>(::GlobalLock(data_handle));
			dropfiles->pFiles = sizeof(DROPFILES);
			dropfiles->fWide = true;

			auto file_buf = reinterpret_cast<char*>(dropfiles) + sizeof(DROPFILES);

			for (auto & f : files)
			{
				std::memcpy(file_buf, f.data(), (f.size() + 1) * sizeof(f.front()));
				file_buf += f.size() + 1;
			}

			::GlobalUnlock(data_handle);


			generator();
		}
	};
	
	dragdrop::dragdrop(window source) :
		impl_(new implementation)
	{
		impl_->source_handle = source;
		
		auto & events = API::events(source);
		impl_->events.destroy = events.destroy.connect_unignorable([this](const arg_destroy&) {
			dragdrop_service::instance().remove(impl_->source_handle);
			API::dev::window_draggable(impl_->source_handle, false);
		});

		impl_->events.mouse_down = events.mouse_down.connect_unignorable([this](const arg_mouse& arg) {
			if (arg.is_left_button() && API::is_window(impl_->source_handle))
			{
				impl_->dragging = ((!impl_->predicate) || impl_->predicate());

				using basic_window = ::nana::detail::basic_window;
				auto real_wd = reinterpret_cast<::nana::detail::basic_window*>(impl_->source_handle);
				real_wd->other.dnd_state = dragdrop_status::ready;
			}
		});

		impl_->events.mouse_move = events.mouse_move.connect_unignorable([this](const arg_mouse& arg) {
			if (!(arg.is_left_button() && impl_->dragging && API::is_window(arg.window_handle)))
				return;

			using basic_window = ::nana::detail::basic_window;
			auto real_wd = reinterpret_cast<::nana::detail::basic_window*>(arg.window_handle);
			real_wd->other.dnd_state = dragdrop_status::in_progress;

			impl_->make_data();

			auto has_dropped = dragdrop_service::instance().dragdrop(arg.window_handle);

			real_wd->other.dnd_state = dragdrop_status::not_ready;
			impl_->dragging = false;

			if (has_dropped)
			{
				auto drop_wd = API::find_window(API::cursor_position());
				//auto i = impl_->targets.find(drop_wd);
				//if ((impl_->targets.end() != i) && i->second)
				//	i->second();
			}
		});
	}

	dragdrop::~dragdrop()
	{
		delete impl_;
	}
	
	void dragdrop::condition(std::function<bool()> predicate_fn)
	{
	
	}

	void dragdrop::make_data(std::function<void()> generator)
	{
		impl_->generator = generator;
	}
}//end namespace nana