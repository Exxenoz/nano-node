#pragma once

#include <nano/store/fwd.hpp>

#include <optional>

namespace nano::store
{
enum class meta_key : uint64_t
{
	ledger_version = 1,
	ext_ledger_version = 2,
	ext_ledger_flags = 3,
};

using meta_value_t = uint64_t;

class meta_view
{
public:
	explicit meta_view (nano::store::backend &);

	void put (nano::store::write_transaction const &, nano::store::meta_key meta_key, nano::store::meta_value_t meta_value);
	void del (nano::store::write_transaction const &, nano::store::meta_key meta_key);
	std::optional<nano::store::meta_value_t> get (nano::store::transaction const &, nano::store::meta_key meta_key) const;
	bool exists (nano::store::transaction const &, nano::store::meta_key meta_key) const;

private:
	nano::store::backend & backend;
};
}