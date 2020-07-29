
#ifndef JSTD_HASH_DICTIONARY_H
#define JSTD_HASH_DICTIONARY_H

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

#ifndef USE_JSTD_DICTIONARY

#define USE_JSTD_DICTIONARY                     1
#define DICTIONARY_ENTRY_USE_PLACEMENT_NEW      1

// The entry's pair whether release on erase the entry.
#define DICTIONARY_ENTRY_RELEASE_ON_ERASE       1
#define DICTIONARY_USE_FAST_REHASH_MODE         1
#define DICTIONARY_SUPPORT_VERSION              0

#endif // USE_JSTD_DICTIONARY

// This macro must define before include file "jstd/nothrow_new.h".
#undef  JSTD_USE_NOTHROW_NEW
#define JSTD_USE_NOTHROW_NEW        1

#include "jstd/nothrow_new.h"
#include "jstd/hash/hash_helper.h"
#include "jstd/hash/dictionary_traits.h"
#include "jstd/allocator.h"
#include "jstd/support/PowerOf2.h"

namespace jstd {

template < typename Key, typename Value,
           std::size_t HashFunc = HashFunc_Default,
           std::size_t Alignment = align_of<std::pair<const Key, Value>>::value,
           typename Hasher = hash<Key, HashFunc>,
           typename KeyEqual = equal_to<Key>,
           typename Allocator = allocator<std::pair<const Key, Value>, Alignment> >
class BasicDictionary {
public:
    typedef Key                             key_type;
    typedef Value                           mapped_type;
    typedef std::pair<const Key, Value>     value_type;

    typedef Hasher                          hasher;
    typedef Hasher                          hasher_type;
    typedef KeyEqual                        key_equal;
    typedef Allocator                       allocator_type;

    typedef std::size_t                     size_type;
    typedef std::size_t                     index_type;
    typedef typename Hasher::result_type    hash_code_t;
    typedef BasicDictionary<Key, Value, HashFunc, Alignment, Hasher, KeyEqual, Allocator>
                                            this_type;

    struct hash_entry {
        hash_entry * next;
        hash_code_t  hash_code;
        uint32_t     flags;
        value_type   value;

        hash_entry() : next(nullptr), hash_code(0), flags(0) {}

        ~hash_entry() {
#ifndef NDEBUG
            this->next = nullptr;
#endif
        }
    };

    typedef hash_entry          entry_type;
    typedef entry_type *        iterator;
    typedef const entry_type *  const_iterator;

    struct entry_list {
        entry_type * entries;
        size_type    capacity;

        entry_list() : entries(nullptr), capacity(0) {}
        entry_list(entry_type * entries, size_type capacity)
            : entries(entries), capacity(capacity) {}
        ~entry_list() {}
    };

    template <typename T>
    class free_list {
    public:
        typedef T                               node_type;
        typedef typename this_type::size_type   size_type;

    protected:
        node_type * head_;
        size_type   size_;

    public:
        free_list(node_type * head = nullptr) : head_(head), size_(0) {}
        ~free_list() {
#ifndef NDEBUG
            this->clear();
#endif
        }

        node_type * begin() const { return this->head_; }
        node_type * end() const { return nullptr; }

        node_type * head() const { return this->head_; }
        size_type   size() const { return this->size_; }

        bool is_valid() const { return (this->head_ != nullptr); }
        bool is_empty() const { return (this->size_ == 0); }

        void set_head(node_type * head) {
            this->head_ = head;
        }
        void set_size(size_type size) {
            this->size_ = size;
        }

        void set_list(node_type * head, size_type size) {
            this->head_ = head;
            this->size_ = size;
        }

        void clear() {
            this->head_ = nullptr;
            this->size_ = 0;
        }

        void reset(node_type * head) {
            this->head_ = head;
            this->size_ = 0;
        }

        void increase() {
            ++(this->size_);
        }

        void decrease() {
            assert(this->size_ > 0);
            --(this->size_);
        }

        void inflate(size_type size) {
            this->size_ += size;
        }

        void deflate(size_type size) {
            assert(this->size_ >= size);
            this->size_ -= size;
        }

