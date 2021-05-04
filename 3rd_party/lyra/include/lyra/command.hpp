// Copyright 2020 René Ferdinand Rivera Morell
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_COMMAND_HPP
#define LYRA_COMMAND_HPP

#include "lyra/group.hpp"
#include "lyra/literal.hpp"
#include <functional>
#include <string>

namespace lyra {

/* tag::reference[]

[#lyra_command]
= `lyra::command`

A parser that encapsulates the pattern of parsing sub-commands. It provides a
quick wrapper for the equivalent arrangement of `group` and `literal` parsers.
For example:

[source]
----
lyra::command c = lyra::command("sub");
----

Is equivalent to:

[source]
----
lyra::command c = lyra::group()
	.sequential()
	.add_argument(literal("sub"))
	.add_argument(group());
lyra::group & g = c.get<lyra::group>(1);
----

I.e. it's conposed of a `literal` followed by the rest of the command arguments.

Is-a <<lyra_group>>.

*/ // end::reference[]
class command : public group
{
	public:
	// Construction, with and without, callback.
	explicit command(const std::string & n);
	command(
		const std::string & n, const std::function<void(const group &)> & f);

	// Help description.
	command & help(const std::string & text);
	command & operator()(std::string const & description);

	// Add arguments.
	template <typename P>
	command & add_argument(P const & p);
	template <typename P>
	command & operator|=(parser const & p);

	// Internal.
	virtual std::unique_ptr<parser> clone() const override
	{
		return make_clone<command>(this);
	}
};

/* tag::reference[]

[#lyra_command_ctor]
== Construction

[source]
----
command::command(const std::string & n);
command::command(
	const std::string & n, const std::function<void(const group &)>& f);
----

To construct an `command` we need a name (`n`) that matches, and triggers, that
command.


end::reference[] */
inline command::command(const std::string & n)
{
	this->sequential().add_argument(literal(n)).add_argument(group());
}
inline command::command(
	const std::string & n, const std::function<void(const group &)> & f)
	: group(f)
{
	this->sequential().add_argument(literal(n)).add_argument(group());
}

/* tag::reference[]

[#lyra_command_specification]
== Specification

end::reference[] */

/* tag::reference[]

[#lyra_command_help]
=== `lyra:command::help`

[source]
----
command & command::help(const std::string& text)
command & command::operator()(std::string const& description)
----

Specify a help description for the command. This sets the help for the
underlying literal of the command.

end::reference[] */
inline command & command::help(const std::string & text)
{
	this->get<literal>(0).help(text);
	return *this;
}
inline command & command::operator()(std::string const & description)
{
	return this->help(description);
}

/* tag::reference[]
[#lyra_command_add_argument]
=== `lyra::command::add_argument`

[source]
----
template <typename P>
command & command::add_argument(P const & p);
template <typename P>
command & command::operator|=(parser const & p);
----

Adds the given argument parser to the considered arguments for this `comand`.
The argument is added to the sub-group argument instead of this one. Hence it
has the effect of adding arguments *after* the command name.

end::reference[] */
template <typename P>
command & command::add_argument(P const & p)
{
	this->get<group>(1).add_argument(p);
	return *this;
}
template <typename P>
command & command::operator|=(parser const & p)
{
	return this->add_argument(p);
}

} // namespace lyra

#endif
