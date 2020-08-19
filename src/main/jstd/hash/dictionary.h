
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

#include "jstd/allocator.h"
#include "jstd/iterator.h"
#include "jstd/hash/hash_helper.h"
#include "jstd/hash/equal_to.h"
#include "jstd/hash/dictionary_traits.h"
#include "jstd/hash/key_extractor.h"
#include "jstd/support/PowerOf2.h"

#define ENABLE_JSTD_DICTIONARY          1
#define DICTIONARY_SUPPORT_VERSION      0

#define USE_JAVA_FIND_ENTRY             1

namespace jstd {

//
// hash_entry_chunk<T>
//
template <typename T>
class hash_entry_chunk {
public:
    typedef T               entry_type;
    typedef std::size_t     size_type;

private:
    entry_type * entries_;
    size_type    size_;
    size_type    capacity_;
    size_type    chunk_id_;

public:
    hash_entry_chunk() : entries_(nullptr), size_(0), capacity_(0), chunk_id_(0) {}
    hash_entry_chunk(size_type chunk_id, entry_type * entries, size_type capacity)
        : entries_(entries), size_(0), capacity_(capacity), chunk_id_(chunk_id) {}
    ~hash_entry_chunk() {}

    entry_type * entries() const { return this->entries_; }
    size_type size() const { return this->size_; }
    size_type capacity() const { return this->capacity_; }
    size_type frees() const { return size_type(this->capacity() - this->size()); }
    size_type chunk_id() const { return this->chunk_id_; }

    size_type is_empty() const { return (this->size_ == 0); }
    size_type is_full() const { return (this->size_ >= this->capacity_); }

    void set_chunk(size_type chunk_id, entry_type * entries, size_type capacity) {
        this->entries_ = entries;
        this->size_ = 0;
        this->capacity_ = capacity;
        this->chunk_id_ = chunk_id;
    }

    void set_chunk(size_type chunk_id, entry_type * entries, size_type size, size_type capacity) {
        this->entries_ = entries;
        this->size_ = size;
        this->capacity_ = capacity;
        this->chunk_id_ = chunk_id;
    }

    void set_entries(entry_type * entries) {
        this->entries_ = entries;
    }

    void set_size(size_type size) {
        this->size_ = size;
    }

    void set_capacity(size_type capacity) {
        this->capacity_ = capacity;
    }

    void set_chunk_id(size_type chunk_id) {
        this->chunk_id_ = chunk_id;
    }

    void clear() {
        this->entries_ = nullptr;
        this->size_ = 0;
        this->capacity_ = 0;
        this->chunk_id_ = 0;
    }

    void reset() {
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
};

template <typename T, typename Allocator, typename EntryAllocator>
class hash_entry_chunk_list {
public:
    typedef T                                       entry_type;
    typedef hash_entry_chunk<T>                     entry_chunk_t;

    typedef std::vector<entry_chunk_t>              vector_type;
    typedef typename vector_type::value_type        value_type;
    typedef typename vector_type::size_type         size_type;
    typedef typename vector_type::difference_type   difference_type;
    typedef typename vector_type::iterator          iterator;
    typedef typename vector_type::const_pointer     const_pointer;
    typedef typename vector_type::reference         reference;
    typedef typename vector_type::const_reference   const_reference;

    typedef Allocator                               allocator_type;
    typedef EntryAllocator                          entry_allocator_type;

    entry_chunk_t           last_chunk_;

private:
    vector_type             chunk_list_;
    allocator_type          allocator_;
    entry_allocator_type    entry_allocator_;

public:
    hash_entry_chunk_list() {}
    ~hash_entry_chunk_list() {
        this->destory();
    }

    size_type size() const       { return this->chunk_list_.size();     }
    size_type capacity() const   { return this->chunk_list_.capacity(); }

    iterator begin()             { return this->chunk_list_.begin();  }
    iterator end()               { return this->chunk_list_.end();    }
    const_pointer begin() const  { return this->chunk_list_.begin();  }
    const_pointer end() const    { return this->chunk_list_.end();    }

    iterator cbegin()            { return this->chunk_list_.cbegin(); }
    iterator cend()              { return this->chunk_list_.cend();   }
    const_pointer cbegin() const { return this->chunk_list_.cbegin(); }
    const_pointer cend() const   { return this->chunk_list_.cend();   }

    reference front() { return this->chunk_list_.front(); }
    reference back()  { return this->chunk_list_.back();  }

    const_reference front() const { return this->chunk_list_.front(); }
    const_reference back() const  { return this->chunk_list_.back();  }

