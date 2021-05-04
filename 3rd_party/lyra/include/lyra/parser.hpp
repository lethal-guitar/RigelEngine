// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_PARSER_HPP
#define LYRA_PARSER_HPP

#include "lyra/args.hpp"
#include "lyra/detail/bound.hpp"
#include "lyra/detail/choices.hpp"
#include "lyra/detail/from_string.hpp"
#include "lyra/detail/result.hpp"
#include "lyra/detail/tokens.hpp"
#include "lyra/detail/trait_utils.hpp"
#include "lyra/val.hpp"
#include "lyra/parser_result.hpp"

#include <memory>
#include <string>
#include <type_traits>

namespace lyra {

/* tag::reference[]

[#lyra_parser_customization]
= `lyra::parser_customization`

Customization interface for parsing of options.

[source]
----
virtual std::string token_delimiters() const = 0;
----

Specifies the characters to use for splitting a cli argument into the option
and its value (if any).

[source]
----
virtual std::string option_prefix() const = 0;
----

Specifies the characters to use as possible prefix, either single or double,
for all options.

end::reference[] */
struct parser_customization
{
	virtual std::string token_delimiters() const = 0;
	virtual std::string option_prefix() const = 0;
};

/* tag::reference[]

[#lyra_default_parser_customization]
= `lyra::default_parser_customization`

Is-a `lyra::parser_customization` that defines token delimiters as space (" ")
or equal (`=`). And specifies the option prefix character as dash (`-`)
resulting in long options with `--` and short options with `-`.

This customization is used as the default if none is given.

end::reference[] */
struct default_parser_customization : parser_customization
{
	std::string token_delimiters() const override { return " ="; }
	std::string option_prefix() const override { return "-"; }
};

namespace detail {
class parse_state
{
	public:
	parse_state(parser_result_type type,
		token_iterator const & remaining_tokens, size_t parsed_tokens = 0)
		: result_type(type)
		, tokens(remaining_tokens)
	{}

	parser_result_type type() const { return result_type; }
	token_iterator remainingTokens() const { return tokens; }
	bool have_tokens() const { return bool(tokens); }

	private:
	parser_result_type result_type;
	token_iterator tokens;
};

struct parser_cardinality
{
	size_t minimum = 0;
	size_t maximum = 0;

	parser_cardinality() = default;

	parser_cardinality(size_t a, size_t b)
		: minimum(a)
		, maximum(b)
	{}

	// If zero or more are accepted, it's optional.
	bool is_optional() const { return (minimum == 0); }

	// Anything that doesn't have an upper bound is considered unbounded.
	bool is_unbounded() const { return (maximum == 0); }

	bool is_bounded() const { return !is_unbounded(); }

	// If one or more values are expected, it's required.
	bool is_required() const { return (minimum > 0); }
};
} // namespace detail

/* tag::reference[]

[#lyra_parser_result]
= `lyra::parser_result`

The result of parsing arguments.

end::reference[] */
class parse_result : public detail::basic_result<detail::parse_state>
{
	public:
	using base = detail::basic_result<detail::parse_state>;
	using base::basic_result;
	using base::logicError;
	using base::ok;
	using base::runtimeError;

	parse_result(const base & other)
		: base(other)
	{}
};

/* tag::reference[]

[#lyra_parser]
= `lyra::parser`

Base for all argument parser types.

end::reference[] */
class parser
{
	public:
	struct help_text_item
	{
		std::string option;
		std::string description;
	};

	using help_text = std::vector<help_text_item>;

	virtual help_text get_help_text() const { return {}; }
	virtual std::string get_usage_text() const { return ""; }
	virtual std::string get_description_text() const { return ""; }

	virtual ~parser() = default;

	virtual parse_result parse(std::string const & exe_name,
		detail::token_iterator const & tokens,
		parser_customization const & customize) const
	{
		return this->parse(tokens, customize);
	}

	virtual parse_result parse(detail::token_iterator const & tokens,
		parser_customization const & customize) const = 0;

