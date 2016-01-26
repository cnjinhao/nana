/*
 *	An Animation Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/animation.cpp
 */


#include <nana/gui/animation.hpp>
#include <nana/gui/drawing.hpp>
#include <nana/system/timepiece.hpp>
#include <nana/system/platform.hpp>

#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <atomic>

#if defined(STD_THREAD_NOT_SUPPORTED)
    #include <nana/std_thread.hpp>
    #include <nana/std_mutex.hpp>
    #include <nana/std_condition_variable.hpp>
#else
    #include <mutex>
    #include <condition_variable>
    #include <thread>
#endif // STD_THREAD_NOT_SUPPORTED

namespace nana
{
	class animation;

	struct output_t
	{
		drawing::diehard_t diehard{ nullptr };
		std::vector<nana::point> points;
	};

	struct framebuilder
	{
		std::size_t length;
		std::function<bool(std::size_t, paint::graphics&, nana::size&)> frbuilder;

		framebuilder(std::function<bool(std::size_t, paint::graphics&, nana::size&)> f, std::size_t l)
			: length(l), frbuilder(std::move(f))
		{}
	};

	struct frame
	{
		enum class kind
		{
			oneshot,
			framebuilder
		};

		frame(paint::image img)
			: type(kind::oneshot)
		{
			u.oneshot = new paint::image(std::move(img));
		}

		frame(std::function<bool(std::size_t, paint::graphics&, nana::size&)> frbuilder, std::size_t length)
			: type(kind::framebuilder)
		{
			u.frbuilder = new framebuilder(std::move(frbuilder), length);
		}

		frame(const frame& r)
			: type(r.type)
		{
			switch(type)
			{
			case kind::oneshot:
				u.oneshot = new paint::image(*r.u.oneshot);
				break;
			case kind::framebuilder:
				u.frbuilder = new framebuilder(*r.u.frbuilder);
				break;
			}
		}

		frame(frame&& r)
			: type(r.type)
		{
			u = r.u;
			r.u.oneshot = nullptr;
		}

		~frame()
		{
			switch(type)
			{
			case kind::oneshot:
				delete u.oneshot;
				break;
			case kind::framebuilder:
				delete u.frbuilder;
				break;
			}
		}

		frame& operator=(const frame& r)
		{
			if(this != &r)
			{
				switch(type)
				{
				case kind::oneshot:
					delete u.oneshot;
					break;
				case kind::framebuilder:
					delete u.frbuilder;
					break;
				}

				type = r.type;
				switch(type)
				{
				case kind::oneshot:
					u.oneshot = new paint::image(*r.u.oneshot);
					break;
				case kind::framebuilder:
					u.frbuilder = new framebuilder(*r.u.frbuilder);
					break;
				}
			}
			return *this;
		}

		frame& operator=(frame&& r)
		{
			if(this != &r)
			{
				switch(type)
				{
				case kind::oneshot:
					delete u.oneshot;
					break;
				case kind::framebuilder:
					delete u.frbuilder;
					break;
				}

				type = r.type;
				u = r.u;
				r.u.oneshot = nullptr;
			}
			return *this;
		}

		std::size_t length() const
		{
			switch(type)
			{
			case kind::oneshot:
				return 1;
			case kind::framebuilder:
				return u.frbuilder->length;
			}
			return 0;
		}

		//
		kind type;
		union uframes
		{
			paint::image * oneshot;
			framebuilder * frbuilder;
		}u;
	};

	//class frameset
		//struct frameset::impl
		struct frameset::impl
		{
			//Only list whos iterator would not invalided after a insertion.
			std::list<frame> frames;
			std::list<frame>::iterator this_frame;
			std::size_t pos_in_this_frame{ 0 };
			mutable bool good_frame_by_frmbuilder{ false };	//It indicates the state of frame whether is valid.

			impl()
				:	this_frame(frames.end())
			{}

