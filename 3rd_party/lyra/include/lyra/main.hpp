// Copyright 2019 René Ferdinand Rivera Morell
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_MAIN_HPP
#define LYRA_MAIN_HPP

#include "lyra/arg.hpp"
#include "lyra/args.hpp"
#include "lyra/cli.hpp"
#include "lyra/help.hpp"
#include "lyra/opt.hpp"
#include "lyra/parser.hpp"
#include "lyra/val.hpp"

#include <iostream>

namespace lyra {
/* tag::reference[]

[#lyra_main]
= `lyra::main`

Encapsulates the common use case of a main program that has a help option and
has a minimal way to specify and parse options. This provides for a way to
specify options and arguments in a simple function form. It handles checking for
errors and reporting problems.

*/ // end::reference[]
class main final : protected cli
{
	bool show_help = false;

	public:
	explicit main(const std::string & text = "");

	template <typename T>
	main & operator()(const T & parser);
	template <typename T>
	main & add_argument(const T & parser);
	template <typename T>
	main & operator|=(const T & parser);

	template <typename V>
	main & operator()(
		std::initializer_list<std::string> arg_names, V && default_value);
	template <typename V>
	main & operator()(const std::string & arg_name, V && default_value);

	template <typename L>
	int operator()(const args & argv, L action);
	template <typename L>
	int operator()(int argc, const char ** argv, L action);

	using cli::operator[];
};

/* tag::reference[]

[#lyra_main_ctor]
== Construction

[source]
----
main::main(const std::string & text);
----

Construct with text for description, which defaults to an empty string. The
description is specified for the help option that is added to the command line.

end::reference[] */
inline main::main(const std::string & text)
{
	this->add_argument(help(show_help).description(text));
}

/* tag::reference[]

[#lyra_main_add_argument]
== Add Argument

[source]
----
template <typename T> main & main::operator()(const T & parser)
template <typename T> main & main::add_argument(const T & parser)
template <typename T> main & main::operator|=(const T & parser)
----

Adds a parser as an argument to the command line. These forward directly to the
`lyra::cli` equivalents. The added parser can be any of the regular Lyra parsers
like `lyra::opt` or `lyra::arg`.

end::reference[] */
template <typename T>
main & main::operator()(const T & parser)
{
	cli::add_argument(parser);
	return *this;
}
template <typename T>
main & main::add_argument(const T & parser)
{
	cli::add_argument(parser);
	return *this;
}
template <typename T>
main & main::operator|=(const T & parser)
{
	cli::operator|=(parser);
	return *this;
}

/* tag::reference[]

[#lyra_main_simple_args]
== Simple Args

[source]
----
template <typename V>
main & main::operator()(
	std::initializer_list<std::string> arg_names, V && default_value)
template <typename V>
main & main::operator()(const std::string & arg_name, V && default_value)
----

Specifies, and adds, a new argument. Depending on the `arg_names` it can be
either a `lyra::opt` or `lyra::arg`. The first item in `arg_names` indicates
the type of argument created and added:

Specify either `-<name>` or `--<name>` to add a `lyra::opt`. You can specify as
many option names following the first name. A name that doesn't follow the
option syntax is considered the as the help text for the option.

Specify a non `-` prefixed name as the first item to signify a positional
`lyra::arg`.

The single `std::string` call is equivalent to specifying just the one option or
argument.

Example specifications:

|===
| `("-o", 0)` | Short `-o` option as `int` value.
| `("--opt", 0)` | Long `--opt` option as `int` value.
| `({"-o", "--opt"}, 1.0f)` | Short and long option as `float` value.
| `({"-o", "The option."}, 1.0f)` | Short option and help description as `float` value.
| `("opt", 2)` | Positional, i.e. `lyra::arg`, argument as `int` value.
| `({"opt", "The option."}, 2)` | Positional argument and help description as `int` value.
| `("--opt", std::vector<float>())` | Long option with as multiple float values.
|===

end::reference[] */
template <typename V>
main & main::operator()(
	std::initializer_list<std::string> arg_names, V && default_value)
{
	auto bound_val = val(std::forward<V>(default_value));
	if ((*arg_names.begin())[0] == '-')
	{
		// An option to add.
		std::string hint = arg_names.begin()->substr(1);
		if (hint[0] == '-') hint = hint.substr(1);
		opt o(std::move(bound_val), hint);
		for (auto arg_name : arg_names)
		{
			if (arg_name[0] == '-')
				o.name(arg_name);
			else
				o.help(arg_name);
		}
		cli::add_argument(o);
	}
	else
	{
		// An argument to add.
		arg a(std::move(bound_val), *arg_names.begin());
		a.optional();
		if (arg_names.size() > 2) a.help(*(arg_names.begin() + 1));
		cli::add_argument(a);
	}
	return *this;
}
template <typename V>
main & main::operator()(const std::string & arg_name, V && default_value)
{
	return (*this)({ arg_name }, std::forward<V>(default_value));
}

/* tag::reference[]

[#lyra_main_execute]
== Execute

[source]
----
template <typename L>
int main::operator()(const args & argv, L action)
template <typename L>
int main::operator()(int argc, const char ** argv, L action)
----

Executes the given action after parsing of the program input arguments. It
returns either `0` or `1` if the execution was successful or failed
respectively. The `action` is called with the `lyra::main` instance to provide
access to the parsed argument values.

end::reference[] */
template <typename L>
int main::operator()(const args & argv, L action)
{
	auto result = cli::parse(argv);
	if (!result) std::cerr << result.errorMessage() << "\n\n";
	if (show_help || !result)
		std::cout << *this << "\n";
	else
		return action(*this);
	return result ? 0 : 1;
}
template <typename L>
int main::operator()(int argc, const char ** argv, L action)
{
	return (*this)({ argc, argv }, action);
}

} // namespace lyra

#endif
