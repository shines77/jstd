
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
#include <cstddef>      // For std::ptrdiff_t, std::size_t
#include <memory>       // For std::swap(), std::pointer_traits<T>
#include <limits>       // For std::numeric_limits<T>
#include <cstring>      // For std::memset()
#include <vector>
#include <type_traits>
#include <utility>
#include <algorithm>    // For std::max(), std::min()

#include "jstd/allocator.h"
#include "jstd/iterator.h"
#include "jstd/hash/hash_helper.h"
#include "jstd/hash/equal_to.h"
#include "jstd/hash/dictionary_traits.h"
#include "jstd/hash/hash_chunk_list.h"
#include "jstd/hash/key_extractor.h"
#include "jstd/support/PowerOf2.h"

#define ENABLE_JSTD_DICTIONARY          1
#define DICTIONARY_SUPPORT_VERSION      0

#undef  USE_FAST_FIND_ENTRY
#define USE_FAST_FIND_ENTRY             1

namespace jstd {

template <typename Key, typename Value>
constexpr bool cValueIsInline()
{
    //return ((sizeof(Key) + sizeof(Value)) > (sizeof(void *) * 2));
    return true;
}

template < typename Key, typename Value,
           std::size_t HashFunc = HashFunc_Default,
           bool ValueIsInline = cValueIsInline<Key, Value>,
           std::size_t Alignment = align_of<std::pair<const Key, Value>>::value,
           typename Hasher = hash<Key, std::uint32_t, HashFunc>,
           typename KeyEqual = equal_to<Key>,
           typename Allocator = allocator<std::pair<const Key, Value>, Alignment, true> >
class BasicDictionary {
public:
    typedef Key                             key_type;
    typedef Value                           mapped_type;
    typedef std::pair<const Key, Value>     value_type;
    typedef std::pair<Key, Value>           n_value_type;

    typedef Hasher                          hasher;
    typedef Hasher                          hasher_type;
    typedef KeyEqual                        key_equal;
    typedef Allocator                       allocator_type;

    typedef std::size_t                     size_type;
    typedef typename std::make_signed<size_type>::type
                                            ssize_type;
    typedef std::size_t                     index_type;
    typedef typename Hasher::result_type    hash_code_t;
    typedef BasicDictionary<Key, Value, HashFunc, ValueIsInline, Alignment, Hasher, KeyEqual, Allocator>
                                            this_type;

    struct ArrangeType {
        enum {
            Reorder = 0,
            Realloc = 1
        };
    };

    enum entry_type_t {
        kEntryTypeShfit  = 30,
        kIsFreeEntry     = 0UL << kEntryTypeShfit,
        kIsInUseEntry    = 1UL << kEntryTypeShfit,
        kIsReusableEntry = 2UL << kEntryTypeShfit,
        kIsRedirectEntry = 3UL << kEntryTypeShfit,

        kEntryTypeMask   = 0xC0000000UL,
        kEntryIndexMask  = 0x3FFFFFFFUL
    };

    struct entry_attr_t {
        uint32_t value;

        entry_attr_t(uint32_t value = 0) : value(value) {}
        entry_attr_t(uint32_t _entry_type, uint32_t chunk_id)
            : value(makeAttr(_entry_type, chunk_id)) {}
        entry_attr_t(const entry_attr_t & src) : value(src.value) {}
        ~entry_attr_t() {}

        entry_attr_t & operator = (const entry_attr_t & rhs) {
            this->value = rhs.value;
        }

        entry_attr_t & operator = (uint32_t value) {
            this->value = value;
        }

        uint32_t makeAttr(uint32_t _entry_type, uint32_t chunk_id) {
            return ((_entry_type & kEntryTypeMask) | (chunk_id & kEntryIndexMask));
        }

        uint32_t getEntryType() const {
            return (this->value & kEntryTypeMask);
        }

        uint32_t getChunkId() const {
            return (this->value & kEntryIndexMask);
        }

        void setValue(uint32_t _entry_type, uint32_t chunk_id) {
            this->value = this->makeAttr(_entry_type, chunk_id);
        }

        void setEntryType(uint32_t _entry_type) {
            // Here is a small optimization.
            // this->value = (_entry_type & kEntryAttrMask) | this->chunk_id();
            this->value = _entry_type | this->getChunkId();
        }

        void setChunkId(uint32_t chunk_id) {
            this->value = this->getEntryType() | (chunk_id & kEntryIndexMask);
        }

        void setChunkIdsz(size_type chunk_id) {
            this->value = this->getEntryType() | (static_cast<uint32_t>(chunk_id) & kEntryIndexMask);
        }

        bool isFreeEntry() {
            return (this->getEntryType() == kIsFreeEntry);
        }

        bool isInUseEntry() {
            return (this->getEntryType() == kIsInUseEntry);
        }

        bool isReusableEntry() {
            return (this->getEntryType() == kIsReusableEntry);
        }

        bool isRedirectEntry() {
            return (this->getEntryType() == kIsRedirectEntry);
        }

        void setFreeEntry() {
            setEntryType(kIsFreeEntry);
        }

        void setInUseEntry() {
            setEntryType(kIsInUseEntry);
        }

        void setReusableEntry() {
            setEntryType(kIsReusableEntry);
        }

        void setRedirectEntry() {
            setEntryType(kIsRedirectEntry);
        }
    };

    // hash_entry
    struct hash_entry {
        typedef hash_entry *                    node_pointer;
        typedef hash_entry &                    node_reference;
        typedef const hash_entry *              const_node_pointer;
        typedef const hash_entry &              const_node_reference;
        typedef typename this_type::value_type  element_type;

#if 0
        typedef typename std::conditional<ValueIsInline, element_type *, element_type>::type
                                                value_type;
#else
        typedef element_type                    value_type;
#endif

        hash_entry * next;
        hash_code_t  hash_code;
        entry_attr_t attrib;
        alignas(Alignment)
        value_type   value;

        hash_entry() : next(nullptr), hash_code(0), attrib(0) {}

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

    // entry_list
    struct entry_list {
        entry_type * entries;
        size_type    capacity;

        entry_list() : entries(nullptr), capacity(0) {}
        entry_list(entry_type * entries, size_type capacity)
            : entries(entries), capacity(capacity) {}
        ~entry_list() {}
    };

    // free_list<T>
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
            this->increase();
        }

