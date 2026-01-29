#pragma once

#include <nano/lib/numbers.hpp>
#include <nano/secure/account_receivable_by_amount_info.hpp>
#include <nano/secure/account_receivable_by_amount_key.hpp>
#include <nano/store/backend.hpp>
#include <nano/store/reverse_iterator.hpp>
#include <nano/store/reverse_iterator_templ.hpp>
#include <nano/store/typed_iterator.hpp>
#include <nano/store/typed_iterator_templ.hpp>

#include <functional>

namespace nano::store::ext_ledger
{
class account_receivables_by_amount_view
{
public:
	using iterator = store::typed_iterator<nano::account_receivable_by_amount_key, nano::account_receivable_by_amount_info>;
	using reverse_iterator = store::reverse_iterator<iterator>;

public:
	explicit account_receivables_by_amount_view (nano::store::backend &);

	void put (nano::store::write_transaction const &, nano::account_receivable_by_amount_key const &, nano::account_receivable_by_amount_info const &);
	void del (nano::store::write_transaction const &, nano::account_receivable_by_amount_key const &);
	bool empty (nano::store::transaction const &) const;
	void clear ();
	iterator begin (nano::store::transaction const &, nano::account_receivable_by_amount_key const &) const;
	iterator begin (nano::store::transaction const &) const;
	iterator end (nano::store::transaction const &) const;
	iterator upper_bound (nano::store::transaction const &, nano::account const &) const;
	reverse_iterator rupper_bound (nano::store::transaction const &, nano::account const &) const;
	reverse_iterator rend (nano::store::transaction const &) const;

private:
	nano::store::backend & backend;
};
}