			//Render A frame on the set of windows.
			void render_this(std::map<window, output_t>& outs, paint::graphics& framegraph, nana::size& framegraph_dimension) const
			{
				if(this_frame == frames.end())
					return;

				frame & frmobj = *this_frame;
				switch(frmobj.type)
				{
				case frame::kind::oneshot:
					_m_render(outs, [&frmobj](paint::graphics& tar, const nana::point& pos)
					{
						frmobj.u.oneshot->paste(tar, pos);
					});
					break;
				case frame::kind::framebuilder:
					good_frame_by_frmbuilder = frmobj.u.frbuilder->frbuilder(pos_in_this_frame, framegraph, framegraph_dimension);
					if(good_frame_by_frmbuilder)
					{
						nana::rectangle r(framegraph_dimension);
						_m_render(outs, [&r, &framegraph](paint::graphics& tar, const nana::point& pos) mutable
						{
							r.x = pos.x;
							r.y = pos.y;
							tar.bitblt(r, framegraph);
						});
					}
					break;
				}
			}

			//Render a frame on a specified window graph
			void render_this(paint::graphics& graph, const nana::point& pos, paint::graphics& framegraph, nana::size& framegraph_dimension, bool rebuild_frame) const
			{
				if(this_frame == frames.end())
					return;

				frame & frmobj = *this_frame;
				switch(frmobj.type)
				{
				case frame::kind::oneshot:
					frmobj.u.oneshot->paste(graph, pos);
					break;
				case frame::kind::framebuilder:
					if(rebuild_frame)
						good_frame_by_frmbuilder = frmobj.u.frbuilder->frbuilder(pos_in_this_frame, framegraph, framegraph_dimension);

					if(good_frame_by_frmbuilder)
					{
						nana::rectangle r(pos, framegraph_dimension);
						graph.bitblt(r, framegraph);
					}
					break;
				}
			}

			bool eof() const
			{
				return (frames.end() == this_frame);
			}

			void next_frame()
			{
				if(frames.end() == this_frame)
					return;

				frame & frmobj = *this_frame;
				switch(frmobj.type)
				{
				case frame::kind::oneshot:
					++this_frame;
					pos_in_this_frame = 0;
					break;
				case frame::kind::framebuilder:
					if(pos_in_this_frame >= frmobj.u.frbuilder->length)
					{
						pos_in_this_frame = 0;
						++this_frame;
					}
					else
						++pos_in_this_frame;
					break;
				default:
					throw std::runtime_error("Nana.GUI.Animation: Bad frame type");
				}
			}

			//Seek to the first frame
			void reset()
			{
				this_frame = frames.begin();
				pos_in_this_frame = 0;
			}
		private:
			template<typename Renderer>
			void _m_render(std::map<window, output_t>& outs, Renderer renderer) const
			{
				for(auto & tar: outs)
				{
					auto graph = API::dev::window_graphics(tar.first);
					if(nullptr == graph)
						continue;

					for(auto & outp : tar.second.points)
						renderer(*graph, outp);

					API::update_window(tar.first);
				}
			}
		};//end struct frameset::impl
	//public:
		frameset::frameset()
			: impl_(new impl)
		{}

		void frameset::push_back(paint::image img)
		{
			bool located = impl_->this_frame != impl_->frames.end();
			impl_->frames.emplace_back(std::move(img));
			if(false == located)
				impl_->this_frame = impl_->frames.begin();
		}

		void frameset::push_back(framebuilder fb, std::size_t length)
		{
			impl_->frames.emplace_back(std::move(fb), length);
			if(1 == impl_->frames.size())
				impl_->this_frame = impl_->frames.begin();
		}
	//end class frameset

	//class animation
		class animation::performance_manager
		{
		public:
			struct thread_variable
			{
				std::mutex mutex;
				std::condition_variable condvar;
				std::vector<impl*> animations;

				std::size_t active;				//The number of active animations
				std::shared_ptr<std::thread> thread;

				std::size_t fps;
				double interval;	//milliseconds between 2 frames.
				double performance_parameter;
			};

