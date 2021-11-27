// Copyright 2020 René Ferdinand Rivera Morell
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_GROUP_HPP
#define LYRA_GROUP_HPP

#include "lyra/detail/print.hpp"
#include "lyra/arguments.hpp"
#include <functional>

namespace lyra {
/* tag::reference[]

[#lyra_group]
= `lyra::group`

A group of arguments provides for parsing, optionally, a set of arguments
together. The group itself is considered successfully parsed only when the
arguments in the group are parsed without errors. A common use case for this
are sub-commands. This implementation is recursive. And hence allows groups
within groups for describing branching argument parsing.

Is-a <<lyra_arguments>>.

end::reference[] */
class group : public arguments
{
	public:
	group();
	group(const group & other);
	explicit group(const std::function<void(const group &)> & f);

	virtual bool is_group() const override { return true; }

	parse_result parse(
		detail::token_iterator const & tokens,
		const option_style & style) const override
	{
		LYRA_PRINT_SCOPE("group::parse");
		LYRA_PRINT_DEBUG("(?)", get_usage_text(style), "?=",
			tokens.argument().name);
		parse_result result = arguments::parse(tokens, style);
		if (result && result.value().type() != parser_result_type::no_match
			&& success_signal)
		{
			// Trigger any success signal for parsing the argument as the group.
			// This allows for executing handlers for commands.
			this->success_signal(*this);
		}
		if (!result)
		{
			LYRA_PRINT_DEBUG("(!)", get_usage_text(style), "!=",
				tokens.argument().name);
		}
		else
		{
			LYRA_PRINT_DEBUG("(=)", get_usage_text(style), "==",
				tokens.argument().name, "==>", result.value().type());
		}
		return result;
	}

	group & optional();
	group & required(size_t n = 1);
	group & cardinality(size_t n);
	group & cardinality(size_t n, size_t m);
	detail::parser_cardinality cardinality() const override
	{
		return m_cardinality;
	}

	virtual std::unique_ptr<parser> clone() const override
	{
		return make_clone<group>(this);
	}

	private:
	std::function<void(const group &)> success_signal;
	detail::parser_cardinality m_cardinality = {0,1};
};

/* tag::reference[]

[#lyra_group_ctor]
== Construction

end::reference[] */

/* tag::reference[]

[#lyra_group_ctor_default]
=== Default

[source]
----
group();
----

Default constructing a `group` does not register the success callback.

end::reference[] */
inline group::group()
	: m_cardinality(0,1)
{}

/* tag::reference[]

[#lyra_group_ctor_copy]
=== Copy

[source]
----
group::group(const group & other);
----

end::reference[] */
inline group::group(const group & other)
	: arguments(other)
	, success_signal(other.success_signal)
	, m_cardinality(other.m_cardinality)
{}

/* tag::reference[]

[#lyra_group_ctor_success]
=== Success Handler

[source]
----
group::group(const std::function<void(const group &)> & f)
----

Registers a function to call when the group is successfully parsed. The
function is called with the group to facilitate customization.

end::reference[] */
inline group::group(const std::function<void(const group &)> & f)
	: success_signal(f)
{}

/* tag::reference[]

[#lyra_group_optional]
=== `lyra::group::optional`

[source]
----
group & group::optional();
----

Indicates that the argument is optional. This is equivalent to specifying
`cardinality(0, 1)`.

end::reference[] */
inline group & group::optional()
{
	m_cardinality.optional();
	return *this;
}

/* tag::reference[]

[#lyra_group_required]
=== `lyra::group::required(n)`

[source]
----
group & group::required(size_t n);
----

Specifies that the argument needs to given the number of `n` times
(defaults to *1*).

end::reference[] */
inline group & group::required(size_t n)
{
	m_cardinality.required(n);
	return *this;
}

/* tag::reference[]

[#lyra_group_cardinality]
=== `lyra::group::cardinality(n)`

[source]
----
group & group::cardinality(size_t n);
group & group::cardinality(size_t n, size_t m);
----

Specifies the number of times the argument can and needs to appear in the list
of arguments. In the first form the argument can appear exactly `n` times. In
the second form it specifies that the argument can appear from `n` to `m` times
inclusive.

end::reference[] */
inline group & group::cardinality(size_t n)
{
	m_cardinality.counted(n);
	return *this;
}
inline group & group::cardinality(size_t n, size_t m)
{
	m_cardinality.bounded(n, m);
	return *this;
}

} // namespace lyra

#endif