	virtual detail::parser_cardinality cardinality() const { return { 0, 1 }; }
	bool is_optional() const { return cardinality().is_optional(); }
	virtual bool is_group() const { return false; }
	virtual result validate() const { return result::ok(); }
	virtual std::unique_ptr<parser> clone() const { return nullptr; }
	virtual bool is_named(const std::string & n) const { return false; }
	virtual const parser * get_named(const std::string & n) const
	{
		if (is_named(n)) return this;
		return nullptr;
	}
	virtual size_t get_value_count() const { return 0; }
	virtual std::string get_value(size_t i) const { return ""; }

	protected:
	void print_help_text(std::ostream & os) const
	{
		std::string usage_test = get_usage_text();
		if (!usage_test.empty())
			os << "USAGE:\n"
			<< "  " << get_usage_text() << "\n\n";

		std::string description_test = get_description_text();
		if (!description_test.empty())
			os << get_description_text() << "\n";

		os << "OPTIONS, ARGUMENTS:\n";
		const std::string::size_type left_col_size = 26 - 3;
		const std::string left_pad(left_col_size, ' ');
		for (auto const & cols : get_help_text())
		{
			if (cols.option.size() > left_pad.size())
				os << "  " << cols.option << "\n  " << left_pad << " "
				   << cols.description << "\n";
			else
				os << "  " << cols.option
				   << left_pad.substr(0, left_pad.size() - cols.option.size())
				   << " " << cols.description << "\n";
		}
	}
};

/* tag::reference[]

[#lyra_parser_specification]
== Specification

[#lyra_parser_help_text_item]
=== `lyra::parser::help_text_item`

[source]
----
struct lyra::parser::help_text_item
{
	std::string option;
	std::string description;
};
----

Holds the help information for a single argument option. The `option` member is
the long name of the option. And the `description` is the text describing the
option. A list of them is returned from the `lyra::parser::get_help_text`
method.

[#lyra_parser_help_text]
=== `lyra::parser::help_text`

[source]
----
using help_text = std::vector<help_text_item>;
----

The set of help texts for any options in the sub-parsers to this one, if any.

[#lyra_parser_get_help_text]
=== `lyra::parser::get_help_text`

[source]
----
virtual help_text get_help_text() const;
----

Collects, and returns, the set of help items for the sub-parser arguments in
this parser, if any. The default is to return an empty set. Which is what most
parsers will return. Parsers like `arguments`, `group`, and `cli` will return a
set for the arguments defined. This is called to print out the help text from
the stream operator.

[#lyra_parser_get_usage_text]
=== `lyra::parser::get_usage_text`

[source]
----
virtual std::string get_usage_text() const;
----

Returns the formatted `USAGE` text for this parser, and any contained
sub-parsers. This is called to print out the help text from the stream operator.

[#lyra_parser_get_description_text]
=== `lyra::parser::get_description_text`

[source]
----
virtual std::string get_description_text() const;
----

Returns the description text for this, and any contained sub-parsers. This is
called to print out the help text from the stream operator.

end::reference[] */

template <typename T, typename U>
std::unique_ptr<parser> make_clone(const U * source)
{
	return std::unique_ptr<parser>(new T(*static_cast<const T *>(source)));
}

/* tag::reference[]

[#lyra_composable_parser]
= `lyra::composable_parser`

A parser that can be composed with other parsers using `operator|`. Composing
two `composable_parser` instances generates a `cli` parser.

end::reference[] */
template <typename DerivedT>
class composable_parser : public parser
{};

// Common code and state for args and Opts
/* tag::reference[]

[#lyra_bound_parser]
= `lyra::bound_parser`

Parser that binds a variable reference or callback to the value of an argument.

end::reference[] */
template <typename Derived>
class bound_parser : public composable_parser<Derived>
{
	protected:
	std::shared_ptr<detail::BoundRef> m_ref;
	std::string m_hint;
	std::string m_description;
	detail::parser_cardinality m_cardinality;
	std::shared_ptr<detail::choices_base> value_choices;

