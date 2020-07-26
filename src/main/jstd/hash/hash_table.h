
#ifndef JSTD_HASH_TABLE_H
#define JSTD_HASH_TABLE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"

#include <memory.h>
#include <assert.h>

#include <cstddef>
#include <memory>
#include <type_traits>

#include "jstd/hash/hash_helper.h"
#include "jstd/hash/dictionary_traits.h"
#include "jstd/string/string_utils.h"
#include "jstd/support/PowerOf2.h"

#ifndef USE_SSE42_STRING_COMPARE
#define USE_SSE42_STRING_COMPARE    1
#endif

namespace jstd {

template <typename Key, typename Value>
struct hash_table_node {
    typedef Key                         key_type;
    typedef Value                       mapped_type;
    typedef std::pair<Key, Value>       value_type;
    typedef std::size_t                 size_type;
    typedef std::uint32_t               hash_type;
    typedef hash_table_node<Key, Value> this_type;

    hash_type hash;
    value_type pair;

    hash_table_node() : hash(0) {}
    hash_table_node(hash_type init_hash) : hash(init_hash) {}

    hash_table_node(hash_type init_hash, const key_type & key, const mapped_type & value)
        : hash(init_hash), pair(key, value) {}
    hash_table_node(hash_type init_hash, key_type && key, mapped_type && value)
        : hash(init_hash), pair(std::forward<key_type>(key), std::forward<mapped_type>(value)) {}

    hash_table_node(const key_type & key, const mapped_type & value)
        : hash(0), pair(key, value) {}
    hash_table_node(key_type && key, mapped_type && value)
        : hash(0), pair(std::forward<key_type>(key), std::forward<mapped_type>(value)) {}

    ~hash_table_node() {}
};

template <typename Key, typename Value, std::size_t HashFunc = HashFunc_Default>
class basic_hash_table {
public:
    typedef Key                                     key_type;
    typedef Value                                   mapped_type;
    typedef std::pair<Key, Value>                   value_type;
    typedef std::size_t                             size_type;
    typedef std::uint32_t                           hash_type;

    typedef hash_table_node<Key, Value>             node_type;
    typedef hash_table_node<Key, Value> *           data_type;
    typedef data_type *                             iterator;
    typedef const data_type *                       const_iterator;
    typedef basic_hash_table<Key, Value, HashFunc>  this_type;

private:
    data_type * table_;
    size_type size_;
    size_type mask_;
    size_type buckets_;

    // Default initial capacity is 64.
    static const size_type kDefaultInitialCapacity = 64;
    // Maximum capacity is 1 << 30.
    static const size_type kMaximumCapacity = 1U << 30;

public:
    basic_hash_table(size_type initialCapacity = kDefaultInitialCapacity) :
        table_(nullptr), size_(0), mask_(0), buckets_(0) {
        this->initialize(initialCapacity);
    }
    ~basic_hash_table() {
        this->destroy();
    }

    iterator begin() const { return &(this->table_[0]); }
    iterator end() const { return &(this->table_[this->buckets_]); }

    size_type size() const { return this->size_; }
    size_type bucket_mask() const { return this->mask_; }
    size_type bucket_count() const { return this->buckets_; }
    data_type * data() const { return this->table_; }

    bool is_valid() const { return (this->table_ != nullptr); }
    bool empty() const { return (this->size() == 0); }

    void clear() {
        // Clear the data only, don't free the table.
        if (likely(this->table_ != nullptr)) {
            for (size_type i = 0; i < this->buckets_; ++i) {
                node_type * node = (node_type *)this->table_[i];
                if (likely(node != nullptr)) {
                    delete node;
                    this->table_[i] = nullptr;
                }
            }
        }
        // Setting status
        this->size_ = 0;
    }

private:
    void initialize(size_type new_buckets) {
        new_buckets = run_time::round_up_to_pow2(new_buckets);
        assert(new_buckets > 0);
        assert((new_buckets & (new_buckets - 1)) == 0);
        data_type * new_table = new data_type[new_buckets];
        if (likely(new_table != nullptr)) {
            // Initialize the table data.
            memset(new_table, 0, sizeof(data_type) * new_buckets);
            // Setting status
            this->table_ = new_table;
            this->size_ = 0;
            this->mask_ = new_buckets - 1;
            this->buckets_ = new_buckets;
        }
    }

