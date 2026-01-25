#include <nano/lib/logging.hpp>
#include <nano/lib/stats.hpp>
#include <nano/store/backend.hpp>
#include <nano/store/ext_ledger/receive_block_by_send_block.hpp>
#include <nano/store/ext_ledger_store.hpp>
#include <nano/store/ledger_store.hpp>

namespace nano::store
{
nano::store::column_schema const ext_ledger_store::schema_current{
	{ nano::store::table::ext_receive_block_by_send_block, "ext_receive_block_by_send_block" },
	{ nano::store::table::meta, "meta" }
};
}

namespace nano::store
{
ext_ledger_store::ext_ledger_store (nano::store::backend & backend_a, nano::stats & stats_a, nano::logger & logger_a) :
	backend{ backend_a },
	stats{ stats_a },
	logger{ logger_a },
	meta_impl{ std::make_unique<nano::store::meta_view> (backend_a) },
	receive_block_by_send_block_impl{ std::make_unique<nano::store::ext_ledger::receive_block_by_send_block_view> (backend_a) },
	meta{ *meta_impl },
	receive_block_by_send_block{ *receive_block_by_send_block_impl }
{
}

ext_ledger_store::~ext_ledger_store () = default;

bool ext_ledger_store::is_initialized () const
{
	return initialized;
}

void ext_ledger_store::initialize (nano::store::backend_meta const & meta, nano::store::open_mode mode, bool backup_before_upgrade)
{
	release_assert (!is_initialized (), "Extended ledger store is already initialized");

	logger.info (nano::log::type::ext_ledger_store, "Initializing extended ledger store");

	// Prevent opening future database versions
	if (meta.ext_ledger_version > version_current)
	{
		logger.error (nano::log::type::ext_ledger_store, "The version of the extended ledger store ({}) is higher than the current ({}) which is supported. Either upgrade your node software or use a different database.", meta.ext_ledger_version, version_current);

		throw std::runtime_error ("Extended ledger store version " + std::to_string (meta.ext_ledger_version) + " is higher than current version " + std::to_string (version_current));
	}

	// Minimum supported upgrade version check
	if (meta.ext_ledger_version < version_minimum && meta.ext_ledger_version > 0)
	{
		logger.error (nano::log::type::ext_ledger_store, "The version of the extended ledger store ({}) is lower than the minimum ({}) which is supported for upgrades. Perform an intermediate upgrade with an older node version.", meta.ext_ledger_version, version_minimum);

		throw std::runtime_error ("Extended ledger store version " + std::to_string (meta.ext_ledger_version) + " is lower than minimum supported version " + std::to_string (version_minimum));
	}

	bool fresh_db = false;
	bool needs_upgrade = false;

	// Version 0 indicates that the extended ledger store has not been initialized yet
	if (meta.ext_ledger_version == 0)
	{
		fresh_db = true;

		logger.info (nano::log::type::ext_ledger_store, "Extended ledger store version is unset. Performing initial initialization.");
	}
	// Older but valid version detected - upgrade required
	else if (meta.ext_ledger_version < version_current)
	{
		needs_upgrade = true;

		logger.info (nano::log::type::ext_ledger_store, "Extended ledger store needs to be upgraded from version {} to {}", meta.ext_ledger_version, version_current);
	}

	// Either an existing version must be present, or this is a fresh initialization
	release_assert (meta.ext_ledger_version > 0 || fresh_db);

	if (fresh_db || needs_upgrade)
	{
		if (mode == nano::store::open_mode::read_only)
		{
			throw std::runtime_error ("Extended ledger store requires upgrade but was opened in read-only mode");
		}
	}

	if (fresh_db)
	{
		logger.info (nano::log::type::ext_ledger_store, "Creating new extended ledger store with version {}", version_current);

		// Create all extended ledger tables and write the initial store version
		backend.create (schema_current, version_current, nano::store::version_key::ext_ledger);
	}
	else if (needs_upgrade)
	{
		perform_upgrades (meta, backup_before_upgrade);
	}

	backend.open (backend::schema_meta, nano::store::open_mode::read_only);
	release_assert (backend.get_meta ().ext_ledger_version == version_current, "Extended ledger store version after initialization is not current");
	backend.close ();

	// Open backend with both base ledger and extended ledger schemas
	nano::store::column_schema merged_schema;
	merged_schema.insert (nano::store::ledger_store::schema_current.begin (), nano::store::ledger_store::schema_current.end ());
	merged_schema.insert (nano::store::ext_ledger_store::schema_current.begin (), nano::store::ext_ledger_store::schema_current.end ());
	backend.open (merged_schema, mode);

	initialized = true;
}

void ext_ledger_store::perform_upgrades (nano::store::backend_meta const & meta, bool backup_before_upgrade)
{
	debug_assert (meta.ext_ledger_version < version_current, "perform_upgrades called but no upgrade is necessary");
	release_assert (meta.ext_ledger_version >= version_minimum, "perform_upgrades called but version is below minimum supported version", std::to_string (meta.ext_ledger_version));

	if (backup_before_upgrade)
	{
		logger.info (nano::log::type::ext_ledger_store, "Creating ledger backup before upgrade...");

		backend.open (backend::schema_meta, nano::store::open_mode::read_only);
		backend.backup ();
		backend.close ();

		logger.info (nano::log::type::ext_ledger_store, "Ledger backup completed, continuing with upgrade...");
	}

	switch (meta.ext_ledger_version)
	{
		default:
			release_assert (false, "Invalid extended ledger store version for upgrade", std::to_string (meta.ext_ledger_version));
	}
}

uint64_t ext_ledger_store::count (nano::store::transaction const & txn, table table) const
{
	return backend.count (txn, table);
}

void ext_ledger_store::clear ()
{
	release_assert (is_initialized (), "Extended ledger store must be initialized");

	meta.put (tx_begin_write (), nano::store::meta_key::ext_ledger_flags, 0);

	// Clear extended ledger tables
	for (auto const & [table, name] : ext_ledger_store::schema_current)
	{
		if (table != nano::store::table::meta)
		{
			backend.clear (table);
		}
	}
}

void ext_ledger_store::drop ()
{
	release_assert (is_initialized (), "Extended ledger store must be initialized");

	meta.del (tx_begin_write (), nano::store::meta_key::ext_ledger_version);
	meta.del (tx_begin_write (), nano::store::meta_key::ext_ledger_flags);

	// Drop extended ledger tables
	for (auto const & [table, name] : ext_ledger_store::schema_current)
	{
		if (table != nano::store::table::meta)
		{
			backend.drop_table (name);
		}
	}

	initialized = false;
}

bool ext_ledger_store::empty (nano::store::transaction const & txn) const
{
	for (auto const & [table, table_name] : schema_current)
	{
		if (backend.begin (txn, table) != backend.end (txn, table))
		{
			return false;
		}
		debug_assert (backend.count (txn, table) == 0);
	}
	return true;
}

nano::store::ext_ledger_flags ext_ledger_store::get_flags (nano::store::transaction const & txn) const
{
	nano::store::ext_ledger_flags ext_ledger_flags = nano::store::ext_ledger_flags::none;
	if (auto ext_ledger_flags_opt = meta.get (txn, nano::store::meta_key::ext_ledger_flags))
	{
		ext_ledger_flags = static_cast<nano::store::ext_ledger_flags> (*ext_ledger_flags_opt);
	}
	return ext_ledger_flags;
}

bool ext_ledger_store::has_flags (nano::store::transaction const & txn, nano::store::ext_ledger_flags flags) const
{
	return (get_flags (txn) & flags) == flags;
}

void ext_ledger_store::add_flags (nano::store::write_transaction const & txn, nano::store::ext_ledger_flags flags)
{
	nano::store::ext_ledger_flags ext_ledger_flags = get_flags (txn) | flags;
	meta.put (txn, nano::store::meta_key::ext_ledger_flags, static_cast<nano::store::meta_value_t> (ext_ledger_flags));
}

nano::store::write_transaction ext_ledger_store::tx_begin_write ()
{
	return backend.tx_begin_write ();
}

nano::store::read_transaction ext_ledger_store::tx_begin_read () const
{
	return backend.tx_begin_read ();
}
}