    bool is_empty() const { return (this->size() == 0); }

    bool hasLastChunk() const {
        return (this->last_chunk_.entries() != nullptr);
    }

    entry_chunk_t & lastChunk() {
        return this->last_chunk_;
    }

    const entry_chunk_t & lastChunk() const {
        return this->last_chunk_;
    }

    uint32_t lastChunkId() const {
        return static_cast<uint32_t>(this->last_chunk_.chunk_id());
    }

    uint32_t lastChunkSize() const {
        return static_cast<uint32_t>(this->last_chunk_.size());
    }

public:
    void destory() {
        if (likely(this->chunk_list_.size() > 0)) {
            size_type last_index = this->chunk_list_.size() - 1;
            for (size_type i = 0; i < last_index; i++) {
                entry_type * entries = this->chunk_list_[i].entries();
                size_type   capacity = this->chunk_list_[i].capacity();
                entry_type * entry = entries;
                assert(entry != nullptr);
                for (size_type j = 0; j < capacity; j++) {
                    if (likely(!entry->attrib.isFreeEntry())) {
                        this->allocator_.destructor(&entry->value);
                    }
                    ++entry;
                }

                // Free the entries buffer.
                this->entry_allocator_.deallocate(entries, capacity);
            }

            // Destroy last entry
            {
                entry_type *    entries = this->last_chunk_.entries();
                size_type     last_size = this->last_chunk_.size();
                size_type last_capacity = this->last_chunk_.capacity();
                assert(last_size <= last_capacity);
                entry_type * entry = entries;
                assert(entry != nullptr);
                for (size_type j = 0; j < last_size; j++) {
                    if (likely(!entry->attrib.isFreeEntry())) {
                        this->allocator_.destructor(&entry->value);
                    }
                    ++entry;
                }

                // Free the entries buffer.
                this->entry_allocator_.deallocate(entries, last_capacity);
            }
        }

        this->clear();
    }

    void clear() {
        this->chunk_list_.clear();
        this->last_chunk_.clear();
    }

    void reserve(size_type count) {
        this->chunk_list_.reserve(count);
    }

    void resize(size_type new_size) {
        this->chunk_list_.resize(new_size);
    }

    void addChunk(const entry_chunk_t & chunk) {
        assert(this->lastChunk().is_full());

        size_type chunk_id = this->chunk_list_.size();
        this->last_chunk_.set_chunk(chunk_id, chunk.entries(), chunk.size(), chunk.capacity());

        this->chunk_list_.emplace_back(chunk);
    }

    void addChunk(entry_chunk_t && chunk) {
        assert(this->lastChunk().is_full());

        size_type chunk_id = this->chunk_list_.size();
        this->last_chunk_.set_chunk(chunk_id, chunk.entries(), chunk.size(), chunk.capacity());

        this->chunk_list_.emplace_back(std::forward<entry_chunk_t>(chunk));
    }

    void addChunk(entry_type * entries, size_type entry_capacity) {
        assert(this->lastChunk().is_full());

        size_type chunk_id = this->chunk_list_.size();
        this->last_chunk_.set_chunk(chunk_id, entries, entry_capacity);

        this->chunk_list_.emplace_back(chunk_id, entries, entry_capacity);
    }

    value_type & operator [] (size_type pos) {
        return this->chunk_list_[pos];
    }

    const value_type & operator [] (size_type pos) const {
        return this->chunk_list_[pos];
    }

    value_type & at(size_type pos) {
        if (pos < this->size())
            return this->chunk_list_[pos];
        else
            throw std::out_of_range("hash_entry_chunk_list<T>::at(pos) out of range.");
    }

    const value_type & at(size_type pos) const {
        if (pos < this->size())
            return this->chunk_list_[pos];
        else
            throw std::out_of_range("hash_entry_chunk_list<T>::at(pos) out of range.");
    }

    void appendEntry(uint32_t chunk_id) {
        assert(chunk_id < this->size());
        this->chunk_list_[chunk_id].increase();
    }

    void appendFreeEntry(entry_type * entry) {
        uint32_t chunk_id = entry->attrib.chunk_id();
        this->appendEntry(chunk_id);
    }

    void removeEntry(uint32_t chunk_id) {
        assert(chunk_id < this->size());
        this->chunk_list_[chunk_id].decrease();
    }

    void removeEntry(entry_type * entry) {
        uint32_t chunk_id = entry->attrib.chunk_id();
        this->removeEntry(chunk_id);
    }