        node_type * pop_front() {
            assert(this->head_ != nullptr);
            node_type * node = this->head_;
            this->head_ = node->next;
            this->decrease();
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

        void erase(node_type * where, node_type * node) {
            if (likely(where != nullptr))
                where->next = node->next;
            else
                this->head_ = node->next;
            this->decrease();
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

    #define JSTD_HASH_DICTIONARY_HEADER_ONLY_H
    #include "jstd/hash/hash_iterator_inc.h"
    #undef  JSTD_HASH_DICTIONARY_HEADER_ONLY_H

    typedef iterator_t<this_type, entry_type>               iterator;
    typedef const_iterator_t<this_type, entry_type>         const_iterator;

    typedef local_iterator_t<this_type, entry_type>         local_iterator;
    typedef const_local_iterator_t<this_type, entry_type>   const_local_iterator;

    typedef std::pair<iterator, bool>   insert_return_type;

    typedef allocator<entry_type *>     bucket_allocator_type;
    typedef allocator<entry_type>       entry_allocator_type;

    typedef free_list<entry_type>       free_list_t;
    typedef hash_entry_chunk_list<entry_type, allocator_type, entry_allocator_type>
                                        entry_chunk_list_t;
    typedef typename entry_chunk_list_t::entry_chunk_t
                                        entry_chunk_t;

protected:
    entry_type **           buckets_;
    entry_type *            entries_;
    size_type               bucket_mask_;
    size_type               bucket_capacity_;
    size_type               entry_size_;
    size_type               entry_capacity_;
#if DICTIONARY_SUPPORT_VERSION
    size_type               version_;
#endif
    free_list_t             freelist_;

    entry_chunk_list_t      chunk_list_;

    hasher_type             hasher_;
    key_equal               key_is_equal_;
    allocator_type          allocator_;

    bucket_allocator_type   bucket_allocator_;
    entry_allocator_type    entry_allocator_;

    allocator<std::pair<Key, Value>, Alignment, allocator_type::kThrowEx>
                                                    n_allocator_;

    std::allocator<mapped_type>                     mapped_allocator_;
    typename std::allocator<value_type>::allocator  value_allocator_;

    // Default initial capacity is 8.
    static const size_type kDefaultInitialCapacity = 8;
    // Minimum capacity is 8.
    static const size_type kMinimumCapacity = 8;
    // Maximum capacity is 1 << (sizeof(std::size_t) - 1).
    static const size_type kMaximumCapacity = (std::numeric_limits<size_type>::max)() / 2 + 1;

    // The maximum entry's chunk bytes, default is 4 MB bytes.
    static const size_type kMaxEntryChunkBytes = 4 * 1024 * 1024;
    // The entry's block size per chunk (entry_type).
    static const size_type kMaxEntryChunkSize =
            compile_time::round_to_power2<kMaxEntryChunkBytes / sizeof(entry_type)>::value;

    // The threshold of treeify to red-black tree.
    static const size_type kTreeifyThreshold = 8;

    //
    // The maximum load factor: maxLoadFactor = A / B,
    // default value is: 3 / 4 = 0.75
    // Only use in rehash() function.
    //

    // The default maximum load factor coefficient of A.
    static const size_type kMaxLoadFactorA = 3;
    // The default maximum load factor coefficient of B.
    static const size_type kMaxLoadFactorB = 4;

    //
    // The default maximum load factor (For efficiency, use integer).
    // Use in all functions, except the rehash() function.
    //
    static const size_type kMaxLoadFactor = 1;

public:
    explicit BasicDictionary(size_type initialCapacity = kDefaultInitialCapacity)
        : buckets_(nullptr), entries_(nullptr),
          bucket_mask_(0), bucket_capacity_(0), entry_size_(0), entry_capacity_(0)
#if DICTIONARY_SUPPORT_VERSION
          , version_(1) /* Since 0 means that the version attribute is not supported,
                           the initial value of version starts from 1. */
#endif
    {
        this->init(initialCapacity);
    }

    BasicDictionary(const this_type & other)
        : buckets_(nullptr), entries_(nullptr),
          bucket_mask_(0), bucket_capacity_(0), entry_size_(0), entry_capacity_(0)
#if DICTIONARY_SUPPORT_VERSION
          , version_(1) /* Since 0 means that the version attribute is not supported,
                           the initial value of version starts from 1. */
#endif
    {
        size_type initialSize = other.size();
        this->init(initialSize);

        for (const_iterator iter = other.cbegin(); iter != other.cend(); ++iter) {
            this->insert_no_return(iter->first, iter->second);
        }
    }

    BasicDictionary(this_type && other)
        : buckets_(nullptr), entries_(nullptr),
          bucket_mask_(0), bucket_capacity_(0), entry_size_(0), entry_capacity_(0)
#if DICTIONARY_SUPPORT_VERSION
          , version_(1) /* Since 0 means that the version attribute is not supported,
                           the initial value of version starts from 1. */
#endif
    {
        this->swap(other);
    }

    virtual ~BasicDictionary() {
        this->destroy();
    }

    // iterator
    iterator begin() {
        return iterator(this, find_first_valid_entry());
    }
    iterator end() {
        return iterator(this, nullptr);
    }

    const_iterator begin() const {
        return const_iterator(this, find_first_valid_entry());
    }
    const_iterator end() const {
        return const_iterator(this, nullptr);
    }

    const_iterator cbegin() const {
        return const_iterator(iterator(this, find_first_valid_entry()));
    }
    const_iterator cend() const {
        return const_iterator(this, nullptr);
    }

    // local_iterator
    local_iterator l_begin() {
        return local_iterator(this, find_first_valid_entry());
    }
    local_iterator l_end() {
        return local_iterator(this, nullptr);
    }

    const_local_iterator l_begin() const {
        return const_local_iterator(this, find_first_valid_entry());
    }
    const_local_iterator l_end() const {
        return const_local_iterator(this, nullptr);
    }

    const_local_iterator l_cbegin() const {
        return const_local_iterator(local_iterator(this, find_first_valid_entry()));
    }
    const_local_iterator l_cend() const {
        return const_local_iterator(this, nullptr);
    }

    bool valid() const { return (this->buckets() != nullptr); }
    bool empty() const { return (this->size() == 0); }
    bool full() const  { return (this->size() == this->entry_count()); }

    entry_type ** buckets() const { return this->buckets_; }
    entry_type *  entries() const { return this->entries_; }

    size_type size() const {
        return this->entry_size_;
    }

    size_type _size() const {
        assert(this->entry_capacity_ >= this->freelist_.size());
        return (this->entry_capacity_ - this->freelist_.size());
    }

    size_type capacity() const { return this->entry_capacity_; }

    size_type bucket_mask() const { return this->bucket_mask_; }
    size_type bucket_count() const { return this->bucket_capacity_; }
    size_type bucket_capacity() const { return this->bucket_capacity_; }

    size_type entry_size() const { return this->size(); }
    size_type entry_count() const { return this->entry_capacity_; }
    size_type entry_capacity() const { return this->entry_capacity_; }

    float load_factor() const {
        return (static_cast<float>(this->size()) / this->bucket_count());
    }

    float max_load_factor() const {
        return static_cast<float>(kMaxLoadFactor);
    }

    void max_load_factor(float ml) {
        // Do nothing !!
    }

    size_type max_bucket_capacity() const {
        return ((std::numeric_limits<size_type>::max)() / 2 + 1);
    }

    size_type max_size() const {
        return this->max_bucket_capacity();
    }

    size_type min_bucket_count() const {
        return (this->entry_size() / kMaxLoadFactor);
    }

    size_type min_bucket_count(size_type entry_size) const {
        return (entry_size / kMaxLoadFactor);
    }

    size_type min_bucket_countf() const {
        return (this->entry_size() * kMaxLoadFactorB / kMaxLoadFactorA);
    }

    size_type min_bucket_countf(size_type entry_size) const {
        return (entry_size * kMaxLoadFactorB / kMaxLoadFactorA);
    }

    size_type max_chunk_size() const {
        return kMaxEntryChunkSize;
    }

    size_type max_chunk_bytes() const {
        return kMaxEntryChunkBytes;
    }

    size_type actual_chunk_bytes() const {
        return (kMaxEntryChunkSize * sizeof(entry_type));
    }

    size_type bucket_size(size_type index) const {
        assert(index < this->bucket_count());

        size_type count = 0;
        entry_type * node = this->get_bucket_head(index);
        while (likely(node != nullptr)) {
            count++;
            node = node->next;
        }
        return count;
    }

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

    inline index_type index_for(hash_code_t hash_code) const {
        return (index_type)((size_type)hash_code & this->bucket_mask());
    }

    inline index_type index_for(hash_code_t hash_code, size_type capacity_mask) const {
        return (index_type)((size_type)hash_code & capacity_mask);
    }

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    inline index_type index_for(size_type hash_code) const {
        return (index_type)(hash_code & this->bucket_mask());
    }

    inline index_type index_for(size_type hash_code, size_type capacity_mask) const {
        return (index_type)(hash_code & capacity_mask);
    }
#endif // __amd64__

    inline index_type next_index(index_type index, size_type capacity_mask) const {
        ++index;
        return (index_type)((size_type)index & capacity_mask);
    }

    void free_buckets_impl() {
        assert(this->buckets() != nullptr);
        bucket_allocator_.deallocate(this->buckets_, this->bucket_capacity_);
    }

    void free_buckets() {
        if (likely(this->buckets() != nullptr)) {
            free_buckets_impl();
        }
    }

    void destory_entries() {
        // Destroy all entries.
        if (likely(this->entries_ != nullptr)) {
            this->chunk_list_.destory();
            this->entries_ = nullptr;
        }
    }

    void destory_resources() {
        // Destroy the resources.
        if (likely(this->buckets_ != nullptr)) {
            // Destroy all entries.
            this->destory_entries();

            // Free the array of bucket.
            this->free_buckets_impl();
            this->buckets_ = nullptr;
        }
    }

    void destroy() {
        // Destroy the resources.
        this->destory_resources();

        this->freelist_.clear();

        // Clear settings
        this->bucket_mask_ = 0;
        this->bucket_capacity_ = 0;
        this->entry_size_ = 0;
        this->entry_capacity_ = 0;
    }

    void assert_buckets_capacity(entry_type ** new_buckets, size_type new_bucket_capacity) {
        assert(this->buckets() != nullptr);
        assert(new_buckets != nullptr);
        assert_bucket_capacity(new_bucket_capacity);
    }

    void assert_entries_capacity(entry_type * new_entries, size_type new_entry_capacity) {
        assert(this->entries() != nullptr);
        assert(new_entries != nullptr);
        assert_entry_capacity(new_entry_capacity);
    }

    void assert_bucket_capacity(size_type bucket_capacity) {
        assert(run_time::is_pow2(bucket_capacity));
        assert(bucket_capacity >= this->min_bucket_count(kMinimumCapacity));
        assert(bucket_capacity >= this->min_bucket_count());
    }

    void assert_entry_capacity(size_type entry_capacity) {
        assert(run_time::is_pow2(entry_capacity));
        assert(entry_capacity >= kMinimumCapacity);
        assert(entry_capacity >= this->entry_size());
    }

    JSTD_FORCEINLINE
    entry_type * get_bucket_head(index_type index) const {
        return this->buckets_[index];
    }

    JSTD_FORCEINLINE
    void bucket_push_front(index_type index,
                           entry_type * new_entry) {
        new_entry->next = this->buckets_[index];
        this->buckets_[index] = new_entry;
    }

    JSTD_FORCEINLINE
    void bucket_push_front(entry_type ** new_buckets,
                           index_type index,
                           entry_type * new_entry) {
        new_entry->next = new_buckets[index];
        new_buckets[index] = new_entry;
    }

    JSTD_FORCEINLINE
    void bucket_push_back(index_type index,
                          entry_type * new_entry) {
        entry_type * first = this->buckets_[index];
        if (likely(first != nullptr)) {
            entry_type * prev = first;
            entry_type * enrty = first->next;
            while (enrty != nullptr) {
                prev = enrty;
                enrty = enrty->next;
            }
            prev->next = new_entry;
        }
        else {
            this->buckets_[index] = new_entry;
        }
    }

    JSTD_FORCEINLINE
    void bucket_push_back(entry_type ** new_buckets,
                          index_type index,
                          entry_type * new_entry) {
        entry_type * first = new_buckets[index];
        if (likely(first != nullptr)) {
            entry_type * prev = first;
            entry_type * enrty = first->next;
            while (enrty != nullptr) {
                prev = enrty;
                enrty = enrty->next;
            }
            prev->next = new_entry;
        }
        else {
            new_buckets[index] = new_entry;
        }
    }

    entry_type * find_first_valid_entry(index_type index = 0) const {
        size_type bucket_capacity = this->bucket_count();
        entry_type * first;
        do {
            first = get_bucket_head(index);
            if (likely(first == nullptr)) {
                index++;
                if (likely(index >= bucket_capacity))
                    return nullptr;
            }
            else break;
        } while (1);

        return first;
    }

    entry_type * next_link_entry(entry_type * node) const {
        if (likely(node->next != nullptr)) {
            return node->next;
        }
        else {
            size_type bucket_capacity = this->bucket_count();
            index_type index = this->index_for(node->hash_code);
            index++;
            if (likely(index < bucket_capacity)) {
                entry_type * first;
                do {
                    first = get_bucket_head(index);
                    if (likely(first == nullptr)) {
                        index++;
                        if (likely(index >= bucket_capacity))
                            return nullptr;
                    }
                    else break;
                } while (1);

                return first;
            }

            return nullptr;
        }
    }

    const entry_type * next_const_link_entry(const entry_type * node_ptr) {
        entry_type * node = const_cast<entry_type *>(node_ptr);
        node = this->next_link_entry(node);
        return const_cast<const entry_type *>(node);
    }

    // Init the new entries's status.
    void init_entries_chunk(entry_type * entries, size_type capacity, uint32_t chunk_id) {
        entry_type * entry = entries;
        for (size_type i = 0; i < capacity; i++) {
            entry->attrib.setValue(kIsFreeEntry, chunk_id);
            entry++;
        }
    }

    // Append all the entries to the free list.
    void add_entries_to_freelist(entry_type * entries, size_type capacity, uint32_t chunk_id) {
        assert(entries != nullptr);
        assert(capacity > 0);
        entry_type * entry = entries + capacity - 1;
        entry_type * prev = entries + capacity;
        for (size_type i = 0; i < capacity; i++) {
            entry->next = prev;
            entry->attrib.setValue(kIsFreeEntry, chunk_id);
            entry--;
            prev--;
        }

        entry_type * last_entry = entries + capacity - 1;
        last_entry->next = this->freelist_.head();
        this->freelist_.set_head(entries);
        this->freelist_.inflate(capacity);
    }

    size_type count_entries_size() {
        size_type entry_size = 0;
        for (size_type index = 0; index < this->bucket_capacity_; index++) {
            size_type list_size = 0;
            entry_type * entry = this->buckets_[index];
            while (likely(entry != nullptr)) {
                hash_code_t hash_code = entry->hash_code;
                index_type now_index = this->index_for(hash_code);
                assert(now_index == index);
                assert(entry->attrib.isInUseEntry());

                list_size++;
                entry = entry->next;
            }
            entry_size += list_size;
        }
        return entry_size;
    }

    void init(size_type init_capacity) {
        size_type entry_capacity = run_time::round_up_to_pow2(init_capacity);
        assert_entry_capacity(entry_capacity);

        size_type bucket_capacity = entry_capacity / kMaxLoadFactor;

        // The the array of bucket's first entry.
        entry_type ** new_buckets = bucket_allocator_.allocate(bucket_capacity);
        if (likely(bucket_allocator_.is_ok(new_buckets))) {
            // Initialize the buckets list.
            std::memset((void *)new_buckets, 0, bucket_capacity * sizeof(entry_type *));

            this->buckets_ = new_buckets;
            this->bucket_mask_ = bucket_capacity - 1;
            this->bucket_capacity_ = bucket_capacity;

            // The array of entries.
            entry_type * new_entries = entry_allocator_.allocate(entry_capacity);
            if (likely(entry_allocator_.is_ok(new_entries))) {
                this->entries_ = new_entries;
                this->entry_size_ = 0;
                this->entry_capacity_ = entry_capacity;

                this->chunk_list_.reserve(4);
                this->chunk_list_.addChunk(new_entries, entry_capacity);

                this->freelist_.clear();
            }
        }
    }

    void rehash_all_entries_sparse(entry_type ** new_buckets, size_type new_bucket_capacity) {
        assert_buckets_capacity(new_buckets, new_bucket_capacity);
        assert(new_bucket_capacity != this->bucket_capacity());

        size_type new_bucket_mask = new_bucket_capacity - 1;
        size_type old_bucket_capacity = this->bucket_capacity_;

        std::memset((void *)new_buckets, 0, new_bucket_capacity * sizeof(entry_type *));

        size_type entry_count = 0;
        index_type index = 0;

        do {
            // Find first valid entry
            entry_type * first;
            do {
                first = this->buckets_[index];
                if (likely(first == nullptr)) {
                    index++;
                    if (likely(index >= old_bucket_capacity))
                        return;
                }
                else break;
            } while (1);

            index_type new_index = this->index_for(first->hash_code, new_bucket_mask);
            entry_type * entry = first->next;
            bucket_push_front(new_buckets, new_index, first);
            entry_count++;

            while (entry != nullptr) {
                new_index = this->index_for(entry->hash_code, new_bucket_mask);
                entry_type * next_entry = entry->next;
                bucket_push_front(new_buckets, new_index, entry);
                entry_count++;
                entry = next_entry;
            }

            // If all entries have been transfer, end early.
            if (unlikely(entry_count >= this->entry_size_))
                break;

            // Find next bucket
            index++;
        } while (index < old_bucket_capacity);

        assert(entry_count == this->entry_size_);
    }

    void rehash_all_entries(entry_type ** new_buckets, size_type new_bucket_capacity) {
        assert_buckets_capacity(new_buckets, new_bucket_capacity);
        assert(new_bucket_capacity != this->bucket_capacity());

        size_type new_bucket_mask = new_bucket_capacity - 1;
        size_type old_bucket_capacity = this->bucket_capacity_;

        if (likely(new_bucket_capacity >= old_bucket_capacity)) {
            if (likely(new_bucket_capacity > old_bucket_capacity)) {
                std::memset((void *)&new_buckets[old_bucket_capacity], 0,
                            (new_bucket_capacity - old_bucket_capacity) * sizeof(entry_type *));
            }

            for (size_type index = 0; index < old_bucket_capacity; index++) {
                entry_type * entry = this->buckets_[index];
                if (likely(entry == nullptr)) {
                    new_buckets[index] = nullptr;
                }
                else {
                    entry_type * prev = nullptr;
                    entry_type * old_list = nullptr;
                    do {
                        hash_code_t hash_code = entry->hash_code;
                        index_type new_index = this->index_for(hash_code, new_bucket_mask);
                        if (likely(new_index == index)) {
                            if (likely(old_list == nullptr)) {
                                old_list = entry;
                            }
                            prev = entry;
                            entry = entry->next;
                        }
                        else {
                            if (prev != nullptr) {
                                prev->next = entry->next;
                            }
                            entry_type * next_entry = entry->next;

                            bucket_push_front(new_buckets, new_index, entry);

                            entry = next_entry;
                        }
                    } while (likely(entry != nullptr));

                    new_buckets[index] = old_list;
                }
            }
        }
        else {
            assert(new_bucket_capacity < old_bucket_capacity);

            // Range: [0, new_bucket_capacity)
            size_type index;
            for (index = 0; index < new_bucket_capacity; index++) {
                entry_type * entry = this->buckets_[index];
                if (likely(entry == nullptr)) {
                    new_buckets[index] = nullptr;
                }
                else {
                    entry_type * prev = nullptr;
                    entry_type * old_list = nullptr;
                    do {
                        hash_code_t hash_code = entry->hash_code;
                        index_type new_index = this->index_for(hash_code, new_bucket_mask);
                        if (likely(new_index == index)) {
                            if (unlikely(old_list == nullptr)) {
                                old_list = entry;
                            }
                            prev = entry;
                            entry = entry->next;
                        }
                        else {
                            if (prev != nullptr) {
                                prev->next = entry->next;
                            }

                            entry_type * next_entry = entry->next;
                            bucket_push_front(new_buckets, new_index, entry);
                            entry = next_entry;
                        }
                    } while (likely(entry != nullptr));

                    new_buckets[index] = old_list;
                }
            }

            // Range: [new_bucket_capacity, old_bucket_capacity)
            for (; index < old_bucket_capacity; index++) {
                entry_type * entry = this->buckets_[index];
                if (likely(entry != nullptr)) {
                    do {
                        hash_code_t hash_code = entry->hash_code;
                        index_type new_index = this->index_for(hash_code, new_bucket_mask);

                        entry_type * next_entry = entry->next;
                        bucket_push_front(new_buckets, new_index, entry);
                        entry = next_entry;
                    } while (likely(entry != nullptr));
                }
            }
        }
    }

    void rehash_all_entries_2x(entry_type ** new_buckets, size_type new_bucket_capacity) {
        assert_buckets_capacity(new_buckets, new_bucket_capacity);
        assert(new_bucket_capacity != this->bucket_capacity());

        size_type new_bucket_mask = new_bucket_capacity - 1;
        size_type old_bucket_capacity = this->bucket_capacity_;

        for (size_type index = 0; index < old_bucket_capacity; index++) {
            entry_type * entry = this->buckets_[index];
            if (likely(entry == nullptr)) {
                index_type new_index = index + old_bucket_capacity;
                new_buckets[index] = nullptr;
                new_buckets[new_index] = nullptr;
            }
            else {
                entry_type * prev = nullptr;
                entry_type * old_list = nullptr;
                entry_type * new_list = nullptr;
                do {
                    hash_code_t hash_code = entry->hash_code;
                    index_type new_index = this->index_for(hash_code, new_bucket_mask);
                    if (likely(new_index == index)) {
                        if (likely(old_list == nullptr)) {
                            old_list = entry;
                        }
                        prev = entry;
                        entry = entry->next;
                    }
                    else {
                        // Remove the entry from old list.
                        if (likely(prev != nullptr)) {
                            prev->next = entry->next;
                        }
                        entry_type * next_entry = entry->next;

                        // Push front the entry to the new list.
                        entry->next = new_list;
                        new_list = entry;

                        entry = next_entry;
                    }
                } while (likely(entry != nullptr));

                index_type new_index = index + old_bucket_capacity;
                new_buckets[index] = old_list;
                new_buckets[new_index] = new_list;
            }
        }
    }

    JSTD_FORCEINLINE
    void rehash_buckets(size_type new_bucket_capacity) {
        assert_bucket_capacity(new_bucket_capacity);
        assert(new_bucket_capacity != this->bucket_capacity_);

        entry_type ** new_buckets = bucket_allocator_.allocate(new_bucket_capacity);
        if (likely(this->bucket_allocator_.is_ok(new_buckets))) {
            // Here, we do not need to initialize the bucket list.
            if (likely(this->entry_size_ != 0)) {
                if (likely(new_bucket_capacity >= this->entry_size_ * 2 ||
                           this->entry_size_ >= kMaxEntryChunkSize)) {
                    rehash_all_entries_sparse(new_buckets, new_bucket_capacity);
                }
                else {
                    if (likely(new_bucket_capacity == this->bucket_capacity_ * 2))
                        rehash_all_entries_2x(new_buckets, new_bucket_capacity);
                    else
                        rehash_all_entries(new_buckets, new_bucket_capacity);
                }
            }
            else {
                std::memset((void *)new_buckets, 0, new_bucket_capacity * sizeof(entry_type *));
            }

            this->free_buckets_impl();

            this->buckets_ = new_buckets;
            this->bucket_mask_ = new_bucket_capacity - 1;
            this->bucket_capacity_ = new_bucket_capacity;
        }
    }

    JSTD_FORCEINLINE
    void add_new_entry_chunk(size_type new_entry_capacity) {
        assert_entry_capacity(new_entry_capacity);

        // We only need to allocate a chunk size,
        // if the allocate size is bigger than kMaxEntryChunkSize.
        assert(new_entry_capacity > this->entry_capacity_);
        size_type new_chunk_capacity = new_entry_capacity - this->entry_capacity_;
        new_chunk_capacity = (std::min)(new_chunk_capacity, kMaxEntryChunkSize);

        // If actual entry capacity is equal or smaller than old entry capacity,
        // don't need to inflate.
        size_type actual_entry_capacity = this->entry_size_ + new_chunk_capacity;
        if (likely(actual_entry_capacity > this->entry_capacity_)) {
            entry_type * new_entries = entry_allocator_.allocate(new_chunk_capacity);
            if (likely(entry_allocator_.is_ok(new_entries))) {
                // Needn't change the entry_size_.
                this->entries_ = new_entries;
                this->entry_capacity_ += new_chunk_capacity;

                assert(this->freelist_.is_empty());
                assert(this->chunk_list_.lastChunk().is_full());

                // Push the new entries pointer to entries list.
                this->chunk_list_.addChunk(new_entries, new_chunk_capacity);
            }
        }
        else {
            assert(false);
        }
    }

    void inflate_entries(size_type delta_size = 1) {
        assert(this->entry_size_ == this->entry_capacity_);
        size_type new_entry_capacity = run_time::round_up_to_pow2(this->entry_size_ + delta_size);

        add_new_entry_chunk(new_entry_capacity);

        // Most of the time, we don't need to reallocate the buckets list.
        size_type new_bucket_capacity = new_entry_capacity / kMaxLoadFactor;
        if (unlikely(new_bucket_capacity > this->bucket_capacity_)) {
            rehash_buckets(new_bucket_capacity);
        }
    }

    template <bool need_shrink = false>
    JSTD_FORCEINLINE
    void rehash_impl(size_type new_bucket_capacity) {
        // [ bucket_capacity = entry_size / kMaxLoadFactor ]
        size_type min_bucket_capacity = run_time::round_up_to_pow2(this->min_bucket_count());

        new_bucket_capacity = (std::max)(new_bucket_capacity, min_bucket_capacity);
        assert_bucket_capacity(new_bucket_capacity);
        if (likely(new_bucket_capacity != this->bucket_capacity_)) {
            // It may be the following two cases:
            // Inflate: We need to expanded the capacity of bucket list.
            // Shrink: We need to reduced the capacity of bucket list.
            this->rehash_buckets(new_bucket_capacity);

            this->update_version();
        }
    }

    JSTD_FORCEINLINE
    void resize_impl(size_type new_entry_size) {
        this->realloc_to(new_entry_size);
    }

#if USE_FAST_FIND_ENTRY

    JSTD_FORCEINLINE
    entry_type * find_entry(const key_type & key) {
        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_for(hash_code);

        entry_type * first = this->buckets_[index];
        if (likely(first != nullptr)) {
            if (likely(first->hash_code == hash_code &&
                       this->key_is_equal_(key, first->value.first))) {
                return first;
            }

            entry_type * entry = first->next;
            while (likely(entry != nullptr)) {
                if (likely(entry->hash_code != hash_code)) {
                    // Do nothing, Continue
                }
                else {
                    if (likely(this->key_is_equal_(key, entry->value.first))) {
                        return entry;
                    }
                }
                entry = entry->next;
            }
        }

        return nullptr;  // Not found
    }

    JSTD_FORCEINLINE
    entry_type * find_entry(const key_type & key, hash_code_t hash_code, index_type index) {
        assert(this->buckets() != nullptr);
        entry_type * first = this->buckets_[index];
        if (likely(first != nullptr)) {
            if (likely(first->hash_code == hash_code &&
                       this->key_is_equal_(key, first->value.first))) {
                return first;
            }

            entry_type * entry = first->next;
            while (likely(entry != nullptr)) {
                if (likely(entry->hash_code != hash_code)) {
                    // Do nothing, Continue
                }
                else {
                    if (likely(this->key_is_equal_(key, entry->value.first))) {
                        return entry;
                    }
                }
                entry = entry->next;
            }
        }

        return nullptr;  // Not found
    }

#else // !USE_FAST_FIND_ENTRY

    JSTD_FORCEINLINE
    entry_type * find_entry(const key_type & key) {
        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_for(hash_code);

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
    entry_type * find_entry(const key_type & key, hash_code_t hash_code, index_type index) {
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

#endif // USE_FAST_FIND_ENTRY

    JSTD_FORCEINLINE
    entry_type * find_before(const key_type & key, entry_type *& before, size_type & index) {
        hash_code_t hash_code = this->get_hash(key);
        index = this->index_for(hash_code);

        assert(this->buckets() != nullptr);
        entry_type * prev = nullptr;
        entry_type * entry = buckets_[index];
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

    JSTD_FORCEINLINE
    const key_type & get_key(value_type * value) const {
        return key_extractor<value_type>::extract(*const_cast<const value_type *>(value));
    }

    JSTD_FORCEINLINE
    const key_type & get_key(const value_type & value) const {
        return key_extractor<value_type>::extract(value);
    }

    JSTD_FORCEINLINE
    const key_type & get_key(n_value_type * value) const {
        return key_extractor<n_value_type>::extract(*const_cast<const n_value_type *>(value));
    }

    JSTD_FORCEINLINE
    const key_type & get_key(const n_value_type & value) const {
        return key_extractor<n_value_type>::extract(value);
    }

    JSTD_FORCEINLINE
    void move_or_swap_value(n_value_type * old_value, n_value_type && new_value) {
        bool has_move_assignment = std::is_nothrow_move_assignable<n_value_type>::value;
        // Is noexcept move assignment operator ?
        if (has_move_assignment) {
            *old_value = std::move(new_value);
        }
        else {
#if 1
            std::swap(*old_value, new_value);
#else
            std::swap(old_value->first, new_value.first);
            std::swap(old_value->second, new_value.second);
#endif
        }
    }

    JSTD_FORCEINLINE
    void move_or_swap_key(key_type * old_key, key_type && new_key) {
        bool has_move_assignment = std::is_nothrow_move_assignable<key_type>::value;
        // Is noexcept move assignment operator ?
        if (has_move_assignment) {   
            *old_key = std::move(new_key);
        }
        else {
            std::swap(*old_key, new_key);
        }
    }

    JSTD_FORCEINLINE
    void move_or_swap_mapped_value(mapped_type * old_value, mapped_type && new_value) {
        bool has_move_assignment = std::is_nothrow_move_assignable<mapped_type>::value;
        // Is noexcept move assignment operator ?
        if (has_move_assignment) {
            *old_value = std::move(new_value);
        }
        else {
            std::swap(*old_value, new_value);
        }
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, const key_type & key,
                                                 const mapped_type & value) {
        if (new_entry->attrib.isFreeEntry()) {
            new_entry->attrib.setInUseEntry();
            // Use placement new method to construct value_type.
            this->allocator_.constructor(&new_entry->value, key, value);
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            new_entry->attrib.setInUseEntry();
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            n_value->first = key;
            n_value->second = value;
        }
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, const key_type & key,
                                                 mapped_type && value) {
        if (new_entry->attrib.isFreeEntry()) {
            new_entry->attrib.setInUseEntry();
            // Use placement new method to construct value_type.
            this->allocator_.constructor(&new_entry->value, key,
                                         std::forward<mapped_type>(value));
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            new_entry->attrib.setInUseEntry();
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            n_value->first = key;
            move_or_swap_mapped_value(&n_value->second, std::forward<mapped_type>(value));
        }
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, key_type && key,
                                                 mapped_type && value) {
        if (new_entry->attrib.isFreeEntry()) {
            new_entry->attrib.setInUseEntry();
            // Use placement new method to construct value_type.
            this->n_allocator_.constructor(reinterpret_cast<n_value_type *>(&new_entry->value),
                                           std::forward<key_type>(key),
                                           std::forward<mapped_type>(value));
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            new_entry->attrib.setInUseEntry();
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            move_or_swap_key(&n_value->first, std::forward<mapped_type>(key));
            move_or_swap_mapped_value(&n_value->second, std::forward<mapped_type>(value));
        }
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, value_type && value) {
        if (new_entry->attrib.isFreeEntry()) {
            new_entry->attrib.setInUseEntry();
            // Use placement new method to construct value_type [by move assignment].
            this->n_allocator_.constructor(&(new_entry->value),
                                           std::forward<value_type>(value));
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            new_entry->attrib.setInUseEntry();
            n_value_type * old_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            n_value_type * new_value = reinterpret_cast<n_value_type *>(&value);
            move_or_swap_value(old_value, std::forward<n_value_type>(*new_value));
        }
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, n_value_type && value) {
        if (new_entry->attrib.isFreeEntry()) {
            new_entry->attrib.setInUseEntry();
            // Use placement new method to construct value_type [by move assignment].
            this->n_allocator_.constructor((n_value_type *)&(new_entry->value),
                                           std::forward<n_value_type>(value));
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            new_entry->attrib.setInUseEntry();
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            move_or_swap_value(n_value, std::forward<n_value_type>(value));
        }
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void construct_value_args(entry_type * new_entry, Args && ... args) {
        if (new_entry->attrib.isFreeEntry()) {
            new_entry->attrib.setInUseEntry();
            // Use placement new method to construct value_type.
            this->n_allocator_.constructor(reinterpret_cast<n_value_type *>(&new_entry->value),
                                           std::forward<Args>(args)...);
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            new_entry->attrib.setInUseEntry();
            n_value_type value_tmp(std::forward<Args>(args)...);

            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            move_or_swap_value(n_value, std::forward<n_value_type>(value_tmp));
        }
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_value_args_impl(entry_type * old_entry, const key_type & key, Args && ... args) {
        mapped_type second(std::forward<Args>(args)...);
        move_or_swap_mapped_value(&old_entry->value.second, std::forward<mapped_type>(second));
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_value_args_impl(entry_type * old_entry, key_type && key, Args && ... args) {
        //key_type key_tmp = std::move(key);
        mapped_type second(std::forward<Args>(args)...);
        move_or_swap_mapped_value(&old_entry->value.second, std::forward<mapped_type>(second));
    }

#if 1
    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_value_args(entry_type * old_entry, Args && ... args) {
        this->update_value_args_impl(old_entry, std::forward<Args>(args)...);
    }
#else
    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_value_args(entry_type * old_entry, Args && ... args) {
        n_value_type value_tmp(std::forward<Args>(args)...);
        move_or_swap_mapped_value(&old_entry->value.second, std::forward<mapped_type>(value_tmp.second));
    }
#endif

    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, const value_type & value) {
        entry->value.second = value.second;
    }

    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, value_type && value) {
        move_or_swap_mapped_value(&entry->value.second, std::forward<mapped_type>(value.second))
    }

    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, const n_value_type & value) {
        entry->value.second = value.second;
    }

    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, n_value_type && value) {
        //key_type key_tmp = std::move(value.first);
        move_or_swap_mapped_value(&entry->value.second, std::forward<mapped_type>(value.second));
    }

    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, const mapped_type & value) {
        entry->value.second = value;
    }

    // Update the existed key's value, maybe by move assignment operator.
    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, mapped_type && value) {
        move_or_swap_mapped_value(&entry->value.second, std::forward<mapped_type>(value));
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_mapped_value_args_impl(entry_type * entry, const key_type & key, Args && ... args) {
#ifndef NDEBUG
        static int display_count = 0;
        display_count++;
        if (display_count < 30) {
            if (has_c_str<key_type, char>::value)
                printf("update_mapped_value_args_impl(), key = %s\n", call_c_str<key_type, char>::c_str(key));
            else
                printf("update_mapped_value_args_impl(), key(non-string) = %u\n", *(uint32_t *)&key);
        }
#endif
#if 1
        mapped_type second(std::forward<Args>(args)...);
        move_or_swap_mapped_value(&entry->value.second, std::forward<mapped_type>(second));
#else
        value_allocator_.destructor(&entry->value.second);
        value_allocator_.construct(&entry->value.second, std::forward<Args>(args)...);
#endif
    }

    JSTD_FORCEINLINE
    void update_mapped_value_args(entry_type * entry, const mapped_type & value) {
        entry->value.second = value;
    }

    JSTD_FORCEINLINE
    void update_mapped_value_args(entry_type * entry, mapped_type && value) {
        move_or_swap_mapped_value(&entry->value.second, std::forward<mapped_type>(value))
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_mapped_value_args(entry_type * entry, Args && ... args) {
        update_mapped_value_args_impl(entry, std::forward<Args>(args)...);
    }

    JSTD_FORCEINLINE
    entry_type * got_a_prepare_entry() {
        if (likely(this->freelist_.is_empty())) {
            if (unlikely(this->chunk_list_.lastChunk().is_full())) {
#ifndef NDEBUG
                size_type old_size = size();
#endif
                // Inflate the entry size for 1.
                this->inflate_entries(1);
#ifndef NDEBUG
                size_type new_size = count_entries_size();
                assert(new_size == old_size);
#endif
            }

            // Get a unused entry.
            entry_type * new_entry = &this->entries_[this->chunk_list_.lastChunkSize()];
            assert(new_entry != nullptr);
            uint32_t chunk_id = this->chunk_list_.lastChunkId();
            new_entry->attrib.setValue(kIsFreeEntry, chunk_id);

            this->chunk_list_.lastChunk().increase();
            this->chunk_list_.appendEntry(chunk_id);
            return new_entry;
        }
        else {
            // Pop a free entry from freelist.
            entry_type * free_entry = this->freelist_.pop_front();
            this->chunk_list_.appendFreeEntry(free_entry);
            return free_entry;
        }
    }

    JSTD_FORCEINLINE
    entry_type * got_a_free_entry(hash_code_t hash_code, index_type & index) {
        if (likely(this->freelist_.is_empty())) {
            if (unlikely(this->chunk_list_.lastChunk().is_full())) {
#ifndef NDEBUG
                size_type old_size = size();
#endif
                // Inflate the entry size for 1.
                this->inflate_entries(1);
#ifndef NDEBUG
                size_type new_size = count_entries_size();
                assert(new_size == old_size);
#endif
                // Recalculate the bucket index.
                index = this->index_for(hash_code);
            }

            // Get a unused entry.
            entry_type * new_entry = &this->entries_[this->chunk_list_.lastChunkSize()];
            assert(new_entry != nullptr);
            uint32_t chunk_id = this->chunk_list_.lastChunkId();
            new_entry->attrib.setValue(kIsFreeEntry, chunk_id);

            this->chunk_list_.lastChunk().increase();
            this->chunk_list_.appendEntry(chunk_id);
            return new_entry;
        }
        else {
            // Pop a free entry from freelist.
            entry_type * free_entry = this->freelist_.pop_front();
            this->chunk_list_.appendFreeEntry(free_entry);
            return free_entry;
        }
    }

    JSTD_FORCEINLINE
    void destroy_prepare_entry(entry_type * entry) {
        assert(entry->attrib.isFreeEntry() || entry->attrib.isReusableEntry());
        uint32_t chunk_id = entry->attrib.getChunkId();
        if (!entry->attrib.isReusableEntry()) {
            entry->attrib.setReusableEntry();
        }

        this->chunk_list_.removeEntry(chunk_id);
        this->freelist_.push_front(entry);

        //
        // We use lazy destroy the entry->value,
        // so the code below is commented out.
        //
        // Call the destructor for entry->value.
        //
        // this->allocator_.destructor(&entry->value);
        //
    }

    JSTD_FORCEINLINE
    void destroy_entry(entry_type * entry) {
        assert(entry->attrib.isInUseEntry());
        uint32_t chunk_id = entry->attrib.getChunkId();
        entry->attrib.setReusableEntry();

        this->chunk_list_.removeEntry(chunk_id);
        this->freelist_.push_front(entry);

        //
        // We use lazy destroy the entry->value,
        // so the code below is commented out.
        //
        // Call the destructor for entry->value.
        //
        // this->allocator_.destructor(&entry->value);
        //
    }

    JSTD_FORCEINLINE
    void insert_to_bucket(entry_type * new_entry, hash_code_t hash_code,
                          index_type index) {
        assert(new_entry != nullptr);

        new_entry->next = this->buckets_[index];
        new_entry->hash_code = hash_code;
        this->buckets_[index] = new_entry;
    }

    JSTD_FORCEINLINE
    entry_type * insert_new_entry(const key_type & key, const mapped_type & value,
                                  hash_code_t hash_code, index_type index) {
        entry_type * new_entry = this->got_a_free_entry(hash_code, index);
        this->insert_to_bucket(new_entry, hash_code, index);
        this->construct_value(new_entry, key, value);
        this->entry_size_++;
        return new_entry;
    }

    JSTD_FORCEINLINE
    entry_type * insert_new_entry(const key_type & key, mapped_type && value,
                                  hash_code_t hash_code, index_type index) {
        entry_type * new_entry = this->got_a_free_entry(hash_code, index);
        this->insert_to_bucket(new_entry, hash_code, index);
        this->construct_value(new_entry, key, std::forward<mapped_type>(value));
        this->entry_size_++;
        return new_entry;
    }

    JSTD_FORCEINLINE
    entry_type * insert_new_entry(key_type && key, mapped_type && value,
                                  hash_code_t hash_code, index_type index) {
        entry_type * new_entry = this->got_a_free_entry(hash_code, index);
        this->insert_to_bucket(new_entry, hash_code, index);
        this->construct_value(new_entry, std::forward<key_type>(key),
                                         std::forward<mapped_type>(value));
        this->entry_size_++;
        return new_entry;
    }

    JSTD_FORCEINLINE
    entry_type * insert_new_entry(hash_code_t hash_code, index_type index,
                                  n_value_type && value) {
        entry_type * new_entry = this->got_a_free_entry(hash_code, index);
        this->insert_to_bucket(new_entry, hash_code, index);
        this->construct_value(new_entry, std::forward<n_value_type>(value));
        this->entry_size_++;
        return new_entry;
    }

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(const key_type & key, const mapped_type & value) {
        assert(this->buckets() != nullptr);
        bool inserted;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_for(hash_code);

        entry_type * entry = this->find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            this->insert_new_entry(key, value, hash_code, index);
            inserted = true;
        }
        else {
            if (!OnlyIfAbsent) {
                this->update_mapped_value(entry, value);
            }
            inserted = false;
        }

        this->update_version();

        return ReturnType(iterator(this, entry), inserted);
    }

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(const key_type & key, mapped_type && value) {
        assert(this->buckets() != nullptr);
        bool inserted;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_for(hash_code);

        entry_type * entry = this->find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            entry = this->insert_new_entry(key, std::forward<mapped_type>(value),
                                           hash_code, index);
            inserted = true;
        }
        else {
            if (!OnlyIfAbsent) {
                this->update_mapped_value(entry, std::forward<mapped_type>(value));
            }
            inserted = false;
        }

        this->update_version();

        return ReturnType(iterator(this, entry), inserted);
    }

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(key_type && key, mapped_type && value) {
        assert(this->buckets() != nullptr);
        bool inserted;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_for(hash_code);

        entry_type * entry = this->find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            entry = this->insert_new_entry(std::forward<key_type>(key),
                                           std::forward<mapped_type>(value),
                                           hash_code, index);
            inserted = true;
        }
        else {
            if (!OnlyIfAbsent) {
                // If key is a rvalue, we move it.
                key_type key_tmp = std::forward<key_type>(key);
                (void)key_tmp;
                this->update_mapped_value(entry, std::forward<mapped_type>(value));
            }
            inserted = false;
        }

        this->update_version();

        return ReturnType(iterator(this, entry), inserted);
    }

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(n_value_type && value) {
        assert(this->buckets() != nullptr);
        bool inserted;

        hash_code_t hash_code = this->get_hash(value.first);
        index_type index = this->index_for(hash_code);

        entry_type * entry = this->find_entry(value.first, hash_code, index);
        if (likely(entry == nullptr)) {
            entry = this->insert_new_entry(hash_code, index,
                                           std::forward<n_value_type>(value));
            inserted = true;
        }
        else {
            if (!OnlyIfAbsent) {
                // If key is a rvalue, we move it.
                key_type key_tmp = std::forward<key_type>(value.first);
                (void)key_tmp;
                this->update_mapped_value(entry, std::forward<mapped_type>(value.second));
            }
            inserted = false;
        }

        this->update_version();

        return ReturnType(iterator(this, entry), inserted);
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    entry_type * emplace_new_entry_args(hash_code_t hash_code, index_type index, Args && ... args) {
        entry_type * new_entry = this->got_a_free_entry(hash_code, index);
        this->insert_to_bucket(new_entry, hash_code, index);
        this->construct_value_args(new_entry, std::forward<Args>(args)...);
        this->entry_size_++;
        return new_entry;
    }

    JSTD_FORCEINLINE
    entry_type * emplace_new_entry(hash_code_t hash_code,
                                   index_type index, n_value_type && value) {
        entry_type * new_entry = this->got_a_free_entry(hash_code, index);
        this->insert_to_bucket(new_entry, hash_code, index);
        this->construct_value(new_entry, std::forward<n_value_type>(value));
        this->entry_size_++;
        return new_entry;
    }

    JSTD_FORCEINLINE
    entry_type * emplace_prepare_entry(entry_type * pre_entry,
                                       hash_code_t hash_code,
                                       index_type index) {
        this->insert_to_bucket(pre_entry, hash_code, index);
        pre_entry->attrib.setInUseEntry();
        this->entry_size_++;
        return pre_entry;
    }

    template <bool OnlyIfAbsent, typename ReturnType, typename ...Args>
    JSTD_FORCEINLINE
    ReturnType emplace_unique(const key_type & key, Args && ... args) {
        assert(this->buckets() != nullptr);
        bool inserted;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_for(hash_code);

        entry_type * entry = this->find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            entry = this->emplace_new_entry_args(hash_code, index,
                                                 std::forward<Args>(args)...);
            inserted = true;
        }
        else {
            if (!OnlyIfAbsent) {
                this->update_value_args(entry, std::forward<Args>(args)...);
            }
            inserted = false;
        }

        this->update_version();

        return ReturnType(iterator(this, entry), inserted);
    }

    template <bool OnlyIfAbsent, typename ReturnType, typename ...Args>
    JSTD_FORCEINLINE
    ReturnType emplace_unique(no_key_t nokey, Args && ... args) {
        assert(this->buckets() != nullptr);
        bool inserted;

        entry_type * pre_entry = this->got_a_prepare_entry();
        n_value_type * n_value = reinterpret_cast<n_value_type *>(&pre_entry->value);
        n_allocator_.constructor(n_value, std::forward<Args>(args)...);

        const key_type & key = pre_entry->value.first;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_for(hash_code);

        entry_type * entry = this->find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            entry = this->emplace_prepare_entry(pre_entry, hash_code, index);
            inserted = true;
        }
        else {
            if (!OnlyIfAbsent) {
                this->update_mapped_value(entry, std::move(n_value->second));
            }
            this->destroy_prepare_entry(pre_entry);
            inserted = false;
        }

        this->update_version();

        return ReturnType(iterator(this, entry), inserted);
    }

