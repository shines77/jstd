
#ifdef  JSTD_HASH_DICTIONARY_HEADER_ONLY_H

#ifndef JSTD_HASH_ITERATOR_INC
#define JSTD_HASH_ITERATOR_INC

template <typename Owner, typename Node>
class iterator_t {
public:
    typedef iterator_t<Owner, Node>         this_iter_t;
    typedef Owner                           owner_type;

    typedef typename Node::node_pointer     node_pointer;
    typedef typename Node::value_type       emlement_type;
    typedef typename Node::value_type       value_type;
    typedef typename std::ptrdiff_t         difference_type;
    typedef value_type *                    pointer;
    typedef value_type &                    reference;

    typedef forward_iterator_tag            iterator_category;

    template <typename, typename>
    friend class const_iterator_t;

protected:
    node_pointer        node_;
    const owner_type *  owner_;

public:
    // construct with null pointer
    iterator_t(const owner_type * owner, node_pointer node = nullptr)
        : node_(node), owner_(owner) {}

#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    iterator_t(const owner_type * owner, std::nullptr_t)
        : node_(nullptr), owner_(owner) {}
#endif

    iterator_t(const iterator_t & src)
        : node_(const_cast<node_pointer>(src.node())),
          owner_(src.owner()) {}

    iterator_t & operator = (const iterator_t & rhs) {
        this->node_ = rhs.node();
        this->owner_ = rhs.owner();
        return *this;
    }

    reference operator * () const {
#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
        assert(this->node_ != std::nullptr_t {});
#endif
        return this->node_->value;
    }

#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    // return pointer to class object
    pointer operator -> () const {
        return std::pointer_traits<pointer>::pointer_to(this->node_->value);
    }
#endif

    // pre-increment
    this_iter_t & operator ++ () {
        const owner_type * owner = this->owner_;
        this->node_ = static_cast<node_pointer>(owner->next_link_entry(this->node_));
        return (*this);
    }

    // post-increment
    this_iter_t & operator ++ (int) {
        this_iter_t copy(*this);
        ++*this;
        return copy;
    }

    // test for iterator equality
    friend bool operator == (const this_iter_t & lhs, const this_iter_t & rhs) noexcept {
        return (lhs.node_ == rhs.node_);
    }

    // test for iterator inequality
    friend bool operator != (const this_iter_t & lhs, const this_iter_t & rhs) noexcept {
        return (lhs.node_ != rhs.node_);
    }

    owner_type * owner() {
        return this->owner_;
    }

    const owner_type * owner() const {
        return const_cast<const owner_type *>(this->owner_);
    }

    node_pointer node()  {
        return this->node_;
    }

    const node_pointer node() const {
        return const_cast<const node_pointer>(this->node_);
    }
};

template <typename Owner, typename Node>
class const_iterator_t {
public:
    typedef const_iterator_t<Owner, Node>   this_iter_t;
    typedef Owner                           owner_type;

    typedef typename Node::node_pointer     node_pointer;
    typedef const typename Node::value_type emlement_type;
    typedef typename Node::value_type       value_type;
    typedef typename std::ptrdiff_t         difference_type;
    typedef const value_type *              pointer;
    typedef const value_type &              reference;

    typedef value_type *                    n_pointer;
    typedef value_type &                    n_reference;

    typedef forward_iterator_tag            iterator_category;

    friend class iterator_t<Owner, Node>;

protected:
    typedef iterator_t<Owner, Node>         normal_iterator;

    node_pointer        node_;
    const owner_type *  owner_;

public:
    // construct with null pointer
    const_iterator_t(const owner_type * owner, node_pointer node = nullptr)
        : node_(node), owner_(owner) {}

#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    const_iterator_t(const owner_type * owner, std::nullptr_t)
        : node_(nullptr), owner_(owner) {}
#endif

    const_iterator_t(const const_iterator_t & src)
        : node_(const_cast<node_pointer>(src.node())),
          owner_(src.owner()) {}

    const_iterator_t(const normal_iterator & src)
        : node_(const_cast<node_pointer>(src.node())),
          owner_(src.owner()) {}

    const_iterator_t & operator = (const const_iterator_t & rhs) {
        this->node_ = rhs.node();
        this->owner_ = rhs.owner();
        return *this;
    }

    const_iterator_t & operator = (const normal_iterator & rhs) {
        this->node_ = rhs.node();
        this->owner_ = rhs.owner();
        return *this;
    }

    reference operator * () const {
#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
        assert(this->node_ != std::nullptr_t {});
#endif
        return this->node_->value;
    }

#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    // return pointer to class object
    pointer operator -> () const {
        return std::pointer_traits<pointer>::pointer_to(this->node_->value);
    }
#endif

    // pre-increment
    this_iter_t & operator ++ () {
        const owner_type * owner = this->owner_;
        this->node_ = owner->next_link_entry(this->node_);
        return (*this);
    }

    // post-increment
    this_iter_t & operator ++ (int) {
        this_iter_t copy(*this);
        ++*this;
        return copy;
    }

    // test for iterator equality
    friend bool operator == (const this_iter_t & lhs, const this_iter_t & rhs) noexcept {
        return (lhs.node_ == rhs.node_);
    }

    // test for iterator inequality
    friend bool operator != (const this_iter_t & lhs, const this_iter_t & rhs) noexcept {
        return (lhs.node_ != rhs.node_);
    }

    owner_type * owner() {
        return this->owner_;
    }

    const owner_type * owner() const {
        return const_cast<const owner_type *>(this->owner_);
    }

    node_pointer node()  {
        return this->node_;
    }

    const node_pointer node() const {
        return const_cast<const node_pointer>(this->node_);
    }
};

