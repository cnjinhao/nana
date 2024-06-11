/*
 *	An Animation Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
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
	class [[deprecated("Deprecated in 1.8")]] animation;
        /// Holds the frames and frame builders. Have reference semantics for efficiency.


	class [[deprecated("Deprecated in 1.8")]] frameset
	{
		friend class animation;
	public:
        /// function which builds frames.
		using framebuilder = std::function<bool(std::size_t pos, paint::graphics&, nana::size&)>;
	public:
		frameset();
		void push_back(paint::image, std::size_t duration = 0);	///< Inserts frames at the end.
		void push_back(framebuilder fb, std::size_t length);  ///< Inserts a framebuilder and the number of frames that it generates.
	private:
		struct impl;
		std::shared_ptr<impl> impl_;
	};
            /// Easy way to display an animation or create an animated GUI
	class [[deprecated("Deprecated in 1.8")]] animation
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
		void push_back(paint::image, std::size_t duration = 0);

		void looped(bool enable);       ///< Enables or disables the animation repeating playback.

		void play();

		void pause();

		/// Renders the animation at a fixed position
		void output(window wd, const nana::point& pos);

		/// Renders the animation at a rectangle
		/**
		 * If the size of rectangle is not equal to the size of frame, it stretches the frame for the size of rectangle.
		 * @param wd Output window.
		 * @param r Generator of the rectangle. The generator gets called every time rendering occurs.
		 */
		void output(window wd, std::function<nana::rectangle()> r);

		void fps(std::size_t n);
		std::size_t fps() const;
	private:
		std::unique_ptr<impl> impl_;
	};
}	//end namespace nana
#include <nana/pop_ignore_diagnostic>
#endif	//NANA_GUI_ANIMATION_HPP