        void push_front(node_type * node) {
            assert(node != nullptr);
            node->next = this->head_;
            this->head_ = node;
            ++(this->size_);
        }

        node_type * pop_front() {
            assert(this->head_ != nullptr);
            node_type * node = this->head_;
            this->head_ = node->next;
            assert(this->size_ > 0);
            --(this->size_);
            return node;
        }

        node_type * front() {
            return this->head();
        }

        node_type * back() {
            node_type * prev = nullptr;
            node_type * node = this->head_;
            while (node != nullptr) {
                prev = node;
                node = node->next;
            }
            return prev;
        }

        void swap(free_list & right) {
            if (&right != this) {
                std::swap(this->head_, right.head_);
                std::swap(this->size_, right.size_);
            }
        }
    };

    template <typename T>
    inline void swap(free_list<T> & lhs, free_list<T> & rhs) {
        lhs.swap(rhs);
    }

    template <typename T>
    class entry_chunk {
    public:
        typedef T                               node_type;
        typedef typename this_type::size_type   size_type;

    protected:
        node_type *  entries_;
        size_type    size_;
        size_type    capacity_;

    public:
        entry_chunk() : entries_(nullptr), size_(0), capacity_(0) {}
        ~entry_chunk() {}

        node_type * entries() const { return this->entries_; }
        size_type size() const { return this->size_; }
        size_type capacity() const { return this->capacity_; }

        size_type is_full() const { return (this->size_ >= this->capacity_); }

        void set_entries(node_type * entries) {
            this->entries_ = entries;
        }

        void set_size(size_type size) {
            this->size_ = size;
        }

        void set_capacity(size_type capacity) {
            this->capacity_ = capacity;
        }

        void init(node_type * entries, size_type size, size_type capacity) {
            this->entries_ = entries;
            this->size_ = size;
            this->capacity_ = capacity;
        }

        void clear() {
            this->entries_ = nullptr;
            this->size_ = 0;
            this->capacity_ = 0;
        }

        void reset() {
            this->size_ = 0;
        }

        void increase() {
            ++(this->size_);
        }

        void decrease() {
            --(this->size_);
        }

        void inflate(size_type size) {
            this->size_ += size;
        }

        void deflate(size_type size) {
            assert(this->size_ >= size);
            this->size_ -= size;
        }
    };

    typedef free_list<entry_type>   free_list_t;
    typedef entry_chunk<entry_type> entry_chunk_t;

protected:
    entry_type **           buckets_;
    entry_type *            entries_;
    size_type               bucket_mask_;
    size_type               bucket_capacity_;
    size_type               entry_size_;
    size_type               entry_capacity_;
    entry_chunk_t           entry_chunk_;
    free_list_t             freelist_;
#if DICTIONARY_SUPPORT_VERSION
    size_type               version_;
#endif
    std::vector<entry_list> entries_list_;

    hasher_type             hasher_;
    key_equal               key_is_equal_;
    allocator_type          allocator_;

    allocator<entry_type *, 16> bucket_allocator_;
    allocator<entry_type, 16>   entry_allocator_;

    // Default initial capacity is 16.
    static const size_type kDefaultInitialCapacity = 16;
    // Minimum capacity is 8.
    static const size_type kMinimumCapacity = 8;
    // Maximum capacity is 1 << 31.
    static const size_type kMaximumCapacity = (std::numeric_limits<size_type>::max)() / 2 + 1;

    // The maximum entry's chunk bytes, default is 8 MB bytes.
    static const size_type kMaxEntryChunkBytes = 8 * 1024 * 1024;
    // The entry's block size per chunk (entry_type).
    static const size_type kEntryChunkSize =
            compile_time::round_to_power2<kMaxEntryChunkBytes / sizeof(entry_type)>::value;