	explicit bound_parser(std::shared_ptr<detail::BoundRef> const & ref)
		: m_ref(ref)
	{
		if (m_ref->isContainer()) m_cardinality = { 0, 0 };
		else
			m_cardinality = { 0, 1 };
	}
	bound_parser(std::shared_ptr<detail::BoundRef> const & ref, std::string const & hint)
		: m_ref(ref), m_hint(hint)
	{
		if (m_ref->isContainer()) m_cardinality = { 0, 0 };
		else
			m_cardinality = { 0, 1 };
	}

	public:

	enum class ctor_lambda_e { val };

	template <typename Reference>
	bound_parser(Reference & ref, std::string const & hint);

	template <typename Lambda>
	bound_parser(Lambda const & ref, std::string const & hint,
	typename std::enable_if<detail::is_invocable<Lambda>::value, ctor_lambda_e>::type = ctor_lambda_e::val);

	template <typename T>
	explicit bound_parser(detail::BoundVal<T> && val)
		: bound_parser(val.move_to_shared())
	{}
	template <typename T>
	bound_parser(detail::BoundVal<T> && val, std::string const & hint)
		: bound_parser(val.move_to_shared(), hint)
	{}

	Derived & help(const std::string & text);
	Derived & operator()(std::string const & description);
	Derived & optional();
	Derived & required(size_t n = 1);
	Derived & cardinality(size_t n);
	Derived & cardinality(size_t n, size_t m);
	detail::parser_cardinality cardinality() const override
	{
		return m_cardinality;
	}
	std::string hint() const { return m_hint; }

	template <typename T, typename... Rest,
		typename std::enable_if<!detail::is_invocable<T>::value, int>::type = 0>
	Derived & choices(T val0, Rest... rest);
	template <typename Lambda,
		typename std::enable_if<detail::is_invocable<Lambda>::value, int>::type
		= 1>
	Derived & choices(Lambda const & check_choice);
	template <typename T, std::size_t N>
	Derived & choices(const T (&choice_values)[N]);

	virtual std::unique_ptr<parser> clone() const override
	{
		return make_clone<Derived>(this);
	}

