/*
 *	A Tree Container class implementation
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/detail/tree_cont.hpp
 */

#ifndef NANA_GUI_WIDGETS_DETAIL_TREE_CONT_HPP
#define NANA_GUI_WIDGETS_DETAIL_TREE_CONT_HPP
#include <stack>
#include <nana/push_ignore_diagnostic>

namespace nana
{
namespace widgets
{
namespace detail
{
		template<typename T>
		struct tree_node
		{
			typedef std::pair<std::string, T>	value_type;

			value_type	value;

			tree_node	*owner;
			tree_node	*next;
			tree_node	*child;

			tree_node(tree_node* owner)
				:owner(owner), next(nullptr), child(nullptr)
			{}

			~tree_node()
			{
				if(owner)
				{
					tree_node * t = owner->child;
					if(t != this)
					{
						while(t->next != this)
							t = t->next;
						t->next = next;
					}
					else
						owner->child = next;
				}

				tree_node * t = child;
				while(t)
				{
					tree_node * t_next = t->next;
					delete t;
					t = t_next;
				}
			}

			bool is_ancestor_of(const tree_node* child) const
			{
				while (child)
				{
					if (child == this)
						return true;

					child = child->owner;
				}
				return false;
			}

			tree_node * front() const
			{
				if (this->owner && (this != this->owner->child))
				{
					auto i = this->owner->child;
					while (i->next != this)
						i = i->next;

					return i;
				}
				return nullptr;
			}
		};

		template<typename UserData>
		class tree_cont
		{
			typedef tree_cont self_type;

		public:
			typedef UserData	element_type;
			typedef tree_node<element_type> node_type;
			typedef typename node_type::value_type	value_type;

			tree_cont()
				:root_(nullptr)
			{}

			~tree_cont()
			{
				clear(&root_);
			}

			void clear(node_type* node)
			{
				while (node->child)
				{
					//If there is a sibling of child, the root_.child
					//will be assigned with the sibling.
					remove(node->child);
				}
			}

			bool verify(const node_type* node) const
			{
				if(node)
				{
					while(node->owner)
					{
						if(node->owner == &root_)
							return true;

						node = node->owner;
					}
				}
				return false;
			}

			node_type* get_root() const
			{
				return &root_;
			}

			node_type* get_owner(const node_type* node) const
			{
				return (verify(node) && (node->owner != &root_) ? node->owner : nullptr);
			}

			node_type * node(node_type* node, const std::string& key)
			{
				if(node)
				{
					for(node_type * child = node->child; child; child = child->next)
					{
						if(child->value.first == key)
							return child;
					}
				}
				return nullptr;
			}

			node_type* insert(node_type* node, const std::string& key, const element_type& elem)
			{
				if(nullptr == node)
					return insert(key, elem);
				
				if(verify(node))
				{
					node_type **new_node_ptr;
					if(node->child)
					{
						node_type* child = node->child;
						for(; child; child = child->next)
						{
							if(child->value.first == key)
							{
								child->value.second = elem;
								return child;
							}
						}

						child = node->child;
						while(child->next)
							child = child->next;

						new_node_ptr = &(child->next);
					}
					else
						new_node_ptr = &(node->child);

					*new_node_ptr = new node_type(node);

					(*new_node_ptr)->value.first = key;
					(*new_node_ptr)->value.second = elem;
					return (*new_node_ptr);
				}
				return nullptr;
			}

			node_type* insert(const std::string& key, const element_type& elem)
			{
				auto node = _m_locate<true>(key);

				//Doesn't return the root node
				if (node == &root_)
					return nullptr;

				if(node)
					node->value.second = elem;
				return node;
			}

			void remove(node_type* node)
			{
				if(verify(node))
					delete node;
			}

			node_type* find(const std::string& path) const
			{
				auto p = _m_locate(path);
				return (&root_ == p ? nullptr : p);
			}

			node_type* ref(const std::string& path)
			{
				auto p = _m_locate<true>(path);
				return (&root_ == p ? nullptr : p);
			}

			unsigned indent_size(const node_type* node) const
			{
				if(node)
				{
					unsigned indent = 0;
					for(;(node = node->owner); ++indent)
					{
						if(node == &root_)	return indent;
					}
				}
				return 0;
			}

			template<typename Functor>
			void for_each(node_type* node, Functor f)
			{
				if(nullptr == node) node = root_.child;
				int state = 0;	//0: Sibling, the last is a sibling of node
								//1: Owner, the last is the owner of node
								//>= 2: Children, the last is is a child of the node that before this node.
				while(node)
				{
					switch(f(*node, state))
					{
					case 0: return;
					case 1:
						{
							if(node->child)
							{
								node = node->child;
								state = 1;
							}
							else
								return;
							continue;
						}
						break;
					}

					if(node->next)
					{
						node = node->next;
						state = 0;
					}
					else
					{
						state = 1;
						if(node == &root_)	return;

						while(true)
						{
							++state;
							if(node->owner->next)
							{
								node = node->owner->next;
								break;
							}
							else
								node = node->owner;

							if(node == &root_)	return;
						}
					}
				}
			}

