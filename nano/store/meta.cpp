#include <nano/store/backend.hpp>
#include <nano/store/db_val.hpp>
#include <nano/store/db_val_templ.hpp>
#include <nano/store/meta.hpp>

namespace nano::store
{
meta_view::meta_view (nano::store::backend & backend_a) :
	backend{ backend_a }
{
}

void meta_view::put (nano::store::write_transaction const & txn, nano::store::meta_key meta_key, nano::store::meta_value_t meta_value)
{
	nano::uint256_union db_key{ static_cast<uint64_t> (meta_key) };
	nano::uint256_union db_value{ meta_value };
	auto status = backend.put (txn, nano::store::table::meta, db_key, db_value);
	backend.release_assert_success (status);
}

void meta_view::del (nano::store::write_transaction const & txn, nano::store::meta_key meta_key)
{
	nano::uint256_union db_key{ static_cast<uint64_t> (meta_key) };
	auto status = backend.del (txn, nano::store::table::meta, db_key);
	backend.release_assert_success (status);
}

auto meta_view::get (nano::store::transaction const & txn, nano::store::meta_key meta_key) const -> std::optional<nano::store::meta_value_t>
{
	nano::uint256_union db_key{ static_cast<uint64_t> (meta_key) };
	nano::store::db_val data;
	auto status = backend.get (txn, nano::store::table::meta, db_key, data);
	std::optional<nano::store::meta_value_t> meta_value;
	if (backend.success (status))
	{
		nano::uint256_union db_value{ data };
		debug_assert (db_value.qwords[2] == 0 && db_value.qwords[1] == 0 && db_value.qwords[0] == 0);
		meta_value = db_value.number ().convert_to<nano::store::meta_value_t> ();
	}
	return meta_value;
}

bool meta_view::exists (nano::store::transaction const & txn, nano::store::meta_key meta_key) const
{
	nano::uint256_union db_key{ static_cast<uint64_t> (meta_key) };
	return backend.exists (txn, nano::store::table::meta, db_key);
}
}
