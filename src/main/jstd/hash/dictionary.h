
#ifndef JSTD_HASH_DICTIONARY_H
#define JSTD_HASH_DICTIONARY_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"
#include "jstd/basic/inttypes.h"

#include <memory.h>
#include <math.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <memory>
#include <limits>
#include <vector>
#include <tuple>
#include <utility>
#include <type_traits>

#ifndef ENABLE_JSTD_DICTIONARY

#define ENABLE_JSTD_DICTIONARY                  1
#define DICTIONARY_ENTRY_USE_PLACEMENT_NEW      1

// The entry's pair whether release on erase the entry.
#define DICTIONARY_ENTRY_RELEASE_ON_ERASE       1
#define DICTIONARY_USE_FAST_REHASH_MODE         1
#define DICTIONARY_SUPPORT_VERSION              0

#endif // ENABLE_JSTD_DICTIONARY

#include "jstd/allocator.h"
#include "jstd/iterator.h"
#include "jstd/hash/hash_helper.h"
#include "jstd/hash/equal_to.h"
#include "jstd/hash/dictionary_traits.h"
#include "jstd/hash/key_extractor.h"
#include "jstd/support/PowerOf2.h"

#define USE_JAVA_FIND_ENTRY     1

namespace jstd {

template < typename Key, typename Value,
           std::size_t HashFunc = HashFunc_Default,
           std::size_t Alignment = align_of<std::pair<const Key, Value>>::value,
           typename Hasher = hash<Key, std::uint32_t, HashFunc>,
           typename KeyEqual = equal_to<Key>,
           typename Allocator = allocator<std::pair<const Key, Value>, Alignment, false> >
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
        typedef hash_entry *                    node_pointer;
        typedef hash_entry &                    node_reference;
        typedef const hash_entry *              const_node_pointer;
        typedef const hash_entry &              const_node_reference;
        typedef typename this_type::value_type  value_type;

        hash_entry * next;
        hash_code_t  hash_code;
        uint32_t     flags;
        this_type *  owner;
        alignas(Alignment)
        value_type   value;

        hash_entry() : next(nullptr), hash_code(0), flags(0), owner(nullptr) {}

        ~hash_entry() {
#ifndef NDEBUG
            next = nullptr;
#endif
        }
    };

    typedef hash_entry                                  entry_type;
    typedef typename hash_entry::node_pointer           node_pointer;
    typedef typename hash_entry::node_reference         node_reference;
    typedef typename hash_entry::const_node_pointer     const_node_pointer;
    typedef typename hash_entry::const_node_reference   const_node_reference;

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
            clear();
#endif
        }

        node_type * begin() const { return head_; }
        node_type * end() const { return nullptr; }

        node_type * head() const { return head_; }
        size_type   size() const { return size_; }

        bool is_valid() const { return (head_ != nullptr); }
        bool is_empty() const { return (size_ == 0); }

        void set_head(node_type * head) {
            head_ = head;
        }
        void set_size(size_type size) {
            size_ = size;
        }

        void set_list(node_type * head, size_type size) {
            head_ = head;
            size_ = size;
        }

        void clear() {
            head_ = nullptr;
            size_ = 0;
        }

        void reset(node_type * head) {
            head_ = head;
            size_ = 0;
        }

        void increase() {
            ++(size_);
        }

        void decrease() {
            assert(size_ > 0);
            --(size_);
        }

        void inflate(size_type size) {
            size_ += size;
        }

        void deflate(size_type size) {
            assert(size_ >= size);
            size_ -= size;
        }

        void push_front(node_type * node) {
            assert(node != nullptr);
            node->next = head_;
            head_ = node;
            ++(size_);
        }

        node_type * pop_front() {
            assert(head_ != nullptr);
            node_type * node = head_;
            head_ = node->next;
            assert(size_ > 0);
            --(size_);
            return node;
        }

        node_type * front() {
            return head();
        }

        node_type * back() {
            node_type * prev = nullptr;
            node_type * node = head_;
            while (node != nullptr) {
                prev = node;
                node = node->next;
            }
            return prev;
        }

        void swap(free_list & right) {
            if (&right != this) {
                std::swap(head_, right.head_);
                std::swap(size_, right.size_);
            }
        }
    };

    template <typename T>
    inline void swap(free_list<T> & lhs, free_list<T> & rhs) {
        lhs.swap(rhs);
    }

    #include "jstd/hash/hash_iterator.inc"

    typedef iterator_t<this_type, entry_type>                iterator;
    typedef const_iterator_t<this_type, entry_type>          const_iterator;

    typedef local_iterator_t<this_type, entry_type>          local_iterator;
    typedef const_local_iterator_t<this_type, entry_type>    const_local_iterator;

    typedef std::pair<iterator, bool>   insert_return_type;

    template <typename T>
    class entry_chunk {
    public:
        typedef T                               entry_type;
        typedef typename this_type::size_type   size_type;

    protected:
        entry_type * entries_;
        size_type    size_;
        size_type    capacity_;

    public:
        entry_chunk() : entries_(nullptr), size_(0), capacity_(0) {}
        ~entry_chunk() {}

        entry_type * entries() const { return entries_; }
        size_type size() const { return size_; }
        size_type capacity() const { return capacity_; }

        size_type is_full() const { return (size_ >= capacity_); }

        void set_entries(entry_type * entries) {
            entries_ = entries;
        }

        void set_size(size_type size) {
            size_ = size;
        }

        void set_capacity(size_type capacity) {
            capacity_ = capacity;
        }

        void init(entry_type * entries, size_type size, size_type capacity) {
            entries_ = entries;
            size_ = size;
            capacity_ = capacity;
        }

        void clear() {
            entries_ = nullptr;
            size_ = 0;
            capacity_ = 0;
        }

        void reset() {
            size_ = 0;
        }

        void increase() {
            ++(size_);
        }

        void decrease() {
            --(size_);
        }

        void inflate(size_type size) {
            size_ += size;
        }

        void deflate(size_type size) {
            assert(size_ >= size);
            size_ -= size;
        }
    };

    typedef free_list<entry_type>   free_list_t;
    typedef entry_chunk<entry_type> entry_chunk_t;

