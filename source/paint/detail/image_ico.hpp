#ifndef NANA_PAINT_DETAIL_IMAGE_ICO_HPP
#define NANA_PAINT_DETAIL_IMAGE_ICO_HPP

#include <nana/paint/detail/image_impl_interface.hpp>
#include <nana/filesystem/filesystem.hpp>

namespace nana{	namespace paint
{
	namespace detail
	{
		class image_ico
			:public image::image_impl_interface
		{
#if defined(NANA_WINDOWS)
			struct handle_deleter
			{
				void operator()(HICON* handle) const;
			};//end struct handle_deleter
			typedef std::shared_ptr<HICON> ptr_t;
#else
			typedef std::shared_ptr<int*> ptr_t;
#endif
		public:
			image_ico(bool is_ico);


			bool open(const ::nana::experimental::filesystem::path& filename) override;
			bool open(const void* data, std::size_t bytes) override;
			bool alpha_channel() const override;
			bool empty() const override;
			void close() override;
			nana::size size() const override;
			virtual void paste(const nana::rectangle& src_r, graph_reference graph, const point& p_dst) const override;
			virtual void stretch(const nana::rectangle&, graph_reference graph, const nana::rectangle& r) const override;
			const ptr_t & ptr() const;
		private:
			const bool	is_ico_;
			nana::size	size_;
			ptr_t ptr_;
		};//end class image_ico
	}
}//end namespace paint
}//end namespace nana

#endif
