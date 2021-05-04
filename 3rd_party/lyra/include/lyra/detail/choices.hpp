// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_DETAIL_CHOICES_HPP
#define LYRA_DETAIL_CHOICES_HPP

#include "lyra/detail/from_string.hpp"
#include "lyra/detail/result.hpp"
#include "lyra/detail/unary_lambda_traits.hpp"
#include "lyra/parser_result.hpp"
#include <algorithm>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <vector>

namespace lyra
{
namespace detail
{
	/*
	Type erased base for set of choices. I.e. it's an "interface".
	*/
	struct choices_base
	{
		virtual ~choices_base() = default;
		virtual parser_result contains_value(std::string const& val) const = 0;
	};

	/*
	Stores a set of choice values and provides checking if a given parsed
	string value is one of the choices.
	*/
	template <typename T>
	struct choices_set : choices_base
	{
		// The allowed values.
		std::vector<T> values;

		template <typename... Vals>
		explicit choices_set(Vals... vals)
			: choices_set({ vals... })
		{
		}

		explicit choices_set(const std::vector<T> & vals)
			: values(vals)
		{
		}

		// Checks if the given string val exists in the set of
		// values. Returns a parsing error if the value is not present.
		parser_result contains_value(std::string const& val) const override
		{
			T value;
			auto parse = parse_string(val, value);
			if (!parse)
			{
				return parser_result::runtimeError(
					parser_result_type::no_match, parse.errorMessage());
			}
			bool result = std::count(values.begin(), values.end(), value) > 0;
			if (result)
			{
				return parser_result::ok(parser_result_type::matched);
			}
			// We consider not finding a choice a parse error.
			return parser_result::runtimeError(
				parser_result_type::no_match,
				"Value '" + val + "' not expected. Allowed values are: "
					+ this->to_string());
		}

		// Returns a comma separated list of the allowed values.
		std::string to_string() const
		{
			std::string result;
			for (const T& val : values)
			{
				if (!result.empty()) result += ", ";
				std::string val_string;
				if (detail::to_string(val, val_string))
				{
					result += val_string;
				}
				else
				{
					result += "<value error>";
				}
			}
			return result;
		}

		protected:
		explicit choices_set(std::initializer_list<T> const& vals)
			: values(vals)
		{
		}
	};

	template <>
	struct choices_set<const char*> : choices_set<std::string>
	{
		template <typename... Vals>
		explicit choices_set(Vals... vals)
			: choices_set<std::string>(vals...)
		{
		}
	};

	/*
	Calls a designated function to check if the choice is valid.
	*/
	template <typename Lambda>
	struct choices_check : choices_base
	{
		static_assert(
			unary_lambda_traits<Lambda>::isValid,
			"Supplied lambda must take exactly one argument");
		static_assert(
			std::is_same<
				bool, typename unary_lambda_traits<Lambda>::ReturnType>::value,
			"Supplied lambda must return bool");

		Lambda checker;
		using value_type = typename unary_lambda_traits<Lambda>::ArgType;

		explicit choices_check(Lambda const& checker)
			: checker(checker)
		{
		}

		// Checks if the given string val exists in the set of
		// values. Returns a parsing error if the value is not present.
		parser_result contains_value(std::string const& val) const override
		{
			value_type value;
			auto parse = parse_string(val, value);
			if (!parse)
			{
				return parser_result::runtimeError(
					parser_result_type::no_match, parse.errorMessage());
			}
			if (checker(value))
			{
				return parser_result::ok(parser_result_type::matched);
			}
			// We consider not finding a choice a parse error.
			return parser_result::runtimeError(
				parser_result_type::no_match,
				"Value '" + val + "' not expected.");
		}
	};
} // namespace detail
} // namespace lyra

#endif
