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

#ifndef JSON_H_
#define JSON_H_

#include <exception>
#include <memory>
#include <iostream>  // std::ostream
#include <string>
#include <sstream>
#include <functional>

#include "memory.h"

#include "JSONFwd.h"

// TODO(fecjanky): P2 Revive ObjectEntry and ArrayEntry classes
// TODO(fecjanky): P1 Solve String Type issues
// TODO(fecjanky): P1 Finish allocator support
// TODO(fecjanky): P3 Mutable and Immutable objects
namespace JSON {

namespace impl {

    template<class T>
    struct VisitorIF_Gen<T> {
        using TT = std::decay_t<T>;
        virtual void visit(TT&) {};
        virtual void visit(const TT&) {};
        virtual void visit(TT&) const{};
        virtual void visit(const TT&) const {};
        virtual ~VisitorIF_Gen() = default;
    };

    template<class T,class... Ts>
    struct VisitorIF_Gen<T,Ts...> : public VisitorIF_Gen<Ts...>{
        using VisitorIF_Gen<Ts...>::visit;
        using TT = std::decay_t<T>;
        virtual void visit(TT&) {};
        virtual void visit(const TT&) {};
        virtual void visit(TT&) const {};
        virtual void visit(const TT&) const {};
        virtual ~VisitorIF_Gen() = default;
    };

}  // namespace impl

struct Exception: public std::exception {
};
struct AttributeMissing: JSON::Exception {
};
struct TypeError: public JSON::Exception {
};
struct AggregateTypeError: public JSON::Exception {
};
struct ValueError: public JSON::Exception {
};
struct OutOfRange: public JSON::Exception {
};
struct AttributeNotUnique: public JSON::Exception {
};


class IObjectRef;

struct IObject {
    using Ptr = std::unique_ptr<IObject,std::function<void(void*)>>;
    //using StringType = std::basic_string<char, std::char_traits<char>, estd::poly_alloc_wrapper<char>>;
    using StringType = std::string;
    using OstreamT = std::ostream;

    using iterator = IteratorRef<Iterator<IObject>>;
    using const_iterator = IteratorRef<Iterator<const IObject>>;

    virtual IObjectRef operator[](const StringType& key) = 0;
    virtual const IObject& operator[](const StringType& key) const = 0;

    virtual IObjectRef operator[](size_t index) = 0;
    virtual const IObject& operator[](size_t index) const = 0;

    virtual const StringType& getValue() const = 0;

    operator StringType() const {
        return getValue();
    }

     // Human readable
    virtual void serialize(StringType&& indentation, OstreamT&) const = 0;
     // no Whitespace (compact,transmission syntax)
    virtual void serialize(OstreamT&) const = 0;

    virtual bool operator==(const IObject&) const noexcept = 0;
    bool operator!=(const IObject& o) const noexcept {
        return !(this->operator==(o));
    }

    virtual void accept(IVisitor&) = 0;
    virtual void accept(IVisitor&) const = 0;
    virtual void accept(const IVisitor&) = 0;
    virtual void accept(const IVisitor&) const = 0;


    virtual iterator begin() = 0;
    virtual iterator end() = 0;
    virtual const_iterator begin() const  = 0;
    virtual const_iterator end() const = 0;
    virtual const_iterator cbegin() const = 0;
    virtual const_iterator cend() const = 0;
    
    virtual ~IObject() = default;
};

class IObjectRef {
public:
    using Ptr = IObject::Ptr;
    using StringType = std::string;
    using OstreamT = std::ostream;

    IObjectRef(Ptr& o) : obj{ o } {}
    IObjectRef(const IObjectRef& o) : obj{ o.obj } {}

    IObjectRef& operator=(const IObjectRef& o) = delete;
    ~IObjectRef() = default;

    operator IObject&() {
        check();
        return *obj;
    }

    
    IObjectRef& operator=(IObjectRef&& o) noexcept {
        if (this != &o) {
            obj = std::move(o.obj);
        }
        return *this;
    }

    IObjectRef operator[](const StringType& key) {
        check();
        return (*obj)[key];
    }

    IObjectRef operator[](size_t index) {
        check();
        return (*obj)[index];
    }

    const StringType& getValue() const {
        check();
        return obj->getValue();
    }

    operator StringType() const {
        check();
        return *obj;
    }

    bool operator==(const IObjectRef& rhs)const noexcept{
        return obj && rhs.obj ? *obj == *rhs.obj : obj == rhs.obj;
    }

    bool operator!=(const IObjectRef& rhs)const noexcept {
        return !(*this == rhs);
    }

    void accept(IVisitor& v) {
        check();
        obj->accept(v);        
    }
    
    IObject::iterator begin();
    IObject::iterator end();
    IObject::const_iterator begin()const ;
    IObject::const_iterator end()const;
    IObject::const_iterator cbegin()const;
    IObject::const_iterator cend()const;

private:
    void check() const {
        if (!obj)throw std::runtime_error("IObjectRef nullptr dereference");
    }

    Ptr& obj;
};

inline std::ostream& operator <<(std::ostream& os, const IObject& obj) {
    obj.serialize(std::string { }, os);
    return os;
}

}  // namespace JSON

#endif  // JSON_H_