    // The threshold of treeify to red-black tree.
    static const size_type kTreeifyThreshold = 8;
    // The invalid hash value.
    static const hash_code_t kInvalidHash = hash_traits<hash_code_t>::kInvalidHash;
    // The replacement value for invalid hash value.
    static const hash_code_t kReplacedHash = hash_traits<hash_code_t>::kReplacedHash;

public:
    BasicDictionary(size_type initialCapacity = kDefaultInitialCapacity)
        : buckets_(nullptr), entries_(nullptr),
          bucket_mask_(0), bucket_capacity_(0), entry_size_(0), entry_capacity_(0)
#if DICTIONARY_SUPPORT_VERSION
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
        return (iterator)&this->buckets_[0];
    }
    iterator unsafe_end() const {
        return (iterator)&this->buckets_[this->bucket_capacity_];
    }

    size_type __size() const {
        assert(this->entry_capacity_ >= this->freelist_.size());
        return (this->entry_capacity_ - this->freelist_.size());
    }
    size_type size() const {
        return this->entry_size_;
    }
    size_type capacity() const { return this->bucket_capacity_; }

    size_type bucket_mask() const { return this->bucket_mask_; }
    size_type bucket_count() const { return this->bucket_capacity_; }
    size_type bucket_capacity() const { return this->bucket_capacity_; }

    size_type entry_size() const { return this->size(); }
    size_type entry_count() const { return this->entry_capacity_; }

    entry_type ** buckets() const { return this->buckets_; }
    entry_type *  entries() const { return this->entries_; }

    size_type max_bucket_capacity() const {
        return ((std::numeric_limits<size_type>::max)() / 2 + 1);
    }
    size_type max_size() const {
        return this->max_bucket_capacity();
    }

    bool valid() const { return (this->buckets() != nullptr); }
    bool empty() const { return (this->size() == 0); }

    size_type version() const {
#if DICTIONARY_SUPPORT_VERSION
        return this->version_;
#else
        return 0;   /* Return 0 means that the version attribute is not supported. */
#endif
    }

protected:
    inline size_type calc_capacity(size_type capacity) const {
        capacity = (capacity >= kMinimumCapacity) ? capacity : kMinimumCapacity;
        capacity = (capacity <= kMaximumCapacity) ? capacity : kMaximumCapacity;
        capacity = run_time::round_up_to_pow2(capacity);
        return capacity;
    }

    inline hash_code_t get_hash(const key_type & key) const {
        hash_code_t hash_code = this->hasher_(key);
        //return hash_traits<hash_code_t>::filter(hash_code);
        return hash_code;
    }

    inline index_type index_of(hash_code_t hash_code, size_type capacity_mask) const {
        return (index_type)((size_type)hash_code & capacity_mask);
    }

    inline index_type index_of(size_type hash_code, size_type capacity_mask) const {
        return (index_type)(hash_code & capacity_mask);
    }

    inline index_type next_index(index_type index, size_type capacity_mask) const {
        ++index;
        return (index_type)((size_type)index & capacity_mask);
    }

    void free_buckets_impl() {
        assert(this->buckets_ != nullptr);
        bucket_allocator_.deallocate(this->buckets_, this->bucket_capacity_);
    }

    void free_buckets() {
        if (likely(this->buckets_ != nullptr)) {
            this->free_buckets_impl();
        }
    }

    void destroy_entries() {
        if (likely(this->entries_list_.size() > 0)) {
            size_type last_index = this->entries_list_.size() - 1;
            for (size_type i = 0; i < last_index; i++) {
                entry_type * entries = this->entries_list_[i].entries;
                size_type   capacity = this->entries_list_[i].capacity;
                entry_type * entry = entries;
                assert(entry != nullptr);
                for (size_type j = 0; j < capacity; j++) {
                    if (likely(entry->flags != 0)) {
                        value_type * pair_ptr = &entry->value;
                        assert(pair_ptr != nullptr);
                        allocator_.destructor(pair_ptr);
                    }
                    ++entry;
                }

                // Free the entries buffer.
                entry_allocator_.deallocate(entries, capacity);
            }

            // last_index
            {
                entry_type *    entries = this->entries_list_[last_index].entries;
                size_type      capacity = this->entries_list_[last_index].capacity;
                size_type last_capacity = this->entry_chunk_.capacity();
                size_type     last_size = this->entry_chunk_.size();
                assert(entries == this->entries_);
                assert(last_size <= capacity);
                assert(last_capacity == capacity);
                entry_type * entry = entries;
                assert(entry != nullptr);
                for (size_type j = 0; j < last_size; j++) {
                    if (likely(entry->flags != 0)) {
                        value_type * pair_ptr = &entry->value;
                        assert(pair_ptr != nullptr);
                        allocator_.destructor(pair_ptr);
                    }
                    ++entry;
                }

                // Free the entries buffer.
                entry_allocator_.deallocate(entries, capacity);
            }
        }
    }

