#ifndef NANA_PAINT_DETAIL_IMAGE_IMPL_INTERFACE_HPP
#define NANA_PAINT_DETAIL_IMAGE_IMPL_INTERFACE_HPP

#include "../image.hpp"
#include <nana/filesystem/filesystem.hpp>

namespace nana{	namespace paint{

	// class image::image_impl_interface
	//		the nana::image refers to an object of image::image_impl_interface by nana::refer. Employing nana::refer to refer the image::implement_t object indirectly is used
	//	for saving the memory that sharing the same image resource with many nana::image objects.
	class image::image_impl_interface
		: private nana::noncopyable
	{
		image_impl_interface& operator=(const image_impl_interface& rhs);
	public:
		typedef nana::paint::graphics& graph_reference;
		virtual ~image_impl_interface() = 0;	//The destructor is defined in ../image.cpp
		virtual bool open(const nana::experimental::filesystem::path& file) = 0;
		virtual bool open(const void* data, std::size_t bytes) = 0; // reads image from memory
		virtual bool alpha_channel() const = 0;
		virtual bool empty() const = 0;
		virtual void close() = 0;
		virtual nana::size size() const = 0;
		virtual void paste(const nana::rectangle& src_r, graph_reference dst, const point& p_dst) const = 0;
		virtual void stretch(const nana::rectangle& src_r, graph_reference dst, const nana::rectangle& r) const = 0;
	};//end class image::image_impl_interface
}//end namespace paint
}//end namespace nana

#endif