    void destroy() {
#ifdef NDEBUG
        // Clear all data, and free the table.
        if (likely(this->table_ != nullptr)) {
            for (size_type i = 0; i < this->buckets_; ++i) {
                node_type * node = (node_type *)this->table_[i];
                if (likely(node != nullptr)) {
                    delete node;
                }
            }
            delete[] this->table_;
            this->table_ = nullptr;
        }
#else
        // Clear all data, and free the table.
        if (likely(this->table_ != nullptr)) {
            for (size_type i = 0; i < this->buckets_; ++i) {
                node_type * node = (node_type *)this->table_[i];
                if (likely(node != nullptr)) {
                    delete node;
                    this->table_[i] = nullptr;
                }
            }
            delete[] this->table_;
            this->table_ = nullptr;
        }
        // Setting status
        this->size_ = 0;
        this->mask_ = 0;
        this->buckets_ = 0;
#endif
    }

    inline size_type calc_buckets(size_type new_buckets) {
        // If new_buckets is less than half of the current hash table size,
        // then double the hash table size.
        new_buckets = (new_buckets > (this->size_ * 2)) ? new_buckets : (this->size_ * 2);
        // The minimum bucket is kBucketsInit = 64.
        new_buckets = (new_buckets >= kDefaultInitialCapacity) ? new_buckets : kDefaultInitialCapacity;
        // The maximum bucket is kMaximumCapacity = 1 << 30.
        new_buckets = (new_buckets <= kMaximumCapacity) ? new_buckets : kMaximumCapacity;
        // Round up the new_buckets to power 2.
        new_buckets = run_time::round_up_to_pow2(new_buckets);
        return new_buckets;
    }

    inline size_type calc_buckets_fast(size_type new_buckets) {
        // If new_buckets is less than half of the current hash table size,
        // then double the hash table size.
        new_buckets = (new_buckets > (this->size_ * 2)) ? new_buckets : (this->size_ * 2);
        // The maximum bucket is kMaximumCapacity = 1 << 30.
        new_buckets = (new_buckets <= kMaximumCapacity) ? new_buckets : kMaximumCapacity;
        // Round up the new_buckets to power 2.
        new_buckets = run_time::round_up_to_pow2(new_buckets);
        return new_buckets;
    }

    static inline
    hash_type hash(const key_type & key) {
        return jstd::hash_helper<key_type, hash_type, HashFunc>::getHashCode(key);
    }

    static inline
    size_type index_of(hash_type hash, size_type mask) {
        size_type index = ((size_type)hash & mask);
        return index;
    }

    static inline
    size_type next_index(size_type index, size_type mask) {
        index = ((index + 1) & mask);
        return index;
    }

    void reserve_internal(size_type new_buckets) {
        assert(new_buckets > 0);
        assert((new_buckets & (new_buckets - 1)) == 0);
        if (likely(new_buckets > this->buckets_)) {
            data_type * new_table = new data_type[new_buckets];
            if (new_table != nullptr) {
                // Initialize the table data.
                ::memset(new_table, 0, sizeof(data_type) * new_buckets);
                if (likely(this->table_ != nullptr)) {
                    delete[] this->table_;
                }
                // Setting status
                this->table_ = new_table;
                this->size_ = 0;
                this->mask_ = new_buckets - 1;
                this->buckets_ = new_buckets;
            }
        }
    }

