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

namespace JSON {
namespace impl {

template<typename Cloneable, typename D = std::default_delete<Cloneable>>
class CloneableUniquePtr: public std::unique_ptr<Cloneable, D> {
 public:
    using Base = std::unique_ptr<Cloneable, D>;
    using Base::Base;

    CloneableUniquePtr(const CloneableUniquePtr& other) :
            Base(nullptr) {
        if (other)
            this->Base::operator=(other->clone());
    }

    explicit CloneableUniquePtr(Base&& other) :
            Base(std::move(other)) {
    }

    CloneableUniquePtr& operator=(const CloneableUniquePtr& other) {
        if (other)
            this->Base::operator=(other->clone());
        else
            this->reset();
        return *this;
    }

    CloneableUniquePtr& operator=(Base&& other) {
        this->Base::operator=(std::move(other));
        return *this;
    }

    bool operator==(const CloneableUniquePtr& rhs) const {
        if (this->get() == rhs.get())
            return true;
        else if ((!*this && rhs) || (*this && !rhs))
            return false;
        else
            return **this == *rhs;
    }

    bool operator!=(const CloneableUniquePtr& rhs) const {
        return !(*this == rhs);
    }
};

struct ObjectIteratorState;
struct ArrayIteratorState;
struct IteratorState {
    virtual std::unique_ptr<IteratorState> clone() const = 0;
    virtual bool operator==(const IteratorState&) const = 0;
    virtual ~IteratorState() = default;
    virtual bool compare(const ObjectIteratorState&) const;
    virtual bool compare(const ArrayIteratorState&) const;
};

}  // namespace impl

class Iterator: public IObject::IVisitor {
 public:
    enum end_t {
        end
    };

    Iterator(const IObject& o, end_t);
    explicit Iterator(const IObject& o);
    Iterator();
    Iterator(const Iterator&) = default;
    Iterator(Iterator&&) = default;
    Iterator& operator=(const Iterator&) = default;
    Iterator& operator=(Iterator&&) = default;

    ~Iterator() = default;

    const IObject& operator*();
    Iterator& operator++();
    Iterator operator++(int);

    const IObject* operator->();
    bool operator==(const Iterator& rhs) const;
    bool operator!=(const Iterator& rhs) const;

 private:
    using StatePtr = impl::CloneableUniquePtr<impl::IteratorState>;
    using State = std::pair<const IObject*, StatePtr>;

    void visit(const Object&) override;
    void visit(const ObjectEntry&) override;
    void visit(const Array&) override;
    void visit(const ArrayEntry&) override;
    void visit(const True&) override;
    void visit(const False&) override;
    void visit(const Null&) override;
    void visit(const Number&) override;
    void visit(const String&) override;

    template<typename T>
    struct ObjTraits;

    void next_elem();
    template<typename Obj>
    void statefulVisit(const Obj&);
    template<typename Obj>
    void statefulVisitEntry(const Obj&);

    const IObject* root;
    std::stack<State> objStack;
};

}  // namespace JSON
#endif  // JSONITERATORFWD_H_
