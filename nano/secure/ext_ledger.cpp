#include <nano/lib/logging.hpp>
#include <nano/lib/numbers.hpp>
#include <nano/lib/stats.hpp>
#include <nano/lib/thread_roles.hpp>
#include <nano/lib/utility.hpp>
#include <nano/secure/common.hpp>
#include <nano/secure/ext_ledger.hpp>
#include <nano/secure/ledger.hpp>
#include <nano/store/ext_ledger/account_delegators_by_weight.hpp>
#include <nano/store/ext_ledger/account_receivables_by_amount.hpp>
#include <nano/store/ext_ledger/receive_block_by_send_block.hpp>
#include <nano/store/ext_ledger_store.hpp>
#include <nano/store/ledger/account.hpp>
#include <nano/store/ledger/block.hpp>
#include <nano/store/ledger/pending.hpp>
#include <nano/store/ledger_store.hpp>
#include <nano/store/meta.hpp>

#include <boost/lockfree/queue.hpp>

#include <atomic>

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

void nano::ext_ledger::initialize (nano::ledger_options const & options)
{
	nano::store::ledger_store & store = ledger.store;

	release_assert (!is_initialized (), "Extended ledger is already initialized");
	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");

	logger.info (nano::log::type::ext_ledger, "Initializing extended ledger");

	if (!options.inactive_node && store.get_mode () != nano::store::open_mode::read_only)
	{
		auto has_flags = [&store] (nano::store::ext_ledger_flags flags) -> bool {
			return store.ext.has_flags (store.tx_begin_read (), flags);
		};

		if (!has_flags (nano::store::ext_ledger_flags::account_delegators_by_weight_initialized))
		{
			store.ext.account_delegators_by_weight.clear ();
			initialize_account_delegators_by_weight_index ();
		}

		if (!has_flags (nano::store::ext_ledger_flags::account_receivables_by_amount_initialized))
		{
			store.ext.account_receivables_by_amount.clear ();
			initialize_account_receivables_by_amount_index ();
		}

		if (!has_flags (nano::store::ext_ledger_flags::receive_block_by_send_block_initialized))
		{
			store.ext.receive_block_by_send_block.clear ();
			initialize_receive_block_by_send_block_index ();
		}
	}

	initialized = true;
}

void nano::ext_ledger::initialize_account_delegators_by_weight_index ()
{
	nano::store::ledger_store & store = ledger.store;

	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	release_assert (store.ext.account_delegators_by_weight.empty (store.tx_begin_read ()), "The index must be cleared before rebuilding to avoid inconsistent or duplicate entries.");
	release_assert (store.get_mode () != nano::store::open_mode::read_only, "The index cannot be built while the backend is opened in read-only mode.");

	logger.info (nano::log::type::ext_ledger, "Building account delegators by weight index from existing ledger data, this may take a while...");

	struct queue_entry
	{
		nano::account representative;
		nano::amount weight;
		nano::account delegator;
	};
	static_assert (std::is_trivially_copyable_v<queue_entry>);

	std::atomic<bool> scan_completed{ false };
	std::atomic<uint64_t> processed{ 0 };
	std::atomic<uint64_t> indexed{ 0 };
	auto queue = std::make_unique<boost::lockfree::queue<queue_entry, boost::lockfree::capacity<1024 * 16>>> ();

	std::thread writer_thread = std::thread ([&] {
		nano::thread_role::set (nano::thread_role::name::ext_ledger_writer);
		nano::store::write_transaction txn = store.tx_begin_write ();
		for (queue_entry entry{}; !queue->empty () || !scan_completed.load ();)
		{
			if (queue->pop (entry))
			{
				store.ext.account_delegators_by_weight.put (txn, nano::account_delegator_by_weight_key (entry.representative, entry.weight, entry.delegator));

				++indexed;
			}
			else
			{
				std::this_thread::yield ();
			}
		}
	});

	store.account.for_each_par ([&] (nano::store::read_transaction const &, nano::store::ledger::account_view::iterator begin_it, nano::store::ledger::account_view::iterator end_it) {
		size_t const processed_log_interval = 100000;
		for (auto it = std::move (begin_it); it != end_it; ++it)
		{
			nano::account const & account = it->first;
			nano::account_info const & account_info = it->second;
			while (!queue->push (queue_entry{ .representative = account_info.representative, .weight = account_info.balance, .delegator = account }))
			{
				std::this_thread::sleep_for (std::chrono::microseconds (100));
			}
			auto current_processed = ++processed;
			if (current_processed % processed_log_interval == 0)
			{
				logger.info (nano::log::type::ext_ledger, "Build progress: processed {} accounts, indexed {} entries", current_processed, indexed.load ());
			}
		}
	});

	scan_completed = true;

	writer_thread.join ();

	logger.info (nano::log::type::ext_ledger, "Build completed: processed {} accounts, indexed {} entries", processed.load (), indexed.load ());

	// Mark index as fully built and consistent with the current ledger state
	store.ext.add_flags (store.tx_begin_write (), nano::store::ext_ledger_flags::account_delegators_by_weight_initialized);
}

