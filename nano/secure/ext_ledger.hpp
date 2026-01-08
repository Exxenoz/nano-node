#pragma once

#include <nano/lib/numbers.hpp>
#include <nano/secure/common.hpp>
#include <nano/secure/fwd.hpp>
#include <nano/secure/transaction.hpp>

#include <memory>

namespace nano
{
class ext_ledger final
{
public:
	ext_ledger (nano::ledger &, nano::stats &, nano::logger &);
	~ext_ledger ();

	bool is_initialized ();
	void initialize ();
	void initialize_receive_block_by_send_block_index ();

	void on_put_block (nano::store::write_transaction const &, nano::block_hash const &, nano::block const &);
	void on_del_block (nano::store::write_transaction const &, nano::block_hash const &, nano::block const &);

	void clear (nano::store::write_transaction const &);
	void drop (nano::store::write_transaction const &);

private:
	nano::ledger & ledger;
	nano::stats & stats;
	nano::logger & logger;
	bool initialized{ false };
};
}
