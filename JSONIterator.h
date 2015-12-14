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

#ifndef JSONITERATOR_H_
#define JSONITERATOR_H_

#include <type_traits>
#include <memory>

#include "JSONFwd.h"
#include "JSON.h"
#include "JSONIteratorFwd.h"
#include "JSONObjectsFwd.h"
#include "JSONUtils.h"

namespace JSON {
namespace impl {

template<class T>
struct ObjectIteratorState: public impl::IteratorState<T> {
    static_assert(std::is_same<std::decay_t<T>, IObject>::value,
        "JSON ObjectIteratorState invalid type");
    
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;
    using const_pointer = std::add_const_t<pointer>;
    using const_reference = std::add_const_t<reference>;

    using iterator = Utils::If_t<
        std::is_const<T>::value,
        Object::Container::const_iterator,
        Object::Container::iterator>;
    
    using UPtr = std::unique_ptr<impl::IteratorState<T>>;

    explicit ObjectIteratorState(iterator,const Object&);
    ObjectIteratorState(const ObjectIteratorState&) = default;
    ObjectIteratorState(ObjectIteratorState&&) = default;
    ~ObjectIteratorState() = default;

    reference getObj() noexcept override;
    bool next()  noexcept override;
    operator bool()const noexcept override;

    UPtr clone() const override;
    bool operator==(const IteratorState& i) const override;
    bool compare(const ObjectIteratorState<IObject>& o) const override;
    bool compare(const ObjectIteratorState<const IObject>& o) const override;
       
    const Object& root;
    iterator it;
};

template<class T>
struct ArrayIteratorState: public impl::IteratorState<T> {
    static_assert(std::is_same<std::decay_t<T>, IObject>::value,
        "JSON ArrayIteratorState invalid type");

    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;
    using const_pointer = std::add_const_t<pointer>;
    using const_reference = std::add_const_t<reference>;

    using iterator = Utils::If_t<
        std::is_const<T>::value,
        Array::Container::const_iterator,
        Array::Container::iterator>;

    using UPtr = std::unique_ptr<impl::IteratorState<T>>;

    explicit ArrayIteratorState(iterator it, const Array&);
    ArrayIteratorState(const ArrayIteratorState&) = default;
    ArrayIteratorState(ArrayIteratorState&&) = default;
    ~ArrayIteratorState() = default;

    reference getObj() noexcept override;
    bool next() noexcept override;
    operator bool()const noexcept override;

    UPtr clone() const override;
    bool operator==(const IteratorState& i) const override;
    bool compare(const ArrayIteratorState<IObject>& o) const override;
    bool compare(const ArrayIteratorState<const IObject>& o) const override;
    
