// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_OPT_HPP
#define LYRA_OPT_HPP

#include "lyra/detail/trait_utils.hpp"
#include "lyra/parser.hpp"
#include "lyra/val.hpp"
#include <memory>

namespace lyra {
/* tag::reference[]

[#lyra_opt]
= `lyra::opt`

A parser for one option with multiple possible names. The option value(s) are
communicated through a reference to a variable, a container, or a callback.

Is-a <<lyra_bound_parser>>.

end::reference[] */
class opt : public bound_parser<opt>
{
	protected:
	std::vector<std::string> opt_names;

	public:
	enum class ctor_lambda_e
	{
		val
	};
	enum class ctor_ref_e
	{
		val
	};

	// Flag option ctors..

	explicit opt(bool & ref);

	template <typename L>
	explicit opt(
		L const & ref,
		typename std::enable_if<detail::is_invocable<L>::value, ctor_lambda_e>::
			type
		= ctor_lambda_e::val);

	// Value option ctors..

	template <typename T>
	opt(T & ref,
		std::string const & hint,
		typename std::enable_if<!detail::is_invocable<T>::value, ctor_ref_e>::
			type
		= ctor_ref_e::val);

	template <typename L>
	opt(L const & ref,
		std::string const & hint,
		typename std::enable_if<detail::is_invocable<L>::value, ctor_lambda_e>::
			type
		= ctor_lambda_e::val);

	// Bound value ctors..
	template <typename T>
	explicit opt(detail::BoundVal<T> && val)
		: bound_parser(val.move_to_shared())
	{}
	template <typename T>
	explicit opt(detail::BoundVal<T> && val, std::string const & hint)
		: bound_parser(val.move_to_shared(), hint)
	{}

	// Option specifications..

	opt & name(const std::string & opt_name);
	opt & operator[](std::string const & opt_name);

	// Internal..

	virtual std::string get_usage_text() const override
	{
		std::string result;
		for (std::size_t o = 0; o < opt_names.size(); ++o)
		{
			if (o > 0) result += "|";
			result += opt_names[o];
		}
		if (!m_hint.empty()) result += " <" + m_hint + ">";
		return result;
	}

	virtual help_text get_help_text() const override
	{
		std::ostringstream oss;
		bool first = true;
		for (auto const & opt : opt_names)
		{
			if (first)
				first = false;
			else
				oss << ", ";
			oss << opt;
		}
		if (!m_hint.empty()) oss << " <" << m_hint << ">";
		return { { oss.str(), m_description } };
	}

	virtual bool is_named(const std::string & n) const override
	{
		return bound_parser::is_named(n)
			|| (std::find(opt_names.begin(), opt_names.end(), n)
				!= opt_names.end());
	}

	bool isMatch(
		std::string const & optToken,
		parser_customization const & customize) const
	{
		auto normalisedToken = normaliseOpt(optToken, customize);
		for (auto const & name : opt_names)
		{
			if (normaliseOpt(name, customize) == normalisedToken) return true;
		}
		return false;
	}

	std::string normaliseOpt(
		std::string const & optName,
		parser_customization const & customize) const
	{
		auto is_prefix_char_0
			= customize.option_prefix().find(optName[0]) != std::string::npos;
		auto is_prefix_char_1
			= customize.option_prefix().find(optName[1]) != std::string::npos;
		if (is_prefix_char_0)
		{
			if (is_prefix_char_1)
				return "--" + optName.substr(2);
			else
				return "-" + optName.substr(1);
		}
		else
			return optName;
	}

	using parser::parse;

