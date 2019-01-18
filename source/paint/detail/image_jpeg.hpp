#ifndef NANA_PAINT_DETAIL_IMAGE_JPEG_HPP
#define NANA_PAINT_DETAIL_IMAGE_JPEG_HPP

#include "image_pixbuf.hpp"

//Separate the libpng from the package that system provides.
#if defined(NANA_LIBJPEG)
	#include <nana_extrlib/jpeglib.h>
#else
	#include <jpeglib.h>
#endif

#include <stdio.h>
#include <csetjmp>

namespace nana
{
	namespace paint{	namespace detail{

		class image_jpeg
			: public basic_image_pixbuf
		{
			struct error_mgr
			{
				struct ::jpeg_error_mgr pub;
				std::jmp_buf	setjmp_buf;
			};

			void _m_read_jpg(jpeg_decompress_struct& jdstru)
			{
				::jpeg_read_header(&jdstru, true);	//Reject a tables-only JPEG file as an error

				::jpeg_start_decompress(&jdstru);

				//JSAMPLEs per row in output buffer
				auto row_stride = jdstru.output_width * jdstru.output_components;

				pixbuf_.open(jdstru.output_width, jdstru.output_height);

				auto row_buf = jdstru.mem->alloc_sarray(reinterpret_cast<j_common_ptr>(&jdstru), JPOOL_IMAGE, row_stride, 1);

				while (jdstru.output_scanline < jdstru.output_height)
				{
					::jpeg_read_scanlines(&jdstru, row_buf, 1);

					pixbuf_.fill_row(jdstru.output_scanline - 1, reinterpret_cast<unsigned char*>(*row_buf), row_stride, jdstru.output_components * 8);
				}
			}
		public:
			bool open(const std::filesystem::path& jpeg_file) override
			{
				auto fp = ::fopen(to_osmbstr(to_utf8(jpeg_file.native())).c_str(), "rb");
				if(nullptr == fp) return false;

				bool is_opened = false;

				struct ::jpeg_decompress_struct jdstru;
				error_mgr	jerr;

				jdstru.err = ::jpeg_std_error(&jerr.pub);
				jerr.pub.error_exit = _m_error_handler;

				if (!setjmp(jerr.setjmp_buf))
				{
					::jpeg_create_decompress(&jdstru);

					::jpeg_stdio_src(&jdstru, fp);

					_m_read_jpg(jdstru);

					jpeg_finish_decompress(&jdstru);
					is_opened = true;
				}

				::jpeg_destroy_decompress(&jdstru);
				::fclose(fp);
				return is_opened;
			}

			bool open(const void* data, std::size_t bytes) override
			{
				bool is_opened = false;

				struct ::jpeg_decompress_struct jdstru;
				error_mgr	jerr;

				jdstru.err = ::jpeg_std_error(&jerr.pub);
				jerr.pub.error_exit = _m_error_handler;

				if (!setjmp(jerr.setjmp_buf))
				{
					::jpeg_create_decompress(&jdstru);

					::jpeg_mem_src(&jdstru, const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(data)), static_cast<unsigned long>(bytes));
					_m_read_jpg(jdstru);

					jpeg_finish_decompress(&jdstru);
					is_opened = true;
				}

				::jpeg_destroy_decompress(&jdstru);
				return is_opened;
			}
		private:
			static void _m_error_handler(::j_common_ptr jdstru)
			{
				auto err_ptr = reinterpret_cast<error_mgr*>(jdstru->err);

				err_ptr->pub.output_message(jdstru);
				longjmp(err_ptr->setjmp_buf, 1);
			}
		};
	}//end namespace detail
	}//end namespace paint
}//end namespace nana

#endif
