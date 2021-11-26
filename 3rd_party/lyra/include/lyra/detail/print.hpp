// Copyright 2021 René Ferdinand Rivera Morell
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_DETAIL_PRINT_HPP
#define LYRA_DETAIL_PRINT_HPP

#include <iostream>
#include <string>

#ifndef LYRA_DEBUG
#	define LYRA_DEBUG 0
#endif

namespace lyra { namespace detail {

constexpr bool is_debug = LYRA_DEBUG;

template <typename T>
std::string to_string(T && t)
{
	return std::string(std::move(t));
}

using std::to_string;

struct print
{
	print(const char * scope = nullptr) : scope(scope)
	{
		if (is_debug) print::depth() += 1;
		if (scope) debug(scope, "...");
	}

	~print()
	{
		if (scope) debug("...", scope);
		if (is_debug) print::depth() -= 1;
	}

	template <typename... A>
	void debug(A... arg)
	{
		if (is_debug)
		{
			std::cerr << "[DEBUG]"
					  << std::string((print::depth() - 1) * 2, ' ');
			std::string args[] = { to_string(arg)... };
			for (auto & arg_string : args)
			{
				std::cerr << " " << arg_string;
			}
			std::cerr << "\n";
		}
	}

	private:
	const char * scope;
	static std::size_t & depth()
	{
		static std::size_t d = 0;
		return d;
	}
};

}} // namespace lyra::detail

#if LYRA_DEBUG
#	define LYRA_PRINT_SCOPE ::lyra::detail::print lyra_print_scope
#	define LYRA_PRINT_DEBUG lyra_print_scope.debug
#else
#	define LYRA_PRINT_SCOPE(...) while(false)
#	define LYRA_PRINT_DEBUG(...) while(false)
#endif

#endif
