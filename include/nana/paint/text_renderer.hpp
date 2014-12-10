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
			typedef graphics & graph_reference;
			
			text_renderer(graph_reference graph, align = align::left);

			void render(int x, int y, nana::color_t, const nana::char_t*, std::size_t len);
			void render(int x, int y, nana::color_t, const nana::char_t*, std::size_t len, unsigned restricted_pixels, bool omitted);

			void render(int x, int y, nana::color_t, const nana::char_t*, std::size_t len, unsigned restricted_pixels);
			nana::size extent_size(int x, int y, const nana::char_t*, std::size_t len, unsigned restricted_pixels) const;
		private:
			graph_reference graph_;
			align text_align_;
		};
	}
}

#endif
