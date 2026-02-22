#pragma once

#include <nano/store/fwd.hpp>

namespace nano::store
{
enum class version_key : uint64_t
{
	ledger = 1,
};

using version_value_t = uint64_t;

class version_view
{
public:
	explicit version_view (nano::store::backend &);

	void put (nano::store::write_transaction const &, nano::store::version_key version_key, nano::store::version_value_t version);
	nano::store::version_value_t get (nano::store::transaction const &, nano::store::version_key version_key) const;
	bool exists (nano::store::transaction const &, nano::store::version_key version_key) const;
	nano::store::meta_key to_meta_key (nano::store::version_key) const;

private:
	nano::store::backend & backend;
};
}
