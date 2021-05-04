// Copyright 2018-2020 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_CLI_HPP
#define LYRA_CLI_HPP

#include "lyra/arguments.hpp"
#include "lyra/detail/from_string.hpp"
#include "lyra/exe_name.hpp"
#include "lyra/group.hpp"

#include <type_traits>

namespace lyra {
/* tag::reference[]

[#lyra_cli]
= `lyra::cli`

A Combined parser made up of any two or more other parsers. Creating and using
one of these as a basis one can incrementally compose other parsers into this
one. For example:

[source]
----
auto cli = lyra::cli();
std::string what;
float when = 0;
std::string where;
cli |= lyra::opt(what, "what")["--make-it-so"]("Make it so.").required();
cli |= lyra::opt(when. "when")["--time"]("When to do <what>.").optional();
cli.add_argument(lyra::opt(where, "where").name("--where")
	.help("There you are.").optional());
----

*/ // end::reference[]
class cli : protected arguments
{
	public:
	cli() = default;

	// Copy construction, needs to copy the exe name and the composed parsers.
	cli(const cli & other);

	// Compose the `exe_name` parser.
	cli & add_argument(exe_name const & exe_name);
	cli & operator|=(exe_name const & exe_name);

	// Compose a regular parser.
	cli & add_argument(parser const & p);
	cli & operator|=(parser const & p);

	// Compose a group, by adding it as a single argument.
	cli & add_argument(group const & p);
	cli & operator|=(group const & p);

	// Compose the parsers from another `cli`.
	cli & add_argument(cli const & other);
	cli & operator|=(cli const & other);

	// Concat composition.
	template <typename T>
	cli operator|(T const & other) const;

	// Result reference wrapper to fetch and convert argument.
	struct value_result
	{
		public:
		explicit value_result(const parser * p)
			: parser_ref(p)
		{}

		template <
			typename T,
			typename std::enable_if<detail::is_convertible_from_string<
				typename detail::remove_cvref<T>::type>::value>::
				type * = nullptr>
		operator T() const
		{
			typename detail::remove_cvref<T>::type result {};
			if (parser_ref)
				detail::from_string<
					std::string,
					typename detail::remove_cvref<T>::type>(
					parser_ref->get_value(0), result);
			return result;
		}

		template <typename T>
		operator std::vector<T>() const
		{
			std::vector<T> result;
			if (parser_ref)
			{
				for (size_t i = 0; i < parser_ref->get_value_count(); ++i)
				{
					T v;
					if (detail::from_string(parser_ref->get_value(i), v))
						result.push_back(v);
				}
			}
			return result;
		}

		operator std::string() const
		{
			if (parser_ref) return parser_ref->get_value(0);
			return "";
		}

		private:
		const parser * parser_ref = nullptr;
	};

	value_result operator[](const std::string & n);

	// Stream out generates the help output.
	friend std::ostream & operator<<(std::ostream & os, cli const & parser)
	{
		return os << static_cast<const arguments &>(parser);
	}

	// Parse from arguments.
	parse_result parse(
		args const & args,
		parser_customization const & customize
		= default_parser_customization()) const;

	// Internal..

	using arguments::parse;
	using arguments::get_named;

	virtual std::unique_ptr<parser> clone() const override
	{
		return std::unique_ptr<parser>(new cli(*this));
	}

	protected:
	mutable exe_name m_exeName;

	virtual std::string get_usage_text() const override
	{
		if (!m_exeName.name().empty())
			return m_exeName.name() + " " + arguments::get_usage_text();
		else
			// We use an empty exe name as an indicator to remove USAGE text.
			return "";
	}

	parse_result parse(
		std::string const & exe_name,
		detail::token_iterator const & tokens,
		parser_customization const & customize) const override
	{
		m_exeName.set(exe_name);
		return parse(tokens, customize);
	}
};

/* tag::reference[]

[#lyra_cli_ctor]
== Construction

end::reference[] */

/* tag::reference[]

[#lyra_cli_ctor_default]
=== Default

[source]
----
cli() = default;
----

Default constructing a `cli` is the starting point to adding arguments
and options for parsing a command line.

end::reference[] */

/* tag::reference[]

[#lyra_cli_ctor_copy]
=== Copy

[source]
----
cli::cli(const cli& other);
----

end::reference[] */
inline cli::cli(const cli & other)
	: arguments(other)
	, m_exeName(other.m_exeName)
{}

/* tag::reference[]

[#lyra_cli_specification]
== Specification

end::reference[] */

// ==

/* tag::reference[]
[#lyra_cli_add_argument]
=== `lyra::cli::add_argument`

[source]
----
cli& cli::add_argument(exe_name const& exe_name);
cli& cli::operator|=(exe_name const& exe_name);
cli& cli::add_argument(parser const& p);
cli& cli::operator|=(parser const& p);
cli& cli::add_argument(group const& p);
cli& cli::operator|=(group const& p);
cli& cli::add_argument(cli const& other);
cli& cli::operator|=(cli const& other);
----

Adds the given argument parser to the considered arguments for this
`cli`. Depending on the parser given it will be: recorded as the exe
name (for `exe_name` parser), directly added as an argument (for
`parser`), or add the parsers from another `cli` to this one.

end::reference[] */
inline cli & cli::add_argument(exe_name const & exe_name)
{
	m_exeName = exe_name;
	return *this;
}
inline cli & cli::operator|=(exe_name const & exe_name)
{
	return this->add_argument(exe_name);
}
inline cli & cli::add_argument(parser const & p)
{
	arguments::add_argument(p);
	return *this;
}
inline cli & cli::operator|=(parser const & p)
{
	arguments::add_argument(p);
	return *this;
}
inline cli & cli::add_argument(group const & other)
{
	arguments::add_argument(static_cast<parser const &>(other));
	return *this;
}
inline cli & cli::operator|=(group const & other)
{
	return this->add_argument(other);
}
inline cli & cli::add_argument(cli const & other)
{
	arguments::add_argument(static_cast<arguments const &>(other));
	return *this;
}
inline cli & cli::operator|=(cli const & other)
{
	return this->add_argument(other);
}

template <typename T>
inline cli cli::operator|(T const & other) const
{
	return cli(*this).add_argument(other);
}

template <typename DerivedT, typename T>
cli operator|(composable_parser<DerivedT> const & thing, T const & other)
{
	return cli() | static_cast<DerivedT const &>(thing) | other;
}

/* tag::reference[]
[#lyra_cli_array_ref]
=== `lyra::cli::operator[]`

[source]
----
cli::value_result cli::operator[](const std::string & n)
----

Finds the given argument by either option name or hint name and returns a
convertible reference to the value, either the one provided by the user or the
default.

end::reference[] */
inline cli::value_result cli::operator[](const std::string & n)
{
	return value_result(this->get_named(n));
}

/* tag::reference[]
[#lyra_cli_parse]
=== `lyra::cli::parse`

[source]
----
parse_result cli::parse(
	args const& args, parser_customization const& customize) const;
----

Parses given arguments `args` and optional parser customization `customize`.
The result indicates success or failure, and if failure what kind of failure
it was. The state of variables bound to options is unspecified and any bound
callbacks may have been called.

end::reference[] */
inline parse_result
	cli::parse(args const & args, parser_customization const & customize) const
{
	return parse(
		args.exe_name(),
		detail::token_iterator(
			args, customize.token_delimiters(), customize.option_prefix()),
		customize);
}

} // namespace lyra

#endif
