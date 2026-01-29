#include <nano/store/db_val_templ.hpp>
#include <nano/store/ext_ledger/account_delegators_by_weight.hpp>

namespace nano::store::ext_ledger
{
account_delegators_by_weight_view::account_delegators_by_weight_view (nano::store::backend & backend_a) :
	backend{ backend_a }
{
}

void account_delegators_by_weight_view::put (nano::store::write_transaction const & txn, nano::account_delegator_by_weight_key const & key)
{
	auto status = backend.put (txn, nano::store::table::ext_account_delegators_by_weight, key, nullptr);
	backend.release_assert_success (status);
}

void account_delegators_by_weight_view::del (nano::store::write_transaction const & txn, nano::account_delegator_by_weight_key const & key)
{
	release_assert (backend.exists (txn, nano::store::table::ext_account_delegators_by_weight, key), "Could not find account delegator by weight key");
	auto status = backend.del (txn, nano::store::table::ext_account_delegators_by_weight, key);
	backend.release_assert_success (status);
}

bool account_delegators_by_weight_view::empty (nano::store::transaction const & txn) const
{
	return backend.empty (txn, nano::store::table::ext_account_delegators_by_weight);
}

void account_delegators_by_weight_view::clear ()
{
	auto status = backend.clear (nano::store::table::ext_account_delegators_by_weight);
	backend.release_assert_success (status);
}

auto account_delegators_by_weight_view::begin (nano::store::transaction const & txn, nano::account_delegator_by_weight_key const & key) const -> iterator
{
	return iterator{ backend.begin (txn, nano::store::table::ext_account_delegators_by_weight, key) };
}

auto account_delegators_by_weight_view::begin (nano::store::transaction const & txn) const -> iterator
{
	return iterator{ backend.begin (txn, nano::store::table::ext_account_delegators_by_weight) };
}

auto account_delegators_by_weight_view::end (nano::store::transaction const & txn) const -> iterator
{
	return iterator{ backend.end (txn, nano::store::table::ext_account_delegators_by_weight) };
}

auto account_delegators_by_weight_view::upper_bound (nano::store::transaction const & txn, nano::account const & representative) const -> iterator
{
	auto it = representative.number () == std::numeric_limits<nano::account::underlying_type>::max () ? end (txn) : begin (txn, nano::account_delegator_by_weight_key (representative.number () + 1, 0, 0));
	if (it == begin (txn))
	{
		return end (txn);
	}
	--it;
	if (it->first.representative == representative)
	{
		return iterator{ std::move (it) };
	}
	return end (txn);
}

auto account_delegators_by_weight_view::rupper_bound (nano::store::transaction const & txn, nano::account const & representative) const -> reverse_iterator
{
	return reverse_iterator{ upper_bound (txn, representative) };
}

auto account_delegators_by_weight_view::rend (nano::store::transaction const & txn) const -> reverse_iterator
{
	return reverse_iterator{ end (txn) };
}
}