    void destroy() {
        // Destroy the resources.
        if (likely(this->buckets_ != nullptr)) {
            if (likely(this->entries_ != nullptr)) {
                // Destroy all entries.
                this->destroy_entries();
                this->entries_ = nullptr;
#ifndef NDEBUG
                this->entries_list_.clear();
#endif
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

    // Init the new entries's status.
    void init_entries_chunk(entry_type * entries, size_type capacity) {
        entry_type * entry = entries;
        for (size_type i = 0; i < capacity; i++) {
            entry->flags = 0;
            entry++;
        }
    }

    // Append all the entries to the free list.
    void add_entries_to_freelist(entry_type * entries, size_type capacity) {
        assert(entries != nullptr);
        assert(capacity > 0);
        entry_type * entry = entries + capacity - 1;
        entry_type * prev = entries + capacity;
        for (size_type i = 0; i < capacity; i++) {
            entry->next = prev;
            entry->flags = 0;
            entry--;
            prev--;
        }

#if 1
        entry_type * last_entry = entries + capacity - 1;
        last_entry->next = this->freelist_.head();
        this->freelist_.set_head(entries);
        this->freelist_.inflate(capacity);
#else
        entry_type * last_entry = entries + capacity - 1;
        last_entry->next = nullptr;

        entry_type * tail = this->freelist_.back();
        if (likely(tail == nullptr)) {
            this->freelist_.set_list(entries, capacity);
        }
        else {
            tail->next = entries;
            this->freelist_.inflate(capacity);
        }
#endif
    }

    size_type count_entries_size() {
        size_type entry_size = 0;
        for (size_type index = 0; index < this->bucket_capacity_; index++) {
            size_type list_size = 0;
            entry_type * entry = this->buckets_[index];
            while (likely(entry != nullptr)) {
                hash_code_t hash_code = entry->hash_code;
                index_type now_index = this->index_of(hash_code, this->bucket_mask_);
                assert(now_index == index);
                assert(entry->flags == 1);

                list_size++;
                entry = entry->next;
            }
            entry_size += list_size;
        }
        return entry_size;
    }

    JSTD_FORCEINLINE
    entry_type * get_free_entry(hash_code_t & hash_code, index_type & index) {
        if (likely(this->freelist_.is_empty())) {
            if (unlikely(this->entry_chunk_.is_full())) {
                size_type old_size = this->size();

                // Inflate the entry for 1.
                this->inflate_entries(1);

                size_type new_size = this->count_entries_size();
                assert(new_size == old_size);

                // Recalculate the bucket index.
                index = this->index_of(hash_code, this->bucket_mask_);
            }

            // Get a unused entry.
            entry_type * new_entry = &this->entries_[this->entry_chunk_.size()];
            assert(new_entry != nullptr);
            this->entry_chunk_.increase();
            return new_entry;
        }
        else {
            // Pop a free entry from freelist.
            return this->freelist_.pop_front();
        }
    }

    void initialize(size_type init_capacity) {
        size_type entry_capacity = run_time::round_up_to_pow2(init_capacity);
        assert(entry_capacity > 0);
        assert((entry_capacity & (entry_capacity - 1)) == 0);

        size_type bucket_capacity = entry_capacity;

        // The the array of bucket's first entry.
        entry_type ** new_buckets = bucket_allocator_.allocate(bucket_capacity);
        if (likely(bucket_allocator_.is_ok(new_buckets))) {
            // Initialize the buckets's data.
            ::memset((void *)new_buckets, 0, bucket_capacity * sizeof(entry_type *));

            // Save the buckets info.
            this->buckets_ = new_buckets;
            this->bucket_mask_ = bucket_capacity - 1;
            this->bucket_capacity_ = bucket_capacity;

            // The array of entries.
            entry_type * new_entries = entry_allocator_.allocate(entry_capacity);
            if (likely(entry_allocator_.is_ok(new_entries))) {
                // Save the entries info.
                this->entries_ = new_entries;
                this->entry_size_ = 0;
                this->entry_capacity_ = entry_capacity;

                this->entries_list_.reserve(4);
                this->entries_list_.emplace_back(new_entries, entry_capacity);

                this->entry_chunk_.init(new_entries, 0, entry_capacity);

                //this->init_entries_chunk(new_entries, entry_capacity);

                this->freelist_.clear();
            }
        }
    }

    void rehash_all_entries_2x(entry_type ** new_buckets, size_type new_bucket_capacity) {
        assert(this->buckets_ != nullptr);
        assert(new_buckets != nullptr);
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_bucket_capacity));

        size_type new_bucket_mask = new_bucket_capacity - 1;

        for (size_type index = 0; index < this->bucket_capacity_; index++) {
            entry_type * entry = this->buckets_[index];
            if (likely(entry != nullptr)) {
                entry_type * first_entry = nullptr;
                entry_type * prev_entry = nullptr;
                entry_type * new_entry = nullptr;
                do {
                    hash_code_t hash_code = entry->hash_code;
                    index_type new_index = this->index_of(hash_code, new_bucket_mask);
                    if (likely(new_index != index)) {
                        if (prev_entry != nullptr) {
                            prev_entry->next = entry->next;
                        }
                        entry_type * next_entry = entry->next;

                        entry->next = new_entry;
                        new_entry = entry;

                        entry = next_entry;
                    }
                    else {
                        prev_entry = entry;
                        entry = entry->next;
                        if (unlikely(first_entry == nullptr)) {
                            first_entry = prev_entry;
                        }
                    }
                } while (likely(entry != nullptr));

                if (likely(first_entry != nullptr)) {
                    index_type new_index;
                    if (likely(new_bucket_capacity >= this->bucket_capacity_))
                        new_index = index;
                    else
                        new_index = this->index_of(index, new_bucket_mask);

                    new_buckets[new_index] = first_entry;
                }

                if (likely(new_entry != nullptr)) {
                    index_type new_index = index + new_bucket_capacity / 2;
                    if (likely(new_bucket_capacity < this->bucket_capacity_)) {
                        new_index = this->index_of(new_index, new_bucket_mask);
                    }

                    new_buckets[new_index] = new_entry;
                }
            }
        }
    }

    void rehash_all_entries(entry_type ** new_buckets, size_type new_bucket_capacity) {
        assert(this->buckets_ != nullptr);
        assert(new_buckets != nullptr);
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_bucket_capacity));