template <typename Owner, typename Node>
class local_iterator_t {
public:
    typedef local_iterator_t<Owner, Node>   this_iter_t;
    typedef Owner                           owner_type;

    typedef typename Node::node_pointer     node_pointer;
    typedef typename Node::value_type       emlement_type;
    typedef typename Node::value_type       value_type;
    typedef typename std::ptrdiff_t         difference_type;
    typedef typename Node::node_pointer     pointer;
    typedef typename Node::node_reference   reference;

    typedef forward_iterator_tag            iterator_category;

    template <typename, typename>
    friend class const_local_iterator_t;

protected:
    node_pointer        node_;
    const owner_type *  owner_;

public:
    // construct with null pointer
    local_iterator_t(const owner_type * owner, node_pointer node = nullptr)
        : node_(node), owner_(owner) {}

#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    local_iterator_t(const owner_type * owner, std::nullptr_t)
        : node_(nullptr), owner_(owner) {}
#endif

    local_iterator_t(const local_iterator_t & src)
        : node_(const_cast<node_pointer>(src.node())),
          owner_(src.owner()) {}

    local_iterator_t & operator = (const local_iterator_t & rhs) {
        this->node_ = rhs.node();
        this->owner_ = rhs.owner();
        return *this;
    }

    reference operator * () const {
#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
        assert(this->node_ != std::nullptr_t {});
#endif
        return *(this->node_);
    }

#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    // return pointer to class object
    pointer operator -> () const {
        return std::pointer_traits<pointer>::pointer_to(**this);
    }
#endif

    // pre-increment
    this_iter_t & operator ++ () {
        owner_type * owner = this->owner_;
        this->node_ = static_cast<node_pointer>(owner->next_link_entry(this->node_));
        return (*this);
    }

    // post-increment
    this_iter_t & operator ++ (int) {
        this_iter_t copy(*this);
        ++*this;
        return copy;
    }

    // test for iterator equality
    friend bool operator == (const this_iter_t & lhs, const this_iter_t & rhs) noexcept {
        return (lhs.node_ == rhs.node_);
    }

    // test for iterator inequality
    friend bool operator != (const this_iter_t & lhs, const this_iter_t & rhs) noexcept {
        return (lhs.node_ != rhs.node_);
    }

    owner_type * owner() {
        return this->owner_;
    }

    const owner_type * owner() const {
        return const_cast<const owner_type *>(this->owner_);
    }

    node_pointer node()  {
        return this->node_;
    }

    const node_pointer node() const {
        return const_cast<const node_pointer>(this->node_);
    }
};

template <typename Owner, typename Node>
class const_local_iterator_t {
public:
    typedef const_local_iterator_t<Owner, Node> this_iter_t;
    typedef Owner                               owner_type;

    typedef typename Node::node_pointer         node_pointer;
    typedef const typename Node::value_type     emlement_type;
    typedef typename Node::value_type           value_type;
    typedef typename std::ptrdiff_t             difference_type;
    typedef typename Node::const_node_pointer   pointer;
    typedef typename Node::const_node_reference reference;

    typedef typename Node::node_pointer         n_pointer;
    typedef typename Node::node_reference       n_reference;

    typedef forward_iterator_tag                iterator_category;

    friend class local_iterator_t<Owner, Node>;

protected:
    typedef local_iterator_t<Owner, Node>       normal_iterator;

    node_pointer        node_;
    const owner_type *  owner_;

public:
    // construct with null pointer
    const_local_iterator_t(const owner_type * owner, node_pointer node = nullptr)
        : node_(node), owner_(owner) {}

#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    const_local_iterator_t(const owner_type * owner, std::nullptr_t)
        : node_(nullptr), owner_(owner) {}
#endif

    const_local_iterator_t(const const_local_iterator_t & src)
        : node_(const_cast<node_pointer>(src.node())),
          owner_(src.owner()) {}

    const_local_iterator_t(const normal_iterator & src)
        : node_(const_cast<node_pointer>(src.node())),
          owner_(src.owner()) {}

    const_local_iterator_t & operator = (const const_local_iterator_t & rhs) {
        this->node_ = rhs.node();
        this->owner_ = rhs.owner();
        return *this;
    }

    const_local_iterator_t & operator = (const normal_iterator & rhs) {
        this->node_ = rhs.node();
        this->owner_ = rhs.owner();
        return *this;
    }

    reference operator * () const {
#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
        assert(this->node_ != std::nullptr_t {});
#endif
        return *(this->node_);
    }

#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    // return pointer to class object
    pointer operator -> () const {
        return std::pointer_traits<pointer>::pointer_to(**this);
    }
#endif

    // pre-increment
    this_iter_t & operator ++ () {
        const owner_type * owner = this->owner_;
        this->node_ = owner->next_link_entry(this->node_);
        return (*this);
    }

    // post-increment
    this_iter_t & operator ++ (int) {
        this_iter_t copy(*this);
        ++*this;
        return copy;
    }

    // test for iterator equality
    friend bool operator == (const this_iter_t & lhs, const this_iter_t & rhs) noexcept {
        return (lhs.node_ == rhs.node_);
    }

    // test for iterator inequality
    friend bool operator != (const this_iter_t & lhs, const this_iter_t & rhs) noexcept {
        return (lhs.node_ != rhs.node_);
    }

    owner_type * owner() {
        return this->owner_;
    }

    const owner_type * owner() const {
        return const_cast<const owner_type *>(this->owner_);
    }

    node_pointer node()  {
        return this->node_;
    }

    const node_pointer node() const {
        return const_cast<const node_pointer>(this->node_);
    }
};

#endif // JSTD_HASH_ITERATOR_INC

#endif // JSTD_HASH_DICTIONARY_ONLY_H
