// Copyright (c) 2015 Ferenc Nandor Janky
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef JSONITERATORFWD_H_
#define JSONITERATORFWD_H_

#include <stack>
#include <utility>  // pair

#include "JSONFwd.h"
#include "JSON.h"
#include "JSONUtils.h"

namespace JSON {
namespace impl {
}  // namespace impl

template<typename T>
class Iterator {
public:
    static_assert(std::is_same<std::decay_t<T>, IObject>::value,
        "JSON iterator invalid type");
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;
    using const_pointer = std::add_const_t<pointer>;
    using const_reference = std::add_const_t<reference>;

    virtual ~Iterator() = default;

    virtual Iterator& operator++() = 0;
    virtual reference operator*() = 0;
    virtual const_reference operator*() const = 0;
    virtual pointer operator->() = 0;
    virtual const_pointer operator->() const = 0;
    virtual bool operator==(const Iterator& rhs) const noexcept = 0;
    virtual pointer get() = 0;
    virtual const_pointer get() const = 0;
    virtual Iterator* clone() const = 0;
    virtual bool isValid() const noexcept = 0;
    operator bool() const noexcept {
        return isValid();
    }

    bool operator!=(const Iterator& rhs) const noexcept {
        return !(*this == rhs);
    }
protected:
    Iterator() = default;
    Iterator(const Iterator&) = default;
    Iterator& operator=(const Iterator&) = default;

};

// TODO(fecjanky): modify IteratorRef to be a regular type (i.e finish cloning impl in Iterator)
template<typename T>
class IteratorRef {
public:
    using value_type = typename T::value_type;
    using pointer = typename T::pointer;
    using reference = typename T::reference;
    using const_pointer = typename T::const_pointer;
    using const_reference = typename T::const_reference;
    template<typename T>
    using UniquePtr = Utils::UniquePtr<T>;

    struct Exception : public std::exception {
        const char * what()const noexcept override {
            return "dereferenced invalid iterator";
        }
    };

    IteratorRef() = default;
    template<typename Impl,typename = std::enable_if_t<std::is_base_of<T,Impl>::value>>
    explicit IteratorRef(UniquePtr<Impl>&& _p) : p{ std::move(_p) } {};
    IteratorRef(const IteratorRef&) = delete;
    IteratorRef(IteratorRef&&) = default;
    IteratorRef& operator=(const IteratorRef&) = delete;
    IteratorRef& operator=(IteratorRef&&) = default;
    ~IteratorRef() = default;

    IteratorRef& operator++() {
        check();
        ++(*p);
        return *this;
    }

    reference operator*() {
        check();
        return **p;
    }

    const_reference operator*() const {
        check();
        return **p;
    }

    pointer operator->() {
        check();
        return *p;
    }

    const_pointer operator->() const {
        check();
        return *p;
    }
    
    operator T&() {
        check();
        return *p;
    }    

    operator const T&() const {
        check();
        return *p;
    }

    bool operator==(const IteratorRef& rhs) const noexcept {
        if (p == rhs.p)
            return true;
        else if (!p || !rhs.p)
            return false;
        else
            return *p == *rhs.p;
    }

    bool operator!=(const IteratorRef& rhs) const noexcept {
        return !(*this == rhs);
    }

    pointer get() {
        check();
        return p->get();
    }

    const_pointer get() const {
        check();
        return p->get();
    }
    
    bool isValid() const noexcept
    {
        return  p ? p->isValid() : false;
    }

    operator bool() const noexcept {
        return isValid();
    }

private:
    void check() const{
        if (!p)throw Exception{};
    }
    UniquePtr<T> p;
};

IObject::iterator IObjectRef::begin(estd::poly_alloc_t& a)
{
    check();
    return obj->begin(a);
}

IObject::iterator IObjectRef::end(estd::poly_alloc_t& a)
{
    check();
    return obj->end(a);
}

IObject::const_iterator IObjectRef::begin(estd::poly_alloc_t& a) const
{
    check();
    return obj->cbegin(a);
}
IObject::const_iterator IObjectRef::end(estd::poly_alloc_t& a) const
{
    check();
    return obj->cend(a);
}

IObject::const_iterator IObjectRef::cbegin(estd::poly_alloc_t& a) const
{
    check();
    return obj->cbegin(a);
}
IObject::const_iterator IObjectRef::cend(estd::poly_alloc_t& a) const
{
    check();
    return obj->cend(a);
}

}  // namespace JSON
#endif  // JSONITERATORFWD_H_