        size_type new_bucket_mask = new_bucket_capacity - 1;

        for (size_type index = 0; index < this->bucket_capacity_; index++) {
            entry_type * entry = this->buckets_[index];
            if (likely(entry != nullptr)) {
                entry_type * first_entry = nullptr;
                entry_type * prev_entry = nullptr;
                do {
                    hash_code_t hash_code = entry->hash_code;
                    index_type new_index = this->index_of(hash_code, new_bucket_mask);
                    if (likely(new_index != index)) {
                        if (prev_entry != nullptr) {
                            prev_entry->next = entry->next;
                        }
                        entry_type * next_entry = entry->next;

                        entry->next = new_buckets[new_index];
                        new_buckets[new_index] = entry;

                        entry = next_entry;
                    }
                    else {
                        prev_entry = entry;
                        entry = entry->next;
                        if (unlikely(first_entry == nullptr)) {
                            first_entry = prev_entry;
                        }
                    }
                } while (likely(entry != nullptr));

                if (likely(first_entry != nullptr)) {
                    index_type new_index;
                    if (likely(new_bucket_capacity >= this->bucket_capacity_))
                        new_index = index;
                    else
                        new_index = this->index_of(index, new_bucket_mask);

                    new_buckets[new_index] = first_entry;
                }
            }
        }
    }

    void rehash_buckets(size_type new_bucket_capacity) {
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_bucket_capacity));
        //assert(new_bucket_capacity > this->bucket_capacity_);
        assert(this->entry_size_ <= this->bucket_capacity_);

        entry_type ** new_buckets = bucket_allocator_.allocate(new_bucket_capacity);
        if (likely(bucket_allocator_.is_ok(new_buckets))) {
            // Initialize the bucket list's data.
            ::memset((void *)new_buckets, 0, new_bucket_capacity * sizeof(entry_type *));

            if (likely(new_bucket_capacity == this->bucket_capacity_ * 2))
                this->rehash_all_entries_2x(new_buckets, new_bucket_capacity);
            else
                this->rehash_all_entries(new_buckets, new_bucket_capacity);

            this->free_buckets_impl();

            this->buckets_ = new_buckets;
            this->bucket_mask_ = new_bucket_capacity - 1;
            this->bucket_capacity_ = new_bucket_capacity;

            this->updateVersion();
        }
    }

    void rehash_buckets_and_entries(size_type new_entry_capacity, size_type new_bucket_capacity) {
        assert(new_entry_capacity > 0);
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_entry_capacity));
        assert(run_time::is_pow2(new_bucket_capacity));
        assert(new_bucket_capacity > this->bucket_capacity_);

        entry_type ** new_buckets = bucket_allocator_.allocate(new_bucket_capacity);
        if (likely(bucket_allocator_.is_ok(new_buckets))) {
            // Initialize the bucket list's data.
            ::memset((void *)new_buckets, 0, new_bucket_capacity * sizeof(entry_type *));

            // Only allocate a chunk size we need.
            assert(new_entry_capacity > this->entry_capacity_);
            size_type new_entry_size = new_entry_capacity - this->entry_capacity_;

            entry_type * new_entries = entry_allocator_.allocate(new_entry_size);
            if (likely(entry_allocator_.is_ok(new_entries))) {
                if (likely(new_bucket_capacity == this->bucket_capacity_ * 2))
                    this->rehash_all_entries_2x(new_buckets, new_bucket_capacity);
                else
                    this->rehash_all_entries(new_buckets, new_bucket_capacity);

                this->free_buckets_impl();

                this->buckets_ = new_buckets;
                this->entries_ = new_entries;
                this->bucket_mask_ = new_bucket_capacity - 1;
                this->bucket_capacity_ = new_bucket_capacity;
                // Here, the entry_size_ doesn't change.
                this->entry_capacity_ = new_entry_capacity;

                // Push the new entries pointer to entries list.
                this->entries_list_.emplace_back(new_entries, new_entry_size);

                assert(this->entry_chunk_.is_full());
                this->entry_chunk_.init(new_entries, 0, new_entry_size);

                //this->init_entries_chunk(new_entries, new_entry_size);

                this->updateVersion();
            }
            else {
                // Failed to allocate new_entries, processing the abnormal exit.
                bucket_allocator_.deallocate(new_buckets, new_bucket_capacity);
            }
        }
    }

    template <bool need_shrink = false>
    void rehash_internal(size_type new_bucket_capacity) {
        assert(new_bucket_capacity > 0);
        assert((new_bucket_capacity & (new_bucket_capacity - 1)) == 0);

        size_type new_entry_capacity = run_time::round_up_to_pow2(this->entry_size_ + 1);
        if (likely(new_bucket_capacity > this->bucket_capacity_)) {
            // Is infalte
        }
        else if (likely(new_bucket_capacity < this->bucket_capacity_)) {
            if (likely(new_entry_capacity > new_bucket_capacity)) {
                new_bucket_capacity = new_entry_capacity;
            }
            if (likely(new_bucket_capacity > this->bucket_capacity_)) {
                // Is infalte
            }
            else {
                // We need to shrink the bucket list capacity.
            }
        }
        else {
            // The bucket capacity is unchanged.
            return;
        }

        if (likely(new_entry_capacity <= this->entry_capacity_)) {
            return this->rehash_buckets(new_bucket_capacity);
        }
        else {
            return this->rehash_buckets_and_entries(new_entry_capacity, new_bucket_capacity);
        }
    }

    void inflate_entries(size_type size = 1) {
        size_type new_entry_capacity = run_time::round_up_to_pow2(this->entry_size_ + size);

        // If new entry capacity is too small, exit directly.
        if (likely(new_entry_capacity > this->entry_capacity_)) {
            // Most of the time, we don't need to reallocate buckets list.
            if (likely(new_entry_capacity <= this->bucket_capacity_)) {
                assert(this->freelist_.is_empty());

                size_type new_entry_size = new_entry_capacity - this->entry_capacity_;
                entry_type * new_entries = entry_allocator_.allocate(new_entry_size);
                if (likely(entry_allocator_.is_ok(new_entries))) {
                    this->entries_ = new_entries;
                    this->entry_capacity_ = new_entry_capacity;

                    // Push the new entries pointer to entries list.
                    this->entries_list_.emplace_back(new_entries, new_entry_size);

                    assert(this->entry_chunk_.is_full());
                    this->entry_chunk_.init(new_entries, 0, new_entry_size);

                    //this->init_entries_chunk(new_entries, new_entry_size);
                }
            }
            else {
                // The bucket list capacity is full, need to reallocate.
                this->rehash_buckets_and_entries(new_entry_capacity, new_entry_capacity);
            }
        }
        else {
            // this->entry_capacity_ = this->bucket_capacity_ ?
            assert(new_entry_capacity <= this->bucket_capacity_);
        }
    }

    JSTD_FORCEINLINE
    entry_type * find_internal(const key_type & key, hash_code_t hash_code, index_type index) {
        assert(this->buckets() != nullptr);
        entry_type * entry = this->buckets_[index];
        while (likely(entry != nullptr)) {
            if (likely(entry->hash_code != hash_code)) {
                entry = entry->next;
            }
            else {
                if (likely(this->key_is_equal_(key, entry->value.first))) {
                    return entry;
                }
                entry = entry->next;
            }
        }

        return nullptr;  // Not found
    }

    JSTD_FORCEINLINE
    entry_type * find_before(const key_type & key, entry_type *& before, size_type & index) {
        hash_code_t hash_code = this->get_hash(key);
        index = this->index_of(hash_code, this->bucket_mask_);

        assert(this->buckets() != nullptr);
        entry_type * prev = nullptr;
        entry_type * entry = this->buckets_[index];
        while (likely(entry != nullptr)) {
            if (likely(entry->hash_code != hash_code)) {
                prev = entry;
                entry = entry->next;
            }
            else {
                if (likely(this->key_is_equal_(key, entry->value.first))) {
                    before = prev;
                    return entry;
                }
                entry = entry->next;
            }
        }

        return nullptr;  // Not found
    }

    void updateVersion() {
#if DICTIONARY_SUPPORT_VERSION
        ++(this->version_);
#endif
    }

