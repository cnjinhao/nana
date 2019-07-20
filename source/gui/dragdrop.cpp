/**
*	Drag and Drop Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2019 Jinhao(cnjinhao@hotmail.com)
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

#include "detail/basic_window.hpp"

#include <map>
#include <set>
#include <cstring>

#ifdef NANA_WINDOWS
#	include <windows.h>
#	include <oleidl.h>
#	include <comdef.h>
#	include <Shlobj.h>
#elif defined(NANA_X11)
#	include "../detail/posix/xdnd_protocol.hpp"
#	include <nana/gui/detail/native_window_interface.hpp>
#	include <X11/Xcursor/Xcursor.h>
#	include <fstream>
#endif

namespace nana
{
	namespace detail
	{
		struct dragdrop_data
		{
			dnd_action requested_action;
			std::vector<std::filesystem::path> files;

#ifdef NANA_X11
			xdnd_data to_xdnd_data() const noexcept
			{
				auto & atombase = nana::detail::platform_spec::instance().atombase();
				xdnd_data xdata;
				xdata.requested_action = atombase.xdnd_action_copy;

				switch(requested_action)
				{
				case dnd_action::copy:
					xdata.requested_action = atombase.xdnd_action_copy;	break;
				case dnd_action::move:
					xdata.requested_action = atombase.xdnd_action_move; break;
				case dnd_action::link:
					xdata.requested_action = atombase.xdnd_action_link; break;
				}

				xdata.files = files;
				return xdata;
			}
#endif
		};
	}


	class dragdrop_session
	{
	public:
		struct target_rep
		{
			std::set<window> target;
			std::map<native_window_type, std::size_t> native_target_count;
		};

		virtual ~dragdrop_session() = default;

		void insert(window source, window target)
		{
			auto &rep = table_[source];
			rep.target.insert(target);
#ifdef NANA_X11
			auto native_wd = API::root(target);
			rep.native_target_count[native_wd] += 1;
			nana::detail::platform_spec::instance().dragdrop_target(native_wd, true, 1);
#endif
		}

		void erase(window source, window target)
		{
			auto i = table_.find(source);
			if (table_.end() == i)
				return;

			i->second.target.erase(target);

#ifdef NANA_WINDOWS
			if ((nullptr == target) || i->second.target.empty())
				table_.erase(i);
#else
			if(nullptr == target)
			{
				//remove all targets of source
				for(auto & native : i->second.native_target_count)
				{
					nana::detail::platform_spec::instance().dragdrop_target(native.first, false, native.second);
				}

				table_.erase(i);
			}
			else
			{
				nana::detail::platform_spec::instance().dragdrop_target(API::root(target), false, 1);
				if(i->second.target.empty())
					table_.erase(i);
			}
#endif
		}

		bool has(window source, window target) const
		{
			auto i = table_.find(source);
			if (i != table_.end())
				return (0 != i->second.target.count(target));

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
		std::map<window, target_rep> table_;
		window current_source_{ nullptr };
	};

#ifdef NANA_WINDOWS

	template<typename Interface, const IID& iid>
	class win32com_iunknown final: public Interface
	{
	public:
		template<typename ...Args>
		win32com_iunknown(Args&& ... args) :
			Interface(std::forward<Args>(args)...)
		{}

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
		win32com_drop_target(bool simple_mode):
			simple_mode_(simple_mode)
		{

		}

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
		STDMETHODIMP DragEnter(IDataObject* data, DWORD grfKeyState, POINTL pt, DWORD* req_effect)
		{
			*req_effect &= DROPEFFECT_COPY;

			FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			STGMEDIUM medium;
			if (S_OK == data->GetData(&fmt, &medium))
			{
				::ReleaseStgMedium(&medium);
				effect_ = DROPEFFECT_COPY;
			}

			return S_OK;
		}

		STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* req_effect)
		{
			//bool found_data = false;
			if (simple_mode_)
			{
				auto hovered_wd = API::find_window(point(pt.x, pt.y));

				if ((hovered_wd && (hovered_wd == this->current_source())) || this->has(this->current_source(), hovered_wd))
					*req_effect &= DROPEFFECT_COPY;
				else
					*req_effect = DROPEFFECT_NONE;
			}
			else
			{
				*req_effect = effect_;
			}
			return S_OK;
		}

		STDMETHODIMP DragLeave()
		{
			effect_ = DROPEFFECT_NONE;
			return S_OK;
		}

		STDMETHODIMP Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
		{
			return E_NOTIMPL;
		}
	private:
		LONG ref_count_{ 1 };
		bool const simple_mode_;		//Simple mode behaves the simple_dragdrop.
		DWORD effect_{ DROPEFFECT_NONE };
	};

class drop_source_impl : public IDropSource
{
public:
	drop_source_impl(window wd) :
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
using drop_source = win32com_iunknown<drop_source_impl, IID_IDropSource>;


class win32_dropdata_impl: public IDataObject
{
public:
	struct data_entry
	{
		FORMATETC	format;
		STGMEDIUM	medium;
		bool		read_from; //Indicates the data which is used for reading.

		~data_entry()
		{
			::CoTaskMemFree(format.ptd);
			::ReleaseStgMedium(&medium);
		}

		bool compare(const FORMATETC& fmt, bool rdfrom) const
		{
			return (format.cfFormat == fmt.cfFormat &&
				(format.tymed & fmt.tymed) != 0 &&
				(format.dwAspect == DVASPECT_THUMBNAIL || format.dwAspect == DVASPECT_ICON || medium.tymed == TYMED_NULL || format.lindex == fmt.lindex || (format.lindex == 0 && fmt.lindex == -1) || (format.lindex == -1 && fmt.lindex == 0)) &&
				format.dwAspect == fmt.dwAspect && read_from == rdfrom);
}
		};

	data_entry * find(const FORMATETC& fmt, bool read_from) const
	{
		data_entry * last_weak_match = nullptr;

		for (auto & entry : entries_)
		{
			if (entry->compare(fmt, read_from))
			{
				auto entry_ptd = entry->format.ptd;
				if (entry_ptd && fmt.ptd && entry_ptd->tdSize == fmt.ptd->tdSize && (0 == std::memcmp(entry_ptd, fmt.ptd, fmt.ptd->tdSize)))
					return entry.get();
				else if (nullptr == entry_ptd && nullptr == fmt.ptd)
					return entry.get();

				last_weak_match = entry.get();
			}
		}
		return last_weak_match;
	}

	void assign(const detail::dragdrop_data& data)
	{
		if (!data.files.empty())
		{
			std::size_t bytes = sizeof(wchar_t);
			for (auto & file : data.files)
			{
				auto file_s = file.wstring();
				bytes += (file_s.size() + 1) * sizeof(file_s.front());
			}

			auto hglobal = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(DROPFILES) + bytes);

			auto dropfiles = reinterpret_cast<DROPFILES*>(::GlobalLock(hglobal));
			dropfiles->pFiles = sizeof(DROPFILES);
			dropfiles->fWide = true;

			auto file_buf = reinterpret_cast<char*>(dropfiles)+sizeof(DROPFILES);

			for (auto & file : data.files)
			{
				auto file_s = file.wstring();
				std::memcpy(file_buf, file_s.data(), (file_s.size() + 1) * sizeof(file_s.front()));
				file_buf += (file_s.size() + 1) * sizeof(file_s.front());
			}
			*reinterpret_cast<wchar_t*>(file_buf) = 0;

			::GlobalUnlock(hglobal);

			assign(hglobal);
		}
	}

	data_entry* assign(HGLOBAL hglobal)
	{
		FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		auto entry = find(fmt, true);
		if (entry)
		{
			//Free the current entry for reuse
			::CoTaskMemFree(entry->format.ptd);
			::ReleaseStgMedium(&entry->medium);
		}
		else
		{
			//Create a new entry
			entries_.emplace_back(new data_entry);
			entry = entries_.back().get();
		}

		//Assign the format to the entry.
		entry->read_from = true;
		entry->format = fmt;

		//Assign the stgMedium
		entry->medium.tymed = TYMED_HGLOBAL;
		entry->medium.hGlobal = hglobal;
		entry->medium.pUnkForRelease = nullptr;

		return entry;
	}
public:
	// Implement IDataObject
	STDMETHODIMP GetData(FORMATETC *request_format, STGMEDIUM *pmedium) override
	{
		if (!(request_format && pmedium))
			return E_INVALIDARG;

		pmedium->hGlobal = nullptr;

		auto entry = find(*request_format, true);
		if (entry)
			return _m_copy_medium(pmedium, &entry->medium, &entry->format);

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

		for (auto & entry : entries_)
		{
			if (entry->format.tymed & pformatetc->tymed)
			{
				if (entry->format.cfFormat == pformatetc->cfFormat)
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

		entries_.emplace_back(new data_entry);
		auto entry = entries_.back().get();

		entry->format = *pformatetc;

		_m_copy_medium(&entry->medium, pmedium, pformatetc);

		if (TRUE == fRelease)
			::ReleaseStgMedium(pmedium);

		return S_OK;
	}

	STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc) override
	{
		if (NULL == ppenumFormatEtc)
			return E_INVALIDARG;

		if (DATADIR_GET != dwDirection)
			return E_NOTIMPL;

		*ppenumFormatEtc = nullptr;

		FORMATETC rgfmtetc[] =
		{
			//{ CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, 0, TYMED_HGLOBAL }
			{ CF_HDROP, nullptr, DVASPECT_CONTENT, 0, TYMED_HGLOBAL }
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
		case TYMED_ENHMF:
			//GDI object can't be copied to an existing HANDLE
			if (stgmed_dst->hGlobal)
				return E_INVALIDARG;

			stgmed_dst->hGlobal = (HBITMAP)OleDuplicateData(stgmed_src->hGlobal, fmt_src->cfFormat, 0);
			break;
		case TYMED_MFPICT:
			stgmed_dst->hMetaFilePict = (HMETAFILEPICT)OleDuplicateData(stgmed_src->hMetaFilePict, fmt_src->cfFormat, 0);
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
	std::vector<std::unique_ptr<data_entry>> entries_;
	};

using win32_dropdata = win32com_iunknown<win32_dropdata_impl, IID_IDataObject>;


#elif defined(NANA_X11)
	class x11_dropdata
	{
	public:
		void assign(const detail::dragdrop_data& data)
		{
			data_ = &data;
		}

		const detail::dragdrop_data* data() const
		{
			return data_;
		}
	private:
		const detail::dragdrop_data* data_{nullptr};
	};

	class x11_dragdrop: public detail::x11_dragdrop_interface, public dragdrop_session
	{
	public:
		x11_dragdrop(bool simple_mode):
			simple_mode_(simple_mode)
		{
		}

		bool simple_mode() const noexcept
		{
			return simple_mode_;
		}
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
		bool const simple_mode_;
		std::atomic<std::size_t> ref_count_{ 1 };
	};
#endif

	class dragdrop_service
	{
		dragdrop_service() = default;
	public:
#ifdef NANA_WINDOWS
		using dragdrop_target = win32com_drop_target;
		using dropdata_type = win32_dropdata;
#else
		using dragdrop_target = x11_dragdrop;
		using dropdata_type = x11_dropdata;
#endif

		static dragdrop_service& instance()
		{
			static dragdrop_service serv;
			return serv;
		}

		dragdrop_session* create_dragdrop(window wd, bool simple_mode)
		{
			auto native_wd = API::root(wd);
			if (nullptr == native_wd)
				return nullptr;

			dragdrop_target * ddrop = nullptr;

			auto i = table_.find(native_wd);
			if(table_.end() == i)
			{
				ddrop = new dragdrop_target{simple_mode};
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

		bool dragdrop(window drag_wd, dropdata_type* dropdata, dnd_action* executed_action)
		{
			auto i = table_.find(API::root(drag_wd));
			if ((!dropdata) && table_.end() == i)
				return false;

			internal_revert_guard rvt_lock;

#ifdef NANA_WINDOWS
			auto drop_src = new drop_source{ drag_wd };

			i->second->set_current_source(drag_wd);

			DWORD result_effect{ DROPEFFECT_NONE };
			::DoDragDrop(dropdata, drop_src, DROPEFFECT_COPY, &result_effect);

			i->second->set_current_source(nullptr);

			delete drop_src;

			if (executed_action)
			{
				switch (result_effect)
				{
				case DROPEFFECT_COPY:
					*executed_action = dnd_action::copy; break;
				case DROPEFFECT_MOVE:
					*executed_action = dnd_action::move; break;
				case DROPEFFECT_LINK:
					*executed_action = dnd_action::link; break;
				}
			}

			return (DROPEFFECT_NONE != result_effect);
#elif defined(NANA_X11)
			auto& atombase = _m_spec().atombase();

			auto ddrop = dynamic_cast<dragdrop_target*>(i->second);

			auto const native_source = reinterpret_cast<Window>(API::root(drag_wd));

			{
				detail::platform_scope_guard lock;
				::XSetSelectionOwner(_m_spec().open_display(), atombase.xdnd_selection, native_source, CurrentTime);
			}


			hovered_.window_handle = nullptr;
			hovered_.native_wd = 0;

			if(executed_action)
				*executed_action = dropdata->data()->requested_action;

			if(ddrop->simple_mode())
			{

				_m_spec().msg_dispatch([this, ddrop, drag_wd, native_source, &atombase](const detail::msg_packet_tag& msg_pkt) mutable{
					if(detail::msg_packet_tag::pkt_family::xevent == msg_pkt.kind)
					{
						auto const disp = _m_spec().open_display();
						if (MotionNotify == msg_pkt.u.xevent.type)
						{
							auto pos = API::cursor_position();
							auto native_cur_wd = reinterpret_cast<Window>(detail::native_interface::find_window(pos.x, pos.y));

							const char* icon = nullptr;
							if(hovered_.native_wd != native_cur_wd)
							{
								if(hovered_.native_wd)
								{
									_m_free_cursor();
									::XUndefineCursor(disp, hovered_.native_wd);
								}

								_m_client_msg(native_cur_wd, native_source, 1, atombase.xdnd_enter, atombase.text_uri_list, XA_STRING);
								hovered_.native_wd = native_cur_wd;
							}


							auto cur_wd = API::find_window(API::cursor_position());

							if(hovered_.window_handle != cur_wd)
							{
								hovered_.window_handle = cur_wd;

								icon = (((drag_wd == cur_wd) || ddrop->has(drag_wd, cur_wd)) ? "dnd-move" : "dnd-none");
							}

							if(icon)
							{
								_m_free_cursor();
								hovered_.cursor = ::XcursorFilenameLoadCursor(disp, icons_.cursor(icon).c_str());
								::XDefineCursor(disp, native_cur_wd, hovered_.cursor);
							}
						}
						else if(msg_pkt.u.xevent.type == ButtonRelease)
						{
							::XUndefineCursor(disp, hovered_.native_wd);
							_m_free_cursor();
							API::release_capture(drag_wd);
							return detail::propagation_chain::exit;
						}

					}
					return detail::propagation_chain::stop;
				});

				//In simple mode, it always returns true. The drag and drop is determined by class simple_dragdrop
				return true;
			}
			else
			{
				auto data = dropdata->data()->to_xdnd_data();

				API::set_capture(drag_wd, true);
				nana::detail::xdnd_protocol xdnd_proto{native_source};

				//Not simple mode
				_m_spec().msg_dispatch([this, &data, drag_wd, &xdnd_proto](const detail::msg_packet_tag& msg_pkt) mutable{
					if(detail::msg_packet_tag::pkt_family::xevent == msg_pkt.kind)
					{
						if (MotionNotify == msg_pkt.u.xevent.type)
						{
							auto pos = API::cursor_position();
							auto native_cur_wd = reinterpret_cast<Window>(detail::native_interface::find_window(pos.x, pos.y));

							xdnd_proto.mouse_move(native_cur_wd, pos, data.requested_action);
						}
						else if(ClientMessage == msg_pkt.u.xevent.type)
						{
							auto & xclient = msg_pkt.u.xevent.xclient;
							if(xdnd_proto.client_message(xclient))
							{
								API::release_capture(drag_wd);
								return detail::propagation_chain::exit;
							}
						}
						else if(SelectionRequest == msg_pkt.u.xevent.type)
						{
							xdnd_proto.selection_request(msg_pkt.u.xevent.xselectionrequest, data);
						}
						else if(msg_pkt.u.xevent.type == ButtonRelease)
						{
							API::release_capture(drag_wd);
							_m_free_cursor();

							//Exits the msg loop if xdnd_proto doesn't send the XdndDrop because of refusal of the DND
							if(!xdnd_proto.mouse_release())
								return detail::propagation_chain::exit;
						}

						return detail::propagation_chain::stop;
					}
					return detail::propagation_chain::pass;
				});

				if(xdnd_proto.executed_action() != 0)
				{
					if(executed_action)
						*executed_action = _m_from_xdnd_action(xdnd_proto.executed_action());

					return true;
				}
			}
#endif
			return false;
		}

#ifdef NANA_X11
	private:
		static nana::detail::platform_spec & _m_spec()
		{
			return nana::detail::platform_spec::instance();
		}

		static dnd_action _m_from_xdnd_action(Atom action) noexcept
		{
			auto & atombase = _m_spec().atombase();
			if(action == atombase.xdnd_action_copy)
				return dnd_action::copy;
			else if(action == atombase.xdnd_action_move)
				return dnd_action::move;
			else if(action == atombase.xdnd_action_link)
				return dnd_action::link;

			return dnd_action::copy;
		}

		//dndversion<<24, fl_XdndURIList, XA_STRING, 0
		static void _m_client_msg(Window wd_target, Window wd_src, int flag, Atom xdnd_atom, Atom data, Atom data_type)
		{
			auto const display = _m_spec().open_display();
			::XEvent evt;
			::memset(&evt, 0, sizeof evt);
			evt.xany.type = ClientMessage;
			evt.xany.display = display;
			evt.xclient.window = wd_target;
			evt.xclient.message_type = xdnd_atom;
			evt.xclient.format = 32;

			//Target window
			evt.xclient.data.l[0] = wd_src;
			//Accept set
			evt.xclient.data.l[1] = flag;
			evt.xclient.data.l[2] = data;
			evt.xclient.data.l[3] = data_type;
			evt.xclient.data.l[4] = 0;

			::XSendEvent(display, wd_target, True, NoEventMask, &evt);

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
		nana::detail::theme icons_;
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
				window_handle->other.dnd_state = dragdrop_status::not_ready;
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

		impl_->ddrop = dragdrop_service::instance().create_dragdrop(drag_wd, true);

		impl_->window_handle = drag_wd;
		API::dev::window_draggable(drag_wd, true);

		auto & events = API::events<>(drag_wd);

		impl_->events.destroy = events.destroy.connect_unignorable([this](const arg_destroy&) {
			dragdrop_service::instance().remove(impl_->window_handle);
			API::dev::window_draggable(impl_->window_handle, false);
		});

		impl_->events.mouse_down = events.mouse_down.connect_unignorable([this](const arg_mouse& arg){
			if (arg.is_left_button() && API::is_window(impl_->window_handle))
			{
				impl_->dragging = ((!impl_->predicate) || impl_->predicate());
				impl_->window_handle->other.dnd_state = dragdrop_status::ready;
			}
		});

		impl_->events.mouse_move = events.mouse_move.connect_unignorable([this](const arg_mouse& arg) {
			if (!(arg.is_left_button() && impl_->dragging && API::is_window(arg.window_handle)))
				return;

			arg.window_handle->other.dnd_state = dragdrop_status::in_progress;

			std::unique_ptr<dragdrop_service::dropdata_type> dropdata{new dragdrop_service::dropdata_type};

			auto has_dropped = dragdrop_service::instance().dragdrop(arg.window_handle, dropdata.get(), nullptr);

			arg.window_handle->other.dnd_state = dragdrop_status::not_ready;
			impl_->dragging = false;

			if (has_dropped)
			{
				auto drop_wd = API::find_window(API::cursor_position());
				auto i = impl_->targets.find(drop_wd);
				if ((impl_->targets.end() != i) && i->second)
					i->second();
			}
		});
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
		window const source_handle;
		bool dragging{ false };
		dragdrop_session * ddrop{nullptr};
		std::function<bool()> predicate;
		std::function<data()> generator;
		std::function<void(bool, dnd_action, data&)> drop_finished;

		struct event_handlers
		{
			nana::event_handle destroy;
			nana::event_handle mouse_move;
			nana::event_handle mouse_down;
		}events;

		implementation(window source):
			source_handle(source)
		{
			ddrop = dragdrop_service::instance().create_dragdrop(source, false);
			API::dev::window_draggable(source, true);
		}

		void make_drop()
		{
			if (!generator)
				return;

			auto transf_data = generator();
			dragdrop_service::dropdata_type dropdata;
			dropdata.assign(*transf_data.real_data_);

			dnd_action executed_action;
			auto has_dropped = dragdrop_service::instance().dragdrop(source_handle, &dropdata, &executed_action);

			if(drop_finished)
				drop_finished(has_dropped, executed_action, transf_data);
		}
	};

	dragdrop::dragdrop(window source) :
		impl_(new implementation(source))
	{
		auto & events = API::events(source);
		impl_->events.destroy = events.destroy.connect_unignorable([this](const arg_destroy&) {
			dragdrop_service::instance().remove(impl_->source_handle);
			API::dev::window_draggable(impl_->source_handle, false);
		});

		impl_->events.mouse_down = events.mouse_down.connect_unignorable([this](const arg_mouse& arg) {
			if (arg.is_left_button() && API::is_window(impl_->source_handle))
			{
				impl_->dragging = ((!impl_->predicate) || impl_->predicate());
				impl_->source_handle->other.dnd_state = dragdrop_status::ready;
			}
		});

		impl_->events.mouse_move = events.mouse_move.connect_unignorable([this](const arg_mouse& arg) {
			if (!(arg.is_left_button() && impl_->dragging && API::is_window(arg.window_handle)))
				return;

			arg.window_handle->other.dnd_state = dragdrop_status::in_progress;
			impl_->make_drop();
			arg.window_handle->other.dnd_state = dragdrop_status::not_ready;
			impl_->dragging = false;
		});
	}

	dragdrop::~dragdrop()
	{
		if (impl_->source_handle)
		{
			dragdrop_service::instance().remove(impl_->source_handle);
			API::dev::window_draggable(impl_->source_handle, false);

			API::umake_event(impl_->events.destroy);
			API::umake_event(impl_->events.mouse_down);
			API::umake_event(impl_->events.mouse_move);
		}
		delete impl_;
	}

	void dragdrop::condition(std::function<bool()> predicate_fn)
	{
		impl_->predicate = predicate_fn;
	}

	void dragdrop::prepare_data(std::function<data()> generator)
	{
		impl_->generator = generator;
	}

	void dragdrop::drop_finished(std::function<void(bool, dnd_action, data&)> finish_fn)
	{
		impl_->drop_finished = finish_fn;
	}


	dragdrop::data::data(dnd_action requested_action):
		real_data_(new detail::dragdrop_data)
	{
		real_data_->requested_action = requested_action;
	}

	dragdrop::data::~data()
	{
		delete real_data_;
	}

	dragdrop::data::data(data&& rhs):
		real_data_(new detail::dragdrop_data)
	{
		std::swap(real_data_, rhs.real_data_);
	}

	dragdrop::data& dragdrop::data::operator=(data&& rhs)
	{
		if (this != &rhs)
		{
			auto moved_data = new detail::dragdrop_data;
			delete real_data_;
			real_data_ = rhs.real_data_;
			rhs.real_data_ = moved_data;
		}
		return *this;
	}

	void dragdrop::data::insert(std::filesystem::path path)
	{
		real_data_->files.emplace_back(std::move(path));
	}
}//end namespace nana