protected:
    entry_type **            buckets_;
    entry_type *             entries_;
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

    allocator<entry_type *> bucket_allocator_;
    allocator<entry_type>   entry_allocator_;

    std::allocator<mapped_type>                     value_allocator_;
    typename std::allocator<value_type>::allocator  pair_allocator_;

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
        initialize(initialCapacity);
    }

    ~BasicDictionary() {
        destroy();
    }

    iterator begin() const {
        return iterator(find_first_valid_node());
    }
    iterator end() const {
        return iterator(nullptr);
    }

    const_iterator cbegin() const {
        return const_iterator(iterator(find_first_valid_node()));
    }
    const_iterator cend() const {
        return const_iterator(nullptr);
    }

    local_iterator l_begin() const {
        return local_iterator(find_first_valid_node());
    }
    local_iterator l_end() const {
        return local_iterator(nullptr);
    }

    const_local_iterator l_cbegin() const {
        return const_local_iterator(local_iterator(find_first_valid_node()));
    }
    const_local_iterator l_cend() const {
        return const_local_iterator(nullptr);
    }

    size_type __size() const {
        assert(entry_capacity_ >= freelist_.size());
        return (entry_capacity_ - freelist_.size());
    }
    size_type size() const {
        return entry_size_;
    }
    size_type capacity() const { return bucket_capacity_; }

    size_type bucket_mask() const { return bucket_mask_; }
    size_type bucket_count() const { return bucket_capacity_; }
    size_type bucket_capacity() const { return bucket_capacity_; }

    size_type entry_size() const { return size(); }
    size_type entry_count() const { return entry_capacity_; }

    entry_type ** buckets() const { return buckets_; }
    entry_type *  entries() const { return entries_; }

    size_type max_bucket_capacity() const {
        return ((std::numeric_limits<size_type>::max)() / 2 + 1);
    }
    size_type max_size() const {
        return max_bucket_capacity();
    }

    bool valid() const { return (buckets() != nullptr); }
    bool empty() const { return (size() == 0); }

    size_type bucket_size(size_type index) const {
        assert(index < bucket_count());

        size_type count = 0;
        entry_type * node = get_bucket_head(index);
        while (likely(node != nullptr)) {
            count++;
            node = node->next;
        }
        return count;
    }

    size_type version() const {
#if DICTIONARY_SUPPORT_VERSION
        return version_;
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

    inline index_type index_of(hash_code_t hash_code) const {
        return (index_type)((size_type)hash_code & bucket_mask());
    }

    inline index_type index_of(hash_code_t hash_code, size_type capacity_mask) const {
        return (index_type)((size_type)hash_code & capacity_mask);
    }

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    inline index_type index_of(size_type hash_code) const {
        return (index_type)(hash_code & bucket_mask());
    }

    inline index_type index_of(size_type hash_code, size_type capacity_mask) const {
        return (index_type)(hash_code & capacity_mask);
    }
#endif // __amd64__

    JSTD_FORCEINLINE
    index_type index_for(hash_code_t hash_code) const {
        return (index_type)((size_type)hash_code & bucket_mask_);
    }

    inline index_type next_index(index_type index, size_type capacity_mask) const {
        ++index;
        return (index_type)((size_type)index & capacity_mask);
    }

    void free_buckets_impl() {
        assert(buckets_ != nullptr);
        bucket_allocator_.deallocate(buckets_, bucket_capacity_);
    }

    void free_buckets() {
        if (likely(buckets_ != nullptr)) {
            free_buckets_impl();
        }
    }

    void destroy_entries() {
        if (likely(entries_list_.size() > 0)) {
            size_type last_index = entries_list_.size() - 1;
            for (size_type i = 0; i < last_index; i++) {
                entry_type * entries = entries_list_[i].entries;
                size_type   capacity = entries_list_[i].capacity;
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

            // Destroy last entry
            {
                entry_type *    entries  = entries_list_[last_index].entries;
                size_type      capacity = entries_list_[last_index].capacity;
                size_type last_capacity = entry_chunk_.capacity();
                size_type     last_size = entry_chunk_.size();
                assert(entries == entries_);
                assert(last_size <= capacity);
                assert(last_capacity == capacity);
                entry_type * entry = entries;
                assert(entry != nullptr);
                for (size_type j = 0; j < last_size; j++) {
                    if (likely(entry->flags != 0)) {
                        value_type * value_ptr = &entry->value;
                        assert(value_ptr != nullptr);
                        allocator_.destructor(value_ptr);
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
        if (likely(buckets_ != nullptr)) {
            if (likely(entries_ != nullptr)) {
                // Destroy all entries.
                destroy_entries();
                entries_ = nullptr;
#ifndef NDEBUG
                entries_list_.clear();
#endif
            }

            // Free the array of bucket.
            free_buckets_impl();
            buckets_ = nullptr;
        }

        freelist_.clear();

        // Clear settings
        bucket_mask_ = 0;
        bucket_capacity_ = 0;
        entry_size_ = 0;
        entry_capacity_ = 0;
    }

    entry_type * get_bucket_head(index_type index) const {
        return buckets_[index];
    }

    entry_type * find_first_valid_node() const {
        index_type index = 0;
        entry_type * first = buckets_[index];
        if (likely(first == nullptr)) {
            do {
                index++;
                if (likely(index < bucket_count())) {
                    first = buckets_[index];
                    if (unlikely(first != nullptr))
                        return first;
                }
                else {
                    return nullptr;
                }
            } while (1);
        }
        else {
            return first;
        }
    }

    entry_type * next_iterator(entry_type * node_ptr) {
        if (likely(node_ptr->next != nullptr)) {
            node_ptr = node_ptr->next;
            return node_ptr;
        }
        else {
            index_type index = index_for(node_ptr->hash_code);
            index++;
            if (likely(index < bucket_count())) {
                do {
                    entry_type * node = get_bucket_head(index);
                    if (likely(node != nullptr)) {
                        node_ptr = node;
                        break;
                    }
                    index++;
                    if (unlikely(index >= bucket_count())) {
                        node_ptr = nullptr;
                        break;
                    }
                } while (1);
            }
            else {
                node_ptr = nullptr;
            }

            return node_ptr;
        }
    }

    const entry_type * next_const_iterator(const entry_type * node_ptr) {
        entry_type * node = const_cast<entry_type *>(node_ptr);
        node = next_iterator(node);
        return const_cast<const entry_type *>(node);
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
        last_entry->next = freelist_.head();
        freelist_.set_head(entries);
        freelist_.inflate(capacity);
#else
        entry_type * last_entry = entries + capacity - 1;
        last_entry->next = nullptr;

        entry_type * tail = freelist_.back();
        if (likely(tail == nullptr)) {
            freelist_.set_list(entries, capacity);
        }
        else {
            tail->next = entries;
            freelist_.inflate(capacity);
        }
#endif
    }

    size_type count_entries_size() {
        size_type entry_size = 0;
        for (size_type index = 0; index < bucket_capacity_; index++) {
            size_type list_size = 0;
            entry_type * entry = buckets_[index];
            while (likely(entry != nullptr)) {
                hash_code_t hash_code = entry->hash_code;
                index_type now_index = index_of(hash_code);
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
    entry_type * get_free_entry(hash_code_t hash_code, index_type & index) {
        if (likely(freelist_.is_empty())) {
            if (unlikely(entry_chunk_.is_full())) {
                size_type old_size = size();

                // Inflate the entry for 1.
                inflate_entries(1);

                size_type new_size = count_entries_size();
                assert(new_size == old_size);

                // Recalculate the bucket index.
                index = index_of(hash_code);
            }

            // Get a unused entry.
            entry_type * new_entry = &entries_[entry_chunk_.size()];
            assert(new_entry != nullptr);
            entry_chunk_.increase();
            return new_entry;
        }
        else {
            // Pop a free entry from freelist.
            return freelist_.pop_front();
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
            buckets_ = new_buckets;
            bucket_mask_ = bucket_capacity - 1;
            bucket_capacity_ = bucket_capacity;

            // The array of entries.
            entry_type * new_entries = entry_allocator_.allocate(entry_capacity);
            if (likely(entry_allocator_.is_ok(new_entries))) {
                // Save the entries info.
                entries_ = new_entries;
                entry_size_ = 0;
                entry_capacity_ = entry_capacity;

                entries_list_.reserve(4);
                entries_list_.emplace_back(new_entries, entry_capacity);

                entry_chunk_.init(new_entries, 0, entry_capacity);

                //init_entries_chunk(new_entries, entry_capacity);

                freelist_.clear();
            }
        }
    }

    void rehash_all_entries_2x(entry_type ** new_buckets, size_type new_bucket_capacity) {
        assert(buckets_ != nullptr);
        assert(new_buckets != nullptr);
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_bucket_capacity));

        size_type new_bucket_mask = new_bucket_capacity - 1;

        for (size_type index = 0; index < bucket_capacity_; index++) {
            entry_type * entry = buckets_[index];
            if (likely(entry != nullptr)) {
                entry_type * first_entry = nullptr;
                entry_type * prev_entry = nullptr;
                entry_type * new_entry = nullptr;
                do {
                    hash_code_t hash_code = entry->hash_code;
                    index_type new_index = index_of(hash_code, new_bucket_mask);
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
                    if (likely(new_bucket_capacity >= bucket_capacity_))
                        new_index = index;
                    else
                        new_index = index_of(index, new_bucket_mask);

                    new_buckets[new_index] = first_entry;
                }

                if (likely(new_entry != nullptr)) {
                    index_type new_index = index + new_bucket_capacity / 2;
                    if (likely(new_bucket_capacity < bucket_capacity_)) {
                        new_index = index_of(new_index, new_bucket_mask);
                    }

                    new_buckets[new_index] = new_entry;
                }
            }
        }
    }

    void rehash_all_entries(entry_type ** new_buckets, size_type new_bucket_capacity) {
        assert(buckets_ != nullptr);
        assert(new_buckets != nullptr);
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_bucket_capacity));

        size_type new_bucket_mask = new_bucket_capacity - 1;

        for (size_type index = 0; index < bucket_capacity_; index++) {
            entry_type * entry = buckets_[index];
            if (likely(entry != nullptr)) {
                entry_type * first_entry = nullptr;
                entry_type * prev_entry = nullptr;
                do {
                    hash_code_t hash_code = entry->hash_code;
                    index_type new_index = index_of(hash_code, new_bucket_mask);
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
                    if (likely(new_bucket_capacity >= bucket_capacity_))
                        new_index = index;
                    else
                        new_index = index_of(index, new_bucket_mask);

                    new_buckets[new_index] = first_entry;
                }
            }
        }
    }

    void rehash_buckets(size_type new_bucket_capacity) {
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_bucket_capacity));
        assert(entry_size_ <= bucket_capacity_);

        entry_type ** new_buckets = bucket_allocator_.allocate(new_bucket_capacity);
        if (likely(bucket_allocator_.is_ok(new_buckets))) {
            // Initialize the bucket list's data.
            ::memset((void *)new_buckets, 0, new_bucket_capacity * sizeof(entry_type *));

            if (likely(new_bucket_capacity == bucket_capacity_ * 2))
                rehash_all_entries_2x(new_buckets, new_bucket_capacity);
            else
                rehash_all_entries(new_buckets, new_bucket_capacity);

            free_buckets_impl();

            buckets_ = new_buckets;
            bucket_mask_ = new_bucket_capacity - 1;
            bucket_capacity_ = new_bucket_capacity;

            updateVersion();
        }
    }

    void rehash_buckets_and_entries(size_type new_entry_capacity, size_type new_bucket_capacity) {
        assert(new_entry_capacity > 0);
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_entry_capacity));
        assert(run_time::is_pow2(new_bucket_capacity));
        assert(new_bucket_capacity > bucket_capacity_);

        entry_type ** new_buckets = bucket_allocator_.allocate(new_bucket_capacity);
        if (likely(bucket_allocator_.is_ok(new_buckets))) {
            // Initialize the bucket list's data.
            ::memset((void *)new_buckets, 0, new_bucket_capacity * sizeof(entry_type *));

            // Only allocate a chunk size we need.
            assert(new_entry_capacity > entry_capacity_);
            size_type new_entry_size = new_entry_capacity - entry_capacity_;

            entry_type * new_entries = entry_allocator_.allocate(new_entry_size);
            if (likely(entry_allocator_.is_ok(new_entries))) {
                if (likely(new_bucket_capacity == bucket_capacity_ * 2))
                    rehash_all_entries_2x(new_buckets, new_bucket_capacity);
                else
                    rehash_all_entries(new_buckets, new_bucket_capacity);

                free_buckets_impl();

                buckets_ = new_buckets;
                entries_ = new_entries;
                bucket_mask_ = new_bucket_capacity - 1;
                bucket_capacity_ = new_bucket_capacity;
                // Here, the entry_size_ doesn't change.
                entry_capacity_ = new_entry_capacity;

                // Push the new entries pointer to entries list.
                entries_list_.emplace_back(new_entries, new_entry_size);

                assert(entry_chunk_.is_full());
                entry_chunk_.init(new_entries, 0, new_entry_size);

                //init_entries_chunk(new_entries, new_entry_size);

                updateVersion();
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

        size_type new_entry_capacity = run_time::round_up_to_pow2(entry_size_ + 1);
        if (likely(new_bucket_capacity > bucket_capacity_)) {
            // Is infalte, do nothing.
        }
        else if (likely(new_bucket_capacity < bucket_capacity_)) {
            if (likely(new_entry_capacity > new_bucket_capacity)) {
                new_bucket_capacity = new_entry_capacity;
            }
            if (likely(new_bucket_capacity > bucket_capacity_)) {
                // Is infalte, do nothing.
            }
            else {
                // We need to shrink the bucket list capacity.
            }
        }
        else {
            // The bucket capacity is unchanged.
            return;
        }

        if (likely(new_entry_capacity <= entry_capacity_)) {
            return rehash_buckets(new_bucket_capacity);
        }
        else {
            return rehash_buckets_and_entries(new_entry_capacity, new_bucket_capacity);
        }
    }

    void inflate_entries(size_type size = 1) {
        size_type new_entry_capacity = run_time::round_up_to_pow2(entry_size_ + size);

        // If new entry capacity is too small, exit directly.
        if (likely(new_entry_capacity > entry_capacity_)) {
            // Most of the time, we don't need to reallocate buckets list.
            if (likely(new_entry_capacity <= bucket_capacity_)) {
                assert(freelist_.is_empty());

                size_type new_entry_size = new_entry_capacity - entry_capacity_;
                entry_type * new_entries = entry_allocator_.allocate(new_entry_size);
                if (likely(entry_allocator_.is_ok(new_entries))) {
                    entries_ = new_entries;
                    entry_capacity_ = new_entry_capacity;

                    // Push the new entries pointer to entries list.
                    entries_list_.emplace_back(new_entries, new_entry_size);

                    assert(entry_chunk_.is_full());
                    entry_chunk_.init(new_entries, 0, new_entry_size);

                    //init_entries_chunk(new_entries, new_entry_size);
                }
            }
            else {
                // The bucket list capacity is full, need to reallocate.
                rehash_buckets_and_entries(new_entry_capacity, new_entry_capacity);
            }
        }
        else {
            // entry_capacity_ = bucket_capacity_ ?
            assert(new_entry_capacity <= bucket_capacity_);
        }
    }

#if USE_JAVA_FIND_ENTRY

    JSTD_FORCEINLINE
    entry_type * find_entry(const key_type & key) {
        hash_code_t hash_code = get_hash(key);
        index_type index = index_of(hash_code);

        entry_type * first = buckets_[index];
        if (likely(first != nullptr)) {
            if (likely(first->hash_code == hash_code &&
                       key_is_equal_(key, first->value.first))) {
                return first;
            }

            entry_type * entry = first->next;
            if (likely(entry != nullptr)) {
                do {
                    if (likely(entry->hash_code != hash_code)) {
                        // Do nothing, Continue
                    }
                    else {
                        if (likely(key_is_equal_(key, entry->value.first))) {
                            return entry;
                        }
                    }
                    entry = entry->next;
                } while (likely(entry != nullptr));
            }
        }

        return nullptr;  // Not found
    }

    JSTD_FORCEINLINE
    entry_type * find_entry(const key_type & key, hash_code_t hash_code, index_type index) {
        assert(buckets() != nullptr);
        entry_type * first = buckets_[index];
        if (likely(first != nullptr)) {
            if (likely(first->hash_code == hash_code &&
                       key_is_equal_(key, first->value.first))) {
                return first;
            }

            entry_type * entry = first->next;
            if (likely(entry != nullptr)) {
                do {
                    if (likely(entry->hash_code != hash_code)) {
                        // Do nothing, Continue
                    }
                    else {
                        if (likely(key_is_equal_(key, entry->value.first))) {
                            return entry;
                        }
                    }
                    entry = entry->next;
                } while (likely(entry != nullptr));
            }
        }

        return nullptr;  // Not found
    }

#else // !USE_JAVA_FIND_ENTRY

    JSTD_FORCEINLINE
    entry_type * find_entry(const key_type & key) {
        hash_code_t hash_code = get_hash(key);
        index_type index = index_of(hash_code);

        entry_type * entry = buckets_[index];
        while (likely(entry != nullptr)) {
            if (likely(entry->hash_code != hash_code)) {
                entry = entry->next;
            }
            else {
                if (likely(key_is_equal_(key, entry->value.first))) {
                    return entry;
                }
                entry = entry->next;
            }
        }

        return nullptr;  // Not found
    }

    JSTD_FORCEINLINE
    entry_type * find_entry(const key_type & key, hash_code_t hash_code, index_type index) {
        assert(buckets() != nullptr);
        entry_type * entry = buckets_[index];
        while (likely(entry != nullptr)) {
            if (likely(entry->hash_code != hash_code)) {
                entry = entry->next;
            }
            else {
                if (likely(key_is_equal_(key, entry->value.first))) {
                    return entry;
                }
                entry = entry->next;
            }
        }

        return nullptr;  // Not found
    }

#endif // USE_JAVA_FIND_ENTRY

    JSTD_FORCEINLINE
    entry_type * find_before(const key_type & key, entry_type *& before, size_type & index) {
        hash_code_t hash_code = get_hash(key);
        index = index_of(hash_code);

        assert(buckets() != nullptr);
        entry_type * prev = nullptr;
        entry_type * entry = buckets_[index];
        while (likely(entry != nullptr)) {
            if (likely(entry->hash_code != hash_code)) {
                prev = entry;
                entry = entry->next;
            }
            else {
                if (likely(key_is_equal_(key, entry->value.first))) {
                    before = prev;
                    return entry;
                }
                entry = entry->next;
            }
        }

        return nullptr;  // Not found
    }

    entry_type * insert_new_entry_to_bucket(hash_code_t hash_code, index_type & index) {
        entry_type * new_entry = get_free_entry(hash_code, index);
        assert(new_entry != nullptr);

        new_entry->next = buckets_[index];
        new_entry->hash_code = hash_code;
        new_entry->flags = 1;
        new_entry->owner = this;
        buckets_[index] = new_entry;

        return new_entry;
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, const key_type & key,
                                                 const mapped_type & value) {
        assert(new_entry != nullptr);

        // Use placement new method to construct value_type.
        void * value_ptr = (void *)&new_entry->value;
        value_type * new_value = allocator_.constructor(value_ptr, key, value);
        assert(new_value == &new_entry->value);
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, const key_type & key,
                                                 mapped_type && value) {
        assert(new_entry != nullptr);

        // Use placement new method to construct value_type.
        void * value_ptr = (void *)&new_entry->value;
        value_type * new_value = allocator_.constructor(value_ptr,
                                        key, std::forward<mapped_type>(value));
        assert(new_value == &new_entry->value);
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, key_type && key,
                                                mapped_type && value) {
        assert(new_entry != nullptr);

        // Use placement new method to construct value_type.
        void * value_ptr = (void *)&new_entry->value;
        value_type * new_value = allocator_.constructor(value_ptr,
                                        std::forward<key_type>(key),
                                        std::forward<mapped_type>(value));
        assert(new_value == &new_entry->value);
    }

    JSTD_FORCEINLINE
    void insert_new_entry(const key_type & key, const mapped_type & value,
                          hash_code_t hash_code, index_type index) {
        entry_type * new_entry = insert_new_entry_to_bucket(hash_code, index);
        construct_value(new_entry, key, value);
        entry_size_++;
    }

    JSTD_FORCEINLINE
    void insert_new_entry(const key_type & key, mapped_type && value,
                          hash_code_t hash_code, index_type index) {
        entry_type * new_entry = insert_new_entry_to_bucket(hash_code, index);
        construct_value(new_entry, key, std::forward<mapped_type>(value));
        entry_size_++;
    }

    JSTD_FORCEINLINE
    void insert_new_entry(key_type && key, mapped_type && value,
                          hash_code_t hash_code, index_type index) {
        entry_type * new_entry = insert_new_entry_to_bucket(hash_code, index);
        construct_value(new_entry, std::forward<key_type>(key),
                                         std::forward<mapped_type>(value));
        entry_size_++;
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void construct_value_args(entry_type * new_entry, Args && ... args) {
        assert(new_entry != nullptr);

        // Use placement new method to construct value_type.
        void * value_ptr = (void *)&new_entry->value;
        value_type * new_value = allocator_.constructor(value_ptr,
                                                        std::forward<Args>(args)...);
        assert(new_value == &new_entry->value);
    }

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(const key_type & key, const mapped_type & value) {
        assert(buckets() != nullptr);

        hash_code_t hash_code = get_hash(key);
        index_type index = index_of(hash_code);

        entry_type * entry = find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            insert_new_entry(key, value,
                                   hash_code, index);
        }
        else {
            if (!OnlyIfAbsent) {
                update_value(entry, value);
            }
        }

        updateVersion();

        return ReturnType(iterator(entry), (entry == nullptr));
    }

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(const key_type & key, mapped_type && value) {
        assert(buckets() != nullptr);

        hash_code_t hash_code = get_hash(key);
        index_type index = index_of(hash_code);

        entry_type * entry = find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            insert_new_entry(key, std::forward<mapped_type>(value),
                                   hash_code, index);
        }
        else {
            if (!OnlyIfAbsent) {
                update_value(entry, std::forward<mapped_type>(value));
            }
        }

        updateVersion();

        return ReturnType(iterator(entry), (entry == nullptr));
    }

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(key_type && key, mapped_type && value) {
        assert(buckets() != nullptr);

        hash_code_t hash_code = get_hash(key);
        index_type index = index_of(hash_code);

        entry_type * entry = find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            insert_new_entry(std::forward<key_type>(key),
                                   std::forward<mapped_type>(value),
                                   hash_code, index);
        }
        else {
            if (!OnlyIfAbsent) {
                update_value(entry, std::forward<mapped_type>(value));
            }
        }

        updateVersion();

        return ReturnType(iterator(entry), (entry == nullptr));
    }

    template <typename ...Args>
    JSTD_INLINE
    void emplace_new_entry(hash_code_t hash_code, index_type index, Args && ... args) {
        entry_type * new_entry = insert_new_entry_to_bucket(hash_code, index);
        construct_value_args(new_entry, std::forward<Args>(args)...);
        entry_size_++;
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, value_type * value) {
        assert(new_entry != nullptr);

        // Use placement new method to construct value_type [by move assignment].
        void * value_ptr = (void *)&new_entry->value;
        value_type * new_value = allocator_.constructor(value_ptr, std::move(*value));
        assert(new_value == &new_entry->value);
    }

    JSTD_INLINE
    void emplace_new_entry_from_value(hash_code_t hash_code, index_type index, value_type * value) {
        entry_type * new_entry = insert_new_entry_to_bucket(hash_code, index);
        construct_value(new_entry, value);
        entry_size_++;
    }

    // Update the existed key's value, maybe by move assignment operator.
    JSTD_FORCEINLINE
    void update_value(entry_type * entry, const mapped_type & value) {
        assert(entry != nullptr);
        entry->value.second = value;
    }

    // Update the existed key's value, maybe by move assignment operator.
    JSTD_FORCEINLINE
    void update_value(entry_type * entry, mapped_type && value) {
        assert(entry != nullptr);
        entry->value.second = std::forward<mapped_type>(value);
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_value_args_impl(entry_type * entry, const key_type & key, Args && ... args) {
        assert(entry != nullptr);
#ifdef NDEBUG
        static int display_count = 0;
        display_count++;
        if (display_count < 50) {
            if (has_c_str<typename key_type, char>::value)
                printf("update_value_args_impl(), key = %s\n", call_c_str<typename key_type, char>::c_str(key));
            else
                printf("update_value_args_impl(), key(non-string) = %u\n", *(uint32_t *)&key);
        }
#endif        
        value_allocator_.destroy(&entry->value.second);
        value_allocator_.construct(&entry->value.second, std::forward<Args>(args)...);
    }

    JSTD_FORCEINLINE
    void update_value_args(entry_type * entry, const mapped_type & value) {
        assert(entry != nullptr);
        entry->value.second = value;
    }

    JSTD_FORCEINLINE
    void update_value_args(entry_type * entry, mapped_type && value) {
        assert(entry != nullptr);
        entry->value.second = std::forward<mapped_type>(value);
    }

    // Update the existed key's value.
    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_value_args(entry_type * entry, Args && ... args) {
        assert(entry != nullptr);
        update_value_args_impl(entry, std::forward<Args>(args)...);
    }

    const key_type & get_key(value_type * value) const {
        return key_extractor<value_type>::extract(*const_cast<const value_type *>(value));
    }

    template <bool OnlyIfAbsent, typename ReturnType, typename ...Args>
    ReturnType emplace_unique(const key_type & key, Args && ... args) {
        assert(buckets() != nullptr);

        hash_code_t hash_code = get_hash(key);
        index_type index = index_of(hash_code);

        entry_type * entry = find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            emplace_new_entry(hash_code, index,
                              std::forward<Args>(args)...);
        }
        else {
            if (!OnlyIfAbsent) {
                update_value_args(entry, std::forward<Args>(args)...);
            }
        }

        updateVersion();

        return ReturnType(iterator(entry), (entry == nullptr));
    }

    template <bool OnlyIfAbsent, typename ReturnType, typename ...Args>
    ReturnType emplace_unique(no_key_t nokey, Args && ... args) {
        assert(buckets() != nullptr);

        value_type * value_tmp = allocator_.create(std::forward<Args>(args)...);
        assert(value_tmp != nullptr);
        const key_type & key = get_key(value_tmp);

        hash_code_t hash_code = get_hash(key);
        index_type index = index_of(hash_code);

        entry_type * entry = find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            emplace_new_entry_from_value(hash_code, index, value_tmp);
        }
        else {
            if (!OnlyIfAbsent) {
                update_value(entry, std::move(value_tmp->second));
            }
        }

        allocator_.destroy(value_tmp);
        updateVersion();

        return ReturnType(iterator(entry), (entry == nullptr));
    }

    void updateVersion() {
#if DICTIONARY_SUPPORT_VERSION
        ++(version_);
#endif
    }

