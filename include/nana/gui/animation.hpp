/*
 *	An Animation Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/animation.hpp
 */

#ifndef NANA_GUI_ANIMATION_HPP
#define NANA_GUI_ANIMATION_HPP
#include <nana/push_ignore_diagnostic>
#include <nana/paint/image.hpp>

#include <functional>
#include <memory>


namespace nana
{
	class animation;
        /// Holds the frames and frame builders. Have reference semantics for efficiency.
	class frameset
	{
		friend class animation;
	public:
        /// function which builds frames.
		using framebuilder = std::function<bool(std::size_t pos, paint::graphics&, nana::size&)>;

		struct impl;
	public:
		frameset();
		void push_back(paint::image);        ///< Inserts frames at the end.
		void push_back(framebuilder fb, std::size_t length);  ///< Inserts a framebuilder and the number of frames that it generates.
	private:
		std::shared_ptr<impl> impl_;
	};
            /// Easy way to display an animation or create an animated GUI 
	class animation
	{
		struct branch_t
		{
			frameset frames;
			std::function<std::size_t(const std::string&, std::size_t, std::size_t&)> condition;
		};
		
		struct impl;
		class performance_manager;

		/// Non-copyable
		animation(const animation&) = delete;
		animation& operator=(const animation&) = delete;
	public:
		animation(std::size_t fps = 23);
		~animation();

		animation(animation&&);
		animation& operator=(animation&&);

		void push_back(frameset frms);

		void looped(bool enable);       ///< Enables or disables the animation repeating playback.

		void play();

		void pause();

		void output(window wd, const nana::point& pos);

		void fps(std::size_t n);
		std::size_t fps() const;
	private:
		impl * impl_;
	};
}	//end namespace nana
#include <nana/pop_ignore_diagnostic>
#endif	//NANA_GUI_ANIMATION_HPP