    template <bool OnlyIfAbsent, typename ReturnType, typename ...Args>
    JSTD_FORCEINLINE
    ReturnType emplace_unique_no_prepare(no_key_t nokey, Args && ... args) {
        assert(this->buckets() != nullptr);
        bool inserted;

        n_value_type value_tmp(std::forward<Args>(args)...);
        const key_type & key = value_tmp.first;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_for(hash_code);

        entry_type * entry = this->find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            entry = this->emplace_new_entry(hash_code, index,
                                            std::forward<n_value_type>(value_tmp));
            inserted = true;
        }
        else {
            if (!OnlyIfAbsent) {
                this->update_mapped_value(entry, std::move(value_tmp.second));
            }
            inserted = false;
        }

        this->update_version();

        return ReturnType(iterator(this, entry), inserted);
    }

    JSTD_FORCEINLINE
    size_type erase_key(const key_type & key) {
        assert(this->buckets_ != nullptr);

        if (likely(this->entry_size_ != 0)) {
            hash_code_t hash_code = this->get_hash(key);
            size_type index = this->index_for(hash_code);

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

                        // Use lazy destroy
                        this->destroy_entry(entry);

                        this->entry_size_--;

                        if (this->entry_capacity_ > kDefaultInitialCapacity &&
                            this->entry_size_ < this->entry_capacity_ / 8) {
                            rearrange_reorder();
                        }

                        this->update_version();

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

    JSTD_FORCEINLINE
    void update_version() {
#if DICTIONARY_SUPPORT_VERSION
        ++(this->version_);
#endif
    }

    void release_and_erase_freelist_in_chunk(size_type target_chunk_id) {
        entry_type * prev = nullptr;
        entry_type * entry = this->freelist_.head();
        while (entry != nullptr) {
            if (entry->attrib.getChunkId() == target_chunk_id) {
                assert(entry->attrib.isReusableEntry());
                this->allocator_.destructor(&entry->value);
                this->freelist_.erase(prev, entry);
            }
            prev = entry;
            entry = entry->next;
        }
    }

    void reorder_last_chunk_is_empty(size_type last_chunk_id, const entry_chunk_t & last_chunk) {
        size_type ahead_chunk_capacity = this->entry_capacity_ - last_chunk.capacity;
        ssize_type last_chunk_free_cnt = this->freelist_.size() - ahead_chunk_capacity;
        assert(last_chunk_free_cnt >= 0);

        // Release all entry->value resource in freelist that it is in last chunk.
        release_and_erase_freelist_in_chunk(last_chunk_id);

        // Clear last chunk.
        {
            entry_type * entries = this->chunk_list_[last_chunk_id].entries;
            size_type   capacity = this->chunk_list_[last_chunk_id].capacity;
            if (entries != nullptr) {
                this->entry_allocator_.deallocate(entries, capacity);
                this->chunk_list_[last_chunk_id].set_entries(nullptr);
                this->chunk_list_[last_chunk_id].set_capacity(0);

                this->chunk_list_.removeLastChunk();
            }

            this->chunk_list_.rebuildLastChunk();
        }

        // Here, we don't change the bucket count.
        this->entry_capacity_ = ahead_chunk_capacity;
    }

    void release_freelist_not_in_chunk(size_type target_chunk_id) {
        entry_type * entry = this->freelist_.head();
        while (entry != nullptr) {
            if (entry->attrib.getChunkId() != target_chunk_id) {
                assert(entry->attrib.isReusableEntry());
                this->allocator_.destructor(&entry->value);
            }
            else {
                assert(entry->attrib.isReusableEntry());
                entry->attrib.setChunkId(0);
            }
            entry = entry->next;
        }
    }

    void reorder_ahead_chunk_is_empty(size_type last_chunk_id, const entry_chunk_t & last_chunk) {
        // Release all entry->value resource in freelist that it's not in last chunk.
        release_freelist_not_in_chunk(last_chunk_id);

        // Clear another the chunks except last chunk.
        for (size_type i = 0; i < last_chunk_id; i++) {
            entry_type * entries = this->chunk_list_[i].entries;
            size_type   capacity = this->chunk_list_[i].capacity;
            if (entries != nullptr) {
                this->entry_allocator_.deallocate(entries, capacity);
                this->chunk_list_[i].set_entries(nullptr);
                this->chunk_list_[i].set_capacity(0);
            }
        }

        // Deal with last chunk
        {
            size_type ahead_chunk_capacity = this->entry_capacity_ - last_chunk.capacity;
            ssize_type last_chunk_free_cnt = this->freelist_.size() - ahead_chunk_capacity;
            assert(last_chunk_free_cnt >= 0);

            // If last_chunk_free_cnt <= 0, all entries in freelist can be clear.
            this->freelist_.clear();

            entry_chunk_t & last_chunk = this->chunk_list_.lastChunk();
            last_chunk.set_chunk_id(0);

            if (last_chunk_free_cnt > 0) {
                // Refresh the freelist and last chunk.
                size_type max_limit = last_chunk.size;
                entry_type * entry = last_chunk.entries;
                for (size_type i = 0; i < max_limit; i++) {
                    assert(!entry->attrib.isFreeEntry());
                    if (entry->attrib.isInUseEntry()) {
                        entry->attrib.setChunkId(0);
                    }
                    else {
                        assert(entry->attrib.isReusableEntry());
                        this->freelist_.push_front(entry);
                    }
                    entry++;
                }
            }
        }

        this->chunk_list_[0] = last_chunk;
        this->chunk_list_.resize(1);

        // Here, we don't change the bucket count.
        this->entry_capacity_ = last_chunk.capacity;
    }

    void release_and_erase_freelist_not_in_chunk(size_type target_chunk_id) {
        entry_type * prev = nullptr;
        entry_type * entry = this->freelist_.head();
        while (entry != nullptr) {
            if (entry->attrib.getChunkId() != target_chunk_id) {
                assert(entry->attrib.isReusableEntry());
                this->allocator_.destructor(&entry->value);
                this->freelist_.erase(prev, entry);
            }
            else {
                assert(entry->attrib.isReusableEntry());
                entry->attrib.setChunkId(0);
            }
            prev = entry;
            entry = entry->next;
        }
    }

    JSTD_FORCEINLINE
    entry_type * find_first_free_entry(entry_type * cur_entry, entry_type * last_entry) {
        while (cur_entry < last_entry) {
            if (likely(cur_entry->attrib.isInUseEntry())) {
                cur_entry++;
            }
            else {
                assert(cur_entry->attrib.isReusableEntry());
                return cur_entry;
            }
        }

        return nullptr;
    }

    void move_entry_to_target_chunk(entry_type * src_entry, entry_type * dest_entry) {
        assert(src_entry->attrib.isInUseEntry());
        assert(dest_entry->attrib.isReusableEntry());

        dest_entry->next      = src_entry->next;
        dest_entry->hash_code = src_entry->hash_code;
        dest_entry->attrib.setValue(kIsInUseEntry, 0);

        src_entry->next = dest_entry;
        src_entry->attrib.setRedirectEntry();

        n_value_type * n_src_value  = reinterpret_cast<n_value_type *>(&src_entry->value);
        n_value_type * n_dest_value = reinterpret_cast<n_value_type *>(&dest_entry->value);
        std::swap(n_src_value->first,  n_dest_value->first);
        std::swap(n_src_value->second, n_dest_value->second);

        this->allocator_.destructor(&src_entry->value);
    }

    void traverse_and_fix_buckets(size_type bucket_capacity) {
        assert(this->buckets() != nullptr);

        size_type index = 0;
        do {
            entry_type * first = this->buckets_[index];
            if (likely(first == nullptr)) {
                index++;
            }
            else {
                entry_type * prev;
                entry_type * entry;
                if (first->attrib.isRedirectEntry()) {
                    assert(first->next != nullptr);
                    this->buckets_[index] = first->next;
                    prev = first->next;
                    entry = first->next->next;
                }
                else {
                    assert(first->attrib.isInUseEntry());
                    prev = first;
                    entry = first->next;
                }

                while (entry != nullptr) {
                    if (entry->attrib.isRedirectEntry()) {
                        assert(entry->next != nullptr);
                        prev->next = entry->next;
                        prev = entry->next;
                        entry = entry->next->next;
                    }
                    else {
                        assert(entry->attrib.isInUseEntry());
                        prev = entry;
                        entry = entry->next;
                    }
                }
                index++;
            }
        } while (index < bucket_capacity);
    }

    void traverse_and_fix_buckets(entry_type ** new_buckets,
                                  size_type new_bucket_capacity,
                                  size_type bucket_capacity) {
        assert(this->buckets() != nullptr);
        assert(new_bucket_capacity < bucket_capacity);

        size_type index = new_bucket_capacity;
        index_type new_index;
        do {
            entry_type * first = this->buckets_[index];
            if (likely(first == nullptr)) {
                index++;
            }
            else {
                entry_type * entry;
                if (first->attrib.isRedirectEntry()) {
                    assert(first->next != nullptr);
                    new_index = this->index_for(first->next->hash_code, new_bucket_capacity - 1);
                    entry = first->next->next;
                    bucket_push_front(new_buckets, new_index, first->next);
                }
                else {
                    assert(first->attrib.isInUseEntry());
                    new_index = this->index_for(first->hash_code, new_bucket_capacity - 1);
                    entry = first->next;
                    bucket_push_front(new_buckets, new_index, first);
                }

                while (entry != nullptr) {
                    if (entry->attrib.isRedirectEntry()) {
                        assert(entry->next != nullptr);
                        new_index = this->index_for(entry->next->hash_code, new_bucket_capacity - 1);
                        entry_type * next_entry = entry->next->next;
                        bucket_push_front(new_buckets, new_index, entry->next);
                        entry = next_entry;
                    }
                    else {
                        assert(entry->attrib.isInUseEntry());
                        new_index = this->index_for(entry->hash_code, new_bucket_capacity - 1);
                        entry_type * next_entry = entry->next;
                        bucket_push_front(new_buckets, new_index, entry);
                        entry = next_entry;
                    }
                }
                index++;
            }
        } while (index < bucket_capacity);
    }

    void copy_and_fix_buckets(entry_type ** new_buckets, size_type new_bucket_capacity) {
        assert(this->buckets() != nullptr);
        assert(new_buckets != nullptr);
        assert(new_bucket_capacity <= this->bucket_count());

        size_type bucket_capacity = this->bucket_count();
        if (likely(new_bucket_capacity <= bucket_capacity)) {
            // Fix the previous entry redirect to new entry.
            traverse_and_fix_buckets(new_bucket_capacity);

            ::memcpy(new_buckets, this->buckets(), new_bucket_capacity * sizeof(entry_type *));

            if (new_bucket_capacity < bucket_capacity) {
                traverse_and_fix_buckets(new_buckets, new_bucket_capacity, bucket_capacity);
            }
        }
        else {
            assert(false);
        }
    }

    void rehash_and_fix_buckets(size_type new_bucket_capacity) {
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_bucket_capacity));
        assert(this->entry_size_ <= this->bucket_capacity_);

        entry_type ** new_buckets = bucket_allocator_.allocate(new_bucket_capacity);
        if (likely(bucket_allocator_.is_ok(new_buckets))) {
            // Here, we do not need to initialize the bucket list.
            copy_and_fix_buckets(new_buckets, new_bucket_capacity);

            this->free_buckets_impl();

            this->buckets_ = new_buckets;
            this->bucket_mask_ = new_bucket_capacity - 1;
            this->bucket_capacity_ = new_bucket_capacity;
        }
    }