			template<typename Functor>
			void for_each(node_type* node, Functor f) const
			{
				if(nullptr == node) node = root_.child;
				int state = 0;	//0: Sibling, the last is a sibling of node
								//1: Owner, the last is the owner of node
								//>= 2: Children, the last is is a child of the node that before this node.
				while(node)
				{
					switch(f(*node, state))
					{
					case 0: return;
					case 1:
						{
							if(node->child)
							{
								node = node->child;
								state = 1;
							}
							else
								return;
							continue;
						}
						break;
					}

					if(node->next)
					{
						node = node->next;
						state = 0;
					}
					else
					{
						state = 1;
						if(node == &root_)	return;

						while(true)
						{
							++state;
							if(node->owner->next)
							{
								node = node->owner->next;
								break;
							}
							else
								node = node->owner;

							if(node == &root_)	return;
						}
					}
				}
			}

			template<typename PredAllowChild>
			unsigned child_size_if(const ::std::string& key, PredAllowChild pac) const
			{
				auto node = _m_locate(key);
				return (node ? child_size_if<PredAllowChild>(*node, pac) : 0);
			}

			template<typename PredAllowChild>
			unsigned child_size_if(const node_type& node, PredAllowChild pac) const
			{
				unsigned size = 0;
				const node_type* pnode = node.child;
				while(pnode)
				{
					++size;
					if(pnode->child && pac(*pnode))
						size += child_size_if<PredAllowChild>(*pnode, pac);

					pnode = pnode->next;
				}
				return size;
			}

			template<typename PredAllowChild>
			std::size_t distance_if(const node_type * node, PredAllowChild pac) const
			{
				if(nullptr == node)	return 0;
				const node_type * iterator = root_.child;

				std::size_t off = 0;
				std::stack<const node_type* > stack;

				while(iterator && iterator != node)
				{
					++off;

					if(iterator->child && pac(*iterator))
					{
						stack.push(iterator);
						iterator = iterator->child;
					}
					else
						iterator = iterator->next;

					while((nullptr == iterator) && stack.size())
					{
						iterator = stack.top()->next;
						stack.pop();
					}
				}
				return off;
			}

			template<typename PredAllowChild>
			node_type* advance_if(node_type* node, std::size_t off, PredAllowChild pac)
			{
				if(nullptr == node)	node = root_.child;

				std::stack<node_type* > stack;

				while(node && off)
				{
					--off;
					if(node->child && pac(*node))
					{
						stack.push(node);
						node = node->child;
					}
					else
						node = node->next;

					while(nullptr == node && stack.size())
					{
						node = stack.top();
						stack.pop();
						node = node->next;
					}
				}

				return node;
			}
		private:
			//Functor defintions

			struct each_make_node
			{
				each_make_node(self_type& self)
					:node(&(self.root_))
				{}

				bool operator()(const ::std::string& key_node)
				{
					node_type *child = node->child;
					node_type *tail = nullptr;
					while(child)
					{
						if(key_node == child->value.first)
						{
							node = child;
							return true;
						}
						tail = child;
						child = child->next;
					}

					child = new node_type(node);
					if(tail)
						tail->next = child;
					else
						node->child = child;

					child->value.first = key_node;
					node = child;
					return true;
				}

				node_type * node;
			};


			struct find_key_node
			{
				find_key_node(const self_type& self)
					:node(&self.root_)
				{}

				bool operator()(const ::std::string& key_node)
				{
					return ((node = _m_find(node->child, key_node)) != nullptr);
				}

				node_type *node;
			};
		private:
			static node_type* _m_find(node_type* node, const ::std::string& key_node)
			{
				while(node)
				{
					if(key_node == node->value.first)
						return node;

					node = node->next;
				}
				return nullptr;
			}

			template<typename Function>
			void _m_for_each(const ::std::string& key, Function function) const
			{
				//Ignores separaters at the begin of key.
				::std::string::size_type beg = key.find_first_not_of("\\/");
				if (key.npos == beg)
					return;

				auto end = key.find_first_of("\\/", beg);


				while(end != ::std::string::npos)
				{
					if(beg != end)
					{
						if(!function(key.substr(beg, end - beg)))
							return;
					}

					auto next = key.find_first_not_of("\\/", end);

					if ((next == ::std::string::npos) && end)
						return;

					if (0 == end)
					{
						if ((!function(key.substr(0, 1))) || (next == ::std::string::npos))
							return;
					}

					beg = next;
					end = key.find_first_of("\\/", beg);

				}

				function(key.substr(beg, key.size() - beg));
			}

			template<bool CreateIfNotExists>
			node_type* _m_locate(const ::std::string& key)
			{
				if(key.size())
				{
					if(CreateIfNotExists)
					{
						each_make_node emn(*this);
						_m_for_each<each_make_node&>(key, emn);
						return emn.node;
					}
					else
					{
						find_key_node fkn(*this);
						_m_for_each<find_key_node&>(key, fkn);
						return const_cast<node_type*>(fkn.node);
					}
				}
				return &root_;
			}

			node_type* _m_locate(const std::string& key) const
			{
				if(key.size())
				{
					find_key_node fkn(*this);
					_m_for_each<find_key_node&>(key, fkn);
					return fkn.node;
				}
				return &root_;
			}
		private:
			mutable node_type root_;
		};//end class tree_cont
}//end namespace detail
}//end namespace widgets
}//end namesace nana

#include <nana/pop_ignore_diagnostic>
#endif
