#pragma once

#include <nano/lib/fwd.hpp>
#include <nano/store/backend.hpp>
#include <nano/store/common.hpp>
#include <nano/store/fwd.hpp>
#include <nano/store/write_queue.hpp>

#include <filesystem>
#include <memory>

namespace nano::store
{
enum class ext_ledger_store_flags : uint64_t
{
	none = 0,
};

inline nano::store::ext_ledger_store_flags operator| (nano::store::ext_ledger_store_flags a, nano::store::ext_ledger_store_flags b)
{
	return static_cast<nano::store::ext_ledger_store_flags> (static_cast<uint64_t> (a) | static_cast<uint64_t> (b));
}
inline nano::store::ext_ledger_store_flags operator& (nano::store::ext_ledger_store_flags a, nano::store::ext_ledger_store_flags b)
{
	return static_cast<nano::store::ext_ledger_store_flags> (static_cast<uint64_t> (a) & static_cast<uint64_t> (b));
}

class ext_ledger_store
{
public:
	explicit ext_ledger_store (nano::store::ledger_store &, nano::store::backend &, nano::stats &, nano::logger &);
	~ext_ledger_store ();

	nano::store::write_transaction tx_begin_write ();
	nano::store::read_transaction tx_begin_read () const;

	bool is_initialized () const;
	void initialize (nano::store::backend_meta const &, nano::store::open_mode, bool backup_before_upgrade = false);
	void initialize_extended_data (nano::store::ext_ledger_store_flags flags);
	void perform_upgrades (nano::store::backend_meta const &, bool backup_before_upgrade);

	uint64_t count (nano::store::transaction const &, table) const;
	bool empty (nano::store::transaction const &) const;

private:
	nano::store::ledger_store & ledger;
	nano::store::backend & backend;
	nano::stats & stats;
	nano::logger & logger;
	bool initialized{ false };

public:
	static nano::store::version_value_t constexpr version_minimum{ 1 };
	static nano::store::version_value_t constexpr version_current{ 1 };

public:
	static nano::store::column_schema const schema_current;
};
};