    inline void rehash_insert(data_type * new_table, size_type new_mask,
                              data_type old_data) {
        assert(new_table != nullptr);
        assert(old_data != nullptr);
        assert(new_mask >= 0);

        hash_type hash = old_data->hash;
        size_type index = this_type::index_of(hash, new_mask);

        if (likely(new_table[index] == nullptr)) {
            new_table[index] = old_data;
        }
        else {
            do {
                index = this_type::next_index(index, new_mask);
                if (likely(new_table[index] == nullptr)) {
                    new_table[index] = old_data;
                    break;
                }
            } while (1);
        }
    }

    void rehash_internal(size_type new_buckets) {
        assert(new_buckets > 0);
        assert((new_buckets & (new_buckets - 1)) == 0);
        assert(new_buckets >= this->size_ * 2);
        if (likely(new_buckets > this->buckets_)) {
            data_type * new_table = new data_type[new_buckets];
            if (likely(new_table != nullptr)) {
                // Initialize the new table data.
                memset(new_table, 0, sizeof(data_type) * new_buckets);

                if (likely(this->table_ != nullptr)) {
                    // Recalculate all hash indexs.
                    size_type new_size = 0;
                    size_type new_mask = new_buckets - 1;

                    for (size_type i = 0; i < this->buckets_; ++i) {
                        if (likely(this->table_[i] != nullptr)) {
                            // Insert the old buckets to the new buckets in the new table.
                            this->rehash_insert(new_table, new_mask, this->table_[i]);
                            ++new_size;
                        }
                    }
                    assert(new_size == this->size_);

                    // Free old table data.
                    delete[] this->table_;
                }
                // Setting status
                this->table_ = new_table;
                this->mask_ = new_buckets - 1;
                this->buckets_ = new_buckets;
            }
        }
    }

    void shrink_internal(size_type new_buckets) {
        assert(new_buckets > 0);
        assert((new_buckets & (new_buckets - 1)) == 0);
        assert(new_buckets >= this->size_ * 2);
        if (likely(new_buckets != this->buckets_)) {
            data_type * new_table = new data_type[new_buckets];
            if (likely(new_table != nullptr)) {
                // Initialize the new table data.
                memset(new_table, 0, sizeof(data_type) * new_buckets);

                if (likely(this->table_ != nullptr)) {
                    // Recalculate all hash values.
                    size_type new_size = 0;
                    size_type new_mask = new_buckets - 1;

                    for (size_type i = 0; i < this->buckets_; ++i) {
                        if (likely(this->table_[i] != nullptr)) {
                            // Insert the old buckets to the new buckets in the new table.
                            this->rehash_insert(new_table, new_mask, this->table_[i]);
                            ++new_size;
                        }
                    }
                    assert(new_size == this->size_);

                    // Free old table data.
                    delete[] this->table_;
                }
                // Setting status
                this->table_ = new_table;
                this->mask_ = new_buckets - 1;
                this->buckets_ = new_buckets;
            }
        }
    }

    void resize_internal(size_type new_buckets) {
        assert(new_buckets > 0);
        assert((new_buckets & (new_buckets - 1)) == 0);
        this->rehash_internal(new_buckets);
    }

public:
    void reserve(size_type new_buckets) {
        // Recalculate the size of new_buckets.
        new_buckets = this->calc_buckets(new_buckets);
        this->reserve_internal(new_buckets);
    }

    void rehash(size_type new_buckets) {
        // Recalculate the size of new_buckets.
        new_buckets = this->calc_buckets(new_buckets);
        this->rehash_internal(new_buckets);
    }

    void resize(size_type new_buckets) {
        this->rehash(new_buckets);
    }

    void shrink_to_fit(size_type new_buckets) {
        // Recalculate the size of new_buckets.
        new_buckets = this->calc_buckets_fast(new_buckets);
        this->shrink_internal(new_buckets);
    }

