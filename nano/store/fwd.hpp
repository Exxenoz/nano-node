#pragma once

namespace nano::store
{
enum class ext_ledger_flags : uint64_t;
enum class table;

class backend;
class ext_ledger_store;
class ledger_store;
class meta_view;
class read_transaction;
class transaction;
class write_transaction;
}

namespace nano::store::ext_ledger
{
class receive_block_by_send_block_view;
}

namespace nano::store::ledger
{
class account_view;
class block_view;
class confirmation_height_view;
class final_vote_view;
class online_weight_view;
class peer_view;
class pending_view;
class pruned_view;
class successor_view;
class rep_weight_view;
}