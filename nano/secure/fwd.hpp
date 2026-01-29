#pragma once

#include <nano/lib/fwd.hpp>
#include <nano/store/fwd.hpp>

namespace nano
{
class account_delegator_by_weight_key;
class account_receivable_by_amount_info;
class account_receivable_by_amount_key;
class account_info;
class ext_ledger;
class ledger;
class ledger_cache;
class ledger_constants;
class ledger_options;
class network_params;
class pending_info;
class pending_key;

enum class block_status;
}

namespace nano::secure
{
class read_transaction;
class transaction;
class write_transaction;
}
