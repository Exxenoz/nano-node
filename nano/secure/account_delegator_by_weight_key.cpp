#include <nano/lib/stream.hpp>
#include <nano/secure/account_delegator_by_weight_key.hpp>
#include <nano/secure/ledger.hpp>

nano::account_delegator_by_weight_key::account_delegator_by_weight_key (nano::account const & represenative_a, nano::amount const & weight_a, nano::account const & delegator_a) :
	representative (represenative_a),
	weight (weight_a),
	delegator (delegator_a)
{
}

bool nano::account_delegator_by_weight_key::deserialize (nano::stream & stream_a)
{
	auto error (false);
	try
	{
		nano::read (stream_a, representative.bytes);
		nano::read (stream_a, weight.bytes);
		nano::read (stream_a, delegator.bytes);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

bool nano::account_delegator_by_weight_key::operator== (nano::account_delegator_by_weight_key const & other_a) const
{
	return representative == other_a.representative && weight == other_a.weight && delegator == other_a.delegator;
}

bool nano::account_delegator_by_weight_key::operator< (nano::account_delegator_by_weight_key const & other_a) const
{
	if (representative == other_a.representative)
	{
		if (weight == other_a.weight)
		{
			return delegator < other_a.delegator;
		}
		return weight < other_a.weight;
	}
	return representative < other_a.representative;
}
