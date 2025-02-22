#include <nano/lib/enum_util.hpp>
#include <nano/lib/logging_enums.hpp>
#include <nano/lib/utility.hpp>

std::string_view nano::log::to_string (nano::log::type tag)
{
	return nano::enum_util::name (tag);
}

std::string_view nano::log::to_string (nano::log::detail detail)
{
	return nano::enum_util::name (detail);
}

std::string_view nano::log::to_string (nano::log::level level)
{
	return nano::enum_util::name (level);
}

const std::vector<nano::log::level> & nano::log::all_levels ()
{
	return nano::enum_util::values<nano::log::level> ();
}

const std::vector<nano::log::type> & nano::log::all_types ()
{
	return nano::enum_util::values<nano::log::type> ();
}

nano::log::level nano::log::parse_level (std::string_view name)
{
	auto value = nano::enum_util::try_parse<nano::log::level> (name);
	if (value.has_value ())
	{
		return value.value ();
	}
	auto all_levels_str = nano::util::join (nano::log::all_levels (), ", ", [] (auto const & lvl) {
		return to_string (lvl);
	});
	throw std::invalid_argument ("Invalid log level: " + std::string (name) + ". Must be one of: " + all_levels_str);
}

nano::log::type nano::log::parse_type (std::string_view name)
{
	auto value = nano::enum_util::try_parse<nano::log::type> (name);
	if (value.has_value ())
	{
		return value.value ();
	}
	throw std::invalid_argument ("Invalid log type: " + std::string (name));
}

nano::log::detail nano::log::parse_detail (std::string_view name)
{
	auto value = nano::enum_util::try_parse<nano::log::detail> (name);
	if (value.has_value ())
	{
		return value.value ();
	}
	throw std::invalid_argument ("Invalid log detail: " + std::string (name));
}

std::string_view nano::log::to_string (nano::log::tracing_format format)
{
	return nano::enum_util::name (format);
}

nano::log::tracing_format nano::log::parse_tracing_format (std::string_view name)
{
	auto value = nano::enum_util::try_parse<nano::log::tracing_format> (name);
	if (value.has_value ())
	{
		return value.value ();
	}
	auto all_formats_str = nano::util::join (nano::log::all_tracing_formats (), ", ", [] (auto const & fmt) {
		return to_string (fmt);
	});
	throw std::invalid_argument ("Invalid tracing format: " + std::string (name) + ". Must be one of: " + all_formats_str);
}

const std::vector<nano::log::tracing_format> & nano::log::all_tracing_formats ()
{
	return nano::enum_util::values<nano::log::tracing_format> ();
}