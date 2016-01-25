/*
 *	A Concepts Definition of Nana C++ Library
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/concepts.hpp
 */
#ifndef NANA_CONCEPTS_HPP
#define NANA_CONCEPTS_HPP

#include <nana/any.hpp>

namespace nana
{
	namespace concepts
	{
		/// The Any Objective is an object that may attach to some other object.
		template<typename IndexType, int Dimension>   // no dep. on IndexType ??
		class any_objective
		{
		public:
			virtual ~any_objective() = default;

			template<typename Target>
			void anyobj(const Target& t)
			{
				nana::any * p = _m_anyobj(true);
				if(nullptr == p)
					throw std::runtime_error("Nana.any_objective: Object does not exist");
				*p = t;
			}

			template<typename Target>
			void anyobj(Target&& t)
			{
				nana::any * p = _m_anyobj(true);
				if(nullptr == p)
					throw std::runtime_error("Nana.any_objective: Object does not exist");

				*p = std::move(t);
			}

			template<typename Target>
			Target * anyobj() const     ///< Retrieves the attached object. Returns a nullptr if empty or if the type not match.
			{
				return any_cast<Target>(_m_anyobj(false));
			}
		private:
			virtual nana::any* _m_anyobj(bool allocate_if_empty) const = 0;
		};

		/// The Any Objective is an object that may attach to some other object.
		template<typename IndexType>
		class any_objective<IndexType, 1>
		{
		public:
			typedef IndexType anyobj_index_t;      ///< The type of index.  It is available if Dimension is greater than 0.

			virtual ~any_objective() = default;

			template<typename Target>
			void anyobj(anyobj_index_t i, const Target& t)
			{
				nana::any * p = _m_anyobj(i, true);
				if(nullptr == p)
					throw std::runtime_error("Nana.any_objective: Object does not exist.");
				*p = t;
			}

			template<typename Target>
			void anyobj(anyobj_index_t i, Target&& t)
			{
				nana::any * p = _m_anyobj(i, true);
				if(nullptr == p)
					throw std::runtime_error("Nana.any_objective: Object does not exist");
				*p = std::move(t);
			}

			template<typename Target>
			Target * anyobj(anyobj_index_t i) const    ///< Retrieves the attached object. Returns a nullptr if empty or if the type not match.
			{
				return any_cast<Target>(_m_anyobj(i, false));
			}
		private:
			virtual nana::any* _m_anyobj(anyobj_index_t i, bool allocate_if_empty) const = 0;
		};

		/// The Any Objective is an object that may attach to some other object.
		template<typename IndexType>
		class any_objective<IndexType, 2>
		{
		public:
			typedef IndexType anyobj_index_t;

			virtual ~any_objective(){}

			template<typename Target>
			void anyobj(anyobj_index_t i0, anyobj_index_t i1, const Target& t)
			{
				nana::any * p = _m_anyobj(i0, i1, true);
				if(nullptr == p)
					throw std::runtime_error("Nana.any_objective: Object does not exist");

				*p = t;
			}

			template<typename Target>
			void anyobj(anyobj_index_t i0, anyobj_index_t i1, Target&& t)
			{
				nana::any * p = _m_anyobj(i0, i1, true);
				if(nullptr == p)
					throw std::runtime_error("Nana.any_objective: Object does not exist");
				*p = std::move(t);
			}

			template<typename Target>
			Target * anyobj(anyobj_index_t i0, anyobj_index_t i1) const    ///< Retrieves the attached object. Returns a nullptr if empty or if the type not match.
			{
				return any_cast<Target>(_m_anyobj(i0, i1, false));
			}
		private:
			virtual nana::any* _m_anyobj(anyobj_index_t i0, anyobj_index_t i1, bool allocate_if_empty) const = 0;
		};
	}//end namespace concepts
}//end namespace nana
#endif
