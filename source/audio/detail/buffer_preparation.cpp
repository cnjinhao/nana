#include <nana/audio/detail/buffer_preparation.hpp>

#ifdef NANA_ENABLE_AUDIO

#include <cstring>

namespace nana{	namespace audio
{
	namespace detail
	{
		//class buffer_preparation
			buffer_preparation::buffer_preparation(audio_stream& as, std::size_t seconds)
				: running_(true), wait_for_buffer_(false), as_(as)
			{
				//Allocate the space
				buffer_.reserve(seconds);
				prepared_.reserve(seconds);

				const wave_spec::format_chunck & ck = as.format();
				block_size_ = ck.nAvgBytesPerSec;
				for(std::size_t i = 0; i < seconds; ++i)
				{
					char * rawbuf = new char[sizeof(meta) + ck.nAvgBytesPerSec];
					meta * m = reinterpret_cast<meta*>(rawbuf);
#if defined(NANA_WINDOWS)
					memset(m, 0, sizeof(meta));
					m->dwBufferLength = static_cast<unsigned long>(block_size_);
					m->lpData = rawbuf + sizeof(meta);
#elif defined(NANA_LINUX)
					m->bufsize = ck.nAvgBytesPerSec;
					m->buf = rawbuf + sizeof(meta);
#endif
					prepared_.push_back(m);
				}

				thr_ = std::move(std::thread([this](){this->_m_prepare_routine();}));
			}

			buffer_preparation::~buffer_preparation()
			{
				running_ = false;

				cond_prepared_.notify_one();
				cond_buffer_.notify_one();

				if(thr_.joinable())
					thr_.join();

				for(auto metaptr : prepared_)
					delete [] reinterpret_cast<char*>(metaptr);
			}

			buffer_preparation::meta * buffer_preparation::read()
			{
				std::unique_lock<decltype(token_buffer_)> lock(token_buffer_);

				//Wait for the buffer
				if(0 == buffer_.size())
				{
					//Before waiting, checks the thread whether it is finished
					//it indicates the preparation is finished.
					if(false == running_)
						return nullptr;

					wait_for_buffer_ = true;
					cond_buffer_.wait(lock);

					//NO more buffers
					if(0 == buffer_.size())
						return nullptr;
				}
				meta * m = buffer_.front();
				buffer_.erase(buffer_.begin());
				return m;
			}

			//Revert the meta that returned by read()
			void buffer_preparation::revert(meta * m)
			{
				std::lock_guard<decltype(token_prepared_)> lock(token_prepared_);
				bool if_signal = prepared_.empty();
				prepared_.push_back(m);
				if(if_signal)
					cond_prepared_.notify_one();
			}

			bool buffer_preparation::data_finished() const
			{
				std::lock_guard<decltype(token_prepared_)> lock(token_prepared_);
				return ((prepared_.size() == prepared_.capacity()) && (running_ == false));
			}

			void buffer_preparation::_m_prepare_routine()
			{
				const std::size_t fixed_bufsize = 1024;
				char buf[fixed_bufsize];
				const std::size_t block_size = block_size_;

				while(running_)
				{
					meta * m = 0;
					{
						std::unique_lock<decltype(token_prepared_)> lock(token_prepared_);

						if(prepared_.size() == 0)
						{
							cond_prepared_.wait(lock);

							if(false == running_)
								return;
						}
						m = prepared_.back();
						prepared_.pop_back();
					}

					std::size_t buffered = 0;
					while(buffered != block_size)
					{
						std::size_t want_bytes = block_size - buffered;
						if(want_bytes > fixed_bufsize)	want_bytes = fixed_bufsize;

						std::size_t read_bytes = as_.read(buf, want_bytes);
						if(read_bytes)
						{
#if defined(NANA_WINDOWS)
							memcpy(m->lpData + buffered, buf, read_bytes);
#elif defined(NANA_LINUX)
							memcpy(m->buf + buffered, buf, read_bytes);
#endif
							buffered += read_bytes;
						}
						else if(0 == as_.data_length())
						{
							if(buffered == 0)
							{
								//PCM data is drained
								std::lock_guard<decltype(token_buffer_)> lock(token_buffer_);
								cond_buffer_.notify_one();
								running_ = false;
								return;
							}
							break;
						}
					}
#if defined(NANA_WINDOWS)
					m->dwBufferLength = static_cast<unsigned long>(buffered);
#elif defined(NANA_LINUX)
					m->bufsize = buffered;
#endif
					std::lock_guard<decltype(token_buffer_)> lock(token_buffer_);
					buffer_.push_back(m);
					if(wait_for_buffer_)
					{
						cond_buffer_.notify_one();
						wait_for_buffer_ = false;
					}
					if(0 == as_.data_length())
						running_ = false;
				}
			}
		//end class buffer_preparation
	}//end namespace detail
}//end namespace audio
}//end namespace nana

#endif //NANA_ENABLE_AUDIO