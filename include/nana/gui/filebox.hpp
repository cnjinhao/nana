/**
 *	Filebox
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
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
		using path_type = std::filesystem::path;

		filebox(window owner, bool is_open_mode);
		filebox(const filebox&);
		~filebox();

		filebox& operator=(const filebox&);

		/// Change owner window
		/**
		 * Changes the owner window for the filebox. When #show()/operator()# are invoked, the dialog of filebox will be created with the specified owner.
		 * @param handle A handle to a window which will be used for the owner of filebox
		 */
		void owner(window handle);

		/// Changes new title
		/**
		 * Changes the title. When #show()/operator()# are invoked, the dialog of filebox will be created with the specified title.
		 * @param text Text of title
		 * @return the reference of *this.
		 */
		filebox& title( ::std::string text);  

		/// Sets a initial path
		/**	
		 * Suggest initial path used to locate a directory when the filebox starts.
		 * @note	the behavior of init_path is different between Win7 and Win2K/XP/Vista, but its behavior under Linux is conformed with Win7.
		 * @param	path a path of initial directory
		 * @return	reference of *this.
		 */
		filebox& init_path(const path_type& path);

		/// Sets a initial filename
		/**
		 * Suggest a filename when filebox starts. If the filename contains a path, the initial path will be replaced with the path presents in initial filename.
		 * @param filename a filename used for a initial filename when filebox starts.
		 * @return reference of *this.
		 */
		filebox& init_file(const ::std::string& filename); ///< Init file, if it contains a path, the init path is replaced by the path of init file.
        
		/// \brief Add a filetype filter. 
        /// To specify multiple filter in a single description, use a semicolon to separate the patterns(for example,"*.TXT;*.DOC;*.BAK").
		filebox& add_filter(const ::std::string& description,  ///< for example: "Text File"
                            const ::std::string& filetype      ///< filter pattern(for example: "*.TXT")
                            );

		filebox& add_filter(const std::vector<std::pair<std::string, std::string>> &filters);

		const path_type& path() const;
		
		filebox& allow_multi_select(bool allow);

		/// Display the filebox dialog
		std::vector<path_type> show() const;

		/// a function object method alternative to show() to display the filebox dialog, 
		std::vector<path_type> operator()() const
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

		/// Enables/disables multi select
		folderbox& allow_multi_select(bool allow);

		std::vector<path_type> show() const;

		std::vector<path_type> operator()() const
		{
			return show();
		}

		/// Changes title
		/**
		 * @param text Text of title
		 */
		folderbox& title(std::string text);
	private:
		implement* impl_;
	};
}//end namespace nana
#endif