			void insert(impl* p);
			void set_fps(impl*, std::size_t new_fps);
			void close(impl* p);
			bool empty() const;
		private:
			mutable std::recursive_mutex mutex_;
			std::vector<thread_variable*> threads_;
		};	//end class animation::performance_manager

		struct animation::impl
		{
			bool	looped{false};
			std::atomic<bool>	paused{true};
			std::size_t fps;

			std::list<frameset> framesets;
			std::map<std::string, branch_t> branches;
			std::map<window, output_t> outputs;

			paint::graphics framegraph;	//framegraph will be created by framebuilder
			nana::size framegraph_dimension;

			struct state_t
			{
				std::list<frameset>::iterator this_frameset;
			}state;

			performance_manager::thread_variable * thr_variable;
			static performance_manager * perf_manager;


			impl(std::size_t fps)
				: fps(fps)
			{
				state.this_frameset = framesets.begin();

				if (!perf_manager)
				{
					nana::internal_scope_guard lock;
					if (!perf_manager)
					{
						auto pm = new performance_manager;
						perf_manager = pm;
					}
				}
				perf_manager->insert(this);
			}

			~impl()
			{
				perf_manager->close(this);
				{
					nana::internal_scope_guard lock;
					if(perf_manager->empty())
					{
						delete perf_manager;
						perf_manager = nullptr;
					}
				}
			}

			void render_this_specifically(paint::graphics& graph, const nana::point& pos)
			{
				if(state.this_frameset != framesets.end())
					state.this_frameset->impl_->render_this(graph, pos, framegraph, framegraph_dimension, false);
			}

			void render_this_frame()
			{
				if(state.this_frameset != framesets.end())
					state.this_frameset->impl_->render_this(outputs, framegraph, framegraph_dimension);
			}

			bool move_to_next()
			{
				if(state.this_frameset != framesets.end())
				{
					state.this_frameset->impl_->next_frame();
					return (!state.this_frameset->impl_->eof());
				}
				return false;
			}

			//Seek to the first frameset
			void reset()
			{
				state.this_frameset = framesets.begin();
				if(state.this_frameset != framesets.end())
					state.this_frameset->impl_->reset();
			}
		};//end struct animation::impl

		//class animation::performance_manager
			void animation::performance_manager::insert(impl* p)
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				for(auto thr : threads_)
				{
					std::lock_guard<decltype(thr->mutex)> privlock(thr->mutex);
					if (thr->fps == p->fps)
					{
						if (thr->animations.empty() || (thr->performance_parameter * (1.0 + 1.0 / thr->animations.size()) <= 43.3))
						{
							p->thr_variable = thr;
							thr->animations.push_back(p);
							return;
						}
					}
				}

				auto thr = new thread_variable;
				thr->animations.push_back(p);
				thr->performance_parameter = 0.0;
				thr->fps = p->fps;
				thr->interval = 1000.0 / double(p->fps);
				thr->thread = std::make_shared<std::thread>([this, thr]()
				{
					nana::system::timepiece tmpiece;
					while (true)
					{
						thr->active = 0;
						tmpiece.start();

						{
							std::lock_guard<decltype(thr->mutex)> lock(thr->mutex);
							for (auto ani : thr->animations)
							{
								if (ani->paused)
									continue;

								ani->render_this_frame();
								if (false == ani->move_to_next())
								{
									if (ani->looped)
									{
										ani->reset();
										++thr->active;
									}
								}
								else
									++thr->active;
							}
						}

						if (thr->active)
						{
							thr->performance_parameter = tmpiece.calc();
							if (thr->performance_parameter < thr->interval)
								nana::system::sleep(static_cast<unsigned>(thr->interval - thr->performance_parameter));
						}
						else
						{
							//There isn't an active frame, then let the thread
							//wait for a signal for an active animation
							std::unique_lock<std::mutex> lock(thr->mutex);
							if (0 == thr->active)
								thr->condvar.wait(lock);
						}
					}
				});

