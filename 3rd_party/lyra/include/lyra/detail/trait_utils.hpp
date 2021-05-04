// Copyright 2020 René Ferdinand Rivera Morell
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_DETAIL_TRAIT_UTILS_HPP
#define LYRA_DETAIL_TRAIT_UTILS_HPP

#include <type_traits>
#include <utility>

namespace lyra { namespace detail {
// Checks that F can be called with arguments of type Args.
// Credit for the technique goes to Richard Hodges (in SO post).
template <class F, class... Args>
struct is_callable
{
	template <class U>
	static auto test(U * p)
		-> decltype((*p)(std::declval<Args>()...), void(), std::true_type());

	template <class U>
	static auto test(...) -> decltype(std::false_type());

	static constexpr bool value = decltype(test<F>(0))::value;
};

template <class T>
struct remove_cvref
{
	typedef
		typename std::remove_cv<typename std::remove_reference<T>::type>::type
			type;
};

// Checks that F can be called, with an unspecified set of arguments.
//
// Currently this only detects function objects, like lambdas.
// Where the operator() is not templated.
template <class F>
struct is_invocable
{
	template <class U>
	static auto test(U * p)
		-> decltype((&U::operator()), void(), std::true_type());

	template <class U>
	static auto test(...) -> decltype(std::false_type());

	static constexpr bool value
		= decltype(test<typename remove_cvref<F>::type>(0))::value;
};

// C++11 compatible void_t equivalent.
template <typename... Ts>
struct make_void
{
	typedef void type;
};
template <typename... Ts>
using valid_t = typename make_void<Ts...>::type;

// Borrowed from https://wg21.link/p2098r1
template <class T, template <class...> class Primary>
struct is_specialization_of : std::false_type
{};
template <template <class...> class Primary, class... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type
{};

}} // namespace lyra::detail

#endif
