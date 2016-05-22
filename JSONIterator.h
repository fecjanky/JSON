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
        else
            return (!isValid() && !rhs.isValid());
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


inline IndividualObject::iterator IndividualObject::begin(estd::poly_alloc_t& a)
{
    return iterator(Utils::MakeUnique<IndividualIterator<IndividualObject>>(std::allocator_arg,a,*this));
}

inline IndividualObject::iterator IndividualObject::end(estd::poly_alloc_t& a)
{
    return iterator();
}

inline IndividualObject::const_iterator IndividualObject::begin(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<IndividualIterator<const IndividualObject>>(std::allocator_arg, a,*this));
}

inline IndividualObject::const_iterator IndividualObject::end(estd::poly_alloc_t& a) const
{
    return const_iterator();
}

inline IndividualObject::const_iterator IndividualObject::cbegin(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<IndividualIterator<const IndividualObject>>(std::allocator_arg, a, *this));
}

inline IndividualObject::const_iterator IndividualObject::cend(estd::poly_alloc_t& a) const
{
    return const_iterator();
}


inline Object::iterator Object::begin(estd::poly_alloc_t& a)
{
    return iterator(Utils::MakeUnique<ObjectIterator<Object>>(std::allocator_arg, a, *this, values.begin()));
}

inline Object::iterator Object::end(estd::poly_alloc_t& a)
{
    return iterator(Utils::MakeUnique<ObjectIterator<Object>>(std::allocator_arg, a, *this, values.end()));
}

inline Object::const_iterator Object::begin(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<ObjectIterator<const Object>>(std::allocator_arg, a, *this, values.begin()));
}

inline Object::const_iterator Object::end(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<ObjectIterator<const Object>>(std::allocator_arg, a, *this, values.end()));
}

inline Object::const_iterator Object::cbegin(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<ObjectIterator<const Object>>(std::allocator_arg, a, *this, values.begin()));
}

inline Object::const_iterator Object::cend(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<ObjectIterator<const Object>>(std::allocator_arg, a, *this, values.end()));
}

inline Array::iterator Array::begin(estd::poly_alloc_t& a)
{
    return iterator(Utils::MakeUnique<ArrayIterator<Array>>(std::allocator_arg, a, *this, values.begin()));
}

inline Array::iterator Array::end(estd::poly_alloc_t& a)
{
    return iterator(Utils::MakeUnique<ArrayIterator<Array>>(std::allocator_arg, a, *this, values.end()));
}

inline Array::const_iterator Array::begin(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<ArrayIterator<const Array>>(std::allocator_arg, a, *this, values.begin()));
}

inline Array::const_iterator Array::end(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<ArrayIterator<const Array>>(std::allocator_arg, a, *this, values.end()));
}

inline Array::const_iterator Array::cbegin(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<ArrayIterator<const Array>>(std::allocator_arg, a, *this, values.begin()));
}

inline Array::const_iterator Array::cend(estd::poly_alloc_t& a) const
{
    return const_iterator(Utils::MakeUnique<ArrayIterator<const Array>>(std::allocator_arg, a, *this, values.end()));
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
        return get() == rhs.get();
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
IteratorRef<Iterator<T>> MakePreorderIterator(std::allocator_arg_t,estd::poly_alloc_t& a,IteratorRef<Iterator<T>>&& i)
{
    return IteratorRef<Iterator<T>>(
        Utils::MakeUnique<PreOrderIterator<T>>(std::allocator_arg, a,std::move(i)));
}

template<typename T>
IteratorRef<Iterator<T>> MakePreorderIterator(IteratorRef<Iterator<T>>&& i)
{
    return MakePreorderIterator(std::allocator_arg, estd::default_poly_allocator::instance(), std::move(i));
}



}  // namespace JSON

#endif  // JSONITERATOR_H_
