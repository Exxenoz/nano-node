#pragma once

#include <string_view>

namespace nano::store
{
enum class open_mode
{
	read_only,
	read_write,
	create,
};

std::string_view to_string (open_mode);
}