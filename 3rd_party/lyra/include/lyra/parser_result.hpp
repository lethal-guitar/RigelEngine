// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_PARSER_RESULT_HPP
#define LYRA_PARSER_RESULT_HPP

#include "lyra/detail/result.hpp"

namespace lyra
{

// enum of result types from a parse
enum class parser_result_type
{
	matched,
	no_match,
	short_circuit_all
};

using result = detail::basic_result<void>;

// Result type for parser operation
using parser_result = detail::basic_result<parser_result_type>;

} // namespace lyra

#endif
