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

private:
	nano::ledger & ledger;
	nano::stats & stats;
	nano::logger & logger;
	bool initialized{ false };
};
}
