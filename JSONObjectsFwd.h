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

#ifndef JSONOBJECTSFWD_H_
#define JSONOBJECTSFWD_H_

#include <unordered_map>
#include <utility>  // pair
#include <vector>
#include <cstddef>  // nullptr_t

#include "JSONFwd.h"
#include "JSON.h"

namespace JSON {
namespace impl {

template<class IF>
class IteratorIFImpl: public IF {
 public:
    Iterator begin() const override;
    Iterator end() const override;
};

template<class IF,class Impl>
class CompareImpl : public IF {
public:
    using IF::IF;
    bool operator==(const IObject&) const noexcept override;
};

template<class IF, class Impl>
class VisitorImpl : public IF {
public:
    using IF::IF;
    using IVisitor = typename IF::IVisitor;
    void accept(IVisitor& v) override;
    void accept(IVisitor& v) const override;
};

template<class IF, class Impl>
class IObjectIFImpl : public VisitorImpl<CompareImpl<IF,Impl>,Impl>{
    using VisitorImpl<CompareImpl<IF, Impl>, Impl>::VisitorImpl;
};

}  // namespace impl

struct AggregateObject: public impl::IteratorIFImpl<IObject> {
    using StringType = IObject::StringType;
    using IObjectPtr = IObject::IObjectPtr;
     // for arrays
    virtual void emplace(IObjectPtr && obj) = 0;
     // for objects
    virtual void emplace(StringType&& name, IObjectPtr && obj) = 0;

     // default impl for getValue
    const StringType& getValue() const override;

    virtual ~AggregateObject() = default;
};

struct IndividualObject: public impl::IteratorIFImpl<IObject> {
    using StringType = IObject::StringType;
    using IObjectPtr = IObject::IObjectPtr;
    IObject& operator[](const StringType& key) override;
    const IObject& operator[](const StringType& key) const override;
    IObject& operator[](size_t index) override;
    const IObject& operator[](size_t index) const override;
    void serialize(StringType&& indentation, OstreamT& os) const override;
};

class Object: public impl::IObjectIFImpl<AggregateObject,Object> {
 public:
    using StringType = AggregateObject::StringType;
    using OstreamT = AggregateObject::OstreamT;
    using IObjectPtr = AggregateObject::IObjectPtr;
    using IObject = AggregateObject::IObject;
    using Key = StringType;
    using Value = IObjectPtr;
    using Entry = std::pair<const Key, Value>;
    using Container = std::unordered_map<Key, Value>;
    using Literals = JSON::Literals;

    Object();
    IObject& operator[](const StringType& key) override;
    const IObject& operator[](const StringType& key) const override;
    IObject& operator[](size_t index) override;
    const IObject& operator[](size_t index) const override;
    const Container& getValues() const;
    void emplace(IObjectPtr && obj) override;
    void emplace(StringType&& name, IObjectPtr && obj) override;
    void serialize(StringType&& indentation, OstreamT& os) const override;
    void serialize(OstreamT& os) const override;
    bool compare(const Object&) const noexcept override;

 private:
    Container values;
};

class ObjectEntry: public impl::IObjectIFImpl<IndividualObject, ObjectEntry> {
 public:
    using It = Object::Container::const_iterator;
    explicit ObjectEntry(It, const Object& parent);
    const StringType& getValue() const override;
    void serialize(OstreamT& os) const override;
    bool compare(const ObjectEntry&) const noexcept override;
    const IObject& obj() const noexcept;

    mutable It it;
    const Object& parent;
};

class Array: public impl::IObjectIFImpl<AggregateObject, Array> {
 public:
    using StringType = AggregateObject::StringType;
    using OstreamT = AggregateObject::OstreamT;
    using IObjectPtr = AggregateObject::IObjectPtr;
    using IObject = AggregateObject::IObject;
    using Value = IObjectPtr;
    using Container = std::vector<Value>;
    using Literals = JSON::Literals;

    Array();
    IObject& operator[](const StringType& key) override;
    const IObject& operator[](const StringType& key) const override;
    IObject& operator[](size_t index) override;
    const IObject& operator[](size_t index) const override;
    const StringType& getValue() const override;
    Container& getValues();
    const Container& getValues() const;
    void emplace(IObjectPtr&& obj) override;
    void emplace(StringType&& name, IObjectPtr&& obj) override;
    void serialize(StringType&& indentation, OstreamT& os) const override;
    void serialize(OstreamT& os) const override;
    bool compare(const Array&) const noexcept override;

