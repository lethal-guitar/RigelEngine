// Copyright 2021 René Ferdinand Rivera Morell

#ifndef LYRA_OPTION_STYLE_HPP
#define LYRA_OPTION_STYLE_HPP

#include <string>

namespace lyra {

/* tag::reference[]

[#lyra_option_style]
= `lyra::option_style`

Specify the syntax style for options to the parser.

[source]
----
std::string value_delimiters;
std::string long_option_prefix;
std::size_t long_option_size = 0;
std::string short_option_prefix;
std::size_t short_option_size = 0;
----

* `value_delimiters` -- Specifies a set of characters that are accepted as a
	delimiter/separator between an option name and an option value when a
	single argument is used for the option+value (i.e. "--option=value").
* `long_option_prefix` -- Specifies a set of characters that are accepted as a
	prefix for long options (i.e. multi-char single option name).
* `long_option_size` -- The number of prefix characters that indicates a long
	option. A value of zero (0) indicates that long options are not accepted.
* `short_option_prefix` -- Specifies a set of characters that are accepted as a
	prefix for short options (i.e. single-char multi-options).
* `short_option_size` -- The number of prefix characters that indicates a short
	option. A value of zero (0) indicates that short options are not accepted.

end::reference[] */
struct option_style
{
	std::string value_delimiters;
	std::string long_option_prefix;
	std::size_t long_option_size = 0;
	std::string short_option_prefix;
	std::size_t short_option_size = 0;

	// Construction..

	option_style(
		std::string && value_delimiters,
		std::string && long_option_prefix = {},
		std::size_t long_option_size = 0,
		std::string && short_option_prefix = {},
		std::size_t short_option_size = 0)
		: value_delimiters(std::move(value_delimiters))
		, long_option_prefix(std::move(long_option_prefix))
		, long_option_size(long_option_size)
		, short_option_prefix(std::move(short_option_prefix))
		, short_option_size(short_option_size)
	{}

	// Definitions..

	std::string long_option_string() const;
	std::string short_option_string() const;

	// Styles..

	static const option_style & posix();
	static const option_style & posix_brief();
	static const option_style & windows();
};

/* tag::reference[]

[#lyra_option_style_ctor]
== Construction

[source]
----
lyra::option_style::option_style(
	std::string && value_delimiters,
	std::string && long_option_prefix = {},
	std::size_t long_option_size = 0,
	std::string && short_option_prefix = {},
	std::size_t short_option_size = 0)
----

Utility constructor that defines all the settings.

end::reference[] */

/* tag::reference[]

[#lyra_option_style_def]
== Definitions

[source]
----
std::string lyra::option_style::long_option_string() const
std::string lyra::option_style::short_option_string() const
----

Gives the default long or short option string, or prefix, for this option
style. If the type of option is not available, i.e. size is zero, an empty
string is returned.

end::reference[] */

inline std::string option_style::long_option_string() const
{
	return long_option_size > 0 ? std::string(long_option_size, long_option_prefix[0]) : "";
}

inline std::string option_style::short_option_string() const
{
	return short_option_size > 0 ? std::string(short_option_size, short_option_prefix[0]) : "";
}

/* tag::reference[]

[#lyra_option_style_styles]
== Styles

[source]
----
static const option_style & lyra::option_style::posix();
static const option_style & lyra::option_style::posix_brief();
static const option_style & lyra::option_style::windows();
----

These provide definitions for common syntax of option styles:

`posix`:: The overall _default_ that is two dashes (`--`) for long option
	names and one dash (`-`) for short option names. Values for long options
	use equal (`=`) between the option and value.
`posix_brief`:: Variant that only allows for long option names with a single
	dash (`-`).
`windows`:: The common option style on Windows `CMD.EXE` shell. It only allows
	long name options that start with slash (`/`) where the value is
	specified after a colon (`:`). Single character flag style options are
	only available as individual long options, for example `/A`.

end::reference[] */

inline const option_style & option_style::posix()
{
	static const option_style style("= ", "-", 2, "-", 1);
	return style;
}

inline const option_style & option_style::posix_brief()
{
	static const option_style style("= ", "-", 1);
	return style;
}

inline const option_style & option_style::windows()
{
	static const option_style style(":", "/", 1);
	return style;
}

} // namespace lyra

#endif
