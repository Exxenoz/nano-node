#include <nano/lib/logging.hpp>
#include <nano/lib/numbers.hpp>
#include <nano/lib/stats.hpp>
#include <nano/lib/utility.hpp>
#include <nano/secure/common.hpp>
#include <nano/secure/ext_ledger.hpp>
#include <nano/secure/ledger.hpp>
#include <nano/store/ext_ledger_store.hpp>
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
	}

	initialized = true;
}
