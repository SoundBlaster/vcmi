/*
 * DiagnosticTextCommand.h, part of VCMI engine
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

/// Parse a single-line, printable ASCII text event within the socket limit.
inline std::optional<std::string> diagnosticTextCommand(std::string_view command)
{
	constexpr std::string_view prefix = "text ";
	if(!command.starts_with(prefix))
		return std::nullopt;
	const auto value = command.substr(prefix.size());
	if(value.empty() || value.size() > 250 || !std::ranges::all_of(value, [](unsigned char c) { return c >= 32 && c <= 126; }))
		return std::nullopt;
	return std::string(value);
}