    JSTD_FORCEINLINE
    size_type move_another_to_target_chunk(size_type target_chunk_id) {
        entry_chunk_t & target_chunk = this->chunk_list_[target_chunk_id];
        entry_type * dest_entry_first = target_chunk.entries;
        size_type    dest_size        = target_chunk.size;
        size_type    dest_capacity    = target_chunk.capacity;
        entry_type * dest_entry_last  = dest_entry_first + dest_capacity;
        entry_type * dest_entry       = dest_entry_first;
        size_type total_move_count = 0;

        // Move all of the inused entries to target chunk
        for (size_type i = 0; i < this->chunk_list_.size(); i++) {
            if (i != target_chunk_id) {
                entry_type * src_entry = this->chunk_list_[i].entries;
                if (src_entry != nullptr) {
                    size_type move_count = 0;
                    size_type src_size = this->chunk_list_[i].size;
                    size_type src_capacity;
                    if (i != this->chunk_list_.size() - 1)
                        src_capacity = this->chunk_list_[i].capacity;
                    else
                        src_capacity = this->chunk_list_.lastChunkSize();
                    entry_type * src_entry_last = src_entry + src_capacity;
                    while (src_entry < src_entry_last) {
                        if (src_entry->attrib.isReusableEntry()) {
                            this->allocator_.destructor(&src_entry->value);
                        }
                        else if (src_entry->attrib.isInUseEntry()) {
                            dest_entry = find_first_free_entry(dest_entry, dest_entry_last);
                            if (dest_entry != nullptr) {
                                //src_entry->attrib.setChunkIdsz(target_chunk_id);
                                move_entry_to_target_chunk(src_entry, dest_entry);
                                move_count++;
                                if (move_count > src_size)
                                    break;
                            }
                            else {
                                assert(false);
                                break;
                            }
                        }
                        src_entry++;
                    }
                    total_move_count += move_count;
                }
            }
        }

        assert((dest_size + total_move_count) <= dest_capacity);
        return total_move_count;
    }

