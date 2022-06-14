
#ifndef JSTD_HASH_DICTIONARY_V1_H
#define JSTD_HASH_DICTIONARY_V1_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <memory>
#include <limits>
#include <vector>
#include <type_traits>

#define ENABLE_JSTD_DICTIONARY_V1               1
#define DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW   1

// The entry's pair whether release on erase the entry.
#define DICTIONARY_V1_ENTRY_RELEASE_ON_ERASE    1
#define DICTIONARY_V1_USE_FAST_REHASH_MODE      1
#define DICTIONARY_V1_SUPPORT_VERSION           0

// This macro must define before include file "jstd/nothrow_new.h".
#undef  JSTD_USE_NOTHROW_NEW
#define JSTD_USE_NOTHROW_NEW        1

#include "jstd/hash/hash_helper.h"
#include "jstd/hash/dictionary_traits.h"
#include "jstd/nothrow_new.h"
#include "jstd/support/Power2.h"

namespace jstd {
namespace v1 {

template < typename Key, typename Value, std::size_t HashFunc = HashFunc_Default,
           typename Hasher = hash<Key, std::uint32_t, HashFunc>,
           typename KeyEqual = equal_to<Key> >
class BasicDictionary {
public:
    typedef Key                             key_type;
    typedef Value                           mapped_type;
    typedef std::pair<const Key, Value>     value_type;

    typedef Hasher                          hasher;
    typedef Hasher                          hasher_type;
    typedef KeyEqual                        key_equal;

    typedef std::size_t                     size_type;
    typedef std::size_t                     index_type;
    typedef typename Hasher::result_type    hash_code_t;
    typedef BasicDictionary<Key, Value, HashFunc, Hasher, KeyEqual>
                                            this_type;
    struct hash_entry {
        hash_entry * next;
        hash_code_t  hash_code;
        value_type   value;

        hash_entry() : next(nullptr), hash_code(0) {}
        hash_entry(hash_code_t hash_code) : next(nullptr), hash_code(hash_code) {}

        hash_entry(hash_code_t hash_code, const key_type & key,
              const mapped_type & value, hash_entry * next_entry = nullptr)
            : next(next_entry), hash_code(hash_code), value(key, value) {}
        hash_entry(hash_code_t hash_code, key_type && key,
              mapped_type && value, hash_entry * next_entry = nullptr)
            : next(next_entry), hash_code(hash_code),
              value(std::forward<key_type>(key), std::forward<mapped_type>(value)) {}

        ~hash_entry() {
#ifndef NDEBUG
            this->next = nullptr;
#endif
        }
    };

    typedef hash_entry                  entry_type;
    typedef entry_type *                iterator;
    typedef const entry_type *          const_iterator;

    class free_list {
    protected:
        entry_type * head_;
        size_type    size_;

    public:
        free_list(hash_entry * head = nullptr) : head_(head), size_(0) {}
        ~free_list() {
#ifndef NDEBUG
            this->clear();
#endif
        }

        entry_type * begin() const { return this->head_; }
        entry_type * end() const { return nullptr; }

        entry_type * head() const { return this->head_; }
        size_type size() const { return this->size_; }

        bool is_valid() const { return (this->head_ != nullptr); }
        bool is_empty() const { return (this->size_ == 0); }

        void set_head(entry_type * head) {
            this->head_ = head;
        }
        void set_size(size_type size) {
            this->size_ = size;
        }

        void set_list(entry_type * head, size_type size) {
            this->head_ = head;
            this->size_ = size;
        }

        void clear() {
            this->head_ = nullptr;
            this->size_ = 0;
        }

        void reset(hash_entry * head) {
            this->head_ = head;
            this->size_ = 0;
        }

        void inc() {
            ++(this->size_);
        }

        void dec() {
            --(this->size_);
        }

        void push_front(entry_type * entry) {
            assert(entry != nullptr);
            entry->next = this->head_;
            this->head_ = entry;
            ++(this->size_);
        }

