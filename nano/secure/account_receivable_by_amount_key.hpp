#pragma once

#include <nano/lib/fwd.hpp>
#include <nano/lib/numbers.hpp>
#include <nano/lib/numbers_templ.hpp>
#include <nano/secure/fwd.hpp>

namespace nano
{
// This class represents the data written into the account receivables by amount database table key
class account_receivable_by_amount_key final
{
public:
	account_receivable_by_amount_key () = default;
	account_receivable_by_amount_key (nano::account const &, nano::amount const &, nano::block_hash const &);
	bool deserialize (nano::stream &);
	bool operator== (nano::account_receivable_by_amount_key const &) const;
	bool operator< (nano::account_receivable_by_amount_key const &) const;
	nano::account account{}; // receiving account
	nano::amount amount{ 0 }; // receivable amount
	nano::block_hash send_block_hash{ 0 }; // send block hash

	friend std::ostream & operator<< (std::ostream & os, const nano::account_receivable_by_amount_key & key)
	{
		os << "Account: " << key.account << ", Amount: " << key.amount << ", Send block hash: " << key.send_block_hash;
		return os;
	}
};
}

namespace std
{
template <>
struct hash<::nano::account_receivable_by_amount_key>
{
	size_t operator() (::nano::account_receivable_by_amount_key const & value) const
	{
		size_t hash = 0;
		boost::hash_combine (hash, value.account.number ());
		boost::hash_combine (hash, value.amount.number ());
		boost::hash_combine (hash, value.send_block_hash.number ());
		return hash;
	}
};
}