    void reorder(size_type entry_size, size_type total_entry_capacity,
                                       size_type maxEntryChunkSize) {
        if (this->chunk_list_.size() > 0) {
            size_type chunk_id = this->chunk_list_.size() - 1;
            entry_chunk_t & last_chunk = this->chunk_list_[chunk_id];
            size_type ahead_chunk_size = total_entry_capacity - last_chunk.size();
            if (ahead_chunk_size > last_chunk.size()) {
                //
            }
            else {
                //
            }
        }
    }
};

template < typename Key, typename Value,
           std::size_t HashFunc = HashFunc_Default,
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
    typedef std::size_t                     index_type;
    typedef typename Hasher::result_type    hash_code_t;
    typedef BasicDictionary<Key, Value, HashFunc, Alignment, Hasher, KeyEqual, Allocator>
                                            this_type;

    enum entry_type_t {
        kEntryTypeShfit  = 30,
        kIsFreeEntry     = 0UL << kEntryTypeShfit,
        kIsInUseEntry    = 1UL << kEntryTypeShfit,
        kIsReusableEntry = 2UL << kEntryTypeShfit,

        kEntryTypeMask   = 0xC0000000UL,
        kEntryIndexMask  = 0x3FFFFFFFUL
    };

    struct entry_attr_t {
        uint32_t value;

        entry_attr_t(uint32_t value = 0) : value(value) {}
        entry_attr_t(uint32_t _entry_type, uint32_t chunk_id)
            : value(makeAttr(_entry_type, chunk_id)) {}
        ~entry_attr_t() {}

        uint32_t makeAttr(uint32_t _entry_type, uint32_t chunk_id) {
            return ((_entry_type & kEntryTypeMask) | (chunk_id & kEntryIndexMask));
        }

        uint32_t getEntryType() const {
            return (this->value & kEntryTypeMask);
        }

        uint32_t chunk_id() const {
            return (this->value & kEntryIndexMask);
        }

        void setValue(uint32_t _entry_type, uint32_t chunk_id) {
            this->value = this->makeAttr(_entry_type, chunk_id); 
        }

        void setEntryType(uint32_t _entry_type) {
            // Here is a small optimization.
            // this->value = (_entry_type & kEntryAttrMask) | this->chunk_id();
            this->value = _entry_type | this->chunk_id();
        }

        void setChunkId(uint32_t chunk_id) {
            this->value = this->type() | (chunk_id & kEntryIndexMask); 
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

        void setFreeEntry() {
            setEntryType(kIsFreeEntry);
        }

        void setInUseEntry() {
            setEntryType(kIsInUseEntry);
        }

        void setReusableEntry() {
            setEntryType(kIsReusableEntry);
        }
    };

    // hash_entry
    struct hash_entry {
        typedef hash_entry *                    node_pointer;
        typedef hash_entry &                    node_reference;
        typedef const hash_entry *              const_node_pointer;
        typedef const hash_entry &              const_node_reference;
        typedef typename this_type::value_type  value_type;

        hash_entry * next;
        hash_code_t  hash_code;
        entry_attr_t attrib;
        alignas(Alignment)
        value_type   value;
        this_type *  owner;

        hash_entry() : next(nullptr), hash_code(0), attrib(0), owner(nullptr) {}

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

    #include "jstd/hash/hash_iterator_inc.h"

    typedef iterator_t<this_type, entry_type>                iterator;
    typedef const_iterator_t<this_type, entry_type>          const_iterator;

    typedef local_iterator_t<this_type, entry_type>          local_iterator;
    typedef const_local_iterator_t<this_type, entry_type>    const_local_iterator;

    typedef std::pair<iterator, bool>   insert_return_type;

    typedef allocator<entry_type *>     bucket_allocator_type;
    typedef allocator<entry_type>       entry_allocator_type;

    typedef free_list<entry_type>       free_list_t;
    typedef hash_entry_chunk_list<entry_type, allocator_type, entry_allocator_type>
                                        entry_chunk_list_t;

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

    allocator<std::pair<Key, Value>, Alignment, allocator_type::kThrowEx> n_allocator_;

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
    explicit BasicDictionary(size_type initialCapacity = kDefaultInitialCapacity)
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

    size_type _size() const {
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
        return max_bucket_capacity();
    }

    bool valid() const { return (this->buckets() != nullptr); }
    bool empty() const { return (this->size() == 0); }

    size_type bucket_size(size_type index) const {
        assert(index < this->bucket_count());

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
        return (index_type)((size_type)hash_code & this->bucket_mask());
    }

