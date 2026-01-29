#include <nano/lib/stream.hpp>
#include <nano/secure/account_receivable_by_amount_key.hpp>
#include <nano/secure/ledger.hpp>

nano::account_receivable_by_amount_key::account_receivable_by_amount_key (nano::account const & account_a, nano::amount const & amount_a, nano::block_hash const & send_block_hash_a) :
	account (account_a),
	amount (amount_a),
	send_block_hash (send_block_hash_a)
{
}

bool nano::account_receivable_by_amount_key::deserialize (nano::stream & stream_a)
{
	auto error (false);
	try
	{
		nano::read (stream_a, account.bytes);
		nano::read (stream_a, amount.bytes);
		nano::read (stream_a, send_block_hash.bytes);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

bool nano::account_receivable_by_amount_key::operator== (nano::account_receivable_by_amount_key const & other_a) const
{
	return account == other_a.account && amount == other_a.amount && send_block_hash == other_a.send_block_hash;
}

bool nano::account_receivable_by_amount_key::operator< (nano::account_receivable_by_amount_key const & other_a) const
{
	if (account == other_a.account)
	{
		if (amount == other_a.amount)
		{
			return send_block_hash < other_a.send_block_hash;
		}
		return amount < other_a.amount;
	}
	return account < other_a.account;
}
