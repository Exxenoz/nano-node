#pragma once

#include <nano/lib/config.hpp>
#include <nano/lib/diagnosticsconfig.hpp>
#include <nano/lib/errors.hpp>
#include <nano/lib/lmdbconfig.hpp>
#include <nano/lib/logging.hpp>
#include <nano/lib/numbers.hpp>
#include <nano/lib/rocksdbconfig.hpp>
#include <nano/lib/stats.hpp>
#include <nano/node/active_elections.hpp>
#include <nano/node/blockprocessor.hpp>
#include <nano/node/bootstrap/bootstrap_config.hpp>
#include <nano/node/bootstrap/bootstrap_server.hpp>
#include <nano/node/confirming_set.hpp>
#include <nano/node/ipc/ipc_config.hpp>
#include <nano/node/local_block_broadcaster.hpp>
#include <nano/node/message_processor.hpp>
#include <nano/node/monitor.hpp>
#include <nano/node/network.hpp>
#include <nano/node/peer_history.hpp>
#include <nano/node/repcrawler.hpp>
#include <nano/node/request_aggregator.hpp>
#include <nano/node/scheduler/bucket.hpp>
#include <nano/node/scheduler/hinted.hpp>
#include <nano/node/scheduler/optimistic.hpp>
#include <nano/node/scheduler/priority.hpp>
#include <nano/node/transport/tcp_listener.hpp>
#include <nano/node/vote_cache.hpp>
#include <nano/node/vote_processor.hpp>
#include <nano/node/websocketconfig.hpp>
#include <nano/secure/common.hpp>
#include <nano/secure/generate_cache_flags.hpp>

#include <chrono>
#include <optional>
#include <vector>

namespace nano
{
class tomlconfig;

enum class frontiers_confirmation_mode : uint8_t
{
	always, // Always confirm frontiers
	automatic, // Always mode if node contains representative with at least 50% of principal weight, less frequest requests if not
	disabled, // Do not confirm frontiers
	invalid
};

class message_processor_config;

/**
 * Node configuration
 */
class node_config
{
public:
	// TODO: Users of this class rely on the default copy consturctor. This prevents using unique_ptrs with forward declared types.
	node_config (nano::network_params & network_params = nano::dev::network_params);
	node_config (const std::optional<uint16_t> &, nano::network_params & network_params = nano::dev::network_params);
	~node_config ();

	nano::error serialize_toml (nano::tomlconfig &) const;
	nano::error deserialize_toml (nano::tomlconfig &);

	bool upgrade_json (unsigned, nano::jsonconfig &);
	nano::account random_representative () const;

	nano::network_params network_params;
	std::optional<uint16_t> peering_port{};
	nano::scheduler::optimistic_config optimistic_scheduler;
	nano::scheduler::hinted_config hinted_scheduler;
	nano::scheduler::priority_config priority_scheduler;
	nano::scheduler::priority_bucket_config priority_bucket;
	std::vector<std::pair<std::string, uint16_t>> work_peers;
	std::vector<std::pair<std::string, uint16_t>> secondary_work_peers{ { "127.0.0.1", 8076 } }; /* Default of nano-pow-server */
	std::vector<std::string> preconfigured_peers;
	std::vector<nano::account> preconfigured_representatives;
	unsigned bootstrap_fraction_numerator{ 1 };
	nano::amount receive_minimum{ nano::xrb_ratio };
	nano::amount vote_minimum{ nano::Gxrb_ratio };
	nano::amount rep_crawler_weight_minimum{ "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF" };
	std::chrono::milliseconds vote_generator_delay{ std::chrono::milliseconds (100) };
	unsigned vote_generator_threshold{ 3 };
	nano::amount online_weight_minimum{ 60000 * nano::Gxrb_ratio };
	/*
	 * The minimum vote weight that a representative must have for its vote to be counted.
	 * All representatives above this weight will be kept in memory!
	 */
	nano::amount representative_vote_weight_minimum{ 10 * nano::Mxrb_ratio };
	unsigned password_fanout{ 1024 };
	unsigned io_threads{ env_io_threads ().value_or (std::max (4u, nano::hardware_concurrency ())) };
	unsigned network_threads{ std::max (4u, nano::hardware_concurrency ()) };
	unsigned work_threads{ std::max (4u, nano::hardware_concurrency ()) };
	unsigned background_threads{ std::max (4u, nano::hardware_concurrency ()) };
	/* Use half available threads on the system for signature checking. The calling thread does checks as well, so these are extra worker threads */
	unsigned signature_checker_threads{ std::max (2u, nano::hardware_concurrency () / 2) };
	bool enable_voting{ false };
	unsigned bootstrap_connections{ 4 };
	unsigned bootstrap_connections_max{ 64 };
	unsigned bootstrap_initiator_threads{ 1 };
	unsigned bootstrap_serving_threads{ 1 };
	uint32_t bootstrap_frontier_request_count{ 1024 * 1024 };
	nano::websocket::config websocket_config;
	nano::diagnostics_config diagnostics_config;
	std::string callback_address;
	uint16_t callback_port{ 0 };
	std::string callback_target;
	bool allow_local_peers{ !(network_params.network.is_live_network () || network_params.network.is_test_network ()) }; // disable by default for live network
	nano::stats_config stats_config;
	nano::ipc::ipc_config ipc_config;
	std::string external_address;
	uint16_t external_port{ 0 };
	std::chrono::milliseconds block_processor_batch_max_time{ std::chrono::milliseconds (500) };
	std::chrono::seconds unchecked_cutoff_time{ std::chrono::seconds (4 * 60 * 60) }; // 4 hours
	/** Timeout for initiated async operations */
	std::chrono::seconds tcp_io_timeout{ (network_params.network.is_dev_network () && !is_sanitizer_build ()) ? std::chrono::seconds (5) : std::chrono::seconds (15) };
	std::chrono::nanoseconds pow_sleep_interval{ 0 };

