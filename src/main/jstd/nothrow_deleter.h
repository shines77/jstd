
#ifndef JSTD_NOTHROW_DELETER_H
#define JSTD_NOTHROW_DELETER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <assert.h>
#include <new>

namespace jstd {

struct normal_deleter {
    template <typename T>
    static void free(T * p) {
        assert(p != nullptr);
        operator delete((void *)p);
    }

    static void destroy(void * p) {
        assert(p != nullptr);
        operator delete(p);
    }

    template <typename T>
    static typename std::enable_if<!std::is_void<T>::value, void>::type
    destroy(T * p) {
        assert(p != nullptr);
        operator delete((void *)p);
    }

    template <typename T>
    static void safe_free(T * p) {
        if (p != nullptr) {
            operator delete((void *)p);
        }
    }

    static void safe_destroy(void * p) {
        if (p != nullptr) {
            operator delete(p);
        }
    }

    template <typename T>
    static typename std::enable_if<!std::is_void<T>::value, void>::type
    safe_destroy(T * p) {
        if (p != nullptr) {
            operator delete((void *)p);
        }
    }
};

struct nothrow_deleter {
    template <typename T>
    static void free(T * p) {
        assert(p != nullptr);
        operator delete((void *)p, std::nothrow);
    }

    static void destroy(void * p) {
        assert(p != nullptr);
        operator delete(p, std::nothrow);
    }

    template <typename T>
    static typename std::enable_if<!std::is_void<T>::value, void>::type
    destroy(T * p) {
        assert(p != nullptr);
        p->~T();
        operator delete((void *)p, std::nothrow);
    }

    template <typename T>
    static void safe_free(T * p) {
        if (p != nullptr) {
            operator delete((void *)p, std::nothrow);
        }
    }

    template <typename T>
    static typename std::enable_if<!std::is_void<T>::value, void>::type
    safe_destroy(void * p) {
        if (p != nullptr) {
            T * pTarget = static_cast<T *>(p);
            assert(pTarget != nullptr);
            if (pTarget != nullptr)
                pTarget->~T();
            operator delete(p, std::nothrow);
        }
    }

    template <typename T>
    static typename std::enable_if<!std::is_void<T>::value, void>::type
    safe_destroy(T * p) {
        if (p != nullptr) {
            p->~T();
            operator delete((void *)p, std::nothrow);
        }
    }
};

struct placement_deleter {
    template <typename T>
    static void free(T * p) {
        assert(p != nullptr);
        operator delete((void *)p, (void *)p);
    }

    static void destroy(void * p) {
        assert(p != nullptr);
        operator delete(p, p);
    }

    template <typename T>
    static typename std::enable_if<!std::is_void<T>::value, void>::type
    destroy(T * p) {
        assert(p != nullptr);
        p->~T();
        operator delete((void *)p, (void *)p);
    }

    template <typename T>
    static void safe_free(T * p) {
        if (p != nullptr) {
            operator delete((void *)p, (void *)p);
        }
    }

    template <typename T>
    static typename std::enable_if<!std::is_void<T>::value, void>::type
    safe_destroy(void * p) {
        if (p != nullptr) {
            T * pTarget = static_cast<T *>(p);
            assert(pTarget != nullptr);
            if (pTarget != nullptr)
                pTarget->~T();
            operator delete(p, p);
        }
    }

    template <typename T>
    static typename std::enable_if<!std::is_void<T>::value, void>::type
    safe_destroy(T * p) {
        if (p != nullptr) {
            p->~T();
            operator delete((void *)p, (void *)p);
        }
    }
};

} // namespace jstd

#endif // JSTD_NOTHROW_DELETER_H