    JSTD_FORCEINLINE
    size_type move_target_chunk_to_another(size_type target_chunk_id) {
        entry_chunk_t & target_chunk = this->chunk_list_[target_chunk_id];
        entry_type * dest_entry_first = target_chunk.entries;
        size_type    dest_size        = target_chunk.size;
        size_type    dest_capacity    = target_chunk.capacity;
        entry_type * dest_entry_last  = dest_entry_first + dest_capacity;
        entry_type * dest_entry       = dest_entry_first;
        size_type total_move_count = 0;

        return total_move_count;
    }

    void reorder_shrink_to(size_type new_entry_capacity) {
        new_entry_capacity = run_time::round_up_to_pow2(new_entry_capacity);
        new_entry_capacity = (std::max)(new_entry_capacity, kMinimumCapacity);
        assert(this->entry_size_ <= new_entry_capacity);

        size_type target_chunk_id = this->chunk_list_.findClosedChunk(new_entry_capacity);
        if (target_chunk_id != size_type(-1)) {
            entry_chunk_t & target_chunk = this->chunk_list_[target_chunk_id];
            entry_type * dest_entry_first = target_chunk.entries;
            size_type    dest_size        = target_chunk.size;
            size_type    dest_capacity    = target_chunk.capacity;
            size_type total_move_count;
            if (1) {
                total_move_count = move_another_to_target_chunk(target_chunk_id);
            }
            else {
                total_move_count = move_target_chunk_to_another(target_chunk_id);
            }

            // Resize the buckets and fix all redirect entries.
            rehash_and_fix_buckets(new_entry_capacity);

            // Clear another the chunks except target chunk.
            for (size_type i = 0; i < this->chunk_list_.size(); i++) {
                if (i != target_chunk_id) {
                    entry_type * entries = this->chunk_list_[i].entries;
                    if (entries != nullptr) {
                        size_type capacity = this->chunk_list_[i].capacity;
                        this->entry_allocator_.deallocate(entries, capacity);
                        this->chunk_list_[i].set_entries(nullptr);
                        this->chunk_list_[i].set_capacity(0);
                    }
                }
            }

            // Deal with target chunk
            {
                entry_chunk_t & target_chunk = this->chunk_list_[target_chunk_id];
                entry_chunk_t & last_chunk = this->chunk_list_.lastChunk();
                target_chunk.size = dest_size + total_move_count;
                last_chunk.set_entries(target_chunk.entries);
                last_chunk.set_capacity(target_chunk.capacity);
                last_chunk.set_chunk_id(0);

                this->freelist_.clear();

                // Refresh the target chunk.
                size_type used_count = 0;
                entry_type * entry = target_chunk.entries;
                size_type capacity = target_chunk.capacity;
                for (size_type i = 0; i < capacity; i++) {
                    assert(!entry->attrib.isFreeEntry());
                    if (entry->attrib.isInUseEntry()) {
                        if (entry->attrib.getChunkId() != 0)
                            entry->attrib.setChunkId(0);
                        used_count++;
                    }
                    else if (entry->attrib.isReusableEntry()) {
                        this->freelist_.push_front(entry);
                        used_count++;
                    }
                    else {
                        assert(false);
                    }
                    entry++;
                }

                last_chunk.set_size(used_count);
            }

            if (target_chunk_id != 0) {
                this->chunk_list_[0] = this->chunk_list_[target_chunk_id];
            }
            this->chunk_list_.resize(1);

            // Here, we don't change the bucket count.
            this->entry_capacity_ = new_entry_capacity;
        }
        else {
            // No matching chunk found
            this->realloc_to(new_entry_capacity);
        }
    }