public:
    void clear() {
        if (likely(this->buckets_ != nullptr)) {
            // Initialize the buckets's data.
            ::memset((void *)this->buckets_, 0, this->bucket_capacity_ * sizeof(entry_type *));
        }

        // Clear settings
        this->entry_size_ = 0;
        this->freelist_.clear();
        this->entry_chunk_.clear();
    }

    void rehash(size_type bucket_count) {
        assert(bucket_count > 0);
        size_type new_bucket_capacity = this->calc_capacity(bucket_count);
        this->rehash_internal<false>(new_bucket_capacity);
    }

    void reserve(size_type bucket_count) {
        this->rehash(bucket_count);
    }

    void shrink_to_fit(size_type bucket_count = 0) {
        size_type entry_capacity = run_time::round_up_to_pow2(this->entry_size_);

        // Choose the maximum size of new bucket capacity and now entry capacity.
        bucket_count = (entry_capacity >= bucket_count) ? entry_capacity : bucket_count;

        size_type new_bucket_capacity = this->calc_capacity(bucket_count);
        this->rehash_internal<true>(new_bucket_capacity);
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
        if (likely(this->buckets() != nullptr)) {
            hash_code_t hash_code = this->get_hash(key);
            index_type index = this->index_of(hash_code, this->bucket_mask_);
            entry_type * entry = this->find_internal(key, hash_code, index);
            if (likely(entry == nullptr)) {
                entry_type * new_entry = this->get_free_entry(hash_code, index);
                assert(new_entry != nullptr);
                //assert(new_entry->flags == 0);

                new_entry->next = this->buckets_[index];
                new_entry->hash_code = hash_code;
                new_entry->flags = 1;
                this->buckets_[index] = new_entry;

                this->entry_size_++;

                // pair_type class placement new
                void * pair_ptr = (void *)&new_entry->value;
                value_type * new_pair = allocator_.constructor(pair_ptr, key, value);
                assert(new_pair == &new_entry->value);
            }
            else {
                // Update the existed key's value.
                entry->value.second = value;
            }

            this->updateVersion();
        }
    }

    void insert(key_type && key, mapped_type && value) {
        if (likely(this->buckets() != nullptr)) {
            hash_code_t hash_code = this->get_hash(std::forward<key_type>(key));
            index_type index = this->index_of(hash_code, this->bucket_mask_);
            entry_type * entry = this->find_internal(std::forward<key_type>(key), hash_code, index);
            if (likely(entry == nullptr)) {
                entry_type * new_entry = this->get_free_entry(hash_code, index);
                assert(new_entry != nullptr);
                //assert(new_entry->flags == 0);

                new_entry->next = this->buckets_[index];
                new_entry->hash_code = hash_code;
                new_entry->flags = 1;
                this->buckets_[index] = new_entry;

                this->entry_size_++;

                // pair_type class placement new
                void * pair_ptr = (void *)&new_entry->value;
                value_type * new_pair = allocator_.constructor(pair_ptr,
                                                               std::forward<key_type>(key),
                                                               std::forward<mapped_type>(value));
                assert(new_pair == &new_entry->value);
            }
            else {
                // Update the existed key's value.
                entry->value.second = std::move(std::forward<mapped_type>(value));
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

            entry_type * prev = nullptr;
            entry_type * entry = this->buckets_[index];
            while (likely(entry != nullptr)) {
                if (likely(entry->hash_code != hash_code)) {
                    prev = entry;
                    entry = entry->next;
                }
                else {
                    if (likely(this->key_is_equal_(key, entry->value.first))) {
                        if (likely(prev != nullptr))
                            prev->next = entry->next;
                        else
                            this->buckets_[index] = entry->next;

                        assert(entry->flags == 1);
                        entry->flags = 0;
                        this->freelist_.push_front(entry);

                        // pair_type class placement delete
                        value_type * pair_ptr = &entry->value;
                        assert(pair_ptr != nullptr);
                        allocator_.destructor(pair_ptr);

                        this->entry_size_--;

                        this->updateVersion();

                        // Has found
                        return size_type(1);
                    }

                    prev = entry;
                    entry = entry->next;
                }
            }
        }

        // Not found
        return size_type(0);
    }

    void dump() {
        printf("jstd::BasicDictionary<K, V>::dump()\n\n");
    }

    static const char * name() {
        switch (HashFunc) {
        case HashFunc_CRC32C:
            return "jstd::Dictionary<K, V> (CRC32c)";
        case HashFunc_Time31:
            return "jstd::Dictionary<K, V> (Time31)";
        case HashFunc_Time31Std:
            return "jstd::Dictionary<K, V> (Time31Std)";
        default:
            return "jstd::Dictionary<K, V> (Unknown)";
        }
    }
}; // BasicDictionary<K, V>

template <typename Key, typename Value, std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary_Time31 = BasicDictionary<Key, Value, HashFunc_Time31, Alignment>;

template <typename Key, typename Value, std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary_Time31Std = BasicDictionary<Key, Value, HashFunc_Time31Std, Alignment>;

#if SUPPORT_SSE42_CRC32C
template <typename Key, typename Value, std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary_crc32c = BasicDictionary<Key, Value, HashFunc_CRC32C, Alignment>;

template <typename Key, typename Value, std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary = BasicDictionary<Key, Value, HashFunc_CRC32C, Alignment>;
#else
template <typename Key, typename Value, std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary = BasicDictionary<Key, Value, HashFunc_Time31, Alignment>;
#endif // SUPPORT_SSE42_CRC32C

} // namespace jstd

#endif // JSTD_HASH_DICTIONARY_H
