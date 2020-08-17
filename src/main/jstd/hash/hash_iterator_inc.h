
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
    node_pointer node_;

public:
    // construct with null pointer
    iterator_t(node_pointer node = nullptr) : node_(node) {}
#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    iterator_t(std::nullptr_t) : node_(nullptr) {}
#endif
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
        owner_type * owner = this->node_->owner;
        this->node_ = static_cast<node_pointer>(owner->next_iterator(this->node_));
        return (*this);
    }

    // post-increment
    this_iter_t & operator ++ (int) {
        this_iter_t tmp(this->node_);
        owner_type * owner = this->node_->owner;
        this->node_ = static_cast<node_pointer>(owner->next_iterator(this->node_));
        return tmp;
    }

    // test for iterator equality
    bool operator == (const this_iter_t & rhs) const noexcept {
        return (this->node_ == rhs.node_);
    }

    // test for iterator inequality
    bool operator != (const this_iter_t & rhs) const noexcept {
        return (this->node_ != rhs.node_);
    }

    node_pointer get_node() {
        return this->node_;
    }

    const node_pointer get_node() const {
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

    node_pointer node_;

public:
    // construct with null pointer
    const_iterator_t(node_pointer node = nullptr) : node_(node) {}
#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    const_iterator_t(std::nullptr_t) : node_(nullptr) {}
#endif
    const_iterator_t(const normal_iterator & src) noexcept
        : node_(const_cast<node_pointer>(src.get_node())) {}

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
        owner_type * owner = this->node_->owner;
        this->node_ = owner->next_iterator(this->node_);
        return (*this);
    }

    // post-increment
    this_iter_t & operator ++ (int) {
        this_iter_t tmp(this->node_);
        owner_type * owner = this->node_->owner;
        this->node_ = owner->next_iterator(this->node_);
        return tmp;
    }

    // test for iterator equality
    bool operator == (const this_iter_t & rhs) const noexcept {
        return (this->node_ == rhs.node_);
    }

    // test for iterator inequality
    bool operator != (const this_iter_t & rhs) const noexcept {
        return (this->node_ != rhs.node_);
    }

    const node_pointer get_node() {
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
    node_pointer node_;

public:
    // construct with null pointer
    local_iterator_t(node_pointer node = nullptr) : node_(node) {}
#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    local_iterator_t(std::nullptr_t) : node_(nullptr) {}
#endif
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
        owner_type * owner = this->node_->owner;
        this->node_ = static_cast<node_pointer>(owner->next_iterator(this->node_));
        return (*this);
    }

    // post-increment
    this_iter_t & operator ++ (int) {
        this_iter_t tmp(this->node_);
        owner_type * owner = this->node_->owner;
        this->node_ = static_cast<node_pointer>(owner->next_iterator(this->node_));
        return tmp;
    }

    // test for iterator equality
    bool operator == (const this_iter_t & rhs) const noexcept {
        return (this->node_ == rhs.node_);
    }

    // test for iterator inequality
    bool operator != (const this_iter_t & rhs) const noexcept {
        return (this->node_ != rhs.node_);
    }

    node_pointer get_node() {
        return this->node_;
    }

    const node_pointer get_node() const {
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

    node_pointer node_;

public:
    // construct with null pointer
    const_local_iterator_t(node_pointer node = nullptr) : node_(node) {}
#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
    const_local_iterator_t(std::nullptr_t) : node_(nullptr) {}
#endif
    const_local_iterator_t(const normal_iterator & src) noexcept
        : node_(const_cast<node_pointer>(src.get_node())) {}

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
        owner_type * owner = this->node_->owner;
        this->node_ = owner->next_iterator(this->node_);
        return (*this);
    }

    // post-increment
    this_iter_t & operator ++ (int) {
        this_iter_t tmp(this->node_);
        owner_type * owner = this->node_->owner;
        this->node_ = owner->next_iterator(this->node_);
        return tmp;
    }

    // test for iterator equality
    bool operator == (const this_iter_t & rhs) const noexcept {
        return (this->node_ == rhs.node_);
    }

    // test for iterator inequality
    bool operator != (const this_iter_t & rhs) const noexcept {
        return (this->node_ != rhs.node_);
    }

    const node_pointer get_node() {
        return const_cast<const node_pointer>(this->node_);
    }
};

#endif // JSTD_HASH_ITERATOR_INC
