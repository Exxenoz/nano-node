#pragma once

#include <nano/secure/store_partial.hpp>

namespace
{
template <typename T>
void parallel_traversal (std::function<void (T const &, T const &, bool const)> const & action);
}

namespace nano
{
template <typename Val, typename Derived_Store>
class store_partial;

template <typename Val, typename Derived_Store>
void release_assert_success (store_partial<Val, Derived_Store> const &, int const);

template <typename Val, typename Derived_Store>
class reverse_link_store_partial : public reverse_link_store
{
private:
	nano::store_partial<Val, Derived_Store> & store;

	friend void release_assert_success<Val, Derived_Store> (store_partial<Val, Derived_Store> const &, int const);

public:
	reverse_link_store_partial (nano::store_partial<Val, Derived_Store> & store_a) :
		store (store_a){};

	void put (nano::write_transaction const & transaction_a, nano::block_hash const & send_block_hash_a, nano::block_hash const & receive_block_hash_a) override
	{
		nano::db_val<Val> send_block_hash (send_block_hash_a);
		nano::db_val<Val> receive_block_hash (receive_block_hash_a);
		auto status (store.put (transaction_a, tables::reverse_links, send_block_hash, receive_block_hash));
		release_assert_success (store, status);
	}

	nano::block_hash get (nano::transaction const & transaction_a, nano::block_hash const & send_block_hash_a) const override
	{
		nano::db_val<Val> send_block_hash (send_block_hash_a);
		nano::db_val<Val> receive_block_hash;
		auto status (store.get (transaction_a, tables::reverse_links, send_block_hash, receive_block_hash));
		release_assert (store.success (status) || store.not_found (status));
		nano::block_hash result{ 0 };
		if (store.success (status))
		{
			result = static_cast<nano::block_hash> (receive_block_hash);
		}
		return result;
	}

	void del (nano::write_transaction const & transaction_a, nano::block_hash const & send_block_hash_a) override
	{
		auto status (store.del (transaction_a, tables::reverse_links, send_block_hash_a));
		release_assert_success (store, status);
	}

	bool exists (nano::transaction const & transaction_a, nano::block_hash const & send_block_hash_a) const override
	{
		nano::db_val<Val> value;
		auto status (store.get (transaction_a, tables::reverse_links, nano::db_val<Val> (send_block_hash_a), value));
		release_assert (store.success (status) || store.not_found (status));
		return (store.success (status));
	}

	size_t count (nano::transaction const & transaction_a) const override
	{
		return store.count (transaction_a, tables::reverse_links);
	}

	void clear (nano::write_transaction const & transaction_a) override
	{
		auto status = store.drop (transaction_a, tables::reverse_links);
		release_assert_success (store, status);
	}

	nano::store_iterator<nano::block_hash, nano::block_hash> begin (nano::transaction const & transaction_a) const override
	{
		return store.template make_iterator<nano::block_hash, nano::block_hash> (transaction_a, tables::reverse_links);
	}

	nano::store_iterator<nano::block_hash, nano::block_hash> begin (nano::transaction const & transaction_a, nano::block_hash const & send_block_hash_a) const override
	{
		return store.template make_iterator<nano::block_hash, nano::block_hash> (transaction_a, tables::reverse_links, nano::db_val<Val> (send_block_hash_a));
	}

	nano::store_iterator<nano::block_hash, nano::block_hash> end () const override
	{
		return nano::store_iterator<nano::block_hash, nano::block_hash> (nullptr);
	}

	void for_each_par (std::function<void (nano::read_transaction const &, nano::store_iterator<nano::block_hash, nano::block_hash>, nano::store_iterator<nano::block_hash, nano::block_hash>)> const & action_a) const override
	{
		parallel_traversal<nano::uint256_t> (
		[&action_a, this] (nano::uint256_t const & start, nano::uint256_t const & end, bool const is_last) {
			auto transaction (this->store.tx_begin_read ());
			action_a (transaction, this->begin (transaction, start), !is_last ? this->begin (transaction, end) : this->end ());
		});
	}
};
}