    iterator find(const key_type & key) {
        if (likely(this->table_ != nullptr)) {
            hash_type hash = this_type::hash(key);
            size_type index = this_type::index_of(hash, this->mask_);

            node_type * node = this->table_[index];
            if (likely(node != nullptr)) {
                // Found, next to check the hash value.
                if (likely(node->hash == hash)) {
                    // If hash value is equal, then compare the key sizes and the strings.
                    if (likely(key.size() == node->pair.first.size())) {
    #if USE_SSE42_STRING_COMPARE
                        if (likely(StrUtils::is_equals_flat(key, node->pair.first))) {
                            return (iterator)&this->table_[index];
                        }
    #else
                        if (likely(strcmp(key.c_str(), node->pair.first.c_str()) == 0)) {
                            return (iterator)&this->table_[index];
                        }
    #endif
                    }
                }
            }

            // If first position is not found, search next bucket continue.
            size_type first_index = index;
            do {
                index = this_type::next_index(index, this->mask_);
                node = this->table_[index];
                if (likely(node != nullptr)) {
                    if (likely(node->hash == hash)) {
                        // If hash value is equal, then compare the key sizes and the strings.
                        if (likely(key.size() == node->pair.first.size())) {
    #if USE_SSE42_STRING_COMPARE
                            if (likely(StrUtils::is_equals_flat(key, node->pair.first))) {
                                return (iterator)&this->table_[index];
                            }
    #else
                            if (likely(strcmp(key.c_str(), node->pair.first.c_str()) == 0)) {
                                return (iterator)&this->table_[index];
                            }
    #endif
                        }
                    }
                }
            } while (likely(index != first_index));
        }

        // Not found
        return this->end();
    }

    iterator find_internal(const key_type & key, hash_type & hash) {
        hash = this_type::hash(key);
        size_type index = this_type::index_of(hash, this->mask_);

        assert(this->table_ != nullptr);
        node_type * node = this->table_[index];
        if (likely(node != nullptr)) {
            // Found, next to check the hash value.
            if (likely(node->hash == hash)) {
                // If hash value is equal, then compare the key sizes and the strings.
                if (likely(key.size() == node->pair.first.size())) {
#if USE_SSE42_STRING_COMPARE
                    if (likely(StrUtils::is_equals_flat(key, node->pair.first))) {
                        return (iterator)&this->table_[index];
                    }
#else
                    if (likely(strcmp(key.c_str(), node->pair.first.c_str()) == 0)) {
                        return (iterator)&this->table_[index];
                    }
#endif
                }
            }
        }

        // If first position is not found, search next bucket continue.
        size_type first_index = index;
        do {
            index = this_type::next_index(index, this->mask_);
            node = this->table_[index];
            if (likely(node != nullptr)) {
                if (likely(node->hash == hash)) {
                    // If hash value is equal, then compare the key sizes and the strings.
                    if (likely(key.size() == node->pair.first.size())) {
#if USE_SSE42_STRING_COMPARE
                        if (likely(StrUtils::is_equals_flat(key, node->pair.first))) {
                            return (iterator)&this->table_[index];
                        }
#else
                        if (likely(strcmp(key.c_str(), node->pair.first.c_str()) == 0)) {
                            return (iterator)&this->table_[index];
                        }
#endif
                    }
                }
            }
        } while (likely(index != first_index));

        // Not found
        return this->end();
    }

    void insert(const key_type & key, const mapped_type & value) {
        if (likely(this->table_ != nullptr)) {
            hash_type hash;
            iterator iter = this->find_internal(key, hash);
            if (likely(iter == this->end())) {
                // Insert the new key.
                if (likely(this->size_ >= (this->buckets_ * 3 / 4))) {
                    this->resize_internal(this->buckets_ * 2);
                }

                node_type * new_data = new node_type(hash, key, value);
                if (likely(new_data != nullptr)) {
                    size_type index = this_type::index_of(hash, this->mask_);
                    if (likely(this->table_[index] == nullptr)) {
                        this->table_[index] = (data_type)new_data;
                        ++(this->size_);
                    }
                    else {
                        do {
                            index = this_type::next_index(index, this->mask_);
                            if (likely(this->table_[index] == nullptr)) {
                                this->table_[index] = (data_type)new_data;
                                ++(this->size_);
                                break;
                            }
                        } while (1);
                    }
                }
            }
            else {
                // Update the existed key's value.
                assert(iter != nullptr && (*iter) != nullptr);
                (*iter)->pair.second = value;
            }
        }
    }

