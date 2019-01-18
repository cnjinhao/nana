/**
 *	Filebox
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/filebox.hpp
 *  @author Jinhao
 *  @brief dialogs to chose file(s) or a directory, implemented "native" in windows but using nana for X11
 */

#ifndef NANA_GUI_FILEBOX_HPP
#define NANA_GUI_FILEBOX_HPP
#include <nana/gui/basis.hpp>
#include <nana/filesystem/filesystem.hpp>
#include <nana/optional.hpp>
#include <vector>
#include <utility>

namespace nana
{       /// Create an Open or Save dialog box to let user select the name of a file.
	class filebox
	{
		struct implement;

		filebox(filebox&&) = delete;
		filebox& operator=(filebox&&) = delete;
	public:
		using filters = std::vector<std::pair< ::std::string, ::std::string>>;

		explicit filebox(bool is_open_mode);
		filebox(window owner, bool is_open_mode);
		filebox(const filebox&);
		~filebox();

		filebox& operator=(const filebox&);

		/// Change owner window
		void owner(window);

		/// Set a new title for the dialog
		/// @param	string	a text for title
		/// @return the old title.
		::std::string title( ::std::string new_title);  

		/**	@brief	Suggest initial path used to locate a directory when the filebox starts.
		 *	@param	string	initial_directory a path of initial directory
		 *	@note	the behavior of init_path is different between Win7 and Win2K/XP/Vista, but its behavior under Linux is conformed with Win7.
		 */
		filebox& init_path(const ::std::string& initial_directory);

		filebox& init_file(const ::std::string&); ///< Init file, if it contains a path, the init path is replaced by the path of init file.
        
		/// \brief Add a filetype filter. 
        /// To specify multiple filter in a single description, use a semicolon to separate the patterns(for example,"*.TXT;*.DOC;*.BAK").
		filebox& add_filter(const ::std::string& description,  ///< for example: "Text File"
                            const ::std::string& filetype      ///< filter pattern(for example: "*.TXT")
                            );

        filebox& add_filter(const filters &ftres)
        {
            for (auto &f : ftres)
                add_filter(f.first, f.second);
            return *this;
        };


		const ::std::string& path() const;
		const ::std::string& file() const;
#if defined(NANA_WINDOWS)
		const ::std::vector<::std::string>& files() const;
		void allow_multi_select(bool allow);
#endif

		/// Display the filebox dialog
		bool show() const;
		
		/// a function object method alternative to show() to display the filebox dialog, 
		bool operator()() const
		{
			return show();
		}
	private:
		implement * impl_;
	};

	class folderbox
	{
		struct implement;

		folderbox(const folderbox&) = delete;
		folderbox& operator=(const folderbox&) = delete;
		folderbox(folderbox&&) = delete;
		folderbox& operator=(folderbox&&) = delete;
	public:
		using path_type = std::filesystem::path;

		explicit folderbox(window owner = nullptr, const path_type& init_path = {}, std::string title={});
		~folderbox();

		std::optional<path_type> show() const;

		std::optional<path_type> operator()() const
		{
			return show();
		}

		/// Set a new title for the dialog
		/// @param	string	a text for title
		/// @return the old title.
		::std::string title( ::std::string new_title);
	private:
		implement* impl_;
	};
}//end namespace nana
#endif