public:
    inline hash_code_t get_hash(const key_type & key) const {
        hash_code_t hash_code = hasher_(key);
        //return hash_traits<hash_code_t>::filter(hash_code);
        //hash_code = hash_code ^ (hash_code >> 16);
        return hash_code;
    }

    void clear() {
        if (likely(buckets_ != nullptr)) {
            // Initialize the buckets's data.
            ::memset((void *)buckets_, 0, bucket_capacity_ * sizeof(entry_type *));
        }

        // Clear settings
        entry_size_ = 0;

        // TODO: clear or rearrange entry chunk and freelist.

        //freelist_.clear();
        //entry_chunk_.clear();
    }

    void rehash(size_type bucket_count) {
        assert(bucket_count > 0);
        size_type new_bucket_capacity = calc_capacity(bucket_count);
        rehash_internal<false>(new_bucket_capacity);
    }

    void reserve(size_type bucket_count) {
        rehash(bucket_count);
    }

    void shrink_to_fit(size_type bucket_count = 0) {
        size_type entry_capacity = run_time::round_up_to_pow2(entry_size_);

        // Choose the maximum size of new bucket capacity and now entry capacity.
        bucket_count = (entry_capacity >= bucket_count) ? entry_capacity : bucket_count;

        size_type new_bucket_capacity = calc_capacity(bucket_count);
        rehash_internal<true>(new_bucket_capacity);
    }

    bool contains(const key_type & key) {
        iterator iter = find(key);
        return (iter != end());
    }

    iterator find(const key_type & key) {
        if (likely(buckets() != nullptr)) {
            entry_type * entry = find_entry(key);
            return iterator(entry);
        }

        return iterator(nullptr);   // Error: buckets data is invalid
    }

    const_iterator find(const key_type & key) const {
        if (likely(buckets() != nullptr)) {
            entry_type * entry = find_entry(key);
            return const_iterator(entry);
        }

        return const_iterator(nullptr);   // Error: buckets data is invalid
    }

    // insert(key, value)

    insert_return_type insert(const key_type & key, const mapped_type & value) {
        return insert_unique<false, insert_return_type>(key, value);
    }

    insert_return_type insert(const key_type & key, mapped_type && value) {
        return insert_unique<false, insert_return_type>(key, std::forward<mapped_type>(value));
    }

    insert_return_type insert(key_type && key, mapped_type && value) {
        return insert_unique<false, insert_return_type>(std::forward<key_type>(key),
                                                              std::forward<mapped_type>(value));
    }

    insert_return_type insert(const value_type & pair) {
        return insert(pair.first, pair.second);
    }

    insert_return_type insert(value_type && pair) {
        bool is_rvalue = std::is_rvalue_reference<decltype(pair)>::value;
        if (is_rvalue)
            return insert(std::move(pair.first), std::move(pair.second));
        else
            return insert(pair.first, pair.second);
    }

    // insert_no_return(key, value)

    void insert_no_return(const key_type & key, const mapped_type & value) {
        insert_unique<false, void_warpper>(key, value);
    }

    void insert_no_return(const key_type & key, mapped_type && value) {
        insert_unique<false, void_warpper>(key, std::forward<mapped_type>(value));
    }

    void insert_no_return(key_type && key, mapped_type && value) {
        insert_unique<false, void_warpper>(std::forward<key_type>(key),
                                                 std::forward<mapped_type>(value));
    }

    void insert_no_return(const value_type & pair) {
        insert_no_return(pair.first, pair.second);
    }

    void insert_no_return(value_type && pair) {
        bool is_rvalue = std::is_rvalue_reference<decltype(pair)>::value;
        if (is_rvalue)
            insert_no_return(std::move(pair.first), std::move(pair.second));
        else
            insert_no_return(pair.first, pair.second);
    }

    /***************************************************************************/
    /*                                                                         */
    /* See: https://en.cppreference.com/w/cpp/container/unordered_map/emplace  */
    /*                                                                         */
    /*   Don't support that emplace() interface:                               */
    /*                                                                         */
    /*   // uses pair's piecewise constructor                                  */
    /*   m.emplace(std::piecewise_construct,                                   */
    /*             std::forward_as_tuple("c"),                                 */
    /*             std::forward_as_tuple(10, 'c'));                            */
    /*   // as of C++17, m.try_emplace("c", 10, 'c'); can be used              */
    /*                                                                         */
    /***************************************************************************/

    template <typename ...Args>
    insert_return_type emplace(Args && ... args) {
        return emplace_unique<false, insert_return_type>(
            key_extractor<value_type>::extract(std::forward<Args>(args)...),
            std::forward<Args>(args)...);
    }

    template <typename ...Args>
    void emplace_no_return(Args && ... args) {
        emplace_unique<false, void_warpper>(
            key_extractor<value_type>::extract(std::forward<Args>(args)...),
            std::forward<Args>(args)...);
    }

    size_type erase(const key_type & key) {
        if (likely(buckets_ != nullptr)) {
            hash_code_t hash_code = get_hash(key);
            size_type index = index_of(hash_code);

            entry_type * prev = nullptr;
            entry_type * entry = buckets_[index];
            while (likely(entry != nullptr)) {
                if (likely(entry->hash_code != hash_code)) {
                    prev = entry;
                    entry = entry->next;
                }
                else {
                    if (likely(key_is_equal_(key, entry->value.first))) {
                        if (likely(prev != nullptr))
                            prev->next = entry->next;
                        else
                            buckets_[index] = entry->next;

                        assert(entry->flags == 1);
                        entry->flags = 0;
                        freelist_.push_front(entry);

                        // destruct the entry->value
                        value_type * value_ptr = &entry->value;
                        allocator_.destructor(value_ptr);

                        entry_size_--;

                        updateVersion();

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

#if JSTD_HAVE_SSE42_CRC32C
template <typename Key, typename Value, std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary_crc32c = BasicDictionary<Key, Value, HashFunc_CRC32C, Alignment>;

template <typename Key, typename Value, std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary = BasicDictionary<Key, Value, HashFunc_CRC32C, Alignment>;
#else
template <typename Key, typename Value, std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary = BasicDictionary<Key, Value, HashFunc_Time31, Alignment>;
#endif // JSTD_HAVE_SSE42_CRC32C

} // namespace jstd

#endif // JSTD_HASH_DICTIONARY_H
