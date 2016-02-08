
#include <nana/audio/detail/audio_device.hpp>

#ifdef NANA_ENABLE_AUDIO

#include <nana/system/platform.hpp>

#if defined(NANA_LINUX)
	#include <pthread.h>
	#include <unistd.h>
	#include <sys/time.h>
	#include <errno.h>
#endif

namespace nana{namespace audio
{
	namespace detail
	{
#if defined(NANA_WINDOWS)
		class wave_native
		{
			typedef MMRESULT (__stdcall *out_open_t)(LPHWAVEOUT, UINT_PTR, LPWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
			typedef MMRESULT (__stdcall *out_close_t)(HWAVEOUT);
			typedef MMRESULT (__stdcall *out_op_header_t)(HWAVEOUT, LPWAVEHDR, UINT);
		public:
			out_open_t out_open;
			out_close_t out_close;
			out_op_header_t out_write;
			out_op_header_t out_prepare;
			out_op_header_t out_unprepare;

			wave_native()
			{
				HMODULE winmm = ::GetModuleHandleA("Winmm.DLL");
				if(0 == winmm)
					winmm = ::LoadLibraryA("Winmm.DLL");

				out_open = reinterpret_cast<out_open_t>(::GetProcAddress(winmm, "waveOutOpen"));
				out_close = reinterpret_cast<out_close_t>(::GetProcAddress(winmm, "waveOutClose"));
				out_write = reinterpret_cast<out_op_header_t>(::GetProcAddress(winmm, "waveOutWrite"));
				out_prepare = reinterpret_cast<out_op_header_t>(::GetProcAddress(winmm, "waveOutPrepareHeader"));
				out_unprepare = reinterpret_cast<out_op_header_t>(::GetProcAddress(winmm, "waveOutUnprepareHeader"));
			}
		}wave_native_if;
#endif
		//class audio_device
			audio_device::audio_device()
#if defined(NANA_WINDOWS)
				: handle_(nullptr), buf_prep_(nullptr)
#elif defined(NANA_LINUX)
				: handle_(nullptr), buf_prep_(nullptr)
#endif
			{}

			audio_device::~audio_device()
			{
				close();
			}

			bool audio_device::empty() const
			{
				return (nullptr == handle_);
			}

