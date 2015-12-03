/*
 *	Handle Manager Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/handle_manager.hpp
 *
 *	@description:
 *	this manages all the window handles
 */

#ifndef NANA_GUI_DETAIL_HANDLE_MANAGER_HPP
#define NANA_GUI_DETAIL_HANDLE_MANAGER_HPP

#include <nana/traits.hpp>
#include <nana/config.hpp>
#if defined(STD_THREAD_NOT_SUPPORTED)
    #include <nana/std_mutex.hpp>
#else
    #include <mutex>
#endif

#include <map>
#include <iterator>

namespace nana
{
	namespace detail
	{
		template<typename Key, typename Value, std::size_t CacheSize>
		class cache
			: noncopyable
		{
		public:
			typedef Key	key_type;
			typedef Value value_type;
			typedef std::pair<key_type, value_type> pair_type;
			typedef std::size_t size_type;

			cache()
				:addr_(reinterpret_cast<pair_type*>(::operator new(sizeof(pair_type) * CacheSize)))
			{
				for(std::size_t i = 0; i < CacheSize; ++i)
				{
					bitmap_[i] = 0;
					seq_[i] = nana::npos;
				}
			}

			~cache()
			{
				for(std::size_t i = 0; i < CacheSize; ++i)
				{
					if(bitmap_[i])
						addr_[i].~pair_type();
				}

				::operator delete(addr_);
			}

			bool insert(key_type k, value_type v)
			{
				size_type pos = _m_find_key(k);
				if(pos != nana::npos)
				{
					addr_[pos].second = v;
				}
				else
				{
					//No key exists
					pos = _m_find_pos();

					if(pos == nana::npos)
					{	//No room, and remove the last pair
						pos = seq_[CacheSize - 1];
						(addr_ + pos)->~pair_type();
					}

					if(seq_[0] != nana::npos)
					{//Need to move
						for(int i = CacheSize - 1; i > 0; --i)
							seq_[i] = seq_[i - 1];
					}

					seq_[0] = pos;

					new (addr_ + pos) pair_type(k, v);
					bitmap_[pos] = 1;
				}
				return v;
			}

			value_type * get(key_type k)
			{
				size_type pos = _m_find_key(k);
				if(pos != nana::npos)
					return &(addr_[pos].second);
				return 0;
			}
		private:
			size_type _m_find_key(key_type k) const
			{
				for(std::size_t i = 0; i < CacheSize; ++i)
				{
					if(bitmap_[i] && (addr_[i].first == k))
						return i;
				}
				return nana::npos;
			}

			size_type _m_find_pos() const
			{
				for(std::size_t i = 0; i < CacheSize; ++i)
				{
					if(bitmap_[i] == 0)
						return i;
				}
				return nana::npos;
			}
		private:
			char bitmap_[CacheSize];
			size_type seq_[CacheSize];
			pair_type * addr_;
		};

		//handle_manager
		//@brief
		//		handle_manager maintains handles of a type. removing a handle dose not destroy it,
		//	it will be inserted to a trash queue for deleting at a safe time.
		//		For efficiency, this class is not a thread-safe.
		template<typename HandleType, typename Condition, typename Deleter>
		class handle_manager
			: nana::noncopyable
		{
		public:
			typedef HandleType	handle_type;
			typedef Condition	cond_type;
			typedef Deleter		deleter_type;
			typedef std::map<handle_type, unsigned>	handle_map_t;
			typedef std::pair<handle_type, unsigned>	holder_pair;

			~handle_manager()
			{
				delete_trash(0);
			}

			void insert(handle_type handle, unsigned tid)
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);

				holder_[handle] = tid;

				is_queue<std::is_same<cond_type, nana::null_type>::value, std::vector<handle_type> >::insert(handle, queue_);
				cacher_.insert(handle, true);
			}

			void operator()(const handle_type handle)
			{
				remove(handle);
			}

			void remove(const handle_type handle)
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);

				auto i = static_cast<const handle_map_t&>(holder_).find(handle);
				if(holder_.cend() != i)
				{
					is_queue<std::is_same<cond_type, nana::null_type>::value, std::vector<handle_type> >::erase(handle, queue_);
					cacher_.insert(handle, false);
					trash_.emplace_back(i->first, i->second);
					holder_.erase(i);
				}
			}

			void delete_trash(unsigned tid)
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);

				if(trash_.size())
				{
					deleter_type del_functor;
					if(tid == 0)
					{
						for(auto & m : trash_)
							del_functor(m.first);
						trash_.clear();
					}
					else
					{
						for(auto i = trash_.begin(), end = trash_.end(); i != end;)
						{
							if(tid == i->second)
							{
								del_functor(i->first);
								i = trash_.erase(i);
								end = trash_.end();
							}
							else
								++i;
						}
					}
				}
			}

			handle_type last() const
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				if(queue_.size())
					return queue_.back();
				return handle_type();
			}

			std::size_t size() const
			{
				return holder_.size();
			}

			handle_type get(unsigned index) const
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				if(index < queue_.size())
					return queue_[index];
				return handle_type();
			}

			bool available(const handle_type handle)	const
			{
				if (nullptr == handle)
					return false;

				std::lock_guard<decltype(mutex_)> lock(mutex_);
				bool * v = cacher_.get(handle);
				if(v) return *v;
				return cacher_.insert(handle, (holder_.count(handle) != 0));
			}

			void all(std::vector<handle_type> & v) const
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				std::copy(queue_.cbegin(), queue_.cend(), std::back_inserter(v));
			}
		private:

			template<bool IsQueueOperation, typename Container>
			struct is_queue
			{
			public:
				static void insert(handle_type handle, Container& queue)
				{
					if(cond_type::is_queue(handle))
						queue.push_back(handle);
				}

				static void erase(handle_type handle, Container& queue)
				{
					if(cond_type::is_queue(handle))
					{
						for (auto i = queue.begin(); i != queue.end(); ++i)
						{
							if (handle == *i)
							{
								queue.erase(i);
								break;
							}
						}
					}
				}
			};

			template<typename Container>
			struct is_queue<true, Container>
			{
			public:
				static void insert(handle_type handle, Container& queue){}
				static void erase(handle_type handle, Container& queue){}
			};

		private:
			mutable std::recursive_mutex mutex_;
			mutable cache<const handle_type, bool, 5> cacher_;
			handle_map_t	holder_;
			std::vector<handle_type>	queue_;
			std::vector<holder_pair>	trash_;
		};//end class handle_manager
	}//end namespace detail
}// end namespace nana
#endif