    const Array& root;
    iterator it;
};

struct IsAggregateObject: public IObject::IVisitor {
    explicit IsAggregateObject(const IObject& o_) :
            o { o_ }, b { } {
        o.accept(*this);
    }
    operator bool() const noexcept {
        return b;
    }
 private:
    void visit(const Object&o) override {
        b = true;
    }
    void visit(const Array& a) override {
        b = true;
    }
    const IObject& o;
    bool b;
};

}  // namespace impl

template<typename T>
inline Iterator<T>::Iterator() :
        root { } {
}

template<typename T>
inline Iterator<T>::Iterator(reference o, end_t) :
        root { &o } {
}

template<typename T>
inline Iterator<T>::Iterator(reference o) :
        root { &o } {
    objStack.emplace(&o, nullptr);
}

template<typename T>
inline auto Iterator<T>::operator*() -> reference {
    return objStack.top().second ? 
        objStack.top().second->getObj() : *objStack.top().first;
}

template<typename T>
inline Iterator<T>& Iterator<T>::operator++() {
    objStack.top().first->accept(*this);
    return *this;
}

template<typename T>
inline Iterator<T> Iterator<T>::operator++(int) {
    Iterator temp = *this;
    ++*this;
    return temp;
}

template<typename T>
inline auto Iterator<T>::operator->() -> pointer {
    return objStack.top().second ?
        &objStack.top().second->getObj() : objStack.top().first;

}

template <typename T>
inline bool Iterator<T>::operator==(const Iterator& rhs) const {
    return root == rhs.root && objStack == rhs.objStack;
}

template <typename T>
inline bool Iterator<T>::operator!=(const Iterator& rhs) const {
    return !(*this == rhs);
}


template<typename T>
struct ObjTraits;

template<>
struct ObjTraits<Object> {
    using State = impl::ObjectIteratorState<IObject>;
};

template<>
struct ObjTraits<const Object> {
    using State = impl::ObjectIteratorState<const IObject>;
};

template<>
struct ObjTraits<Array> {
    using State = impl::ArrayIteratorState<IObject>;
};

template<>
struct ObjTraits<const Array> {
    using State = impl::ArrayIteratorState<const IObject>;
};


template <typename T>
inline void Iterator<T>::visit(ObjectParam o) {
    //Utils::TypeTester<T, ObjectParam> ot;
    statefulVisit(o);
}

template <typename T>
inline void Iterator<T>::visit(ArrayParam a) {
    //Utils::TypeTester<T, ArrayParam> at;
    statefulVisit(a);
}

template <typename T>
inline void Iterator<T>::visit(TrueParam) {
    next_elem();
}

template <typename T>
inline void Iterator<T>::visit(FalseParam) {
    next_elem();
}

template <typename T>
inline void Iterator<T>::visit(NullParam) {
    next_elem();
}

template <typename T>
inline void Iterator<T>::visit(NumberParam) {
    next_elem();
}

template <typename T>
inline void Iterator<T>::visit(StringParam) {
    next_elem();
}

template <typename T>
inline void Iterator<T>::next_elem() {
    objStack.pop();
    if (!objStack.empty()) {
        objStack.top().first->accept(*this);
    }
}

template<typename T>
template<typename Obj>
inline void Iterator<T>::statefulVisit(Obj& o) {
    using StateT = typename ObjTraits<Obj>::State;
    
    if (!objStack.top().second && !o.getValues().empty()) {
        auto next_state = std::make_unique<StateT>(o.getValues().begin(),o);
        auto& next_obj = next_state->getObj();
        objStack.top().second = std::move(next_state);
        objStack.emplace(&next_obj, nullptr);
    } else {
        if (objStack.top().second && objStack.top().second->next()) {
            auto& obj = objStack.top().second->getObj();
            if (impl::IsAggregateObject(obj))
                objStack.emplace(&obj, nullptr);
        } else {
            next_elem();
        }
    }
}

namespace impl {

template<class T>
inline ObjectIteratorState<T>::ObjectIteratorState(iterator it_, const Object& o) : it{ it_ }, root{ o } {
}

template<class T>
inline auto ObjectIteratorState<T>::getObj() noexcept -> reference {
    return *it->second;
}

template<class T>
inline bool ObjectIteratorState<T>::next() noexcept {
    return ++it != root.getValues().end();
}

template<class T>
inline auto ObjectIteratorState<T>::clone() const -> UPtr {
    return UPtr(std::make_unique<ObjectIteratorState>(*this));
}

template<class T>
inline bool ObjectIteratorState<T>::operator==(const IteratorState& i) const {
    return i.compare(*this);
}

template<class T>
ObjectIteratorState<T>::operator bool()const noexcept {
    return root.getValues().end() != it;
}

template<class T>
inline bool ObjectIteratorState<T>::compare(const ObjectIteratorState<IObject>& o) const {
    return it == o.it;
}

template<class T>
inline bool ObjectIteratorState<T>::compare(const ObjectIteratorState<const IObject>& o) const {
    return it == o.it;
}


template<class T>
inline ArrayIteratorState<T>::ArrayIteratorState(iterator it_, const Array& a) : it{ it_ }, root{ a } {
}

template<class T>
inline auto ArrayIteratorState<T>::getObj() noexcept -> reference {
    return **(it);
}

template<class T>
inline bool ArrayIteratorState<T>::next() noexcept {
    return ++it != root.getValues().end();
}

template<class T>
inline auto ArrayIteratorState<T>::clone() const -> UPtr {
    return UPtr(std::make_unique<ArrayIteratorState>(*this));
}

template<class T>
inline bool ArrayIteratorState<T>::operator==(const IteratorState& i) const {
    return i.compare(*this);
}

template<class T>
ArrayIteratorState<T>::operator bool()const noexcept {
    return root.getValues().end() != it;
}

template<class T>
inline bool ArrayIteratorState<T>::compare(const ArrayIteratorState<IObject>& o) const {
    return it == o.it;
}

template<class T>
inline bool ArrayIteratorState<T>::compare(const ArrayIteratorState<const IObject>& o) const {
    return it == o.it;
}


}  // namespace impl
}  // namespace JSON

#endif  // JSONITERATOR_H_