    void reorder_shrink_to_fit() {
        reorder_shrink_to(this->entry_size_);
    }

    void entry_value_move_construct(entry_type * old_entry, entry_type * new_entry) {
        assert(old_entry != new_entry);

        n_value_type * n_old_value = reinterpret_cast<n_value_type *>(&old_entry->value);
        n_value_type * n_new_value = reinterpret_cast<n_value_type *>(&new_entry->value);

        bool first_has_move_construct =
            std::is_nothrow_move_constructible<typename n_value_type::first_type>::value;
        bool second_has_move_construct =
            std::is_nothrow_move_constructible<typename n_value_type::second_type>::value;

        if (first_has_move_construct && second_has_move_construct) {
            this->n_allocator_.constructor(n_new_value, std::move(n_old_value->first),
                                                        std::move(n_old_value->second));
        }
        else if (!first_has_move_construct && second_has_move_construct) {
            this->n_allocator_.constructor(n_new_value, n_old_value->first,
                                              std::move(n_old_value->second));
        }
        else if (first_has_move_construct && !second_has_move_construct) {
            this->n_allocator_.constructor(n_new_value, std::move(n_old_value->first),
                                                                  n_old_value->second);
        }
        else {
            assert(!first_has_move_construct && !second_has_move_construct);
            this->n_allocator_.constructor(n_new_value, n_old_value->first,
                                                        n_old_value->second);
        }

        this->n_allocator_.destructor(n_old_value);
    }

    void entry_value_move_assignment(entry_type * old_entry, entry_type * new_entry) {
        assert(old_entry != new_entry);

        n_value_type * n_old_value = reinterpret_cast<n_value_type *>(&old_entry->value);
        n_value_type * n_new_value = reinterpret_cast<n_value_type *>(&new_entry->value);

        bool first_has_move_assignment =
            std::is_nothrow_move_assignable<typename n_value_type::first_type>::value;
        bool second_has_move_assignment =
            std::is_nothrow_move_assignable<typename n_value_type::second_type>::value;

        if (first_has_move_assignment)
            n_new_value->first = std::move(n_old_value->first);
        else
            n_new_value->first = n_old_value->first;

        if (second_has_move_assignment)
            n_new_value->second = std::move(n_old_value->second);
        else
            n_new_value->second = n_old_value->second;
    }

    JSTD_FORCEINLINE
    void transfer_entry_to_new(entry_type * old_entry, entry_type * new_entry) {
        assert(old_entry != new_entry);
        assert(old_entry->attrib.isInUseEntry());
        new_entry->next      = new_entry + 1;
        new_entry->hash_code = old_entry->hash_code;
        new_entry->attrib.setValue(old_entry->attrib.getEntryType(), 0);
        old_entry->attrib.setFreeEntry();

        entry_value_move_construct(old_entry, new_entry);
    }

