// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_DETAIL_FROM_STRING_HPP
#define LYRA_DETAIL_FROM_STRING_HPP

#include "lyra/detail/trait_utils.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <type_traits>

#ifndef LYRA_CONFIG_OPTIONAL_TYPE
#	ifdef __has_include
#		if __has_include(<optional>) && __cplusplus >= 201703L
#			include <optional>
#			define LYRA_CONFIG_OPTIONAL_TYPE std::optional
#		endif
#	endif
#endif

namespace lyra { namespace detail {
template <typename T>
bool to_string(const T & source, std::string & target)
{
	std::stringstream ss;
	ss << source;
	ss >> target;
	return !ss.fail();
}

inline bool to_string(const std::string & source, std::string & target)
{
	target = source;
	return true;
}

inline bool to_string(const char * source, std::string & target)
{
	target = source;
	return true;
}

inline bool to_string(bool source, std::string & target)
{
	target = source ? "true" : "false";
	return true;
}

#ifdef LYRA_CONFIG_OPTIONAL_TYPE
template <typename T>
inline bool
	to_string(LYRA_CONFIG_OPTIONAL_TYPE<T> & source, std::string & target)
{
	if (source)
		return to_string(*source, target);
	else
		target = "<nullopt>";
	return true;
}
#endif // LYRA_CONFIG_OPTIONAL_TYPE

template <typename, typename = void>
struct is_convertible_from_string : std::false_type
{};

template <typename T>
struct is_convertible_from_string<
	T,
	typename std::enable_if<std::is_arithmetic<T>::value>::type>
	: std::true_type
{};

template <typename S, typename T>
inline bool from_string(S const & source, T & target)
{
	std::stringstream ss;
	ss << source;
	T temp {};
	ss >> temp;
	if (!ss.fail() && ss.eof())
	{
		target = temp;
		return true;
	}
	return false;
}

template <typename S, typename... C>
inline bool from_string(S const & source, std::basic_string<C...> & target)
{
	to_string(source, target);
	return true;
}

template <typename T>
struct is_convertible_from_string<
	T,
	typename std::enable_if<std::is_same<T, bool>::value>::type>
	: std::true_type
{};

template <typename S>
inline bool from_string(S const & source, bool & target)
{
	std::string srcLC;
	to_string(source, srcLC);
	std::transform(srcLC.begin(), srcLC.end(), srcLC.begin(), [](char c) {
		return static_cast<char>(::tolower(c));
	});
	if (srcLC == "y" || srcLC == "1" || srcLC == "true" || srcLC == "yes"
		|| srcLC == "on")
		target = true;
	else if (
		srcLC == "n" || srcLC == "0" || srcLC == "false" || srcLC == "no"
		|| srcLC == "off")
		target = false;
	else
		return false;
	return true;
}

#ifdef LYRA_CONFIG_OPTIONAL_TYPE
template <typename T>
struct is_convertible_from_string<
	T,
	typename std::enable_if<
		is_specialization_of<T, LYRA_CONFIG_OPTIONAL_TYPE>::value>::type>
	: std::true_type
{};

template <typename S, typename T>
inline bool from_string(S const & source, LYRA_CONFIG_OPTIONAL_TYPE<T> & target)
{
	std::string srcLC;
	to_string(source, srcLC);
	std::transform(srcLC.begin(), srcLC.end(), srcLC.begin(), [](char c) {
		return static_cast<char>(::tolower(c));
	});
	if (srcLC == "<nullopt>")
	{
		target.reset();
		return true;
	}
	else
	{
		T temp;
		auto result = from_string(source, temp);
		if (result) target = std::move(temp);
		return result;
	}
}
#endif // LYRA_CONFIG_OPTIONAL_TYPE

}} // namespace lyra::detail

#endif
