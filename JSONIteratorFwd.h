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

using JSON::Utils::CloneableUniquePtr;

template<class T>
struct ObjectIteratorState;
template<class T>
struct ArrayIteratorState;

template<class T>
struct IteratorState {
    virtual std::unique_ptr<IteratorState> clone() const = 0;
    virtual bool operator==(const IteratorState&) const = 0;
    virtual ~IteratorState() = default;
    virtual bool compare(const ObjectIteratorState<IObject>&) const { return false; }
    virtual bool compare(const ObjectIteratorState<const IObject>&) const { return false; }
    virtual bool compare(const ArrayIteratorState<IObject>&) const { return false; }
    virtual bool compare(const ArrayIteratorState<const IObject>&) const { return false; }
    virtual T& getObj() noexcept = 0;
    virtual bool next() noexcept = 0;
    virtual operator bool()const noexcept = 0;
};

}  // namespace impl

template<typename T>
class Iterator: public IObject::IVisitor {
 public:
    static_assert(std::is_same<std::decay_t<T>, IObject>::value, 
        "JSON iterator invalid type");
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;
    using const_pointer = std::add_const_t<pointer>;
    using const_reference = std::add_const_t<reference>;
    
    template<typename TT>
    struct visitor_param {
        using type = Utils::If_t<std::is_const<T>::value, const TT&, TT&>;
    };

    template<typename TT>
    using visitor_param_t = typename visitor_param<TT>::type;

    using ObjectParam = visitor_param_t<Object>;
    using ArrayParam = visitor_param_t<Array>;
    using TrueParam = visitor_param_t<True>;
    using FalseParam = visitor_param_t<False>;
    using NullParam = visitor_param_t<Null>;
    using NumberParam = visitor_param_t<Number>;
    using StringParam = visitor_param_t<String>;

    enum end_t {
        end
    };

    Iterator(reference& o, end_t);
    explicit Iterator(reference& o);
    Iterator();
    Iterator(const Iterator&) = default;
    Iterator(Iterator&&) = default;
    Iterator& operator=(const Iterator&) = default;
    Iterator& operator=(Iterator&&) = default;

    ~Iterator() = default;

    reference operator*();
    const_reference operator*() const;
    pointer operator->();
    const_pointer operator->() const;

    Iterator& operator++();
    Iterator operator++(int);

    bool operator==(const Iterator& rhs) const;
    bool operator!=(const Iterator& rhs) const;

 private:
    using StatePtr = impl::CloneableUniquePtr<impl::IteratorState<T>>;
    using State = std::pair<pointer, StatePtr>;

    void visit(ObjectParam) override;
    void visit(ArrayParam) override;
    void visit(TrueParam) override;
    void visit(FalseParam) override;
    void visit(NullParam) override;
    void visit(NumberParam) override;
    void visit(StringParam) override;

    void next_elem();
    
    template<typename Obj>
    void statefulVisit(Obj& o);

    const_pointer root;
    std::stack<State> objStack;
};



inline auto IObject::begin() -> iterator {
    return iterator(*this);
}

inline auto IObject::end() -> iterator {
    return iterator(*this, iterator::end);
}

inline auto IObject::begin() const  -> const_iterator {
    return const_iterator(*this);
}

inline auto IObject::end() const  -> const_iterator {
    return const_iterator(*this, const_iterator::end);
}

inline auto IObject::cbegin() const -> const_iterator {
    return begin();
}

inline auto IObject::cend() const -> const_iterator {
    return end();
}

inline IObject::iterator IObjectRef::begin() {
    check();
    obj->begin();
}

inline IObject::iterator IObjectRef::end() {
    check();
    obj->end();
}

inline IObject::const_iterator IObjectRef::begin() const
{
    check();
    return static_cast<const IObject&>(*obj).begin();
}

inline IObject::const_iterator IObjectRef::end() const
{
    check();
    return static_cast<const IObject&>(*obj).end();
}

inline IObject::const_iterator IObjectRef::cbegin() const {
    check();
    return obj->cbegin();
}

inline IObject::const_iterator IObjectRef::cend() const {
    check();
    return obj->cend();
}

}  // namespace JSON
#endif  // JSONITERATORFWD_H_