        entry_type * pop_front() {
            entry_type * entry = this->head_;
            assert(entry != nullptr);
            this->head_ = entry->next;
            assert(this->size_ > 0);
            --(this->size_);
            return entry;
        }

        void swap(free_list & right) {
            if (&right != this) {
                std::swap(this->head_, right.head_);
                std::swap(this->size_, right.size_);
            }
        }
    };

    inline void swap(free_list & lhs, free_list & rhs) {
        lhs.swap(rhs);
    }

protected:
    entry_type **   buckets_;
    entry_type *    entries_;
    size_type       bucket_mask_;
    size_type       bucket_capacity_;
    size_type       entry_size_;
    size_type       entry_capacity_;
    free_list       freelist_;
#if DICTIONARY_V1_SUPPORT_VERSION
    size_type       version_;
#endif
    hasher_type     hasher_;
    key_equal       key_is_equal_;

    // Default initial capacity is 16.
    static const size_type kDefaultInitialCapacity = 16;
    // Minimum capacity is 8.
    static const size_type kMinimumCapacity = 8;
    // Maximum capacity is 1 << 31.
    static const size_type kMaximumCapacity = 1U << 30;

    // The bucket block size (bytes), default is 64 KB bytes.
    static const size_type kBucketBlockSize = 64 * 1024;
    // The bucket block entries capacity (entry_type *).
    static const size_type kBucketBlockCapacity = kBucketBlockSize / sizeof(entry_type *);

    // The threshold of treeify to red-black tree.
    static const size_type kTreeifyThreshold = 8;
    // The invalid hash value.
    static const hash_code_t kInvalidHash = hash_traits<hash_code_t>::kInvalidHash;
    // The replacement value for invalid hash value.
    static const hash_code_t kReplacedHash = hash_traits<hash_code_t>::kReplacedHash;

    // The factor of bucket per entry.
    static const size_type kBucketPerEntryFactor = 1;

public:
    BasicDictionary(size_type initialCapacity = kDefaultInitialCapacity)
        : buckets_(nullptr), entries_(nullptr), bucket_mask_(0), bucket_capacity_(0),
          entry_size_(0), entry_capacity_(0)
#if DICTIONARY_V1_SUPPORT_VERSION
          , version_(1) /* Since 0 means that the version attribute is not supported,
                           the initial value of version starts from 1. */
#endif
    {
        this->initialize(initialCapacity);
    }

    ~BasicDictionary() {
        this->destroy();
    }

    iterator begin() const {
        return (this->entries() != nullptr) ? this->unsafe_begin() : nullptr;
    }
    iterator end() const {
        return (this->entries() != nullptr) ? this->unsafe_end() : nullptr;
    }

    iterator unsafe_begin() const {
        return (iterator)&this->entries_[0];
    }
    iterator unsafe_end() const {
        return (iterator)&this->entries_[this->entry_capacity_];
    }

    size_type size() const {
        assert(this->entry_size_ >= this->freelist_.size());
        return (this->entry_size_ - this->freelist_.size());
    }
    size_type capacity() const { return this->entry_capacity_; }

    size_type bucket_mask() const { return this->bucket_mask_; }
    size_type bucket_count() const { return this->bucket_capacity_; }
    size_type bucket_capacity() const { return this->bucket_capacity_; }

    size_type entry_size() const { return this->size(); }
    size_type entry_count() const { return this->entry_capacity_; }

    entry_type ** buckets() const { return this->buckets_; }
    entry_type *  entries() const { return this->entries_; }

    size_type max_bucket_capacity() const {
        return (std::min)(this_type::kMaximumCapacity, (std::numeric_limits<size_type>::max)());
    }
    size_type max_size() const {
        return this->max_bucket_capacity();
    }

    bool valid() const { return (this->buckets() != nullptr); }
    bool empty() const { return (this->size() == 0); }

