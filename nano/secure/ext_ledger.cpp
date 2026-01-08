#include <nano/lib/logging.hpp>
#include <nano/lib/numbers.hpp>
#include <nano/lib/stats.hpp>
#include <nano/lib/utility.hpp>
#include <nano/secure/common.hpp>
#include <nano/secure/ext_ledger.hpp>
#include <nano/secure/ledger.hpp>
#include <nano/store/ext_ledger/receive_block_by_send_block.hpp>
#include <nano/store/ext_ledger_store.hpp>
#include <nano/store/ledger/block.hpp>
#include <nano/store/ledger_store.hpp>
#include <nano/store/meta.hpp>

nano::ext_ledger::ext_ledger (nano::ledger & ledger_a, nano::stats & stats_a, nano::logger & logger_a) :
	ledger{ ledger_a },
	stats{ stats_a },
	logger{ logger_a }
{
}

nano::ext_ledger::~ext_ledger ()
{
}

bool nano::ext_ledger::is_initialized ()
{
	return initialized;
}

void nano::ext_ledger::initialize ()
{
	nano::store::ledger_store & store = ledger.store;

	release_assert (!is_initialized (), "Extended ledger is already initialized");
	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");

	logger.info (nano::log::type::ext_ledger, "Initializing extended ledger");

	if (store.get_mode () != nano::store::open_mode::read_only)
	{
		auto has_flags = [&store] (nano::store::ext_ledger_flags flags) -> bool {
			return store.ext.has_flags (store.tx_begin_read (), flags);
		};

		if (!has_flags (nano::store::ext_ledger_flags::receive_block_by_send_block_initialized))
		{
			store.ext.receive_block_by_send_block.clear ();
			initialize_receive_block_by_send_block_index ();
		}
	}

	initialized = true;
}

void nano::ext_ledger::initialize_receive_block_by_send_block_index ()
{
	nano::store::ledger_store & store = ledger.store;
	nano::store::write_transaction txn = store.tx_begin_write ();

	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	release_assert (store.ext.receive_block_by_send_block.empty (txn), "The index must be cleared before rebuilding to avoid inconsistent or duplicate entries.");
	release_assert (store.get_mode () != nano::store::open_mode::read_only, "The index cannot be built while the backend is opened in read-only mode.");

	logger.info (nano::log::type::ext_ledger, "Building receive block by send block index from existing ledger data, this may take a while...");

	size_t processed = 0;
	size_t indexed = 0;
	size_t const batch_size = 100000;

	for (auto itr = store.block.begin (txn), end = store.block.end (txn); itr != end; ++itr)
	{
		auto const & sideband = itr->second;
		auto block = sideband.block;
		if (block->is_receive ())
		{
			nano::block_hash receive_block_hash = itr->first;
			nano::block_hash send_block_hash = block->source ();
			store.ext.receive_block_by_send_block.put (txn, send_block_hash, receive_block_hash);
			++indexed;
		}
		++processed;
		if (processed % batch_size == 0)
		{
			logger.info (nano::log::type::ext_ledger, "Build progress: processed {} blocks, indexed {} entries", processed, indexed);

			txn.refresh ();
		}
	}

	logger.info (nano::log::type::ext_ledger, "Build completed: processed {} blocks, indexed {} entries", processed, indexed);

	// Mark index as fully built and consistent with the current ledger state
	store.ext.add_flags (txn, nano::store::ext_ledger_flags::receive_block_by_send_block_initialized);
}

void nano::ext_ledger::on_put_block (nano::store::write_transaction const & txn, nano::block_hash const & hash, nano::block const & block)
{
	nano::store::ledger_store & store = ledger.store;
	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	if (block.is_receive ())
	{
		store.ext.receive_block_by_send_block.put (txn, block.source (), hash);
	}
}

void nano::ext_ledger::on_del_block (nano::store::write_transaction const & txn, nano::block_hash const & hash, nano::block const & block)
{
	nano::store::ledger_store & store = ledger.store;
	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	if (block.is_receive ())
	{
		store.ext.receive_block_by_send_block.del (txn, block.source ());
	}
}

void nano::ext_ledger::clear (nano::store::write_transaction const & txn)
{
	nano::store::ledger_store & store = ledger.store;

	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");

	store.ext.clear (txn);
}

void nano::ext_ledger::drop (nano::store::write_transaction const & txn)
{
	nano::store::ledger_store & store = ledger.store;

	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");

	store.ext.drop (txn);

	initialized = false;
}
