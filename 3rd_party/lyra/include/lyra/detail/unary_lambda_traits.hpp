// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_DETAIL_UNARY_LAMBDA_TRAITS_HPP
#define LYRA_DETAIL_UNARY_LAMBDA_TRAITS_HPP

#include <type_traits>

namespace lyra
{
namespace detail
{
	// Traits for extracting arg and return type of lambdas (for single argument
	// lambdas)
	template <typename L>
	struct unary_lambda_traits : unary_lambda_traits<decltype(&L::operator())>
	{
	};

	template <typename ClassT, typename ReturnT, typename... Args>
	struct unary_lambda_traits<ReturnT (ClassT::*)(Args...) const>
	{
		static const bool isValid = false;
	};

	template <typename ClassT, typename ReturnT, typename ArgT>
	struct unary_lambda_traits<ReturnT (ClassT::*)(ArgT) const>
	{
		static const bool isValid = true;
		using ArgType = typename std::remove_const<
			typename std::remove_reference<ArgT>::type>::type;
		using ReturnType = ReturnT;
	};

} // namespace detail
} // namespace lyra

#endif