    size_type version() const {
#if DICTIONARY_V1_SUPPORT_VERSION
        return this->version_;
#else
        return 0;   /* Return 0 means that the version attribute is not supported. */
#endif
    }

protected:
    inline size_type calc_capacity(size_type capacity) const {
        // The minimum bucket is kMinimumCapacity = 16.
        //capacity = (capacity >= kMinimumCapacity) ? capacity : kMinimumCapacity;

        // The maximum bucket is kMaximumCapacity = 1 << 30.
        capacity = (capacity <= kMaximumCapacity) ? capacity : kMaximumCapacity;

        // Round up the new_capacity to power 2.
        capacity = detail::round_up_to_pow2(capacity);
        return capacity;
    }

    inline size_type calc_shrink_capacity(size_type capacity) {
        // The maximum bucket is kMaximumCapacity = 1 << 30.
        capacity = (capacity <= kMaximumCapacity) ? capacity : kMaximumCapacity;

        // Round up the new_capacity to power 2.
        capacity = detail::round_up_to_pow2(capacity);
        return capacity;
    }

    inline hash_code_t get_hash(const key_type & key) const {
        hash_code_t hash_code = this->hasher_(key);
        return hash_traits<hash_code_t>::filter(hash_code);
    }

    inline hash_code_t get_hash(key_type && key) const {
        hash_code_t hash_code = this->hasher_(std::forward<key_type>(key));
        return hash_traits<hash_code_t>::filter(hash_code);
    }

    inline index_type index_of(hash_code_t hash_code, size_type capacity_mask) const {
        return (index_type)((size_type)hash_code & capacity_mask);
    }

    inline index_type next_index(index_type index, size_type capacity_mask) const {
        ++index;
        return (index_type)((size_type)index & capacity_mask);
    }

    void initialize(size_type init_capacity) {
        size_type entry_capacity = init_capacity;
        assert(entry_capacity > 0);
        size_type bucket_capacity = detail::round_up_to_pow2(init_capacity * kBucketPerEntryFactor);
        assert(bucket_capacity > 0);
        assert((bucket_capacity & (bucket_capacity - 1)) == 0);

        // The the array of bucket's first entry.
        // entry_type ** new_buckets = new (std::nothrow) entry_type *[bucket_capacity];
        entry_type ** new_buckets = JSTD_NEW_ARRAY(entry_type *, bucket_capacity);
        IF_LIKELY(new_buckets != nullptr) {
            // Initialize the buckets's data.
            ::memset((void *)new_buckets, 0, bucket_capacity * sizeof(entry_type *));

            // Initialize the buckets info.
            this->buckets_ = new_buckets;
            this->bucket_mask_ = bucket_capacity - 1;
            this->bucket_capacity_ = bucket_capacity;

            // The array of entries.
#if DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
            // entry_type * new_entries = (entry_type *)operator new(
            //                             entry_capacity * sizeof(entry_type), std::nothrow);
            entry_type * new_entries = JSTD_PLACEMENT_NEW(entry_type, entry_capacity);
#else
            // entry_type * new_entries = new (std::nothrow) entry_type[entry_capacity];
            entry_type * new_entries = JSTD_NEW_ARRAY(entry_type, entry_capacity);
#endif
            IF_LIKELY(new_entries != nullptr) {
                // Linked all new entries to the free list.
                //fill_freelist(this->freelist_, new_entries, entry_capacity);

                // Initialize the entries info.
                this->entries_ = new_entries;
                this->entry_size_ = 0;
                this->entry_capacity_ = entry_capacity;
            }
        }

        this->freelist_.clear();
    }

    void free_buckets_impl() {
        assert(this->buckets_ != nullptr);
        //operator delete((void *)this->buckets_, std::nothrow);
        //jstd::nothrow_deleter::free(this->buckets_);
        JSTD_FREE_ARRAY(this->buckets_);
    }

    void free_buckets() {
        if (likely(this->buckets_ != nullptr)) {
            this->free_buckets_impl();
        }
    }

    void free_entries() {
#if DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
        assert(this->entries_ != nullptr);
        //operator delete((void *)this->entries_, std::nothrow);
        //jstd::nothrow_deleter::free(this->entries_);
        JSTD_PLACEMENT_FREE(this->entries_);
#else
        //operator delete((void *)this->entries_, std::nothrow);
        //jstd::nothrow_deleter::destroy(this->entries_);
        JSTD_DELETE_ARRAY(this->entries_);
#endif // DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
    }

