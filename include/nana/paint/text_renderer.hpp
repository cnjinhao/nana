#ifndef NANA_PAINT_TEXT_RENDERER_HPP
#define NANA_PAINT_TEXT_RENDERER_HPP
#include <nana/paint/graphics.hpp>

namespace nana
{
	namespace paint
	{
		class text_renderer
		{
		public:
			using graph_reference = graphics &;

			enum class mode
			{
				truncate_with_ellipsis,
				truncate_letter_with_ellipsis,
				word_wrap
			};
			
			text_renderer(graph_reference graph, align = align::left);

			nana::size extent_size(int x, int y, const wchar_t*, std::size_t len, unsigned space_pixels) const;

			void render(const point&, const wchar_t*, std::size_t len);
			void render(const point&, const wchar_t*, std::size_t len, unsigned space_pixels, mode);
		private:
			graph_reference graph_;
			align text_align_;
		};

		/// Draw aligned string
		class aligner
		{
		public:
			using graph_reference = graphics&;

			/// Constructor
			/**
			 * @param graph Reference to a graphics object
			 * @param text_align Alignment of text
			 * @param text_align_if_too_long Alignment of text if the pixels of string is larger than text area
			 */
			aligner(graph_reference graph, align text_align = align::left);
			aligner(graph_reference graph, align text_align, align text_align_if_too_long);

			/// Draws a text with specified text alignment.
			/**
			 * @param text	Text to draw
			 * @param pos	Postion where the text to draw
			 * @param width The width of text area. If the pixels of text is larger than this parameter, it draws ellipsis  
			 */
			void draw(const std::string& text, point pos, unsigned width);
			void draw(const std::wstring& text, point pos, unsigned width);
		private:
			graph_reference graph_;
			align text_align_;
			align text_align_ex_;
		};
	}
}

#endif
