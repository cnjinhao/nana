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

			nana::size extent_size(int x, int y, const wchar_t*, std::size_t len, unsigned restricted_pixels) const;

			void render(const point&, const wchar_t*, std::size_t len);
			void render(const point&, const wchar_t*, std::size_t len, unsigned restricted_pixels, bool omitted);
			void render(const point&, const wchar_t*, std::size_t len, unsigned restricted_pixels);
		private:
			graph_reference graph_;
			align text_align_;
		};
	}
}

#endif