    void destroy_entries() {
#if DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
        assert(this->entries_ != nullptr);
        entry_type * entry = this->entries_;
        for (size_type i = 0; i < this->entry_size_; i++) {
#if DICTIONARY_V1_ENTRY_RELEASE_ON_ERASE
            if (likely(entry->hash_code != kInvalidHash)) {
                assert(entry != nullptr);
                value_type * __pair = &entry->value;
                assert(__pair != nullptr);
                __pair->~value_type();
            }
#else
            assert(entry != nullptr);
            value_type * __pair = &entry->value;
            assert(__pair != nullptr);
            __pair->~value_type();
#endif // DICTIONARY_V1_ENTRY_RELEASE_ON_ERASE
            ++entry;
        }

        // Free the entries buffer.
        this->free_entries();
#endif // DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
    }

    // Linked the entries to the free list.
    void fill_freelist(free_list & freelist, entry_type * entries, size_type capacity) {
        assert(entries != nullptr);
        assert(capacity > 0);
        entry_type * entry = entries + capacity - 1;
        entry_type * prev = entries + capacity;
        entry_type * last_entry = entry;
        for (size_type i = 0; i < capacity; i++) {
            entry->next = prev;
            prev--;
            entry--;
        }
        last_entry->next = nullptr;
        freelist.set_list(entries, capacity);
    }

    void reinsert_list(entry_type ** new_buckets, size_type new_mask,
                       entry_type * old_entry) {
        assert(new_buckets != nullptr);
        assert(old_entry != nullptr);
        assert(new_mask > 0);

        do {
            hash_code_t hash_code = old_entry->hash_code;
            size_type index = this->index_of(hash_code, new_mask);

            // Save the value of old_entry->next.
            entry_type * next_entry = old_entry->next;

            // Push the old entry to front of new list.
            old_entry->next = new_buckets[index];
            new_buckets[index] = old_entry;
            ++(this->entry_size_);

            // Scan next entry
            old_entry = next_entry;
        } while (likely(old_entry != nullptr));
    }