 private:
    Container values;
};

class ArrayEntry: public impl::IObjectIFImpl<IndividualObject, ArrayEntry> {
 public:
    using It = Array::Container::const_iterator;
    using index_t = It::difference_type;
    ArrayEntry(It elem, const Array& parent);
    const StringType& getValue() const override;
    void serialize(OstreamT& os) const override;
    bool compare(const ArrayEntry&) const noexcept override;
    index_t index() const noexcept;
    const IObject& obj() const noexcept;
    mutable It it;
    const Array& parent;
};

class BuiltIn: public IndividualObject {
 public:
    using StringType = IObject::StringType;
    using OstreamT = IObject::OstreamT;
    using IObjectPtr = IObject::IObjectPtr;
    using Literals = JSON::Literals;

    const StringType& getValue() const override;
    void serialize(StringType&&, OstreamT& os) const override;
    void serialize(OstreamT& os) const override;
 protected:
    BuiltIn();
    explicit BuiltIn(const StringType& s);
    explicit BuiltIn(StringType&& s);
    StringType value;
};

class Bool: public impl::CompareImpl<BuiltIn, Bool> {
 public:
    using Base = impl::CompareImpl<BuiltIn, Bool>;
    using StringType = BuiltIn::StringType;
    using Literals = JSON::Literals;

    bool getNativeValue() const noexcept;

    bool compare(const Bool&) const noexcept override;

 protected:
    explicit Bool(bool b = false);
    explicit Bool(const StringType& s);
    explicit Bool(StringType&& s);
    explicit Bool(const char * s);
 private:
    bool nativeValue;
};

class True: public impl::VisitorImpl<Bool,True> {
 public:
    using Base = impl::VisitorImpl<Bool, True>;
    using StringType = Bool::StringType;

    True();
    explicit True(const StringType& s);
    explicit True(StringType&& s);
    static const char* Literal();
};

class False: public impl::VisitorImpl<Bool, False> {
 public:
    using Base = impl::VisitorImpl<Bool, False>;
    using StringType = Bool::StringType;

    False();
    explicit False(const StringType& s);
    explicit False(StringType&& s);
    static const char* Literal();
};

class String: public impl::IObjectIFImpl<BuiltIn, String> {
 public:
    using Base = impl::IObjectIFImpl<BuiltIn, String>;
    using StringType = BuiltIn::StringType;
    using OstreamT = BuiltIn::OstreamT;

    String();
    explicit String(const StringType& t);
    explicit String(StringType&& t);
    void serialize(StringType&&, OstreamT& os) const override;
    void serialize(OstreamT& os) const override;
    bool compare(const String&) const noexcept override;
};

class Number: public impl::IObjectIFImpl<BuiltIn, Number> {
 public:
    using Base = impl::IObjectIFImpl<BuiltIn, Number>;
    using StringType = BuiltIn::StringType;

    explicit Number(double d = 0.0);
    explicit Number(const StringType& s);
    explicit Number(StringType&& s);
    double getNativeValue() const noexcept;
    bool compare(const Number&) const noexcept override;

 private:
    double nativeValue;
};

class Null: public impl::IObjectIFImpl<BuiltIn, Null> {
 public:
    using Base = impl::IObjectIFImpl<BuiltIn, Null>;
    using StringType = BuiltIn::StringType;

    Null();
    explicit Null(const StringType& s);
    explicit Null(StringType&& s);
    std::nullptr_t getNativeValue() const noexcept;
    static const char* Literal();
    bool compare(const Null&) const noexcept override;
};

template<typename T, typename B = void>
struct IsJSONType : public std::false_type {
};

template<typename T>
struct IsJSONType<
    T,
    std::enable_if_t<std::is_convertible<T*, IObject*>::value>
> : public std::true_type {
};

template<typename T, typename ... Args>
std::enable_if_t<IsJSONType<T>::value, IObjectPtr> Create(Args&&... args) {
    return IObjectPtr(new T(std::forward<Args>(args)...));
}

}  // namespace JSON

#endif  // JSONOBJECTSFWD_H_
