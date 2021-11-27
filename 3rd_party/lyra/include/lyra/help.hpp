// Copyright 2018-2019 René Ferdinand Rivera Morell
// Copyright 2017 Two Blue Cubes Ltd. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_HELP_HPP
#define LYRA_HELP_HPP

#include "lyra/opt.hpp"

namespace lyra {
/* tag::reference[]

[#lyra_help]
= `lyra::help`

Utility function that defines a default `--help` option. You can specify a
`bool` flag to indicate if the help option was specified and that you could
display a help message.

The option accepts `-?`, `-h`, and `--help` as allowed option names.

*/ // end::reference[]
class help : public opt
{
	public:
	help(bool & showHelpFlag)
		: opt([&](bool flag) {
			showHelpFlag = flag;
			return parser_result::ok(parser_result_type::short_circuit_all);
		})
	{
		this->description("Display usage information.")
			.optional()
			.name("-?")
			.name("-h")
			.name("--help");
	}

	help & description(const std::string & text);

	virtual std::string get_description_text(const option_style &) const override
	{
		return description_text;
	}

	virtual std::unique_ptr<parser> clone() const override
	{
		return make_clone<help>(this);
	}

	private:
	std::string description_text;
};

/* tag::reference[]
[source]
----
help & help::description(const std::string &text)
----

Sets the given `text` as the general description to show with the help and
usage output for CLI parser. This text is displayed between the "Usage"
and "Options, arguments" sections.

end::reference[] */
inline help & help::description(const std::string & text)
{
	description_text = text;
	return *this;
}
} // namespace lyra

#endif