    template <bool force_shrink = false>
    void rehash_internal(size_type new_entry_capacity) {
        assert(new_entry_capacity > 0);
        //assert((new_entry_capacity & (new_entry_capacity - 1)) == 0);

        // bucket_capacity = enrty_capacity * kBucketPerEntryFactor;
        size_type new_bucket_capacity = calc_capacity(new_entry_capacity * kBucketPerEntryFactor);

        if (likely((force_shrink == false && (new_entry_capacity > this->entry_capacity_ ||
                                              new_bucket_capacity > this->bucket_capacity_)) ||
                   (force_shrink == true && (new_entry_capacity != this->entry_capacity_ ||
                                             new_bucket_capacity != this->bucket_capacity_)))) {
            // The the array of bucket's first entry.
            // entry_type ** new_buckets = new (std::nothrow) entry_type *[new_bucket_capacity];
            entry_type ** new_buckets = JSTD_NEW_ARRAY(entry_type *, new_bucket_capacity);
            IF_LIKELY(new_buckets != nullptr) {
                // Initialize the buckets's data.
                ::memset((void *)new_buckets, 0, new_bucket_capacity * sizeof(entry_type *));

                // The the array of entries.
#if DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
                // entry_type * new_entries = (entry_type *)operator new(
                //                             sizeof(entry_type) * new_entry_capacity, std::nothrow);
                entry_type * new_entries = JSTD_PLACEMENT_NEW(entry_type, new_entry_capacity);
#else
                // entry_type * new_entries = new (std::nothrow) entry_type[new_entry_capacity];
                entry_type * new_entries = JSTD_NEW_ARRAY(entry_type, new_entry_capacity);
#endif
                IF_LIKELY(new_entries != nullptr) {
                    // Linked all new entries to the new free list.
                    //free_list new_freelist;
                    //fill_freelist(new_freelist, new_entries, new_entry_capacity);

#if DICTIONARY_V1_USE_FAST_REHASH_MODE
                    // Recalculate the bucket of all keys.
                    if (likely(this->entries_ != nullptr)) {
                        entry_type * new_entry = new_entries;
                        entry_type * old_entry = this->entries_;

                        // Copy the old entries to new entries.
                        size_type new_count = 0;
                        for (size_type i = 0; i < this->entry_size_; i++) {
                            assert(new_entry != nullptr);
                            assert(old_entry != nullptr);
                            if (likely(old_entry->hash_code != kInvalidHash)) {
#if DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
                                // Swap old_entry and new_entry.
                                new_entry->next = old_entry->next;
                                new_entry->hash_code = old_entry->hash_code;

                                // pair_type class placement new
                                void * pair_buf = (void *)&(new_entry->value);
                                value_type * new_pair = new (pair_buf) value_type(std::move(old_entry->value));
                                assert(new_pair == &new_entry->value);
                                //new_entry->pair.swap(old_entry->pair);

                                // pair_type class placement delete
                                value_type * pair_ptr = &old_entry->value;
                                assert(pair_ptr != nullptr);
                                pair_ptr->~value_type();
#else // !DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
                                // Swap old_entry and new_entry.
                                //new_entry->next = old_entry->next;
                                new_entry->hash_code = old_entry->hash_code;
                                new_entry->value.swap(old_entry->value);
#endif // DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
                                ++new_entry;
                                ++old_entry;
                                ++new_count;
                            }
                            else {
#if DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
#if (DICTIONARY_V1_ENTRY_RELEASE_ON_ERASE == 0)
                                // pair_type class placement delete
                                value_type * pair_ptr = &old_entry->value;
                                assert(pair_ptr != nullptr);
                                pair_ptr->~value_type();
#endif // DICTIONARY_V1_ENTRY_RELEASE_ON_ERASE
#endif // DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
                                ++old_entry;
                            }
                        }
                        assert(new_count == this->size());

                        // Free old entries data.
                        this->free_entries();

                        // Insert and adjust the new entries to the new buckets.
                        size_type new_bucket_mask = new_bucket_capacity - 1;
                        new_entry = new_entries;
                        for (size_type i = 0; i < new_count; i++) {
                            assert(new_entry != nullptr);
                            hash_code_t hash_code = new_entry->hash_code;
                            assert(hash_code != kInvalidHash);
                            size_type index = this->index_of(hash_code, new_bucket_mask);

                            // Insert the new entry to the new bucket.
                            new_entry->next = new_buckets[index];
                            new_buckets[index] = new_entry;
                            ++new_entry;
                        }
                    }

                    // Free old buckets data.
                    this->free_buckets();

#else // !DICTIONARY_V1_USE_FAST_REHASH_MODE

                    // Recalculate the bucket of all keys.
                    if (likely(this->buckets_ != nullptr)) {
                        size_type old_size = this->entry_size_;
                        this->entry_size_ = 0;
                        size_type new_bucket_mask = new_bucket_capacity - 1;

                        entry_type ** old_buckets = this->buckets_;
                        for (size_type i = 0; i < this->entry_capacity_; i++) {
                            assert(old_buckets != nullptr);
                            entry_type * old_entry = *old_buckets;
                            if (likely(old_entry == nullptr)) {
                                old_buckets++;
                            }
                            else {
                                // Insert and adjust the old entries to the new buckets.
                                this->reinsert_list(new_buckets, new_bucket_mask, old_entry);
#ifndef NDEBUG
                                // Set the old_list.head to nullptr.
                                *old_buckets = nullptr;
#endif
                                old_buckets++;
                            }
                        }
                        assert(this->entry_size_ == old_size);

                        // Free old buckets data.
                        this->free_buckets();
                    }

#endif // DICTIONARY_V1_USE_FAST_REHASH_MODE

                    // Setting status
                    this->buckets_ = new_buckets;
                    this->entries_ = new_entries;
                    this->bucket_mask_ = new_bucket_capacity - 1;
                    this->bucket_capacity_ = new_bucket_capacity;
                    //this->entry_size_ = ??;
                    this->entry_capacity_ = new_entry_capacity;
                    this->freelist_.clear();

                    this->updateVersion();
                }
                else {
                    // Free the array of new bucket.
                    //operator delete((void *)new_buckets, std::nothrow);
                    //jstd::nothrow_deleter::free(new_buckets);
                    JSTD_FREE_ARRAY(new_buckets);
                }
            }
        }
    }