    size_type realloc_high_bucket_push_back(
                size_type new_entry_count, entry_type * prev_last,
                index_type index, index_type new_index,
                entry_type ** new_buckets, size_type new_bucket_capacity,
                entry_type * new_entries, size_type new_entry_capacity) {
        entry_type * old_first = this->buckets_[new_index];
        if (likely(old_first == nullptr)) {
            if (prev_last == nullptr) {
                new_buckets[index] = nullptr;
            }
        }
        else {
            // Get the first free entry in the new entries list.
            entry_type * new_entry = &new_entries[new_entry_count++];
            assert(new_entry_count <= this->entry_size_);

            // Transfer the first old entry to new entry.
            transfer_entry_to_new(old_first, new_entry);

            entry_type * new_first = new_entry;
            entry_type * old_entry = old_first->next;
            while (likely(old_entry != nullptr)) {
                // Get the first free entry in the new entries list.
                new_entry = &new_entries[new_entry_count++];
                assert(new_entry_count <= this->entry_size_);

                // Transfer the old entry to new entry.
                transfer_entry_to_new(old_entry, new_entry);
#ifndef NDEBUG
                hash_code_t hash_code = old_entry->hash_code;
                index_type new_index = this->index_for(hash_code, new_bucket_capacity - 1);
                assert(new_index == index);
#endif
                old_entry = old_entry->next;
            }

            // Set the next pointer of last entry to nullptr.
            assert(new_entry != nullptr);
            new_entry->next = nullptr;

            if (prev_last != nullptr) {
                prev_last->next = new_first;
            }
            else {
                new_buckets[index] = new_first;
            }
        }

        return new_entry_count;
    }

    void realloc_all_entries_half(entry_type ** new_buckets, size_type new_bucket_capacity,
                                  entry_type * new_entries, size_type new_entry_capacity) {
        assert_buckets_capacity(new_buckets, new_bucket_capacity);
        assert_entries_capacity(new_entries, new_entry_capacity);
        assert(this->bucket_capacity_ == new_bucket_capacity * 2);

        size_type new_bucket_mask = new_bucket_capacity - 1;
        size_type old_bucket_capacity = this->bucket_capacity_;

        size_type new_entry_count = 0;

        for (size_type index = 0; index < new_bucket_capacity; index++) {
            entry_type * old_first = this->buckets_[index];
            if (likely(old_first == nullptr)) {
                index_type new_index = index + new_bucket_capacity;
                new_entry_count = realloc_high_bucket_push_back(
                                            new_entry_count, nullptr,
                                            index, new_index,
                                            new_buckets, new_bucket_capacity,
                                            new_entries, new_entry_capacity);
            }
            else {
                // Get the first free entry in the new entries list.
                entry_type * new_entry = &new_entries[new_entry_count++];
                assert(new_entry_count <= this->entry_size_);

                // Transfer the first old entry to new entry.
                transfer_entry_to_new(old_first, new_entry);

                new_buckets[index] = new_entry;

                entry_type * new_prev = new_entry;
                entry_type * old_entry = old_first->next;
                while (likely(old_entry != nullptr)) {
                    // Get the first free entry in the new entries list.
                    new_entry = &new_entries[new_entry_count++];
                    assert(new_entry_count <= this->entry_size_);

                    // Transfer the old entry to new entry.
                    transfer_entry_to_new(old_entry, new_entry);
#ifndef NDEBUG
                    hash_code_t hash_code = old_entry->hash_code;
                    index_type new_index = this->index_for(hash_code, new_bucket_mask);
                    assert(new_index == index);
#endif
                    new_prev = new_entry;
                    old_entry = old_entry->next;
                }

                // Set the next pointer of last entry to nullptr.
                assert(new_entry != nullptr);
                new_entry->next = nullptr;

                index_type new_index = index + new_bucket_capacity;
                new_entry_count = realloc_high_bucket_push_back(
                                            new_entry_count, new_prev,
                                            index, new_index,
                                            new_buckets, new_bucket_capacity,
                                            new_entries, new_entry_capacity);
            }
        }
    }

    void realloc_all_entries_2x(entry_type ** new_buckets, size_type new_bucket_capacity,
                                entry_type * new_entries, size_type new_entry_capacity) {
        assert_buckets_capacity(new_buckets, new_bucket_capacity);
        assert_entries_capacity(new_entries, new_entry_capacity);
        assert(new_bucket_capacity == this->bucket_capacity_ * 2);

        size_type new_bucket_mask = new_bucket_capacity - 1;
        size_type old_bucket_capacity = this->bucket_capacity_;

        size_type new_entry_count = 0;

        for (size_type index = 0; index < old_bucket_capacity; index++) {
            entry_type * old_entry = this->buckets_[index];
            if (likely(old_entry == nullptr)) {
                index_type new_index = index + old_bucket_capacity;
                new_buckets[index] = nullptr;
                new_buckets[new_index] = nullptr;
            }
            else {
                entry_type * new_prev = nullptr;
                entry_type * old_list = nullptr;
                entry_type * new_list = nullptr;
                entry_type * new_entry = nullptr;

                do {
                    // Get the first free entry in the new entries list.
                    new_entry = &new_entries[new_entry_count++];
                    assert(new_entry_count <= this->entry_size_);

                    // Transfer the old entry to new entry.
                    transfer_entry_to_new(old_entry, new_entry);

                    hash_code_t hash_code = old_entry->hash_code;
                    index_type new_index = this->index_for(hash_code, new_bucket_mask);
                    if (likely(new_index != index)) {
                        // Remove the new entry from old list.
                        if (likely(new_prev != nullptr)) {
                            new_prev->next = new_entry->next;
                        }
                        entry_type * next_entry = old_entry->next;

                        // Push front the new entry to the new list.
                        new_entry->next = new_list;
                        new_list = new_entry;

                        old_entry = next_entry;
                    }
                    else {
                        if (unlikely(old_list == nullptr)) {
                            old_list = new_entry;
                        }
                        new_prev = new_entry;
                        old_entry = old_entry->next;
                    }
                } while (likely(old_entry != nullptr));

                // Set the next pointer of last entry in the old list to nullptr .
                if (new_entry != nullptr && new_entry == new_prev) {
                    new_entry->next = nullptr;
                }

                index_type new_index = index + old_bucket_capacity;
                new_buckets[index] = old_list;
                new_buckets[new_index] = new_list;
            }
        }
    }

    void realloc_all_entries(entry_type ** new_buckets, size_type new_bucket_capacity,
                             entry_type * new_entries, size_type new_entry_capacity) {
        assert_buckets_capacity(new_buckets, new_bucket_capacity);
        assert_entries_capacity(new_entries, new_entry_capacity);
        assert(new_entry_capacity != this->entry_capacity_);
        assert(new_bucket_capacity != this->bucket_capacity_);

        size_type new_bucket_mask = new_bucket_capacity - 1;
        size_type old_bucket_capacity = this->bucket_capacity_;

        if (likely(new_bucket_capacity < old_bucket_capacity)) {
            size_type new_entry_count = 0;
            size_type index;

            // Range: [0, new_bucket_capacity)
            for (index = 0; index < new_bucket_capacity; index++) {
                entry_type * old_entry = this->buckets_[index];
                if (likely(old_entry == nullptr)) {
                    new_buckets[index] = nullptr;
                }
                else {
                    entry_type * new_prev = nullptr;
                    entry_type * old_list = nullptr;
                    entry_type * new_entry;
                    do {
                        // Get the first free entry in the new entries list.
                        new_entry = &new_entries[new_entry_count++];
                        assert(new_entry_count <= this->entry_size_);

                        // Transfer the old entry to new entry.
                        transfer_entry_to_new(old_entry, new_entry);

                        hash_code_t hash_code = old_entry->hash_code;
                        index_type new_index = this->index_for(hash_code, new_bucket_mask);
                        if (likely(new_index != index)) {
                            if (new_prev != nullptr) {
                                new_prev->next = new_entry->next;
                            }

                            entry_type * next_entry = old_entry->next;
                            bucket_push_front(new_buckets, new_index, new_entry);
                            old_entry = next_entry;
                        }
                        else {
                            if (unlikely(old_list == nullptr)) {
                                old_list = new_entry;
                            }
                            new_prev = new_entry;
                            old_entry = old_entry->next;
                        }
                    } while (likely(old_entry != nullptr));

                    // Set the next pointer of last entry in the old list to nullptr .
                    if (new_entry != nullptr && new_entry == new_prev) {
                        new_entry->next = nullptr;
                    }

                    new_buckets[index] = old_list;
                }
            }

            // Range: [new_bucket_capacity, old_bucket_capacity)
            for (; index < old_bucket_capacity; index++) {
                entry_type * old_entry = this->buckets_[index];
                if (likely(old_entry != nullptr)) {
                    do {
                        // Get the first free entry in the new entries list.
                        entry_type * new_entry = &new_entries[new_entry_count++];
                        assert(new_entry_count <= this->entry_size_);

                        // Transfer the old entry to new entry.
                        transfer_entry_to_new(old_entry, new_entry);

                        hash_code_t hash_code = old_entry->hash_code;
                        index_type new_index = this->index_for(hash_code, new_bucket_mask);

                        entry_type * next_entry = old_entry->next;
                        bucket_push_front(new_buckets, new_index, new_entry);
                        old_entry = next_entry;

                    } while (likely(old_entry != nullptr));
                }
            }

            assert(new_entry_count == this->entry_size_);
        }
        else {
            assert(new_bucket_capacity > old_bucket_capacity);
            std::memset((void *)&new_buckets[old_bucket_capacity], 0,
                        (new_bucket_capacity - old_bucket_capacity) * sizeof(entry_type *));

            size_type new_entry_count = 0;

            for (size_type index = 0; index < old_bucket_capacity; index++) {
                entry_type * old_entry = this->buckets_[index];
                if (likely(old_entry == nullptr)) {
                    new_buckets[index] = nullptr;
                }
                else {
                    entry_type * new_prev = nullptr;
                    entry_type * old_list = nullptr;
                    entry_type * new_entry = nullptr;

                    do {
                        // Get the first free entry in the new entries list.
                        new_entry = &new_entries[new_entry_count++];
                        assert(new_entry_count <= this->entry_size_);

                        // Transfer the old entry to new entry.
                        transfer_entry_to_new(old_entry, new_entry);

                        hash_code_t hash_code = old_entry->hash_code;
                        index_type new_index = this->index_for(hash_code, new_bucket_mask);
                        if (likely(new_index != index)) {
                            // Remove the new entry from old list.
                            if (likely(new_prev != nullptr)) {
                                new_prev->next = new_entry->next;
                            }
                            entry_type * next_entry = old_entry->next;

                            // Push front the new entry to the new bucket.
                            bucket_push_front(new_buckets, new_index, new_entry);

                            old_entry = next_entry;
                        }
                        else {
                            if (unlikely(old_list == nullptr)) {
                                old_list = new_entry;
                            }
                            new_prev = new_entry;
                            old_entry = old_entry->next;
                        }
                    } while (likely(old_entry != nullptr));

                    // Set the next pointer of last entry in the old list to nullptr .
                    if (new_entry != nullptr && new_entry == new_prev) {
                        new_entry->next = nullptr;
                    }

                    new_buckets[index] = old_list;
                }
            }

            assert(new_entry_count == this->entry_size_);
        }
    }

    void realloc_all_entries(entry_type * new_entries, size_type new_entry_capacity) {
        assert_entries_capacity(new_entries, new_entry_capacity);
        assert(new_entry_capacity != this->entry_capacity_);

        size_type old_bucket_capacity = this->bucket_capacity_;
        size_type old_bucket_mask = this->bucket_mask_;

        size_type new_entry_count = 0;

        for (size_type index = 0; index < old_bucket_capacity; index++) {
            entry_type * old_entry = this->buckets_[index];
            if (likely(old_entry == nullptr)) {
                // Do nothing !!
            }
            else {
                do {
                    // Get the first free entry in the new entries list.
                    entry_type * new_entry = &new_entries[new_entry_count++];
                    assert(new_entry_count <= this->entry_size_);

                    // Transfer the old entry to new entry.
                    transfer_entry_to_new(old_entry, new_entry);

                    old_entry = old_entry->next;
                } while (likely(old_entry != nullptr));
            }
        }

        assert(new_entry_count == this->entry_size_);
    }

