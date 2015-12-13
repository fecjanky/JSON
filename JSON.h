﻿// Copyright (c) 2015 Ferenc Nandor Janky
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

#include "JSONFwd.h"

  // TODO(fecjanky): Mutable and Immutable objects
  // TODO(fecjanky): Add IObjectWrapper to be able to runtime modify the underlying object type
namespace JSON {

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

using IObjectPtr = std::unique_ptr<IObject>;

struct IObject {
    using IObjectPtr = JSON::IObjectPtr;
    using StringType = std::string;
    using OstreamT = std::ostream;

    virtual IObject& operator[](const StringType& key) = 0;
    virtual const IObject& operator[](const StringType& key) const = 0;

    virtual IObject& operator[](size_t index) = 0;
    virtual const IObject& operator[](size_t index) const = 0;

    virtual const StringType& getValue() const = 0;

    operator StringType() const {
        return getValue();
    }

     // Human readable
    virtual void serialize(StringType&& indentation, OstreamT&) const = 0;
     // no Whitespace (compact,transmission syntax)
    virtual void serialize(OstreamT&) const = 0;

    virtual bool compare(const Object&) const noexcept {
        return false;
    }
    virtual bool compare(const Array&) const noexcept {
        return false;
    }
    virtual bool compare(const Bool&) const noexcept {
        return false;
    }
    virtual bool compare(const Null&) const noexcept {
        return false;
    }
    virtual bool compare(const Number&) const noexcept {
        return false;
    }
    virtual bool compare(const String&) const noexcept {
        return false;
    }
    virtual bool operator==(const IObject&) const noexcept = 0;
    bool operator!=(const IObject& o) const noexcept {
        return !(this->operator==(o));
    }

    struct IVisitor {
        virtual void visit(Object&) {}
        virtual void visit(Array&) {}
        virtual void visit(True&) {}
        virtual void visit(False&) {}
        virtual void visit(Null&) {}
        virtual void visit(Number&) {}
        virtual void visit(String&) {}
        virtual void visit(const Object&) {}
        virtual void visit(const Array&) {}
        virtual void visit(const True&) {}
        virtual void visit(const False&) {}
        virtual void visit(const Null&) {}
        virtual void visit(const Number&) {}
        virtual void visit(const String&) {}
        virtual ~IVisitor() = default;
    };

    virtual void accept(IVisitor&) = 0;
    virtual void accept(IVisitor&) const = 0;

    using iterator = Iterator<IObject>;
    using const_iterator = Iterator<const IObject>;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;


    virtual ~IObject() = default;
};

inline std::ostream& operator <<(std::ostream& os, const IObject& obj) {
    obj.serialize(std::string { }, os);
    return os;
}

}  // namespace JSON

#endif  // JSON_H_
