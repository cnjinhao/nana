
#include <nana/audio/detail/audio_device.hpp>

#ifdef NANA_ENABLE_AUDIO

#include <nana/system/platform.hpp>

#if defined(NANA_POSIX)
	#include <pthread.h>
	#include <unistd.h>
	#include <sys/time.h>
	#include <errno.h>
	#include <fcntl.h>

#	ifndef NANA_LINUX
    	static bool get_default_audio(std::string &, bool);
#	endif
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
			audio_device::audio_device():
#if defined(NANA_WINDOWS) || defined(NANA_LINUX)
				handle_(nullptr),
#elif defined(NANA_POSIX)
				handle_(-1),
#endif
				buf_prep_(nullptr)
			{}

			audio_device::~audio_device()
			{
				close();
			}

			bool audio_device::empty() const
			{
#if defined(NANA_POSIX) && !defined(NANA_LINUX)
				return (-1 == handle_);
#else
				return (nullptr == handle_);
#endif
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
                // assumes ALSA sub-system
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
#elif defined(NANA_POSIX)
                std::string dsp;
                if ( !get_default_audio(dsp, true) )
                    return false;
                handle_ = ::open(dsp.c_str(), O_WRONLY);
                if (handle_ == -1)
                    return false;

                int zero = 0;
                int caps = 0;
                int fragment = 0x200008L;
                int ok;
                ok = ioctl(handle_, SNDCTL_DSP_COOKEDMODE, &zero);
                if (ok >= 0)
                {
                    ok = ioctl(handle_, SNDCTL_DSP_SETFRAGMENT, &fragment);
                    if (ok >= 0)
                    {
                        ok = ioctl(handle_, SNDCTL_DSP_GETCAPS, &caps);
                        if (ok >= 0)
                        {
                            ok = ioctl(handle_, SNDCTL_DSP_SETFMT, &bits_per_sample);
                            if (ok >= 0)
                            {
                                ok = ioctl(handle_, SNDCTL_DSP_CHANNELS, &channels);
                                if (ok >= 0)
                                {
                                    ok = ioctl(handle_, SNDCTL_DSP_SPEED, &rate);
                                    if (ok >= 0)
                                    {
                                        channels_ = channels;
                                        rate_ = rate;
                                        bytes_per_sample_ = ( (bits_per_sample + 7) >> 3 );
                                        bytes_per_frame_ = bytes_per_sample_ * channels;
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
                // failure so close handle.
                ::close(handle_);
                handle_ = -1;
                return false;
#endif
			}

			void audio_device::close()
			{
				if(handle_)
				{
#if defined(NANA_WINDOWS)
					wave_native_if.out_close(handle_);
					handle_ = nullptr;
#elif defined(__FreeBSD__)
                    ::close(handle_);
					handle_ = 0;
#elif defined(NANA_LINUX)
					::snd_pcm_close(handle_);
					handle_ = nullptr;
#endif
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
				done_queue_.emplace_back(m);
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
#elif defined(NANA_POSIX)
                // consider moving this to a background thread.
                // currently this blocks calling thread.
                ::write(handle_, m->buf, m->bufsize);
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

#if defined(NANA_POSIX) && !defined(NANA_LINUX)
// parse input securely, no-overruns or overflows.
static bool match(char *&cursor, const char *pattern, const char *tail)
{
	char *skim = cursor;
	while (*skim != '\n' && *pattern != 0 && cursor != tail)
	{
		if (*pattern != *skim)
			return false;
		pattern++;
		skim++;
	}
	if (*pattern == 0)
	{
		cursor = skim;
		return true;
	}
	return false;
}

// parse input securely, no-overruns or overflows.
static bool skip(char *&cursor, const char stop, const char *tail)
{
	char *skim = cursor;
	while (*skim != '\n' && cursor != tail)
	{
		if (stop == *skim)
		{
			cursor = skim;
			return true;
		}
		skim++;
	}
	return false;
}

struct audio_device
{
    // pcm name
    std::string device;
    // /dev/dsp
    std::string path;
    // terse description
    std::string description;
    // can record eg. microphone or line-in.
    bool rec;
    // can play - speaker, headphone or line-out.
    bool play;
    // is the default device.
    bool chosen;
};

static bool get_audio_devices(std::vector<audio_device> &list)
{
	// read the sndstat device to get a list of audio devices.
	// mostly OSS but some ALSA installs mimic this.
	FILE *cat = fopen("/dev/sndstat", "r");
	if (cat != 0)
	{
		char line[128] = {0};
		const char *last = line + sizeof line;
		while ( fgets(line, sizeof line, cat) != 0 )
		{
			// extract five things about a device: name, description, play, rec, and default.
		    audio_device next;
			char *cursor = line;
			// ignore these lines
			if ( match(cursor, "Installed", last) || match(cursor, "No devices", last) )
				continue;

			const char *device = cursor;
			if ( !skip(cursor, ':', last) )
				continue;
			// nul terminate device name.
			*cursor++ = 0;
			next.device = device;

			if ( !skip(cursor, '<', last) )
				continue;

			const char *description = ++cursor;
			if ( !skip(cursor, '>', last) )
				continue;
			// nul terminate description.
			*cursor++ = 0;
			next.description = description;

			if ( !skip(cursor, '(', last) )
				continue;

			cursor++;
			// supports play?
			next.play = match(cursor, "play", last);
			if (next.play)
				match(cursor, "/", last);

			// supports record?
			next.rec = match(cursor, "rec", last);
			if ( !skip(cursor, ')', last) )
				continue;

			cursor++;
			// default ?
			if ( match(cursor, " ", last) )
				next.chosen = match(cursor, "default", last);

            if (next.device.compare(0, 3, "pcm") == 0)
            {
                // proper dev path with number appended.
                next.path = "/dev/dsp";
                next.path += next.device.c_str() + 3;
            }

            list.push_back(next);
		}
		fclose(cat);
	}
	return list.size() > 0;
}

static bool get_default_audio(std::string &dsp, bool play)
{
    std::vector<audio_device> list;
    if ( !get_audio_devices(list) )
        return false;
    for (auto it = list.begin(); it != list.end(); it++)
    {
        if ( (it->play && play) || (it->rec && !play) )
        {
            if (it->chosen)
            {
                dsp = it->path;
                return true;
            }
        }
    }
    return false;
}
#endif //NANA_POSIX

#endif //NANA_ENABLE_AUDIO
