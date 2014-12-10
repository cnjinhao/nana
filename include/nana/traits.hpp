/*
 *	Traits Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	@file: nana/traits.hpp
 */

#ifndef NANA_TRAITS_HPP
#define NANA_TRAITS_HPP
#include <type_traits>

namespace nana
{
	class null_type{};

	//The class noncopyable and nonmovable will be deprecated while the compiler
	//supports the deleted functions
	struct noncopyable
	{
		noncopyable(const noncopyable&) = delete;
		noncopyable& operator=(const noncopyable&) = delete;
		noncopyable();
	};

	struct nonmovable
	{
		nonmovable(nonmovable&&) = delete;
		nonmovable& operator=(nonmovable&&) = delete;
		nonmovable();
	};

	namespace traits
	{
		//traits types for const-volatile specifier
		
		struct no_specifier{};
		struct const_specifier{};
		struct volatile_specifier{};
		struct const_volatile_specifier{};
		
		template<typename T>
		struct cv_specifier
		{
			typedef no_specifier value_type;
		};
		
		template<typename T>
		struct cv_specifier<const T>
		{
			typedef const_specifier value_type;
		};
		
		template<typename T>
		struct cv_specifier<volatile T>
		{
			typedef volatile_specifier value_type;
		};
		
		template<typename T>
		struct cv_specifier<const volatile T>
		{
			typedef const_volatile_specifier value_type;
		};

		template<typename T>
		struct is_function_pointer
			: public std::integral_constant<bool, std::is_pointer<T>::value && std::is_function<typename std::remove_pointer<T>::type>::value>
		{};

		//The traits of pointer to member function
		template<typename MF>
		struct mfptr_traits
		{
			typedef void function();
			typedef void return_type;
			typedef void concept_type;
			enum{parameter = 0};
		};

		template<typename R, typename Concept>
		struct mfptr_traits<R(Concept::*)()>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef return_type function();
			enum{parameter = 0};	
		};