    inline index_type index_of(hash_code_t hash_code, size_type capacity_mask) const {
        return (index_type)((size_type)hash_code & capacity_mask);
    }

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    inline index_type index_of(size_type hash_code) const {
        return (index_type)(hash_code & this->bucket_mask());
    }

    inline index_type index_of(size_type hash_code, size_type capacity_mask) const {
        return (index_type)(hash_code & capacity_mask);
    }
#endif // __amd64__

    inline index_type index_for(hash_code_t hash_code) const {
        return (index_type)((size_type)hash_code & this->bucket_mask());
    }

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

    void destroy() {
        // Destroy the resources.
        if (likely(this->buckets_ != nullptr)) {
            if (likely(this->entries_ != nullptr)) {
                // Destroy all entries.
                this->chunk_list_.destory();
                this->entries_ = nullptr;
            }

            // Free the array of bucket.
            this->free_buckets_impl();
            this->buckets_ = nullptr;
        }

        this->freelist_.clear();

        // Clear settings
        this->bucket_mask_ = 0;
        this->bucket_capacity_ = 0;
        this->entry_size_ = 0;
        this->entry_capacity_ = 0;
    }

    entry_type * get_bucket_head(index_type index) const {
        return this->buckets_[index];
    }

    entry_type * find_first_valid_node() const {
        index_type index = 0;
        entry_type * first = this->buckets_[index];
        if (likely(first == nullptr)) {
            do {
                index++;
                if (likely(index < this->bucket_count())) {
                    first = this->buckets_[index];
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
            index_type index = this->index_for(node_ptr->hash_code);
            index++;
            if (likely(index < this->bucket_count())) {
                do {
                    entry_type * node = get_bucket_head(index);
                    if (likely(node != nullptr)) {
                        node_ptr = node;
                        break;
                    }
                    index++;
                    if (unlikely(index >= this->bucket_count())) {
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
        node = this->next_iterator(node);
        return const_cast<const entry_type *>(node);
    }

    // Init the new entries's status.
    void init_entries_chunk(entry_type * entries, size_type capacity, uint32_t chunk_id) {
        entry_type * entry = entries;
        for (size_type i = 0; i < capacity; i++) {
            entry->attrib.value = chunk_id;
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
            entry->attrib.value = chunk_id;
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
        for (size_type index = 0; index < bucket_capacity_; index++) {
            size_type list_size = 0;
            entry_type * entry = this->buckets_[index];
            while (likely(entry != nullptr)) {
                hash_code_t hash_code = entry->hash_code;
                index_type now_index = this->index_of(hash_code);
                assert(now_index == index);
                assert(entry->attrib.isInUseEntry());

                list_size++;
                entry = entry->next;
            }
            entry_size += list_size;
        }
        return entry_size;
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

                this->chunk_list_.reserve(4);
                this->chunk_list_.addChunk(new_entries, entry_capacity);

                this->freelist_.clear();
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
                    if (likely(new_bucket_capacity < bucket_capacity_)) {
                        new_index = this->index_of(new_index, new_bucket_mask);
                    }

                    new_buckets[new_index] = new_entry;
                }
            }
        }
    }

    void rehash_buckets(size_type new_bucket_capacity) {
        assert(new_bucket_capacity > 0);
        assert(run_time::is_pow2(new_bucket_capacity));
        assert(this->entry_size_ <= this->bucket_capacity_);

        entry_type ** new_buckets = bucket_allocator_.allocate(new_bucket_capacity);
        if (likely(bucket_allocator_.is_ok(new_buckets))) {
            // Initialize the bucket list's data.
            ::memset((void *)new_buckets, 0, new_bucket_capacity * sizeof(entry_type *));

            if (likely(new_bucket_capacity == bucket_capacity_ * 2))
                rehash_all_entries_2x(new_buckets, new_bucket_capacity);
            else
                rehash_all_entries(new_buckets, new_bucket_capacity);

            free_buckets_impl();

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
            size_type new_chunk_capacity = new_entry_capacity - this->entry_capacity_;

            entry_type * new_entries = entry_allocator_.allocate(new_chunk_capacity);
            if (likely(entry_allocator_.is_ok(new_entries))) {
                if (likely(new_bucket_capacity == this->bucket_capacity_ * 2))
                    rehash_all_entries_2x(new_buckets, new_bucket_capacity);
                else
                    rehash_all_entries(new_buckets, new_bucket_capacity);

                this->free_buckets_impl();

                this->buckets_ = new_buckets;
                this->entries_ = new_entries;
                this->bucket_mask_ = new_bucket_capacity - 1;
                this->bucket_capacity_ = new_bucket_capacity;
                // Here, the entry_size_ doesn't change.
                this->entry_capacity_ = new_entry_capacity;

                assert(this->chunk_list_.lastChunk().is_full());

                // Push the new entries pointer to entries list.
                this->chunk_list_.addChunk(new_entries, new_chunk_capacity);

                this->updateVersion();
            }
            else {
                // Failed to allocate new_entries, processing the abnormal exit.
                bucket_allocator_.deallocate(new_buckets, new_bucket_capacity);
            }
        }
    }

    template <bool need_shrink = false>
    void rehash_impl(size_type new_bucket_capacity) {
        assert(new_bucket_capacity > 0);
        assert((new_bucket_capacity & (new_bucket_capacity - 1)) == 0);

        size_type new_entry_capacity = run_time::round_up_to_pow2(this->entry_size_ + 1);
        if (likely(new_bucket_capacity > this->bucket_capacity_)) {
            // Is infalte, do nothing.
        }
        else if (likely(new_bucket_capacity < this->bucket_capacity_)) {
            if (likely(new_entry_capacity > new_bucket_capacity)) {
                new_bucket_capacity = new_entry_capacity;
            }
            if (likely(new_bucket_capacity > this->bucket_capacity_)) {
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

        if (likely(new_entry_capacity <= this->entry_capacity_)) {
            return rehash_buckets(new_bucket_capacity);
        }
        else {
            return rehash_buckets_and_entries(new_entry_capacity, new_bucket_capacity);
        }
    }

    void inflate_entries(size_type delta_size = 1) {
        size_type new_entry_capacity = run_time::round_up_to_pow2(entry_size_ + delta_size);

        // If new entry capacity is too small, exit directly.
        if (likely(new_entry_capacity > this->entry_capacity_)) {
            // Most of the time, we don't need to reallocate buckets list.
            if (likely(new_entry_capacity <= this->bucket_capacity_)) {
                assert(this->freelist_.is_empty());

                size_type new_chunk_capacity = new_entry_capacity - this->entry_capacity_;
                entry_type * new_entries = entry_allocator_.allocate(new_chunk_capacity);
                if (likely(entry_allocator_.is_ok(new_entries))) {
                    this->entries_ = new_entries;
                    this->entry_capacity_ = new_entry_capacity;

                    assert(this->chunk_list_.lastChunk().is_full());

                    // Push the new entries pointer to entries list.
                    this->chunk_list_.addChunk(new_entries, new_chunk_capacity);
                }
            }
            else {
                // The bucket list capacity is full, need to reallocate.
                rehash_buckets_and_entries(new_entry_capacity, new_entry_capacity);
            }
        }
        else {
            // entry_capacity_ = bucket_capacity_ ?
            assert(new_entry_capacity <= this->bucket_capacity_);
        }
    }

#if USE_JAVA_FIND_ENTRY

    JSTD_FORCEINLINE
    entry_type * find_entry(const key_type & key) {
        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_of(hash_code);

        entry_type * first = this->buckets_[index];
        if (likely(first != nullptr)) {
            if (likely(first->hash_code == hash_code &&
                       this->key_is_equal_(key, first->value.first))) {
                return first;
            }

            entry_type * entry = first->next;
            if (likely(entry != nullptr)) {
                do {
                    if (likely(entry->hash_code != hash_code)) {
                        // Do nothing, Continue
                    }
                    else {
                        if (likely(this->key_is_equal_(key, entry->value.first))) {
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
        assert(this->buckets() != nullptr);
        entry_type * first = this->buckets_[index];
        if (likely(first != nullptr)) {
            if (likely(first->hash_code == hash_code &&
                       this->key_is_equal_(key, first->value.first))) {
                return first;
            }

            entry_type * entry = first->next;
            if (likely(entry != nullptr)) {
                do {
                    if (likely(entry->hash_code != hash_code)) {
                        // Do nothing, Continue
                    }
                    else {
                        if (likely(this->key_is_equal_(key, entry->value.first))) {
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
        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_of(hash_code);

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

#endif // USE_JAVA_FIND_ENTRY

    JSTD_FORCEINLINE
    entry_type * find_before(const key_type & key, entry_type *& before, size_type & index) {
        hash_code_t hash_code = this->get_hash(key);
        index = this->index_of(hash_code);

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
    void update_mapped_value(entry_type * entry, const value_type & value) {
        entry->value.second = value.second;
    }

    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, value_type && value) {
        entry->value.second = std::move(value.second);
    }

    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, const n_value_type & value) {
        entry->value.second = value.second;
    }

    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, n_value_type && value) {
        key_type key_tmp = std::move(value.first);
        entry->value.second = std::move(value.second);
    }

    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, const mapped_type & value) {
        entry->value.second = value;
    }

    // Update the existed key's value, maybe by move assignment operator.
    JSTD_FORCEINLINE
    void update_mapped_value(entry_type * entry, mapped_type && value) {
        entry->value.second = std::forward<mapped_type>(value);
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
#if 0
        value_allocator_.destructor(&entry->value.second);
        value_allocator_.construct(&entry->value.second, std::forward<Args>(args)...);
#else
        mapped_type second(std::forward<Args>(args)...);
        std::swap(entry->value.second, second);
#endif
    }

    JSTD_FORCEINLINE
    void update_mapped_value_args(entry_type * entry, const mapped_type & value) {
        entry->value.second = value;
    }

    JSTD_FORCEINLINE
    void update_mapped_value_args(entry_type * entry, mapped_type && value) {
        entry->value.second = std::forward<mapped_type>(value);
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_mapped_value_args(entry_type * entry, Args && ... args) {
        update_mapped_value_args_impl(entry, std::forward<Args>(args)...);
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, const key_type & key,
                                                 const mapped_type & value) {
        if (new_entry->attrib.isFreeEntry()) {
            // Use placement new method to construct value_type.
            this->allocator_.constructor(&new_entry->value, key, value);
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            n_value->first = key;
            n_value->second = value;
        }

        new_entry->attrib.setInUseEntry();
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, const key_type & key,
                                                 mapped_type && value) {
        if (new_entry->attrib.isFreeEntry()) {
            // Use placement new method to construct value_type.
            this->allocator_.constructor(&new_entry->value, key,
                                         std::forward<mapped_type>(value));
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            n_value->first = key;
            n_value->second = std::forward<mapped_type>(value);
        }

        new_entry->attrib.setInUseEntry();
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, key_type && key,
                                                 mapped_type && value) {
        if (new_entry->attrib.isFreeEntry()) {
            // Use placement new method to construct value_type.
            this->n_allocator_.constructor(reinterpret_cast<n_value_type *>(&new_entry->value),
                                           std::forward<key_type>(key),
                                           std::forward<mapped_type>(value));
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            n_value->first = std::forward<key_type>(key);
            n_value->second = std::forward<mapped_type>(value);
        }

        new_entry->attrib.setInUseEntry();
    }

    JSTD_FORCEINLINE
    void construct_value(entry_type * new_entry, n_value_type * value) {
        if (new_entry->attrib.isFreeEntry()) {
            // Use placement new method to construct value_type [by move assignment].
            this->n_allocator_.constructor((n_value_type *)&(new_entry->value),
                                           std::move(value->first),
                                           std::move(value->second));
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            //std::swap(*n_value, *value);
            std::swap(n_value->first, value->first);
            std::swap(n_value->second, value->second);
        }

        new_entry->attrib.setInUseEntry();
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void construct_value_args(entry_type * new_entry, Args && ... args) {
        if (new_entry->attrib.isFreeEntry()) {
            // Use placement new method to construct value_type.
            this->n_allocator_.constructor(reinterpret_cast<n_value_type *>(&new_entry->value),
                                           std::forward<Args>(args)...);
        }
        else {
            assert(new_entry->attrib.isReusableEntry());
            n_value_type * value_tmp = this->n_allocator_.create(std::forward<Args>(args)...);

            n_value_type * n_value = reinterpret_cast<n_value_type *>(&new_entry->value);
            //std::swap(*n_value, *value_tmp);
            std::swap(n_value->first, value_tmp->first);
            std::swap(n_value->second, value_tmp->second);

            this->n_allocator_.destroy(value_tmp);
        }

        new_entry->attrib.setInUseEntry();
    }

    JSTD_FORCEINLINE
    void update_value(entry_type * old_entry, const key_type & key,
                                              const mapped_type & value) {
        value_type value_tmp(key, value);
        std::swap(old_entry->value, value_tmp);
    }

    JSTD_FORCEINLINE
    void update_value(entry_type * old_entry, const key_type & key,
                                              mapped_type && value) {
        value_type value_tmp(key, std::forward<mapped_type>(value));
        std::swap(old_entry->value, value_tmp);
    }

    JSTD_FORCEINLINE
    void update_value(entry_type * old_entry, key_type && key,
                                              mapped_type && value) {
        n_value_type value_tmp(std::forward<key_type>(key),
                               std::forward<mapped_type>(value));
        n_value_type * n_value = reinterpret_cast<n_value_type *>(&old_entry->value);
        std::swap(*n_value, value_tmp);
    }

    JSTD_FORCEINLINE
    void update_value(entry_type * old_entry, const value_type & value) {
        old_entry->value = value;
    }

    JSTD_FORCEINLINE
    void update_value(entry_type * old_entry, value_type && value) {
        old_entry->value = std::forward<value_type>(key);
    }

    JSTD_FORCEINLINE
    void update_value(entry_type * old_entry, n_value_type * value) {
        n_value_type * n_value = reinterpret_cast<n_value_type *>(&old_entry->value);
        std::swap(*n_value, *value);
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_value_args_impl(entry_type * old_entry, const key_type & key, Args && ... args) {
        mapped_type mapped_value(std::forward<Args>(args)...);
        std::swap(old_entry->value.second, mapped_value);
    }

    template <typename ...Args>
    JSTD_FORCEINLINE
    void update_value_args_impl(entry_type * old_entry, key_type && key, Args && ... args) {
        key_type key_tmp = std::move(key);
        mapped_type mapped_value(std::forward<Args>(args)...);
        std::swap(old_entry->value.second, mapped_value);
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
        n_value_type * value_tmp = this->n_allocator_.create(std::forward<Args>(args)...);

        n_value_type * n_value = reinterpret_cast<n_value_type *>(&old_entry->value);
        std::swap(*n_value, *value_tmp);

        this->n_allocator_.destroy(value_tmp);
    }
#endif

    JSTD_FORCEINLINE
    entry_type * got_a_free_entry(hash_code_t hash_code, index_type & index) {
        if (likely(this->freelist_.is_empty())) {
            if (unlikely(this->chunk_list_.lastChunk().is_full())) {
                size_type old_size = size();

                // Inflate the entry size for 1.
                this->inflate_entries(1);

                size_type new_size = count_entries_size();
                assert(new_size == old_size);

                // Recalculate the bucket index.
                index = this->index_of(hash_code);
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
    void insert_to_bucket(entry_type * new_entry, hash_code_t hash_code,
                          index_type index) {
        assert(new_entry != nullptr);

        new_entry->next = this->buckets_[index];
        new_entry->hash_code = hash_code;
        new_entry->owner = this;
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

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(const key_type & key, const mapped_type & value) {
        assert(this->buckets() != nullptr);
        bool inserted;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_of(hash_code);

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

        this->updateVersion();

        return ReturnType(iterator(entry), inserted);
    }

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(const key_type & key, mapped_type && value) {
        assert(this->buckets() != nullptr);
        bool inserted;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_of(hash_code);

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

        this->updateVersion();

        return ReturnType(iterator(entry), inserted);
    }

    template <bool OnlyIfAbsent, typename ReturnType>
    ReturnType insert_unique(key_type && key, mapped_type && value) {
        assert(this->buckets() != nullptr);
        bool inserted;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_of(hash_code);

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
                this->update_mapped_value(entry, std::forward<mapped_type>(value));
            }
            inserted = false;
        }

        this->updateVersion();

        return ReturnType(iterator(entry), inserted);
    }

    template <typename ...Args>
    JSTD_INLINE
    entry_type * emplace_new_entry(hash_code_t hash_code, index_type index, Args && ... args) {
        entry_type * new_entry = this->got_a_free_entry(hash_code, index);
        this->insert_to_bucket(new_entry, hash_code, index);
        this->construct_value_args(new_entry, std::forward<Args>(args)...);
        this->entry_size_++;
        return new_entry;
    }

    JSTD_INLINE
    entry_type * emplace_new_entry_from_value(hash_code_t hash_code,
                                              index_type index, n_value_type * value) {
        entry_type * new_entry = this->got_a_free_entry(hash_code, index);
        this->insert_to_bucket(new_entry, hash_code, index);
        this->construct_value(new_entry, value);
        this->entry_size_++;
        return new_entry;
    }

    template <bool OnlyIfAbsent, typename ReturnType, typename ...Args>
    ReturnType emplace_unique(const key_type & key, Args && ... args) {
        assert(this->buckets() != nullptr);
        bool inserted;

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_of(hash_code);

        entry_type * entry = this->find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            entry = this->emplace_new_entry(hash_code, index,
                                            std::forward<Args>(args)...);
            inserted = true;
        }
        else {
            if (!OnlyIfAbsent) {
                this->update_value_args(entry, std::forward<Args>(args)...);
            }
            inserted = false;
        }

        this->updateVersion();

        return ReturnType(iterator(entry), inserted);
    }

    template <bool OnlyIfAbsent, typename ReturnType, typename ...Args>
    ReturnType emplace_unique(no_key_t nokey, Args && ... args) {
        assert(this->buckets() != nullptr);
        bool inserted;

        n_value_type * value_tmp = this->n_allocator_.create(std::forward<Args>(args)...);
        const key_type & key = this->get_key(value_tmp);

        hash_code_t hash_code = this->get_hash(key);
        index_type index = this->index_of(hash_code);

        entry_type * entry = this->find_entry(key, hash_code, index);
        if (likely(entry == nullptr)) {
            entry = this->emplace_new_entry_from_value(hash_code, index, value_tmp);
            inserted = true;
        }
        else {
            if (!OnlyIfAbsent) {
                this->update_mapped_value(entry, *value_tmp);
            }
            inserted = false;
        }

        this->n_allocator_.destroy(value_tmp);
        this->updateVersion();

        return ReturnType(iterator(entry), inserted);
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
        this->destroy();
        this->initialize(kDefaultInitialCapacity);
    }

    void rehash(size_type bucket_count) {
        assert(bucket_count > 0);
        size_type new_bucket_capacity = this->calc_capacity(bucket_count);
        this->rehash_impl<false>(new_bucket_capacity);
    }

    void reserve(size_type bucket_count) {
        this->rehash(bucket_count);
    }

    void shrink_to_fit(size_type bucket_count = 0) {
        size_type entry_capacity = run_time::round_up_to_pow2(entry_size_);

        // Choose the maximum size of new bucket capacity and now entry capacity.
        bucket_count = (entry_capacity >= bucket_count) ? entry_capacity : bucket_count;

        size_type new_bucket_capacity = this->calc_capacity(bucket_count);
        this->rehash_impl<true>(new_bucket_capacity);
    }

    struct RangeType {
        enum {
            Reorder = 0,
            Realloc = 1
        };
    };

    void rearrange_reorder() {
        this->chunk_list_.reorder(this->entry_size_, this->entry_capacity_, kEntryChunkSize);
    }

    void rearrange_realloc() {
        //
    }

    void rearrange(size_type rangeType) {
        if (rangeType == RangeType::Reorder) {
            rearrange_reorder();
        }
        else {
            // RangeType::Realloc
            rearrange_realloc();
        }
    }

    bool contains(const key_type & key) {
        iterator iter = this->find(key);
        return (iter != this->end());
    }

    iterator find(const key_type & key) {
        if (likely(this->buckets() != nullptr)) {
            entry_type * entry = this->find_entry(key);
            return iterator(entry);
        }

        return iterator(nullptr);   // Error: buckets data is invalid
    }

    const_iterator find(const key_type & key) const {
        if (likely(this->buckets() != nullptr)) {
            entry_type * entry = this->find_entry(key);
            return const_iterator(entry);
        }

        return const_iterator(nullptr);   // Error: buckets data is invalid
    }

    // insert(key, value)

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
        bool is_rvalue = std::is_rvalue_reference<decltype(std::forward<value_type>(value))>::value;
        if (is_rvalue) {
#if 1
            n_value_type * n_value = reinterpret_cast<n_value_type *>(&value);
            return this->insert(std::move(n_value->first), std::move(n_value->second));
#else
            return this->insert(std::move(value.first), std::move(value.second));
#endif
        }
        else {
            return this->insert(value.first, value.second);
        }
    }

    // insert_no_return(key, value)

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
        assert(this->buckets_ != nullptr);

        if (likely(this->entry_size_ != 0)) {
            hash_code_t hash_code = this->get_hash(key);
            size_type index = this->index_of(hash_code);

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

                        assert(entry->attrib.isInUseEntry());
                        uint32_t chunk_id = entry->attrib.chunk_id();
                        entry->attrib.setReusableEntry();

                        this->chunk_list_.removeEntry(chunk_id);
                        this->freelist_.push_front(entry);

                        // destruct the entry->value
                        //
                        // this->allocator_.destructor(&entry->value);
                        //

                        this->entry_size_--;

                        if (this->bucket_capacity_ > kDefaultInitialCapacity &&
                            this->entry_size_ < this->bucket_capacity_ * 3 / 16) {
                            this->rearrange(RangeType::Reorder);
                        }
                        else {
                            this->updateVersion();
                        }

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