    void insert(key_type && key, mapped_type && value) {
        if (likely(this->table_ != nullptr)) {
            hash_type hash;
            iterator iter = this->find_internal(std::forward<key_type>(key), hash);
            if (likely(iter == this->end())) {
                // Insert the new key.
                if (likely(this->size_ >= (this->buckets_ * 3 / 4))) {
                    this->resize_internal(this->buckets_ * 2);
                }

                node_type * new_data = new node_type(hash, std::forward<key_type>(key),
                                                     std::forward<mapped_type>(value));
                if (likely(new_data != nullptr)) {
                    size_type index = this_type::index_of(hash, this->mask_);
                    if (likely(this->table_[index] == nullptr)) {
                        this->table_[index] = (data_type)new_data;
                        ++(this->size_);
                    }
                    else {
                        do {
                            index = this_type::next_index(index, this->mask_);
                            if (likely(this->table_[index] == nullptr)) {
                                this->table_[index] = (data_type)new_data;
                                ++(this->size_);
                                break;
                            }
                        } while (1);
                    }
                }
            }
            else {
                // Update the existed key's value.
                assert(iter != nullptr && (*iter) != nullptr);
                (*iter)->pair.second = std::move(std::forward<mapped_type>(value));
            }
        }
    }

    void insert(const value_type & pair) {
        this->insert(pair.first, pair.second);
    }

    void insert(value_type && pair) {
        this->insert(std::forward<key_type>(pair.first),
                     std::forward<mapped_type>(pair.second));
    }

    void emplace(const value_type & pair) {
        this->insert(pair.first, pair.second);
    }

    void emplace(value_type && pair) {
        this->insert(std::forward<key_type>(pair.first),
                     std::forward<mapped_type>(pair.second));
    }

    void emplace(const key_type & key, const mapped_type & value) {
        this->emplace(std::make_pair(key, value));
    }

    void emplace(key_type && key, mapped_type && value) {
        this->emplace(std::make_pair(std::forward<key_type>(key),
                                     std::forward<mapped_type>(value)));
    }

    size_type erase(const key_type & key) {
        if (likely(this->table_ != nullptr)) {
            iterator iter = this->find(key);
            if (likely(iter != this->end())) {
                assert(this->size_ > 0);
                if (likely(iter != nullptr)) {
                    if (likely(*iter != nullptr)) {
                        delete *iter;
                        *iter = nullptr;
                        assert(this->size_ > 0);
                        --(this->size_);
                        return size_type(1);
                    }
                }
            }
            else {
                // Not found the key
            }
        }
        return size_type(0);
    }

    static const char * name() {
        switch (HashFunc) {
            case HashFunc_CRC32C:
                return "jstd::hash_table<K, V> (CRC32c)";
            case HashFunc_Time31:
                return "jstd::hash_table<K, V> (Time31)";
            case HashFunc_Time31Std:
                return "jstd::hash_table<K, V> (Time31Std)";
            default:
                return "jstd::hash_table<K, V> (Unknown)";
        }
    }
};

template <typename Key, typename Value>
using hash_table_time31 = basic_hash_table<Key, Value, HashFunc_Time31>;

template <typename Key, typename Value>
using hash_table_time31_std = basic_hash_table<Key, Value, HashFunc_Time31Std>;

#if SUPPORT_SSE42_CRC32C
template <typename Key, typename Value>
using hash_table = basic_hash_table<Key, Value, HashFunc_CRC32C>;
#else
template <typename Key, typename Value>
using hash_table = basic_hash_table<Key, Value, HashFunc_Time31>;
#endif

} // namespace jstd

#endif // JSTD_HASH_TABLE_H
