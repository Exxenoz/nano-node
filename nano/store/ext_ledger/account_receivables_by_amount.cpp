#include <nano/store/db_val_templ.hpp>
#include <nano/store/ext_ledger/account_receivables_by_amount.hpp>

namespace nano::store::ext_ledger
{
account_receivables_by_amount_view::account_receivables_by_amount_view (nano::store::backend & backend_a) :
	backend{ backend_a }
{
}

void account_receivables_by_amount_view::put (nano::store::write_transaction const & txn, nano::account_receivable_by_amount_key const & key, nano::account_receivable_by_amount_info const & info)
{
	auto status = backend.put (txn, nano::store::table::ext_account_receivables_by_amount, key, info);
	backend.release_assert_success (status);
}

void account_receivables_by_amount_view::del (nano::store::write_transaction const & txn, nano::account_receivable_by_amount_key const & key)
{
	release_assert (backend.exists (txn, nano::store::table::ext_account_receivables_by_amount, key), "Could not find account receivable by amount key");
	auto status = backend.del (txn, nano::store::table::ext_account_receivables_by_amount, key);
	backend.release_assert_success (status);
}

bool account_receivables_by_amount_view::empty (nano::store::transaction const & txn) const
{
	return backend.empty (txn, nano::store::table::ext_account_receivables_by_amount);
}

void account_receivables_by_amount_view::clear ()
{
	auto status = backend.clear (nano::store::table::ext_account_receivables_by_amount);
	backend.release_assert_success (status);
}

auto account_receivables_by_amount_view::begin (nano::store::transaction const & txn, nano::account_receivable_by_amount_key const & key) const -> iterator
{
	return iterator{ backend.begin (txn, nano::store::table::ext_account_receivables_by_amount, key) };
}

auto account_receivables_by_amount_view::begin (nano::store::transaction const & txn) const -> iterator
{
	return iterator{ backend.begin (txn, nano::store::table::ext_account_receivables_by_amount) };
}

auto account_receivables_by_amount_view::end (nano::store::transaction const & txn) const -> iterator
{
	return iterator{ backend.end (txn, nano::store::table::ext_account_receivables_by_amount) };
}

auto account_receivables_by_amount_view::upper_bound (nano::store::transaction const & txn, nano::account const & account) const -> iterator
{
	auto it = account.number () == std::numeric_limits<nano::account::underlying_type>::max () ? end (txn) : begin (txn, nano::account_receivable_by_amount_key (account.number () + 1, 0, 0));
	if (it == begin (txn))
	{
		return end (txn);
	}
	--it;
	if (it->first.account == account)
	{
		return iterator{ std::move (it) };
	}
	return end (txn);
}

auto account_receivables_by_amount_view::rupper_bound (nano::store::transaction const & txn, nano::account const & account) const -> reverse_iterator
{
	return reverse_iterator{ upper_bound (txn, account) };
}

auto account_receivables_by_amount_view::rend (nano::store::transaction const & txn) const -> reverse_iterator
{
	return reverse_iterator{ end (txn) };
}
}
