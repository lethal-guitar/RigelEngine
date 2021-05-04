// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_DETAIL_RESULT_HPP
#define LYRA_DETAIL_RESULT_HPP

#include <memory>
#include <string>

namespace lyra
{
namespace detail
{
	class result_base
	{
		public:
		enum Type
		{
			Ok,
			LogicError,
			RuntimeError
		};

		result_base const& base() const { return *this; }

		explicit operator bool() const
		{
			return this->m_type == result_base::Ok;
		}

		Type type() const { return m_type; }

		std::string errorMessage() const { return m_errorMessage; }

		protected:
		Type m_type;
		std::string m_errorMessage; // Only populated if resultType is an error

		explicit result_base(Type type, const std::string& message = "")
			: m_type(type)
			, m_errorMessage(message)
		{
		}

		virtual ~result_base() = default;
	};

	template <typename T>
	class result_value_base : public result_base
	{
		public:
		using value_type = T;

		value_type const& value() const { return *m_value; }
		bool has_value() const { return m_value; }

		protected:
		std::unique_ptr<value_type> m_value;

		explicit result_value_base(Type type, const std::string& message = "")
			: result_base(type, message)
		{
		}

		explicit result_value_base(
			Type type, const value_type& val, const std::string& message = "")
			: result_base(type, message)
		{
			m_value.reset(new value_type(val));
		}

		explicit result_value_base(result_value_base const& other)
			: result_base(other)
		{
			if (other.m_value) m_value.reset(new value_type(*other.m_value));
		}

		result_value_base& operator=(result_value_base const& other)
		{
			result_base::operator=(other);
			if (other.m_value) m_value.reset(new T(*other.m_value));
			return *this;
		}
	};

	template <>
	class result_value_base<void> : public result_base
	{
		public:
		using value_type = void;

		protected:
		using result_base::result_base;
	};

	template <typename T>
	class basic_result : public result_value_base<T>
	{
		public:
		using value_type = typename result_value_base<T>::value_type;

		explicit basic_result(result_base const& other)
			: result_value_base<T>(other.type(), other.errorMessage())
		{
		}

		// With-value results..

		static basic_result ok(value_type const& val)
		{
			return basic_result(result_base::Ok, val);
		}

		static basic_result
		logicError(value_type const& val, std::string const& message)
		{
			return basic_result(result_base::LogicError, val, message);
		}

		static basic_result
		runtimeError(value_type const& val, const std::string& message)
		{
			return basic_result(result_base::RuntimeError, val, message);
		}

		protected:
		using result_value_base<T>::result_value_base;
	};

	template <>
	class basic_result<void> : public result_value_base<void>
	{
		public:
		using value_type = typename result_value_base<void>::value_type;

		explicit basic_result(result_base const& other)
			: result_value_base<void>(other.type(), other.errorMessage())
		{
		}

		// Value-less results.. (only kind as void is a value-less type)

		static basic_result ok() { return basic_result(result_base::Ok); }

		static basic_result logicError(std::string const& message)
		{
			return basic_result(result_base::LogicError, message);
		}

		static basic_result runtimeError(std::string const& message)
		{
			return basic_result(result_base::RuntimeError, message);
		}

		protected:
		using result_value_base<void>::result_value_base;
	};
} // namespace detail
} // namespace lyra

#endif