	parse_result parse(
		detail::token_iterator const & tokens,
		parser_customization const & customize) const override
	{
		auto validationResult = validate();
		if (!validationResult) return parse_result(validationResult);

		auto remainingTokens = tokens;
		if (remainingTokens && remainingTokens.has_option_prefix())
		{
			auto const & token = remainingTokens.option();
			if (isMatch(token.name, customize))
			{
				if (m_ref->isFlag())
				{
					remainingTokens.pop(token);
					auto flagRef
						= static_cast<detail::BoundFlagRefBase *>(m_ref.get());
					auto result = flagRef->setFlag(true);
					if (!result) return parse_result(result);
					if (result.value() == parser_result_type::short_circuit_all)
						return parse_result::ok(detail::parse_state(
							result.value(), remainingTokens));
				}
				else
				{
					auto const & argToken = remainingTokens.value();
					if (argToken.type == detail::token_type::unknown)
						return parse_result::runtimeError(
							{ parser_result_type::no_match, remainingTokens },
							"Expected argument following " + token.name);
					remainingTokens.pop(token, argToken);
					auto valueRef
						= static_cast<detail::BoundValueRefBase *>(m_ref.get());
					if (value_choices)
					{
						auto choice_result
							= value_choices->contains_value(argToken.name);
						if (!choice_result) return parse_result(choice_result);
					}
					auto result = valueRef->setValue(argToken.name);
					if (!result) return parse_result(result);
					if (result.value() == parser_result_type::short_circuit_all)
						return parse_result::ok(detail::parse_state(
							result.value(), remainingTokens));
				}
				return parse_result::ok(detail::parse_state(
					parser_result_type::matched, remainingTokens));
			}
		}
		return parse_result::ok(
			detail::parse_state(parser_result_type::no_match, remainingTokens));
	}

	result validate() const override
	{
		if (opt_names.empty())
			return result::logicError("No options supplied to opt");
		for (auto const & name : opt_names)
		{
			if (name.empty())
				return result::logicError("Option name cannot be empty");
			if (name[0] != '-')
				return result::logicError("Option name must begin with '-'");
		}
		return bound_parser::validate();
	}

	virtual std::unique_ptr<parser> clone() const override
	{
		return make_clone<opt>(this);
	}
};

/* tag::reference[]

[#lyra_opt_ctor]
== Construction

end::reference[] */

/* tag::reference[]

[#lyra_opt_ctor_flags]
=== Flags

[source]
----
lyra::opt::opt(bool& ref);

template <typename L>
lyra::opt::opt(L const& ref);
----

Constructs a flag option with a target `bool` to indicate if the flag is
present. The first form takes a reference to a variable to receive the
`bool`. The second takes a callback that is called with `true` when the
option is present.

end::reference[] */
inline opt::opt(bool & ref)
	: bound_parser(std::make_shared<detail::BoundFlagRef>(ref))
{}
template <typename L>
opt::opt(
	L const & ref,
	typename std::
		enable_if<detail::is_invocable<L>::value, opt::ctor_lambda_e>::type)
	: bound_parser(std::make_shared<detail::BoundFlagLambda<L>>(ref))
{}

/* tag::reference[]

[#lyra_opt_ctor_values]
=== Values

[source]
----
template <typename T>
lyra::opt::opt(T& ref, std::string const& hint);

template <typename L>
lyra::opt::opt(L const& ref, std::string const& hint)
----

Constructs a value option with a target `ref`. The first form takes a reference
to a variable to receive the value. The second takes a callback that is called
with the value when the option is present.

end::reference[] */
template <typename T>
opt::opt(
	T & ref,
	std::string const & hint,
	typename std::enable_if<!detail::is_invocable<T>::value, opt::ctor_ref_e>::
		type)
	: bound_parser(ref, hint)
{}
template <typename L>
opt::opt(
	L const & ref,
	std::string const & hint,
	typename std::
		enable_if<detail::is_invocable<L>::value, opt::ctor_lambda_e>::type)
	: bound_parser(ref, hint)
{}

/* tag::reference[]

[#lyra_opt_specification]
== Specification

end::reference[] */

/* tag::reference[]

[#lyra_opt_name]
=== `lyra::opt::name`

[source]
----
lyra::opt& lyra::opt::name(const std::string &opt_name)
lyra::opt& lyra::opt::operator[](const std::string &optName)
----

Add a spelling for the option of the form `--<name>` or `-n`.
One can add multiple short spellings at once with `-abc`.

end::reference[] */
inline opt & opt::name(const std::string & opt_name)
{
	if (opt_name.size() > 2 && opt_name[0] != opt_name[1])
		for (auto o : opt_name.substr(1))
			opt_names.push_back(std::string(1, opt_name[0]) + o);
	else
		opt_names.push_back(opt_name);
	return *this;
}
inline opt & opt::operator[](const std::string & opt_name)
{
	return this->name(opt_name);
}

} // namespace lyra

#endif
