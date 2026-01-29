#pragma once

#include <nano/lib/epoch.hpp>
#include <nano/lib/fwd.hpp>
#include <nano/lib/numbers.hpp>
#include <nano/lib/numbers_templ.hpp>
#include <nano/secure/fwd.hpp>

namespace nano
{
// This class represents the data written into the account delegators by weight database table key
class account_delegator_by_weight_key final
{
public:
	account_delegator_by_weight_key () = default;
	account_delegator_by_weight_key (nano::account const &, nano::amount const &, nano::account const &);
	bool deserialize (nano::stream &);
	bool operator== (nano::account_delegator_by_weight_key const &) const;
	bool operator< (nano::account_delegator_by_weight_key const &) const;
	nano::account representative{}; // representative account
	nano::amount weight{ 0 }; // weight of the delegator account
	nano::account delegator{}; // delegator account

	friend std::ostream & operator<< (std::ostream & os, const nano::account_delegator_by_weight_key & key)
	{
		os << "Representative: " << key.representative << ", Weight: " << key.weight << ", Delegator: " << key.delegator;
		return os;
	}
};
}

namespace std
{
template <>
struct hash<::nano::account_delegator_by_weight_key>
{
	size_t operator() (::nano::account_delegator_by_weight_key const & value) const
	{
		size_t hash = 0;
		boost::hash_combine (hash, value.representative.number ());
		boost::hash_combine (hash, value.weight.number ());
		boost::hash_combine (hash, value.delegator.number ());
		return hash;
	}
};
}