void nano::ext_ledger::initialize_account_receivables_by_amount_index ()
{
	nano::store::ledger_store & store = ledger.store;

	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	release_assert (store.ext.account_receivables_by_amount.empty (store.tx_begin_read ()), "The index must be cleared before rebuilding to avoid inconsistent or duplicate entries.");
	release_assert (store.get_mode () != nano::store::open_mode::read_only, "The index cannot be built while the backend is opened in read-only mode.");

	logger.info (nano::log::type::ext_ledger, "Building account receivables by amount index from existing ledger data, this may take a while...");

	struct queue_entry
	{
		nano::account account;
		nano::amount amount;
		nano::block_hash send_block_hash;
		nano::account source;
		nano::epoch epoch;
	};
	static_assert (std::is_trivially_copyable_v<queue_entry>);

	std::atomic<bool> scan_completed{ false };
	std::atomic<uint64_t> processed{ 0 };
	std::atomic<uint64_t> indexed{ 0 };
	auto queue = std::make_unique<boost::lockfree::queue<queue_entry, boost::lockfree::capacity<1024 * 16>>>();

	std::thread writer_thread = std::thread ([&] {
		nano::thread_role::set (nano::thread_role::name::ext_ledger_writer);
		nano::store::write_transaction txn = store.tx_begin_write ();
		for (queue_entry entry{}; !queue->empty () || !scan_completed.load ();)
		{
			if (queue->pop (entry))
			{
				store.ext.account_receivables_by_amount.put (txn, nano::account_receivable_by_amount_key (entry.account, entry.amount, entry.send_block_hash), nano::account_receivable_by_amount_info (entry.source, entry.epoch));

				++indexed;
			}
			else
			{
				std::this_thread::yield ();
			}
		}
	});

	store.pending.for_each_par ([&] (nano::store::read_transaction const &, nano::store::ledger::pending_view::iterator begin_it, nano::store::ledger::pending_view::iterator end_it) {
		size_t const processed_log_interval = 100000;
		for (auto it = std::move (begin_it); it != end_it; ++it)
		{
			nano::pending_key const & key = it->first;
			nano::pending_info const & info = it->second;
			while (!queue->push (queue_entry{ .account = key.account, .amount = info.amount, .send_block_hash = key.hash, .source = info.source, .epoch = info.epoch }))
			{
				std::this_thread::sleep_for (std::chrono::microseconds (100));
			}
			auto current_processed = ++processed;
			if (current_processed % processed_log_interval == 0)
			{
				logger.info (nano::log::type::ext_ledger, "Build progress: processed {} receivables, indexed {} entries", current_processed, indexed.load ());
			}
		}
	});

	scan_completed = true;

	writer_thread.join ();

	logger.info (nano::log::type::ext_ledger, "Build completed: processed {} receivables, indexed {} entries", processed.load (), indexed.load ());

	// Mark index as fully built and consistent with the current ledger state
	store.ext.add_flags (store.tx_begin_write (), nano::store::ext_ledger_flags::account_receivables_by_amount_initialized);
}

