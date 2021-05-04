// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_EXE_NAME_HPP
#define LYRA_EXE_NAME_HPP

#include "lyra/detail/bound.hpp"
#include "lyra/detail/tokens.hpp"
#include "lyra/parser.hpp"
#include "lyra/parser_result.hpp"

namespace lyra
{
/* tag::reference[]

[#lyra_exe_name]
= `lyra::exe_name`

Specifies the name of the executable.

Is-a <<lyra_composable_parser>>.

end::reference[] */
class exe_name : public composable_parser<exe_name>
{
	public:
	exe_name()
		: m_name(std::make_shared<std::string>("<executable>"))
	{
	}

	explicit exe_name(std::string& ref);

	template <typename LambdaT>
	explicit exe_name(LambdaT const& lambda);

	std::string name() const;
	parser_result set(std::string const& newName);

	// The exe name is not parsed out of the normal tokens, but is handled
	// specially
	virtual parse_result parse(
		detail::token_iterator const& tokens,
		parser_customization const&) const override
	{
		return parse_result::ok(
			detail::parse_state(parser_result_type::no_match, tokens));
	}

	virtual std::unique_ptr<parser> clone() const override
	{
		return make_clone<exe_name>(this);
	}

	private:
	std::shared_ptr<std::string> m_name;
	std::shared_ptr<detail::BoundValueRefBase> m_ref;
};

/* tag::reference[]

[#lyra_exe_name_ctor]
== Construction

end::reference[] */

/* tag::reference[]
[source]
----
exe_name::exe_name(std::string& ref)
----

Constructs with a target string to receive the name of the executable. When
the `cli` is run the target string will contain the exec name.

end::reference[] */
inline exe_name::exe_name(std::string& ref)
	: exe_name()
{
	m_ref = std::make_shared<detail::BoundValueRef<std::string>>(ref);
}

/* tag::reference[]
[source]
----
template <typename LambdaT>
exe_name::exe_name(LambdaT const& lambda)
----

Construct with a callback that is called with the value of the executable name
when the `cli` runs.

end::reference[] */
template <typename LambdaT>
exe_name::exe_name(LambdaT const& lambda)
	: exe_name()
{
	m_ref = std::make_shared<detail::BoundLambda<LambdaT>>(lambda);
}

/* tag::reference[]

[#lyra_exe_name_accessors]
== Accessors

end::reference[] */

/* tag::reference[]

[#lyra_exe_name_name]
=== `lyra::exe_name::name`

[source]
----
std::string exe_name::name() const
----

Returns the executable name when available. Otherwise it returns a default
value.

end::reference[] */
inline std::string exe_name::name() const { return *m_name; }

/* tag::reference[]

[#lyra_exe_name_set]
=== `lyra::exe_name::set`

[source]
----
parser_result exe_name::set(std::string const& newName)
----

Sets the executable name with the `newName` value. The new value is reflected
in the bound string reference or callback.

end::reference[] */
inline parser_result exe_name::set(std::string const& newName)
{
	auto lastSlash = newName.find_last_of("\\/");
	auto filename = (lastSlash == std::string::npos)
		? newName
		: newName.substr(lastSlash + 1);

	*m_name = filename;
	if (m_ref)
		return m_ref->setValue(filename);
	else
		return parser_result::ok(parser_result_type::matched);
}

} // namespace lyra

#endif