    inline iterator find_internal(const key_type & key, hash_code_t hash_code, index_type index) {
        assert(this->buckets() != nullptr);
        entry_type * entry = this->buckets_[index];
        while (likely(entry != nullptr)) {
            if (likely(entry->hash_code != hash_code)) {
                entry = entry->next;
            }
            else {
                if (likely(this->key_is_equal_(key, entry->value.first))) {
                    return (iterator)entry;
                }
                entry = entry->next;
            }
        }

        return this->unsafe_end();  // Not found
    }

    inline iterator find_before(const key_type & key, entry_type *& before_out, size_type & index) {
        hash_code_t hash_code = this->get_hash(key);
        index = this->index_of(hash_code, this->bucket_mask_);

        assert(this->buckets() != nullptr);
        entry_type * before = nullptr;
        entry_type * entry = this->buckets_[index];
        while (likely(entry != nullptr)) {
            if (likely(entry->hash_code != hash_code)) {
                before = entry;
                entry = entry->next;
            }
            else {
                if (likely(this->key_is_equal_(key, entry->value.first))) {
                    before_out = before;
                    return (iterator)entry;
                }
                entry = entry->next;
            }
        }

        return this->unsafe_end();  // Not found
    }

    void updateVersion() {
#if DICTIONARY_V1_SUPPORT_VERSION
        ++(this->version_);
#endif
    }

public:
    void destroy() {
        // Free the resources.
        if (likely(this->buckets_ != nullptr)) {
            if (likely(this->entries_ != nullptr)) {
                // Free all entries.
#if DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
                this->destroy_entries();
#else
                //jstd::nothrow_deleter::destroy(this->entries_);
                JSTD_DELETE_ARRAY(this->entries_);
#endif
                this->entries_ = nullptr;
            }

            // Free the array of bucket.
            this->free_buckets_impl();
            this->buckets_ = nullptr;
        }

        // Clear settings
        this->bucket_mask_ = 0;
        this->bucket_capacity_ = 0;
        this->entry_size_ = 0;
        this->entry_capacity_ = 0;

        this->freelist_.clear();
    }

    void clear() {
        if (likely(this->buckets_ != nullptr)) {
            // Initialize the buckets's data.
            memset((void *)this->buckets_, 0, this->entry_capacity_ * sizeof(entry_type *));
        }

        // Clear settings
        this->entry_size_ = 0;
        this->freelist_.clear();
    }

    void reserve(size_type capacity) {
        this->rehash_internal<false>(capacity);
    }

    void rehash(size_type new_size) {
        assert(new_size > 0);
        // Recalculate the size of new_capacity.
        size_type new_capacity = this->calc_capacity(new_size);
        this->rehash_internal<false>(new_capacity);
    }

    void resize(size_type new_size) {
        this->rehash(new_size);
    }

    void shrink_to_fit(size_type new_size = 0) {
        // Recalculate the size of new_capacity.
        new_size = (this->entry_size_ >= new_size) ? this->entry_size_ : new_size;
        size_type new_capacity = this->calc_shrink_capacity(new_size);
        this->rehash_internal<true>(new_capacity);
    }

    iterator find(const key_type & key) {
        if (likely(this->buckets() != nullptr)) {
            hash_code_t hash_code = this->get_hash(key);
            index_type index = this->index_of(hash_code, this->bucket_mask_);

            entry_type * entry = this->buckets_[index];
            while (likely(entry != nullptr)) {
                if (likely(entry->hash_code != hash_code)) {
                    entry = entry->next;
                }
                else {
                    if (likely(this->key_is_equal_(key, entry->value.first))) {
                        return (iterator)entry;
                    }
                    entry = entry->next;
                }
            }

            return this->unsafe_end();  // Not found
        }

        return nullptr; // Error: buckets data is invalid
    }

