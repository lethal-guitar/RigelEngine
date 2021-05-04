// Copyright 2020 René Ferdinand Rivera Morell
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_GROUP_HPP
#define LYRA_GROUP_HPP

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
	group() = default;
	group(const group & other);
	explicit group(const std::function<void(const group &)> & f);

	virtual bool is_group() const override { return true; }

	parse_result parse(detail::token_iterator const & tokens,
		parser_customization const & customize) const override
	{
		parse_result result = arguments::parse(tokens, customize);
		if (result && result.value().type() != parser_result_type::no_match
			&& success_signal)
		{
			// Trigger any success signal for parsing the argument as the group.
			// This allows for executing handlers for commands.
			this->success_signal(*this);
		}
		return result;
	}

	virtual std::unique_ptr<parser> clone() const override
	{
		return make_clone<group>(this);
	}

	private:
	std::function<void(const group &)> success_signal;
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
group() = default;
----

Default constructing a `group` does not register the success callback.

end::reference[] */

/* tag::reference[]

[#lyra_group_ctor_copy]
=== Copy

[source]
----
group::group(const group& other);
----

end::reference[] */
inline group::group(const group & other)
	: arguments(other)
	, success_signal(other.success_signal)
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

} // namespace lyra

#endif
