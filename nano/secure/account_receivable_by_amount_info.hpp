#pragma once

#include <nano/lib/epoch.hpp>
#include <nano/lib/fwd.hpp>
#include <nano/lib/numbers.hpp>
#include <nano/lib/numbers_templ.hpp>
#include <nano/secure/fwd.hpp>

namespace nano
{
/**
 * Information stored in the account receivables by amount index
 */
class account_receivable_by_amount_info final
{
public:
	account_receivable_by_amount_info () = default;
	account_receivable_by_amount_info (nano::account const &, nano::epoch);
	size_t db_size () const;
	bool deserialize (nano::stream &);
	bool operator== (nano::account_receivable_by_amount_info const &) const;
	nano::account source{}; // the account sending the funds
	nano::epoch epoch{ nano::epoch::epoch_0 }; // epoch of sending block

	friend std::ostream & operator<< (std::ostream & os, const nano::account_receivable_by_amount_info & info)
	{
		const int epoch = nano::normalized_epoch (info.epoch);
		os << "Source: " << info.source << ", Epoch: " << epoch;
		return os;
	}
};
}