    bool contains(const key_type & key) {
        iterator iter = this->find(key);
        return (iter != this->end());
    }

    void insert(const key_type & key, const mapped_type & value) {
        if (likely(this->buckets_ != nullptr)) {
            hash_code_t hash_code = this->get_hash(key);
            index_type index = this->index_of(hash_code, this->bucket_mask_);
            iterator iter = this->find_internal(key, hash_code, index);
            if (likely(iter == this->unsafe_end())) {
                entry_type * new_entry;
                if (likely(this->freelist_.is_empty())) {
                    if (likely(this->entry_size_ >= this->entry_capacity_)) {
                        // Resize the buckets and the entries.
                        this->resize(this->entry_size_ + 1);
                        // Recalculate the bucket index.
                        index = this->index_of(hash_code, this->bucket_mask_);
                    }

                    // Get a unused entry.
                    new_entry = &this->entries_[this->entry_size_];
                    assert(new_entry != nullptr);
                    ++(this->entry_size_);
                }
                else {
                    // Pop a free entry from freelist.
                    new_entry = this->freelist_.pop_front();
                    assert(new_entry != nullptr);
                }

                new_entry->next = this->buckets_[index];
                new_entry->hash_code = hash_code;
                this->buckets_[index] = new_entry;

#if (DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW != 0) && (DICTIONARY_V1_ENTRY_RELEASE_ON_ERASE != 0)
                // pair_type class placement new
                void * pair_buf = (void *)&(new_entry->value);
                value_type * new_pair = new (pair_buf) value_type(key, value);
                assert(new_pair == &new_entry->value);
#else
                new_entry->value.first = key;
                new_entry->value.second = value;
#endif
            }
            else {
                // Update the existed key's value.
                assert(iter != nullptr);
                iter->value.second = value;
            }

            this->updateVersion();
        }
    }

    void insert(key_type && key, mapped_type && value) {
        if (likely(this->buckets_ != nullptr)) {
            hash_code_t hash_code = this->get_hash(std::forward<key_type>(key));
            index_type index = this->index_of(hash_code, this->bucket_mask_);
            iterator iter = this->find_internal(std::forward<key_type>(key), hash_code, index);
            if (likely(iter == this->unsafe_end())) {
                entry_type * new_entry;
                if (likely(this->freelist_.is_empty())) {
                    if (likely(this->entry_size_ >= this->entry_capacity_)) {
                        // Resize the buckets and the entries.
                        this->resize(this->entry_size_ + 1);
                        // Recalculate the bucket index.
                        index = this->index_of(hash_code, this->bucket_mask_);
                    }

                    // Get a unused entry.
                    new_entry = &this->entries_[this->entry_size_];
                    assert(new_entry != nullptr);
                    ++(this->entry_size_);
                }
                else {
                    // Pop a free entry from freelist.
                    new_entry = this->freelist_.pop_front();
                    assert(new_entry != nullptr);
                }

                new_entry->next = this->buckets_[index];
                new_entry->hash_code = hash_code;
                this->buckets_[index] = new_entry;

#if (DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW != 0) && (DICTIONARY_V1_ENTRY_RELEASE_ON_ERASE != 0)
                // pair_type class placement new
                void * pair_buf = (void *)&(new_entry->value);
                value_type * new_pair = new (pair_buf) value_type(std::forward<key_type>(key),
                                                                  std::forward<mapped_type>(value));
                assert(new_pair == &new_entry->value);
#else
                new_entry->value.first.swap(key);
                new_entry->value.second.swap(value);
#endif
            }
            else {
                // Update the existed key's value.
                assert(iter != nullptr);
                iter->value.second = std::move(std::forward<mapped_type>(value));
            }

            this->updateVersion();
        }
    }

    void insert(const value_type & pair) {
        this->insert(pair.first, pair.second);
    }