		template<typename R, typename Concept>
		struct mfptr_traits<R(Concept::*)() const>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef return_type function();
			enum{parameter = 0};	
		};

		template<typename R, typename Concept>
		struct mfptr_traits<R(Concept::*)() volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef return_type function();
			enum{parameter = 0};	
		};

		template<typename R, typename Concept>
		struct mfptr_traits<R(Concept::*)() const volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef return_type function();
			enum{parameter = 0};	
		};

		template<typename R, typename Concept, typename P>
		struct mfptr_traits<R(Concept::*)(P)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P param0_type;
			typedef return_type function(param0_type);	
			enum{parameter = 1};	
		};

		template<typename R, typename Concept, typename P>
		struct mfptr_traits<R(Concept::*)(P) const>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P param0_type;
			typedef return_type function(param0_type);	
			enum{parameter = 1};	
		};

		template<typename R, typename Concept, typename P>
		struct mfptr_traits<R(Concept::*)(P) const volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P param0_type;
			typedef return_type function(param0_type);	
			enum{parameter = 1};	
		};

		template<typename R, typename Concept, typename P0, typename P1>
		struct mfptr_traits<R(Concept::*)(P0, P1)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef return_type function(param0_type, param1_type);
			enum{parameter = 2};
		};

		template<typename R, typename Concept, typename P0, typename P1>
		struct mfptr_traits<R(Concept::*)(P0, P1) const>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef return_type function(param0_type, param1_type);
			enum{parameter = 2};
		};

		template<typename R, typename Concept, typename P0, typename P1>
		struct mfptr_traits<R(Concept::*)(P0, P1) const volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef return_type function(param0_type, param1_type);
			enum{parameter = 2};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef return_type function(param0_type, param1_type, param2_type);
			enum{parameter =3};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2) const>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef return_type function(param0_type, param1_type, param2_type);
			enum{parameter =3};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2) volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef return_type function(param0_type, param1_type, param2_type);
			enum{parameter =3};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2) const volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef return_type function(param0_type, param1_type, param2_type);
			enum{parameter =3};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type);
			enum{parameter = 4};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3) const>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type);
			enum{parameter = 4};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3) volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type);
			enum{parameter = 4};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3) const volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type);
			enum{parameter = 4};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3, typename P4>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3, P4)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef P4 param4_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type, param4_type);
			enum{parameter = 5};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3, typename P4>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3, P4) const>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef P4 param4_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type, param4_type);
			enum{parameter = 5};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3, typename P4>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3, P4) volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef P4 param4_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type, param4_type);
			enum{parameter = 5};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3, typename P4>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3, P4) const volatile>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef P4 param4_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type, param4_type);
			enum{parameter = 5};
		};
		
		

		template<typename Function, typename Concept, typename CVSpecifier>
		struct make_mf
		{
			typedef int type;
		};

		template<typename R, typename Concept>
		struct make_mf<R(), Concept, no_specifier>
		{
			typedef R(Concept::*type)();
		};
		
		template<typename R, typename Concept>
		struct make_mf<R(), Concept, const_specifier>
		{
			typedef R(Concept::*type)() const;
		};
		
		template<typename R, typename Concept>
		struct make_mf<R(), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)() volatile;
		};
		
		template<typename R, typename Concept>
		struct make_mf<R(), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)() const volatile;
		};

		template<typename R, typename P0, typename Concept>
		struct make_mf<R(P0), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0);
		};
		
		template<typename R, typename P0, typename Concept>
		struct make_mf<R(P0), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0) const;
		};
		
		template<typename R, typename P0, typename Concept>
		struct make_mf<R(P0), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0) volatile;
		};
		
		template<typename R, typename P0, typename Concept>
		struct make_mf<R(P0), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0) const volatile;
		};

		template<typename R, typename P0, typename P1, typename Concept>
		struct make_mf<R(P0, P1), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1);
		};
		
		template<typename R, typename P0, typename P1, typename Concept>
		struct make_mf<R(P0, P1), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1) const;
		};
		
		template<typename R, typename P0, typename P1, typename Concept>
		struct make_mf<R(P0, P1), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename Concept>
		struct make_mf<R(P0, P1), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1) const volatile;
		};

		template<typename R, typename P0, typename P1, typename P2, typename Concept>
		struct make_mf<R(P0, P1, P2), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2);
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename Concept>
		struct make_mf<R(P0, P1, P2), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2) const;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename Concept>
		struct make_mf<R(P0, P1, P2), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename Concept>
		struct make_mf<R(P0, P1, P2), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2) const volatile;
		};

		template<typename R, typename P0, typename P1, typename P2, typename P3, typename Concept>
		struct make_mf<R(P0, P1, P2, P3), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3);
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename Concept>
		struct make_mf<R(P0, P1, P2, P3), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3) const;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename Concept>
		struct make_mf<R(P0, P1, P2, P3), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename Concept>
		struct make_mf<R(P0, P1, P2, P3), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3) const volatile;
		};

		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4);
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4) const;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4) const volatile;
		};

		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4, P5), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4, P5);
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4, P5), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4, P5) const;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4, P5), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4, P5) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4, P5), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4, P5) const volatile;
		};
	}//end namespace traits

	namespace meta
	{
		template<	typename Param0 = null_type, typename Param1 = null_type,
					typename Param2 = null_type, typename Param3 = null_type,
					typename Param4 = null_type, typename Param5 = null_type,
					typename Param6 = null_type, typename Param7 = null_type,
					typename Param8 = null_type, typename Param9 = null_type>
		struct fixed_type_set
		{
			template<typename T>
			struct count
			{
				enum{value =	std::is_same<Param0, T>::value +
								std::is_same<Param1, T>::value +
								std::is_same<Param2, T>::value +
								std::is_same<Param3, T>::value +
								std::is_same<Param4, T>::value +
								std::is_same<Param5, T>::value +
								std::is_same<Param6, T>::value +
								std::is_same<Param7, T>::value +
								std::is_same<Param8, T>::value +
								std::is_same<Param9, T>::value};
			};
		};
	}//end namespace meta
}//end namespace nana

#endif
