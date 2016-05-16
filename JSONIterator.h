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
#include <stack>

#include "JSONFwd.h"
#include "JSON.h"
#include "JSONIteratorFwd.h"
#include "JSONObjectsFwd.h"


namespace JSON {
namespace impl {

struct IsAggregateObject: public IVisitor {
    explicit IsAggregateObject(const IObject& o_) :
            o { o_ }, b { } {}
    operator bool() const noexcept {
        b = false;
        o.accept(*this);
        return b;
    }
 private:
    void visit(const Object&o) const override {
        b = true;
    }
    void visit(const Array& a) const  override {
        b = true;
    }
    const IObject& o;
    mutable bool b;
};

template<class T>
struct IteratorTHelper {
    using type = IObject;
};

template<class T>
struct IteratorTHelper<const T> {
    using type = const IObject;
};

template<class T>
struct IteratorHelper {
    using iterator_t = typename T::Container::iterator;
};

template<class T>
struct IteratorHelper<const T> {
    using iterator_t = typename T::Container::const_iterator;
};

template<class T>
using IteratorTHelper_t = typename IteratorTHelper<T>::type;

template<class T>
using IteratorHelper_t = typename IteratorHelper<T>::iterator_t;

}  // namespace impl

template<typename T, class DereferencePolicy>
class AggregateObjectIterator : public Iterator<impl::IteratorTHelper_t<T>> {
public:
    using base = Iterator<impl::IteratorTHelper_t<T>>;
    using value_type = typename base::value_type;
    using pointer = typename base::pointer;
    using reference = typename base::reference;
    using const_pointer = typename base::const_pointer;
    using const_reference = typename base::const_reference;
    using iterator_t = impl::IteratorHelper_t<T>;
    using iteration_descr_t = std::pair<T*, iterator_t>;

    AggregateObjectIterator(T& aggr_obj, iterator_t i)
        : iteration_descr{ &aggr_obj,i } {}


    Iterator& operator++() override {
        nextElem();
        return *this;
    }

    reference operator*() override {
        return *get();
    }

    const_reference operator*() const override
    {
        return *get();
    }

    pointer operator->() override {
        return get();
    }

    const_pointer operator->() const override {
        return get();
    }

    pointer get() override {
        return DereferencePolicy::dereference(iteration_descr.second);
    }

    const_pointer get() const override {
        return DereferencePolicy::dereference(iteration_descr.second);
    }

    void nextElem() {
        ++iteration_descr.second;
    }

    bool operator==(const Iterator& rhs) const noexcept override {
        if (isValid() && rhs.isValid())
            return get() == rhs.get();
        else if (!isValid() && !rhs.isValid())
            return true;
        else
            return false;

    }

    Iterator* clone() const override {
        return  nullptr;
    }

    bool isValid() const noexcept override {
        return iteration_descr.first->getValues().end()
            != iteration_descr.second;
    }

private:
    iteration_descr_t iteration_descr;
};


struct ObjectIteratorDerefPolicy {
    static IObject* dereference(Object::Container::iterator i) {
        return &(*i->second);
    }
    static const IObject* dereference(Object::Container::const_iterator i) {
        return &(*i->second);
    }
};

struct ArrayIteratorDerefPolicy {
    static IObject* dereference(Array::Container::iterator i) {
        return &(**i);
    }
    static const IObject* dereference(Array::Container::const_iterator i) {
        return &(**i);
    }
};


template<typename T>
using ObjectIterator = AggregateObjectIterator<T, ObjectIteratorDerefPolicy>;

template<typename T>
using ArrayIterator = AggregateObjectIterator<T, ArrayIteratorDerefPolicy>;

template<typename T>
class IndividualIterator : public Iterator<impl::IteratorTHelper_t<T>> {
public:
    using base = Iterator<impl::IteratorTHelper_t<T>>;
    using value_type = typename base::value_type;
    using pointer = typename base::pointer;
    using reference = typename base::reference;
    using const_pointer = typename base::const_pointer;
    using const_reference = typename base::const_reference;

    IndividualIterator() : obj{} {}
    IndividualIterator(reference o) : obj{ &o } {}
    Iterator& operator++() override {
        obj = nullptr;
        return *this;
    }

    reference operator*() override {
        return *obj;
    }

    const_reference operator*() const override
    {
        return *obj;
    }

    pointer operator->() override {
        return obj;
    }

    const_pointer operator->() const override {
        return obj;
    }

    pointer get() override {
        return obj;
    }

    const_pointer get() const override {
        return obj;
    }

