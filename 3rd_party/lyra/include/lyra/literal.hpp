// Copyright 2020 René Ferdinand Rivera Morell
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_LITERAL_HPP
#define LYRA_LITERAL_HPP

#include "lyra/parser.hpp"
#include <string>

namespace lyra {
/* tag::reference[]

[#lyra_literal]
= `lyra::literal`

A parser that matches a single fixed value.

Is-a <<lyra_parser>>.

end::reference[] */
class literal : public parser
{
	public:
	// Construction.
	literal(std::string const & n);

	// Help description.
	literal & help(const std::string & text);
	literal & operator()(std::string const & description);

	// Singular argument allowed and required.
	virtual detail::parser_cardinality cardinality() const override
	{
		return { 1, 1 };
	}

	// Internal.

	virtual std::string get_usage_text() const override { return name; }

	virtual std::string get_description_text() const override
	{
		return description;
	}

	virtual help_text get_help_text() const override
	{
		return { { name, description } };
	}

	using parser::parse;

	virtual parse_result parse(detail::token_iterator const & tokens,
		parser_customization const &) const override
	{
		auto validationResult = validate();
		if (!validationResult) return parse_result(validationResult);

		auto const & token = tokens.argument();
		if (name == token.name)
		{
			auto remainingTokens = tokens;
			remainingTokens.pop(token);
			return parse_result::ok(detail::parse_state(
				parser_result_type::matched, remainingTokens));
		}
		else
		{
			return parse_result(parser_result::runtimeError(
				parser_result_type::no_match, "Expected '" + name + "'."));
		}
	}

	virtual std::unique_ptr<parser> clone() const override
	{
		return make_clone<literal>(this);
	}

	protected:
	std::string name;
	std::string description;
};

/* tag::reference[]

[#lyra_literal_ctor]
== Construction

end::reference[] */

/* tag::reference[]

=== Token

[#lyra_literal_ctor_token]

[source]
----
inline literal::literal(std::string const& n)
----

Constructs the literal with the name of the token to match.

end::reference[] */
inline literal::literal(std::string const & n)
	: name(n)
{}

/* tag::reference[]

[#lyra_literal_specification]
== Specification

end::reference[] */

/* tag::reference[]

[#lyra_literal_help]
=== `lyra:literal::help`

[source]
----
literal& literal::help(const std::string& text)
literal& literal::operator()(std::string const& description)
----

Specify a help description for the literal.

end::reference[] */
inline literal & literal::help(const std::string & text)
{
	description = text;
	return *this;
}
inline literal & literal::operator()(std::string const & description)
{
	return this->help(description);
}

} // namespace lyra

#endif