			bool audio_device::open(std::size_t channels, std::size_t rate, std::size_t bits_per_sample)
			{
#if defined(NANA_WINDOWS)
				close();

				WAVEFORMATEX wfx;
				wfx.wFormatTag = WAVE_FORMAT_PCM;
				wfx.nChannels = static_cast<WORD>(channels);
				wfx.nSamplesPerSec = static_cast<DWORD>(rate);
				wfx.wBitsPerSample = static_cast<WORD>(bits_per_sample);

				wfx.nBlockAlign = (wfx.wBitsPerSample >> 3 ) * wfx.nChannels;
				wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
				wfx.cbSize = 0;

				MMRESULT mmr = wave_native_if.out_open(&handle_, WAVE_MAPPER, &wfx, reinterpret_cast<DWORD_PTR>(&audio_device::_m_dev_callback), reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION);
				return (mmr == MMSYSERR_NOERROR);
#elif defined(NANA_LINUX)
				if(nullptr == handle_)
				{
					if(::snd_pcm_open(&handle_, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0) < 0)
						return false;
				}

				if(handle_)
				{
					channels_ = channels;
					rate_ = rate;
					bytes_per_sample_ = (bits_per_sample >> 3);
					bytes_per_frame_ = bytes_per_sample_ * channels;

					snd_pcm_hw_params_t * params;
					if(snd_pcm_hw_params_malloc(&params) < 0)
					{
						close();
						return false;
					}

					if(::snd_pcm_hw_params_any(handle_, params) < 0)
					{
						close();
						::snd_pcm_hw_params_free(params);
						return false;
					}

					if(::snd_pcm_hw_params_set_access(handle_, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
					{
						close();
						::snd_pcm_hw_params_free(params);
						return false;
					}

					snd_pcm_format_t format = SND_PCM_FORMAT_U8;
					switch(bits_per_sample)
					{
					case 8:
						format = SND_PCM_FORMAT_U8;	break;
					case 16:
						format = SND_PCM_FORMAT_S16_LE;	break;
					case 32:
						format = SND_PCM_FORMAT_S32_LE;	break;
					}

					if(::snd_pcm_hw_params_set_format(handle_, params, format) < 0)
					{
						close();
						::snd_pcm_hw_params_free(params);
						return false;
					}

					unsigned tmp = rate;
					if(::snd_pcm_hw_params_set_rate_near(handle_, params, &tmp, 0) < 0)
					{
						close();
						::snd_pcm_hw_params_free(params);
						return false;
					}

					if(::snd_pcm_hw_params_set_channels(handle_, params, channels) < 0)
					{
						close();
						::snd_pcm_hw_params_free(params);
						return false;
					}

					if(::snd_pcm_hw_params(handle_, params) < 0)
					{
						close();
						::snd_pcm_hw_params_free(params);
						return false;
					}

					::snd_pcm_hw_params_free(params);
					::snd_pcm_prepare(handle_);
					return true;
				}
				return false;
#endif
			}

			void audio_device::close()
			{
				if(handle_)
				{
#if defined(NANA_WINDOWS)
					wave_native_if.out_close(handle_);
#elif defined(NANA_LINUX)
					::snd_pcm_close(handle_);
#endif
					handle_ = nullptr;
				}
			}

			void audio_device::prepare(buffer_preparation & buf_prep)
			{
				buf_prep_ = & buf_prep;
			}

			void audio_device::write(buffer_preparation::meta * m)
			{
#if defined(NANA_WINDOWS)
				std::lock_guard<decltype(queue_lock_)> lock(queue_lock_);
				done_queue_.push_back(m);
				if(m->dwFlags & WHDR_PREPARED)
					wave_native_if.out_unprepare(handle_, m, sizeof(WAVEHDR));

				wave_native_if.out_prepare(handle_, m, sizeof(WAVEHDR));
				wave_native_if.out_write(handle_, m, sizeof(WAVEHDR));
#elif defined(NANA_LINUX)
				std::size_t frames = m->bufsize / bytes_per_frame_;
				std::size_t buffered = 0; //in bytes
				while(frames > 0)
				{
					int err = ::snd_pcm_writei(handle_, m->buf + buffered, frames);
					if(err > 0)
					{
						frames -= err;
						buffered += err * bytes_per_frame_;
					}
					else if(-EPIPE == err)
						::snd_pcm_prepare(handle_);
				}
				buf_prep_->revert(m);
#endif
			}

			void audio_device::wait_for_drain() const
			{
#if defined(NANA_WINDOWS)
				while(buf_prep_->data_finished() == false)
					nana::system::sleep(200);
#elif defined(NANA_LINUX)
				while(::snd_pcm_state(handle_) == SND_PCM_STATE_RUNNING)
					nana::system::sleep(200);
#endif
			}

#if defined(NANA_WINDOWS)
			void __stdcall audio_device::_m_dev_callback(HWAVEOUT handle, UINT msg, audio_device * self, DWORD_PTR, DWORD_PTR)
			{
				if(WOM_DONE == msg)
				{
					buffer_preparation::meta * m;
					{
						std::lock_guard<decltype(queue_lock_)> lock(self->queue_lock_);
						m = self->done_queue_.front();
						self->done_queue_.erase(self->done_queue_.begin());
					}
					wave_native_if.out_unprepare(handle, m, sizeof(WAVEHDR));
					self->buf_prep_->revert(m);
				}
			}
#endif
		//end class audio_device
	}//end namespace detail
}//end namespace audio
}//end namespace nana

#endif //NANA_ENABLE_AUDIO