    void insert(value_type && pair) {
        this->insert(std::forward<typename value_type::first_type>(pair.first),
                     std::forward<typename value_type::second_type>(pair.second));
    }

    void emplace(const key_type & key, const mapped_type & value) {
        this->insert(key, value);
    }

    void emplace(const key_type & key, mapped_type && value) {
        this->insert(key, std::forward<mapped_type>(value));
    }

    void emplace(key_type && key, mapped_type && value) {
        this->insert(std::forward<key_type>(key), std::forward<mapped_type>(value));
    }

    void emplace(const value_type & pair) {
        this->insert(pair.first, pair.second);
    }

    void emplace(value_type && pair) {
        this->insert(std::forward<typename value_type::first_type>(pair.first),
                     std::forward<typename value_type::second_type>(pair.second));
    }

    size_type erase(const key_type & key) {
        if (likely(this->buckets_ != nullptr)) {
            hash_code_t hash_code = this->get_hash(key);
            size_type index = this->index_of(hash_code, this->bucket_mask_);

            assert(this->buckets() != nullptr);
            assert(this->entries() != nullptr);
            entry_type * before = nullptr;
            entry_type * entry = this->buckets_[index];
            while (likely(entry != nullptr)) {
                if (likely(entry->hash_code != hash_code)) {
                    before = entry;
                    entry = entry->next;
                }
                else {
                    if (likely(this->key_is_equal_(key, entry->value.first))) {
                        if (likely(before != nullptr))
                            before->next = entry->next;
                        else
                            this->buckets_[index] = entry->next;

                        entry->hash_code = kInvalidHash;
                        this->freelist_.push_front(entry);

#if DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW
#if DICTIONARY_V1_ENTRY_RELEASE_ON_ERASE
                        // pair_type class placement delete
                        value_type * pair_ptr = &entry->value;
                        assert(pair_ptr != nullptr);
                        pair_ptr->~value_type();
#endif // DICTIONARY_V1_ENTRY_RELEASE_ON_ERASE
#else
#ifdef _MSC_VER
                        entry->value.first.clear();
                        entry->value.second.clear();
#else
                        entry->value.first = std::move(key_type());
                        entry->value.second = std::move(mapped_type());
#endif // _MSC_VER
#endif // DICTIONARY_ENTRY_V1_USE_PLACEMENT_NEW

                        this->updateVersion();

                        // Has found
                        return size_type(1);
                    }

                    before = entry;
                    entry = entry->next;
                }
            }
        }

        // Not found
        return size_type(0);
    }

    void dump() {
        printf("jstd::v1::BasicDictionary<K, V>::dump()\n\n");
    }

    static const char * name() {
        switch (HashFunc) {
        case HashFunc_CRC32C:
            return "jstd::v1::Dictionary<K, V> (CRC32c)";
        case HashFunc_Time31:
            return "jstd::v1::Dictionary<K, V> (Time31)";
        case HashFunc_Time31Std:
            return "jstd::v1::Dictionary<K, V> (Time31Std)";
        default:
            return "jstd::v1::Dictionary<K, V> (Unknown)";
        }
    }
}; // BasicDictionary<K, V>

template <typename Key, typename Value>
using Dictionary_Time31 = BasicDictionary<Key, Value, HashFunc_Time31>;

template <typename Key, typename Value>
using Dictionary_Time31Std = BasicDictionary<Key, Value, HashFunc_Time31Std>;

#if JSTD_HAVE_SSE42_CRC32C
template <typename Key, typename Value>
using Dictionary_crc32c = BasicDictionary<Key, Value, HashFunc_CRC32C>;

template <typename Key, typename Value>
using Dictionary = BasicDictionary<Key, Value, HashFunc_CRC32C>;
#else
template <typename Key, typename Value>
using Dictionary = BasicDictionary<Key, Value, HashFunc_Time31>;
#endif // JSTD_HAVE_SSE42_CRC32C

} // namespace v1
} // namespace jstd

#endif // JSTD_HASH_DICTIONARY_V1_H
