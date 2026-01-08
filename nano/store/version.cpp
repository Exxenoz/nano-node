#include <nano/store/backend.hpp>
#include <nano/store/db_val_templ.hpp>
#include <nano/store/version.hpp>

namespace nano::store
{
version_view::version_view (nano::store::backend & backend_a) :
	backend{ backend_a }
{
}

void version_view::put (nano::store::write_transaction const & txn, nano::store::version_key version_key, nano::store::version_value_t version)
{
	backend.set_meta_value (txn, to_meta_key (version_key), version);
}

nano::store::version_value_t version_view::get (nano::store::transaction const & txn, nano::store::version_key version_key) const
{
	if (auto version = backend.get_meta_value (txn, to_meta_key (version_key)))
	{
		return *version;
	}
	return 0; // Default minimum version
}

bool version_view::exists (nano::store::transaction const & txn, nano::store::version_key version_key) const
{
	return backend.exists_meta_value (txn, to_meta_key (version_key));
}

nano::store::meta_key version_view::to_meta_key (nano::store::version_key version_key) const
{
	switch (version_key)
	{
		case nano::store::version_key::ledger:
			return nano::store::meta_key::ledger_version;
		case nano::store::version_key::ext_ledger:
			return nano::store::meta_key::ext_ledger_version;
		default:
			release_assert (false);
	}
}
}