				threads_.push_back(thr);
				p->thr_variable = thr;
			}

			void animation::performance_manager::set_fps(impl* p, std::size_t new_fps)
			{
				if (p->fps == new_fps)
					return;

				std::lock_guard<decltype(mutex_)> lock(mutex_);
				auto i = std::find(threads_.begin(), threads_.end(), p->thr_variable);
				if (i == threads_.end())
					return;

				p->fps = new_fps;
				auto thr = *i;

				//Simply modify the fps parameter if the thread just has one animation.
				if (thr->animations.size() == 1)
				{
					thr->fps = new_fps;
					thr->interval = 1000.0 / double(new_fps);
					return;
				}

				std::lock_guard<decltype(thr->mutex)> privlock(thr->mutex);
				auto u = std::find(thr->animations.begin(), thr->animations.end(), p);
				if (u != thr->animations.end())
					thr->animations.erase(u);

				p->thr_variable = nullptr;
				insert(p);
			}

			void animation::performance_manager::close(impl* p)
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				auto i = std::find(threads_.begin(), threads_.end(), p->thr_variable);
				if (i == threads_.end())
					return;

				auto thr = *i;
				std::lock_guard<decltype(thr->mutex)> privlock(thr->mutex);

				auto u = std::find(thr->animations.begin(), thr->animations.end(), p);
				if(u != thr->animations.end())
					thr->animations.erase(u);
			}

			bool animation::performance_manager::empty() const
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				for(auto thr : threads_)
				{
					if(thr->animations.size())
						return false;
				}
				return true;
			}
		//end class animation::performance_manager

		animation::animation(std::size_t fps)
			: impl_(new impl(fps))
		{
		}

		animation::~animation()
		{
			delete impl_;
		}

		void animation::push_back(frameset frms)
		{
			impl_->framesets.emplace_back(std::move(frms));
			if(1 == impl_->framesets.size())
				impl_->state.this_frameset = impl_->framesets.begin();
		}
		/*
		void branch(const std::string& name, const frameset& frms)
		{
			impl_->branches[name].frames = frms;
		}

		void branch(const std::string& name, const frameset& frms, std::function<std::size_t(const std::string&, std::size_t, std::size_t&)> condition)
		{
			auto & br = impl_->branches[name];
			br.frames = frms;
			br.condition = condition;
		}
		*/

		void animation::looped(bool enable)
		{
			if(impl_->looped != enable)
			{
				impl_->looped = enable;
				if(enable)
				{
					std::unique_lock<std::mutex> lock(impl_->thr_variable->mutex);
					if(0 == impl_->thr_variable->active)
					{
						impl_->thr_variable->active = 1;
						impl_->thr_variable->condvar.notify_one();
					}
				}
			}
		}

		void animation::play()
		{
			impl_->paused = false;
			std::unique_lock<std::mutex> lock(impl_->thr_variable->mutex);
			if(0 == impl_->thr_variable->active)
			{
				impl_->thr_variable->active = 1;
				impl_->thr_variable->condvar.notify_one();
			}
		}

		void animation::pause()
		{
			impl_->paused = true;
		}

		void animation::output(window wd, const nana::point& pos)
		{
			auto & output = impl_->outputs[wd];

			if(nullptr == output.diehard)
			{
				drawing dw(wd);
				output.diehard = dw.draw_diehard([this, pos](paint::graphics& tar){
					impl_->render_this_specifically(tar, pos);
				});

				API::events(wd).destroy.connect([this](const arg_destroy& arg){
					std::lock_guard<decltype(impl_->thr_variable->mutex)> lock(impl_->thr_variable->mutex);
					impl_->outputs.erase(arg.window_handle);
				});
			}
			output.points.push_back(pos);
		}

		void animation::fps(std::size_t n)
		{
			if (n == impl_->fps)
				return;

			impl::perf_manager->set_fps(impl_, n);
		}

		std::size_t animation::fps() const
		{
			return impl_->fps;
		}
	//end class animation


	animation::performance_manager * animation::impl::perf_manager;
}	//end namespace nana

