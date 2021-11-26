// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_DETAIL_RESULT_HPP
#define LYRA_DETAIL_RESULT_HPP

#include <memory>
#include <string>

namespace lyra { namespace detail {
class result_base
{
	public:
	result_base const & base() const { return *this; }
	explicit operator bool() const { return is_ok(); }
	bool is_ok() const { return kind_ == result_kind::ok; }
	std::string message() const { return message_; }
	[[deprecated]] std::string errorMessage() const { return message(); }

	protected:
	enum class result_kind
	{
		ok,
		error
	};

	explicit result_base(result_kind kind, const std::string & message = "")
		: kind_(kind)
		, message_(message)
	{}

	explicit result_base(const result_base & other)
		: kind_(other.kind_)
		, message_(other.message_)
	{}

	virtual ~result_base() = default;

	result_base& operator=(const result_base &) = default;

	private:
	result_kind kind_;
	std::string message_;
};

template <typename T>
class result_value_base : public result_base
{
	public:
	using value_type = T;

	value_type const & value() const { return *value_; }
	bool has_value() const { return bool(value_); }

	protected:
	std::unique_ptr<value_type> value_;

	explicit result_value_base(
		result_kind kind, const std::string & message = "")
		: result_base(kind, message)
	{}

	explicit result_value_base(
		result_kind kind,
		const value_type & val,
		const std::string & message = "")
		: result_base(kind, message)
	{
		value_.reset(new value_type(val));
	}

	explicit result_value_base(result_value_base const & other)
		: result_base(other)
	{
		if (other.value_) value_.reset(new value_type(*other.value_));
	}

	explicit result_value_base(const result_base & other)
		: result_base(other)
	{}

	result_value_base & operator=(result_value_base const & other)
	{
		result_base::operator=(other);
		if (other.value_) value_.reset(new T(*other.value_));
		return *this;
	}
};

template <>
class result_value_base<void> : public result_base
{
	public:
	using value_type = void;

	protected:
	// using result_base::result_base;
	explicit result_value_base(const result_base & other)
		: result_base(other)
	{}
	explicit result_value_base(
		result_kind kind, const std::string & message = "")
		: result_base(kind, message)
	{}
};

template <typename T>
class basic_result : public result_value_base<T>
{
	public:
	using value_type = typename result_value_base<T>::value_type;

	explicit basic_result(result_base const & other)
		: result_value_base<T>(other)
	{}

	// With-value results..

	static basic_result ok(value_type const & val)
	{
		return basic_result(result_base::result_kind::ok, val);
	}

	static basic_result
		error(value_type const & val, std::string const & message)
	{
		return basic_result(result_base::result_kind::error, val, message);
	}

	protected:
	using result_value_base<T>::result_value_base;
};

template <>
class basic_result<void> : public result_value_base<void>
{
	public:
	using value_type = typename result_value_base<void>::value_type;

	explicit basic_result(result_base const & other)
		: result_value_base<void>(other)
	{}

	// Value-less results.. (only kind as void is a value-less kind)

	static basic_result ok()
	{
		return basic_result(result_base::result_kind::ok);
	}

	static basic_result error(std::string const & message)
	{
		return basic_result(result_base::result_kind::error, message);
	}

	protected:
	using result_value_base<void>::result_value_base;
};
}} // namespace lyra::detail

#endif