	/** Default maximum incoming TCP connections, including realtime network & bootstrap */
	unsigned tcp_incoming_connections_max{ 2048 };
	bool use_memory_pools{ true };
	static std::chrono::minutes constexpr wallet_backup_interval = std::chrono::minutes (5);
	/** Default outbound traffic shaping is 10MB/s */
	std::size_t bandwidth_limit{ 10 * 1024 * 1024 };
	/** By default, allow bursts of 15MB/s (not sustainable) */
	double bandwidth_limit_burst_ratio{ 3. };
	/** Default boostrap outbound traffic limit is 5MB/s */
	std::size_t bootstrap_bandwidth_limit{ 5 * 1024 * 1024 };
	/** Bootstrap traffic does not need bursts */
	double bootstrap_bandwidth_burst_ratio{ 1. };
	nano::bootstrap_ascending_config bootstrap_ascending;
	nano::bootstrap_server_config bootstrap_server;
	std::chrono::milliseconds confirming_set_batch_time{ 250 };
	bool backup_before_upgrade{ false };
	double max_work_generate_multiplier{ 64. };
	uint32_t max_queued_requests{ 512 };
	unsigned request_aggregator_threads{ std::min (nano::hardware_concurrency (), 4u) }; // Max 4 threads if available
	unsigned max_unchecked_blocks{ 65536 };
	std::chrono::seconds max_pruning_age{ !network_params.network.is_beta_network () ? std::chrono::seconds (24 * 60 * 60) : std::chrono::seconds (5 * 60) }; // 1 day; 5 minutes for beta network
	uint64_t max_pruning_depth{ 0 };
	nano::rocksdb_config rocksdb_config;
	nano::lmdb_config lmdb_config;
	nano::frontiers_confirmation_mode frontiers_confirmation{ nano::frontiers_confirmation_mode::automatic };
	/** Number of accounts per second to process when doing backlog population scan */
	unsigned backlog_scan_batch_size{ 10 * 1000 };
	/** Number of times per second to run backlog population batches. Number of accounts per single batch is `backlog_scan_batch_size / backlog_scan_frequency` */
	unsigned backlog_scan_frequency{ 10 };
	bool enable_upnp{ true };
	nano::vote_cache_config vote_cache;
	nano::rep_crawler_config rep_crawler;
	nano::block_processor_config block_processor;
	nano::active_elections_config active_elections;
	nano::vote_processor_config vote_processor;
	nano::peer_history_config peer_history;
	nano::transport::tcp_config tcp;
	nano::request_aggregator_config request_aggregator;
	nano::message_processor_config message_processor;
	nano::network_config network;
	nano::local_block_broadcaster_config local_block_broadcaster;
	nano::confirming_set_config confirming_set;
	nano::monitor_config monitor;

public:
	std::string serialize_frontiers_confirmation (nano::frontiers_confirmation_mode) const;
	nano::frontiers_confirmation_mode deserialize_frontiers_confirmation (std::string const &);
	/** Entry is ignored if it cannot be parsed as a valid address:port */
	void deserialize_address (std::string const &, std::vector<std::pair<std::string, uint16_t>> &) const;

private:
	static std::optional<unsigned> env_io_threads ();
};

class node_flags final
{
public:
	std::vector<std::string> config_overrides;
	std::vector<std::string> rpc_config_overrides;
	bool disable_add_initial_peers{ false }; // For testing only
	bool disable_activate_successors{ false }; // For testing only
	bool disable_backup{ false };
	bool disable_lazy_bootstrap{ false };
	bool disable_legacy_bootstrap{ false };
	bool disable_wallet_bootstrap{ false };
	bool disable_bootstrap_listener{ false };
	bool disable_bootstrap_bulk_pull_server{ false };
	bool disable_bootstrap_bulk_push_client{ false };
	bool disable_ongoing_bootstrap{ false }; // For testing only
	bool disable_ascending_bootstrap{ false };
	bool disable_rep_crawler{ false };
	bool disable_request_loop{ false }; // For testing only
	bool disable_tcp_realtime{ false };
	bool disable_providing_telemetry_metrics{ false };
	bool disable_ongoing_telemetry_requests{ false };
	bool disable_block_processor_unchecked_deletion{ false };
	bool disable_block_processor_republishing{ false };
	bool allow_bootstrap_peers_duplicates{ false };
	bool disable_max_peers_per_ip{ false }; // For testing only
	bool disable_max_peers_per_subnetwork{ false }; // For testing only
	bool force_use_write_queue{ false }; // For testing only. RocksDB does not use the database queue, but some tests rely on it being used.
	bool disable_search_pending{ false }; // For testing only
	bool enable_pruning{ false };
	bool fast_bootstrap{ false };
	bool read_only{ false };
	bool disable_connection_cleanup{ false };
	nano::generate_cache_flags generate_cache;
	bool inactive_node{ false };
	std::size_t block_processor_batch_size{ 0 };
	std::size_t block_processor_full_size{ 65536 };
	std::size_t block_processor_verification_size{ 0 };
	std::size_t vote_processor_capacity{ 144 * 1024 };
	std::size_t bootstrap_interval{ 0 }; // For testing only
};
}