    void realloc_buckets_and_entries(size_type new_entry_capacity, size_type new_bucket_capacity) {
        assert_entry_capacity(new_entry_capacity);
        assert_bucket_capacity(new_bucket_capacity);
        assert(new_bucket_capacity != this->bucket_capacity_);

        entry_type ** new_buckets = bucket_allocator_.allocate(new_bucket_capacity);
        if (likely(bucket_allocator_.is_ok(new_buckets))) {
            entry_type * new_entries = entry_allocator_.allocate(new_entry_capacity);
            if (likely(entry_allocator_.is_ok(new_entries))) {
                if (likely(this->entry_size_ != 0)) {
                    size_type old_bucket_capacity = this->bucket_capacity_;
                    if (likely(new_bucket_capacity == (old_bucket_capacity * 2))) {
                        realloc_all_entries_2x(new_buckets, new_bucket_capacity,
                                               new_entries, new_entry_capacity);
                    }
                    else if (likely(old_bucket_capacity == (new_bucket_capacity * 2))) {
                        realloc_all_entries_half(new_buckets, new_bucket_capacity,
                                                 new_entries, new_entry_capacity);
                    }
                    else {
                        realloc_all_entries(new_buckets, new_bucket_capacity,
                                            new_entries, new_entry_capacity);
                    }
                }
                else {
                    std::memset((void *)new_buckets, 0, new_bucket_capacity * sizeof(entry_type *));
                }

                this->destory_resources();

                this->buckets_ = new_buckets;
                this->entries_ = new_entries;
                this->bucket_mask_ = new_bucket_capacity - 1;
                this->bucket_capacity_ = new_bucket_capacity;
                // Needn't change the entry_size_.
                this->entry_capacity_ = new_entry_capacity;

                // Push the new entries pointer to entries list.
                this->chunk_list_.addChunk(new_entries, this->entry_size_, new_entry_capacity);

                this->freelist_.clear();
            }
            else {
                // Failed to allocate new_entries, processing the abnormal exit.
                assert(new_buckets != nullptr);
                bucket_allocator_.deallocate(new_buckets, new_bucket_capacity);
            }
        }
    }

    void realloc_entries(size_type new_entry_capacity) {
        assert_entry_capacity(new_entry_capacity);

        entry_type * new_entries = entry_allocator_.allocate(new_entry_capacity);
        if (likely(entry_allocator_.is_ok(new_entries))) {
            if (likely(this->entry_size_ != 0)) {
                realloc_all_entries(new_entries, new_entry_capacity);
            }

            this->destory_entries();

            this->entries_ = new_entries;
            // Needn't change the entry_size_.
            this->entry_capacity_ = new_entry_capacity;

            // Push the new entries pointer to entries list.
            this->chunk_list_.addChunk(new_entries, this->entry_size_, new_entry_capacity);

            this->freelist_.clear();
        }
    }

    JSTD_FORCEINLINE
    void realloc_to(size_type new_entry_size) {
        size_type new_entry_capacity = run_time::round_up_to_pow2(new_entry_size);
        new_entry_capacity = (std::max)(new_entry_capacity, kMinimumCapacity);
        assert(this->entry_size_ <= new_entry_capacity);

        size_type new_bucket_capacity = new_entry_capacity / kMaxLoadFactor;
        if (new_entry_capacity != this->entry_capacity_) {
            if (likely(new_bucket_capacity != this->bucket_capacity_)) {
                realloc_buckets_and_entries(new_entry_capacity, new_bucket_capacity);
            }
            else {
                realloc_entries(new_entry_capacity);
            }
        }
    }

    JSTD_FORCEINLINE
    void realloc_to_fit() {
        this->realloc_to(this->entry_size_);
    }

#if 1
    JSTD_FORCEINLINE
    void rearrange_reorder() {
        if (this->chunk_list_.size() > 0) {
            size_type last_chunk_capacity = this->chunk_list_.lastChunk().capacity;
            if (last_chunk_capacity < kMaxEntryChunkSize) {
                if (likely(this->chunk_list_.size() > 1)) {
                    reorder_shrink_to(this->entry_capacity_ / 4);
                }
                else {
                    assert(this->chunk_list_.size() == 1);
                    realloc_to_fit();
                }
            }
            else {
                assert(last_chunk_capacity >= kMaxEntryChunkSize);
            }
        }
    }
#else
    JSTD_FORCEINLINE
    void rearrange_reorder() {
        if (this->chunk_list_.size() > 0) {
            size_type first_chunk_capacity = this->chunk_list_[0].capacity;
            if (first_chunk_capacity < kMaxEntryChunkSize) {
                assert(run_time::is_pow2(first_chunk_capacity));
                if (likely(this->chunk_list_.size() > 1)) {
                    size_type last_chunk_id = this->chunk_list_.size() - 1;
                    const entry_chunk_t & last_chunk = this->chunk_list_[last_chunk_id];
                    size_type last_chunk_capacity = this->chunk_list_[last_chunk_id].capacity;
                    assert(run_time::is_pow2(last_chunk_capacity));
                    size_type ahead_chunk_size = this->entry_size_ - last_chunk.size;
                    if (ahead_chunk_size != 0) {
                        if (last_chunk.size != 0) {
                            reorder_shrink_to(this->entry_capacity_ / 4));
                        }
                        else {
                            assert(last_chunk.size == 0);
                            reorder_last_chunk_is_empty(last_chunk_id, last_chunk);
                        }
                    }
                    else {
                        assert(ahead_chunk_size == 0);
                        reorder_ahead_chunk_is_empty(last_chunk_id, last_chunk);
                    }
                }
                else if (this->chunk_list_.size() == 1) {
                    realloc_to_fit();
                }
            }
            else {
                assert(first_chunk_capacity >= kMaxEntryChunkSize);
            }
        }
    }
#endif

public:
    inline hash_code_t get_hash(const key_type & key) const {
        hash_code_t hash_code = this->hasher_(key);
        //hash_code = hash_code ^ (hash_code >> 16);
        return hash_code;
    }

    void clear() {
        this->destroy();
        this->init(kDefaultInitialCapacity);

        this->update_version();
    }

    void rehash(size_type bucket_count) {
        assert(bucket_count > 0);
        size_type new_bucket_capacity = this->calc_capacity(bucket_count);
        this->rehash_impl<false>(new_bucket_capacity);
    }

#if 1
    void reserve(size_type new_size) {
        this->resize_impl(new_size);
    }
#else
    void reserve(size_type bucket_count) {
        this->rehash(bucket_count);
    }
#endif

    void resize(size_type new_size) {
        this->resize_impl(new_size);
    }

    void shrink_to_fit(size_type bucket_count = 0) {
        size_type entry_capacity = run_time::round_up_to_pow2(this->entry_size_);

        // Choose the maximum size of new bucket capacity and now entry capacity.
        bucket_count = (entry_capacity >= bucket_count) ? entry_capacity : bucket_count;

        size_type new_bucket_capacity = this->calc_capacity(bucket_count);
        this->rehash_impl<true>(new_bucket_capacity);
    }

    JSTD_FORCEINLINE
    void rearrange(size_type arrangeType) {
        if (arrangeType == ArrangeType::Reorder) {
            rearrange_reorder();
        }
        else if (arrangeType == ArrangeType::Realloc) {
            realloc_to_fit();
        }
        else {
            assert(false);
        }
    }

    bool contains(const key_type & key) {
        iterator iter = this->find(key);
        return (iter != this->end());
    }

    iterator find(const key_type & key) {
        if (likely(this->buckets() != nullptr)) {
            entry_type * entry = this->find_entry(key);
            return iterator(this, entry);
        }

        return iterator(this, nullptr);   // Error: buckets data is invalid
    }

    const_iterator find(const key_type & key) const {
        if (likely(this->buckets() != nullptr)) {
            entry_type * entry = this->find_entry(key);
            return const_iterator(this, entry);
        }

        return const_iterator(this, nullptr);   // Error: buckets data is invalid
    }

    //
    // insert(key, value)
    //
    insert_return_type insert(const key_type & key, const mapped_type & value) {
        return this->insert_unique<false, insert_return_type>(key, value);
    }

    insert_return_type insert(const key_type & key, mapped_type && value) {
        return this->insert_unique<false, insert_return_type>(key, std::forward<mapped_type>(value));
    }

    insert_return_type insert(key_type && key, mapped_type && value) {
        return this->insert_unique<false, insert_return_type>(std::forward<key_type>(key),
                                                              std::forward<mapped_type>(value));
    }

    insert_return_type insert(const value_type & value) {
        return this->insert(value.first, value.second);
    }

    insert_return_type insert(value_type && value) {
#if 1
        bool has_move_constructor = std::is_trivially_move_constructible<value_type>::value;
        bool has_move_assignment = std::is_trivially_move_assignable<value_type>::value;
        return this->insert_unique<false, insert_return_type>(
            std::forward<n_value_type>(*reinterpret_cast<n_value_type *>(&value)));
#else   
        bool is_rvalue = std::is_rvalue_reference<decltype(std::forward<value_type>(value))>::value;
        if (is_rvalue) {
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&value);
            return this->insert(std::move(n_value->first), std::move(n_value->second));
        }
        else {
            return this->insert(value.first, value.second);
        }
#endif
    }

    //
    // insert_no_return(key, value)
    //
    void insert_no_return(const key_type & key, const mapped_type & value) {
        this->insert_unique<false, void_warpper>(key, value);
    }

    void insert_no_return(const key_type & key, mapped_type && value) {
        this->insert_unique<false, void_warpper>(key, std::forward<mapped_type>(value));
    }

    void insert_no_return(key_type && key, mapped_type && value) {
        this->insert_unique<false, void_warpper>(std::forward<key_type>(key),
                                                 std::forward<mapped_type>(value));
    }

    void insert_no_return(const value_type & value) {
        this->insert_no_return(value.first, value.second);
    }

    void insert_no_return(value_type && value) {
        bool is_rvalue = std::is_rvalue_reference<decltype(std::forward<value_type>(value))>::value;
        if (is_rvalue) {
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&value);
            this->insert_no_return(std::move(n_value->first), std::move(n_value->second));
        }
        else {
            this->insert_no_return(value.first, value.second);
        }
    }

    /***************************************************************************/
    /*                                                                         */
    /* See: https://en.cppreference.com/w/cpp/container/unordered_map/emplace  */
    /*                                                                         */
    /*   emplace() don't support this interface:                               */
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
        return this->emplace_unique<false, insert_return_type>(
            key_extractor<value_type>::extract(std::forward<Args>(args)...),
            std::forward<Args>(args)...);
    }

    template <typename ...Args>
    void emplace_no_return(Args && ... args) {
        this->emplace_unique<false, void_warpper>(
            key_extractor<value_type>::extract(std::forward<Args>(args)...),
            std::forward<Args>(args)...);
    }

    size_type erase(const key_type & key) {
        return this->erase_key(key);
    }

    void swap(this_type & other) {
        if (&other != this) {
            std::swap(this->buckets_,           other.buckets_);
            std::swap(this->entries_,           other.entries_);
            std::swap(this->bucket_mask_,       other.bucket_mask_);
            std::swap(this->bucket_capacity_,   other.bucket_capacity_);

            std::swap(this->entry_size_,        other.entry_size_);
            std::swap(this->entry_capacity_,    other.entry_capacity_);
#if DICTIONARY_SUPPORT_VERSION
            std::swap(this->version_,           other.version_);
#endif
            this->freelist_.swap(other.freelist_);
            this->chunk_list_.swap(other.chunk_list_);
        }
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

template <typename Key, typename Value,
          typename Hasher = hash<Key, std::uint32_t, HashFunc_Time31>,
          std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary_Time31 = BasicDictionary<Key, Value, HashFunc_Time31, false, Alignment, Hasher>;

template <typename Key, typename Value,
          typename Hasher = hash<Key, std::uint32_t, HashFunc_Time31Std>,
          std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary_Time31Std = BasicDictionary<Key, Value, HashFunc_Time31Std, false, Alignment, Hasher>;

#if JSTD_HAVE_SSE42_CRC32C
template <typename Key, typename Value,
          typename Hasher = hash<Key, std::uint32_t, HashFunc_CRC32C>,
          std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary = BasicDictionary<Key, Value, HashFunc_CRC32C, false, Alignment, Hasher>;
#else
template <typename Key, typename Value,
          typename Hasher = hash<Key, std::uint32_t, HashFunc_Time31>,
          std::size_t Alignment = align_of<std::pair<const Key, Value>>::value>
using Dictionary = BasicDictionary<Key, Value, HashFunc_Time31, false, Alignment, Hasher>;
#endif // JSTD_HAVE_SSE42_CRC32C

} // namespace jstd

#endif // JSTD_HASH_DICTIONARY_H