	virtual bool is_named(const std::string &n) const override
	{
		return n == m_hint;
	}
	virtual size_t get_value_count() const override { return m_ref->get_value_count(); }
	virtual std::string get_value(size_t i) const override { return m_ref->get_value(i); }
};

/* tag::reference[]

[#lyra_bound_parser_ctor]
== Construction

end::reference[] */

/* tag::reference[]
[source]
----
template <typename Derived>
template <typename Reference>
bound_parser<Derived>::bound_parser(Reference& ref, std::string const& hint);

template <typename Derived>
template <typename Lambda>
bound_parser<Derived>::bound_parser(Lambda const& ref, std::string const& hint);
----

Constructs a value option with a target typed variable or callback. These are
options that take a value as in `--opt=value`. In the first form the given
`ref` receives the value of the option after parsing. The second form the
callback is called during the parse with the given value. Both take a
`hint` that is used in the help text. When the option can be specified
multiple times the callback will be called consecutively for each option value
given. And if a container is given as a reference on the first form it will
contain all the specified values.

end::reference[] */
template <typename Derived>
template <typename Reference>
bound_parser<Derived>::bound_parser(Reference & ref, std::string const & hint)
	: bound_parser(std::make_shared<detail::BoundValueRef<Reference>>(ref), hint)
{
}

template <typename Derived>
template <typename Lambda>
bound_parser<Derived>::bound_parser(
	Lambda const & ref, std::string const & hint,
	typename std::enable_if<detail::is_invocable<Lambda>::value, ctor_lambda_e>::type)
	: bound_parser(std::make_shared<detail::BoundLambda<Lambda>>(ref), hint)
{
}

/* tag::reference[]

[#lyra_bound_parser_specification]
== Specification

end::reference[] */

/* tag::reference[]

[#lyra_bound_parser_help]
=== `lyra::bound_parser::help`, `lyra::bound_parser::operator(help)`

[source]
----
template <typename Derived>
Derived& bound_parser<Derived>::help(std::string const& text);
template <typename Derived>
Derived& bound_parser<Derived>::operator()(std::string const& help);
----

Defines the help description of an argument.

end::reference[] */
template <typename Derived>
Derived & bound_parser<Derived>::help(const std::string & text)
{
	m_description = text;
	return static_cast<Derived &>(*this);
}
template <typename Derived>
Derived & bound_parser<Derived>::operator()(std::string const & help_text)
{
	return this->help(help_text);
}

/* tag::reference[]

[#lyra_bound_parser_optional]
=== `lyra::bound_parser::optional`

[source]
----
template <typename Derived>
Derived& bound_parser<Derived>::optional();
----

Indicates that the argument is optional. This is equivalent to specifying
`cardinality(0, 1)`.

end::reference[] */
template <typename Derived>
Derived & bound_parser<Derived>::optional()
{
	return this->cardinality(0, 1);
}

/* tag::reference[]

[#lyra_bound_parser_required]
=== `lyra::bound_parser::required(n)`

[source]
----
template <typename Derived>
Derived& bound_parser<Derived>::required(size_t n);
----

Specifies that the argument needs to given the number of `n` times
(defaults to *1*).

end::reference[] */
template <typename Derived>
Derived & bound_parser<Derived>::required(size_t n)
{
	if (m_ref->isContainer()) return this->cardinality(1, 0);
	else
		return this->cardinality(n);
}

/* tag::reference[]

[#lyra_bound_parser_cardinality]
=== `lyra::bound_parser::cardinality(n, m)`

[source]
----
template <typename Derived>
Derived& bound_parser<Derived>::cardinality(size_t n);

template <typename Derived>
Derived& bound_parser<Derived>::cardinality(size_t n, size_t m);
----

Specifies the number of times the argument can and needs to appear in the list
of arguments. In the first form the argument can appear exactly `n` times. In
the second form it specifies that the argument can appear from `n` to `m` times
inclusive.

end::reference[] */
template <typename Derived>
Derived & bound_parser<Derived>::cardinality(size_t n)
{
	m_cardinality = { n, n };
	return static_cast<Derived &>(*this);
}

template <typename Derived>
Derived & bound_parser<Derived>::cardinality(size_t n, size_t m)
{
	m_cardinality = { n, m };
	return static_cast<Derived &>(*this);
}

/* tag::reference[]

[#lyra_bound_parser_choices]
=== `lyra::bound_parser::choices`

[source]
----
template <typename Derived>
template <typename T, typename... Rest>
lyra::opt& lyra::bound_parser<Derived>::choices(T val0, Rest... rest)

template <typename Derived>
template <typename Lambda>
lyra::opt& lyra::bound_parser<Derived>::choices(Lambda const &check_choice)
----

Limit the allowed values of an argument. In the first form the value is
limited to the ones listed in the call (two or more values). In the second
form the `check_choice` function is called with the parsed value and returns
`true` if it's an allowed value.

end::reference[] */
template <typename Derived>
template <typename T, typename... Rest,
	typename std::enable_if<!detail::is_invocable<T>::value, int>::type>
Derived & bound_parser<Derived>::choices(T val0, Rest... rest)
{
	value_choices = std::make_shared<detail::choices_set<T>>(val0, rest...);
	return static_cast<Derived &>(*this);
}

template <typename Derived>
template <typename Lambda,
	typename std::enable_if<detail::is_invocable<Lambda>::value, int>::type>
Derived & bound_parser<Derived>::choices(Lambda const & check_choice)
{
	value_choices
		= std::make_shared<detail::choices_check<Lambda>>(check_choice);
	return static_cast<Derived &>(*this);
}

template <typename Derived>
template <typename T, std::size_t N>
Derived & bound_parser<Derived>::choices(const T (&choice_values)[N])
{
	value_choices = std::make_shared<detail::choices_set<T>>(
		std::vector<T> { choice_values, choice_values + N });
	return static_cast<Derived &>(*this);
}

} // namespace lyra

#endif