    bool operator==(const Iterator& rhs) const noexcept override {
        return get() == rhs.get();
    }

    Iterator* clone() const override {
        return  nullptr;
    }

    bool isValid() const noexcept override {
        return obj != nullptr;
    }
private:
    pointer obj;
};


inline IndividualObject::iterator IndividualObject::begin()
{
    return iterator(std::make_unique<IndividualIterator<IndividualObject>>(*this));
}

inline IndividualObject::iterator IndividualObject::end()
{
    return iterator();
}

inline IndividualObject::const_iterator IndividualObject::begin() const
{
    return const_iterator(std::make_unique<IndividualIterator<const IndividualObject>>(*this));
}

inline IndividualObject::const_iterator IndividualObject::end() const
{
    return const_iterator();
}

inline IndividualObject::const_iterator IndividualObject::cbegin() const
{
    return const_iterator(std::make_unique<IndividualIterator<const IndividualObject>>(*this));
}

inline IndividualObject::const_iterator IndividualObject::cend() const
{
    return const_iterator();
}


inline Object::iterator Object::begin()
{
    return iterator(std::make_unique<ObjectIterator<Object>>(*this, values.begin()));
}

inline Object::iterator Object::end()
{
    return iterator(std::make_unique<ObjectIterator<Object>>(*this, values.end()));
}

inline Object::const_iterator Object::begin() const
{
    return const_iterator(std::make_unique<ObjectIterator<const Object>>(*this, values.begin()));
}

inline Object::const_iterator Object::end() const
{
    return const_iterator(std::make_unique<ObjectIterator<const Object>>(*this, values.end()));
}

inline Object::const_iterator Object::cbegin() const
{
    return const_iterator(std::make_unique<ObjectIterator<const Object>>(*this, values.begin()));
}

inline Object::const_iterator Object::cend() const
{
    return const_iterator(std::make_unique<ObjectIterator<const Object>>(*this, values.end()));
}

inline Array::iterator Array::begin()
{
    return iterator(std::make_unique<ArrayIterator<Array>>(*this, values.begin()));
}

inline Array::iterator Array::end()
{
    return iterator(std::make_unique<ArrayIterator<Array>>(*this, values.end()));
}

inline Array::const_iterator Array::begin() const
{
    return const_iterator(std::make_unique<ArrayIterator<const Array>>(*this, values.begin()));
}

inline Array::const_iterator Array::end() const
{
    return const_iterator(std::make_unique<ArrayIterator<const Array>>(*this, values.end()));
}

inline Array::const_iterator Array::cbegin() const
{
    return const_iterator(std::make_unique<ArrayIterator<const Array>>(*this, values.begin()));
}

inline Array::const_iterator Array::cend() const
{
    return const_iterator(std::make_unique<ArrayIterator<const Array>>(*this, values.end()));
}



template<class T>
class PreOrderIterator : public Iterator<T> {
public:
    template<typename T>
    PreOrderIterator(IteratorRef<T>&& r)
    {
        stack.push(std::move(r));
    }

    Iterator& operator++() override 
    {
        Utils::ForTypes<Object, Array>(*stack.top().get())(
            [&]() {stack.push(stack.top().get()->begin()); },
            [&]() {++stack.top(); }
        );

        while (!stack.empty() && !stack.top().isValid()) {
            stack.pop();
            if(!stack.empty())++stack.top();
        }
        return *this;
    }

    reference operator*() override 
    {
        return *stack.top();
    }
    const_reference operator*() const override 
    {
        return *stack.top();
    }
    pointer operator->() override 
    {
        return stack.top().get();
    }
    const_pointer operator->() const override 
    {
        return stack.top().get();
    }
    
    bool operator==(const Iterator& rhs) const noexcept override 
    {
        return true;
    }

    pointer get() override 
    {
        return stack.empty() ? nullptr : stack.top().get();
    }
    const_pointer get() const override 
    {
        return stack.empty() ? nullptr : stack.top().get();
    }

    Iterator* clone() const override 
    {
        return nullptr;
    }
    bool isValid() const noexcept override 
    {
        return !stack.empty();
    }

private:
    std::stack<IteratorRef<Iterator<T>>> stack;
};

template<typename T>
IteratorRef<Iterator<T>> MakePreorderIterator(IteratorRef<Iterator<T>>&& i)
{
    return IteratorRef<Iterator<T>>(
        std::make_unique<PreOrderIterator<T>>(std::move(i)));
}


}  // namespace JSON

#endif  // JSONITERATOR_H_