void nano::ext_ledger::initialize_receive_block_by_send_block_index ()
{
	nano::store::ledger_store & store = ledger.store;

	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	release_assert (store.ext.receive_block_by_send_block.empty (store.tx_begin_read ()), "The index must be cleared before rebuilding to avoid inconsistent or duplicate entries.");
	release_assert (store.get_mode () != nano::store::open_mode::read_only, "The index cannot be built while the backend is opened in read-only mode.");

	logger.info (nano::log::type::ext_ledger, "Building receive block by send block index from existing ledger data, this may take a while...");

	struct queue_entry
	{
		nano::block_hash send_block_hash;
		nano::block_hash receive_block_hash;
	};
	static_assert (std::is_trivially_copyable_v<queue_entry>);

	std::atomic<bool> scan_completed{ false };
	std::atomic<uint64_t> processed{ 0 };
	std::atomic<uint64_t> indexed{ 0 };
	auto queue = std::make_unique<boost::lockfree::queue<queue_entry, boost::lockfree::capacity<1024 * 16>>> ();

	std::thread writer_thread = std::thread ([&] {
		nano::thread_role::set (nano::thread_role::name::ext_ledger_writer);
		nano::store::write_transaction txn = store.tx_begin_write ();
		for (queue_entry entry{}; !queue->empty () || !scan_completed.load ();)
		{
			if (queue->pop (entry))
			{
				store.ext.receive_block_by_send_block.put (txn, entry.send_block_hash, entry.receive_block_hash);

				++indexed;
			}
			else
			{
				std::this_thread::yield ();
			}
		}
	});

	store.block.for_each_par ([&] (nano::store::read_transaction const &, nano::store::ledger::block_view::iterator begin_it, nano::store::ledger::block_view::iterator end_it) {
		size_t const processed_log_interval = 100000;
		for (auto it = std::move (begin_it); it != end_it; ++it)
		{
			auto const & sideband = it->second;
			auto block = sideband.block;
			if (block->is_receive ())
			{
				while (!queue->push (queue_entry{ .send_block_hash = block->source (), .receive_block_hash = it->first }))
				{
					std::this_thread::sleep_for (std::chrono::microseconds (100));
				}
			}
			auto current_processed = ++processed;
			if (current_processed % processed_log_interval == 0)
			{
				logger.info (nano::log::type::ext_ledger, "Build progress: processed {} blocks, indexed {} entries", current_processed, indexed.load ());
			}
		}
	});

	scan_completed = true;

	writer_thread.join ();

	logger.info (nano::log::type::ext_ledger, "Build completed: processed {} blocks, indexed {} entries", processed.load (), indexed.load ());

	// Mark index as fully built and consistent with the current ledger state
	store.ext.add_flags (store.tx_begin_write (), nano::store::ext_ledger_flags::receive_block_by_send_block_initialized);
}

void nano::ext_ledger::on_put_account (nano::store::write_transaction const & txn, nano::account const & account, nano::account_info const & account_info)
{
	nano::store::ledger_store & store = ledger.store;
	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	store.ext.account_delegators_by_weight.put (txn, nano::account_delegator_by_weight_key (account_info.representative, account_info.balance, account));
}

void nano::ext_ledger::on_del_account (nano::store::write_transaction const & txn, nano::account const & account, nano::account_info const & account_info)
{
	nano::store::ledger_store & store = ledger.store;
	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	store.ext.account_delegators_by_weight.del (txn, nano::account_delegator_by_weight_key (account_info.representative, account_info.balance, account));
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

void nano::ext_ledger::on_put_receivable (nano::store::write_transaction const & txn, nano::pending_key const & key, nano::pending_info const & info)
{
	nano::store::ledger_store & store = ledger.store;
	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	store.ext.account_receivables_by_amount.put (txn, nano::account_receivable_by_amount_key (key.account, info.amount, key.hash), nano::account_receivable_by_amount_info (info.source, info.epoch));
}

void nano::ext_ledger::on_del_receivable (nano::store::write_transaction const & txn, nano::pending_key const & key, nano::pending_info const & info)
{
	nano::store::ledger_store & store = ledger.store;
	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");
	store.ext.account_receivables_by_amount.del (txn, nano::account_receivable_by_amount_key (key.account, info.amount, key.hash));
}

void nano::ext_ledger::clear ()
{
	nano::store::ledger_store & store = ledger.store;

	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");

	store.ext.clear ();
}

void nano::ext_ledger::drop ()
{
	nano::store::ledger_store & store = ledger.store;

	release_assert (store.ext.is_initialized (), "Extended ledger store must be initialized");

	store.ext.drop ();

	initialized = false;
}
