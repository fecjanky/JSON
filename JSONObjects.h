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

#ifndef JSONOBJECTS_H_
#define JSONOBJECTS_H_

#include <unordered_map>
#include <vector>
#include <type_traits>
#include <utility>
#include <sstream>
#include <algorithm>  // std::equal
#include <string>

#include "JSONFwd.h"
#include "JSON.h"
#include "JSONObjectsFwd.h"
#include "JSONIteratorFwd.h"
#include "JSONLiterals.h"

namespace JSON {

namespace impl {

template<typename IF>
Iterator IteratorIFImpl<IF>::begin() const {
    return Iterator(*this);
}

template<typename IF>
Iterator IteratorIFImpl<IF>::end() const {
    return Iterator { *this, Iterator::end };
}

template<class IF, class Impl>
bool CompareImpl<IF,Impl>::operator==(const IObject& o) const noexcept {
    return o.compare(static_cast<const Impl&>(*this));
}

template<class IF, class Impl>
void VisitorImpl<IF,Impl>::accept(IVisitor& v) {
    v.visit(static_cast<Impl&>(*this));
}

template<class IF, class Impl>
void VisitorImpl<IF, Impl>::accept(IVisitor& v) const {
    v.visit(static_cast<const Impl&>(*this));
}

template<typename T>
struct Validator;

template<>
struct Validator<double> {
    const std::string& operator()(const std::string& rep) const {
        return validate(rep);
    }

    std::string&& operator()(std::string&& rep) const {
        return validate(std::move(rep));
    }
 private:
    template<typename StringType>
    StringType&& validate(StringType&& str) const {
        double temp { };
        std::stringstream ss(str);
        ss << std::scientific;
        ss >> temp;
        if (ss.bad() || ss.fail() || ss.peek() != std::char_traits<char>::eof())
            throw ValueError();
        return std::forward<StringType>(str);
    }
};

template<>
struct Validator<std::nullptr_t> {
    const std::string& operator()(const std::string& rep) const {
        return validate(rep);
    }

    std::string&& operator()(std::string&& rep) const {
        return validate(std::move(rep));
    }
 private:
    template<typename StringType>
    StringType&& validate(StringType&& str) const {
        if (str != std::remove_reference_t<StringType>(Literals::ValueNull()))
            throw ValueError();
        return std::forward<StringType>(str);
    }
};

template<>
struct Validator<bool> {
    const std::string& operator()(const std::string& rep) const {
        return validate(rep);
    }

    std::string&& operator()(std::string&& rep) const {
        return validate(std::move(rep));
    }
 private:
    template<typename StringType>
    StringType&& validate(StringType&& str) const {
        if (str != std::basic_string<char>(Literals::ValueFalse())
                && str != std::string(Literals::ValueTrue()))
            throw ValueError();
        return std::forward<StringType>(str);
    }
};

std::string to_string(bool b) {
    if (b)
        return std::string(Literals::ValueTrue());
    else
        return std::string(Literals::ValueFalse());
}

}  // namespace impl

inline const AggregateObject::StringType& AggregateObject::getValue() const {
    throw TypeError { };
}

inline IObject& IndividualObject::operator[](const StringType& key) {
    throw TypeError { };
}

inline const IObject& IndividualObject::operator[](
        const StringType& key) const {
    throw TypeError { };
}

inline IObject& IndividualObject::operator[](size_t index) {
    throw TypeError { };
}

inline const IObject& IndividualObject::operator[](size_t index) const {
    throw TypeError { };
}

inline void IndividualObject::serialize(StringType&& indentation,
        OstreamT& os) const {
    static_cast<const IObject*>(this)->serialize(os);
}

inline Object::Object() {
}

inline IObject& Object::operator[](const StringType& key) {
    auto obj = values.find(key);
    if (obj == values.end())
        throw AttributeMissing();
    return *obj->second;
}

inline const IObject& Object::operator[](const StringType& key) const {
    auto obj = values.find(key);
    if (obj == values.end())
        throw AttributeMissing();
    return *obj->second;
}

inline IObject& Object::operator[](size_t index) {
    throw TypeError();
}

inline const IObject& Object::operator[](size_t index) const {
    throw TypeError();
}

inline const Object::Container& Object::getValues() const {
    return values;
}

inline void Object::emplace(IObjectPtr && obj) {
    throw AggregateTypeError();
}

inline void Object::emplace(StringType&& name, IObjectPtr && obj) {
    if (values.find(name) != values.end())
        throw AttributeNotUnique();
    values.emplace(std::move(name), std::move(obj));
}

inline void Object::serialize(StringType&& indentation, OstreamT& os) const {
    os << Literals::newline << indentation << Literals::beginObject
            << Literals::newline;
    indentation.push_back(Literals::space);
    indentation.push_back(Literals::space);
    for (auto i = values.begin(); i != values.end();) {
        os << indentation << Literals::quotationMark << i->first
                << Literals::quotationMark << Literals::space
                << Literals::nameSeparator << Literals::space;

        i->second->serialize(std::move(indentation), os);
        ++i;
        if (i != values.end())
            os << Literals::space << Literals::valueSeparator
                    << Literals::newline;
        else
            os << Literals::newline;
    }
    indentation.pop_back();
    indentation.pop_back();
    os << indentation << Literals::endObject;
}

inline void Object::serialize(OstreamT& os) const {
    os << Literals::beginObject;
    for (auto i = values.begin(); i != values.end();) {
        os << Literals::quotationMark << i->first << Literals::quotationMark
                << Literals::nameSeparator;
        i->second->serialize(os);
        ++i;
        if (i != values.end())
            os << Literals::valueSeparator;
    }
    os << Literals::endObject;
}

inline bool Object::compare(const Object& o) const noexcept {
    if (o.values.size() != values.size())
        return false;
    for (auto& x : o.values) {
        auto it = values.find(x.first);
        if (it == values.end())
            return false;
        if (*x.second != *it->second)
            return false;
    }
    return true;
}

inline ObjectEntry::ObjectEntry(It it_, const Object& parent_) :
        it { it_ }, parent { parent_ } {
}

inline const ObjectEntry::StringType& ObjectEntry::getValue() const {
    return it->second->getValue();
}

inline void ObjectEntry::serialize(OstreamT& os) const {
    os << Literals::quotationMark << it->first << Literals::quotationMark
            << Literals::nameSeparator;
    it->second->serialize(os);
}


inline bool ObjectEntry::compare(const ObjectEntry& e) const noexcept {
    return e.it->first == it->first && *e.it->second == *it->second;
}

inline const IObject& ObjectEntry::obj() const noexcept {
    return *it->second;
}

inline Array::Array() {
}

inline IObject& Array::operator[](const StringType& key) {
    throw TypeError();
}

inline const IObject& Array::operator[](const StringType& key) const {
    throw TypeError();
}

inline IObject& Array::operator[](size_t index) {
    if (index >= values.size())
        throw OutOfRange();
    return *values[index];
}

inline const IObject& Array::operator[](size_t index) const {
    if (index >= values.size())
        throw OutOfRange();
    return *values[index];
}

inline const Array::StringType& Array::getValue() const {
    throw TypeError();
}

inline Array::Container& Array::getValues() {
    return values;
}

inline const Array::Container& Array::getValues() const {
    return values;
}

inline void Array::emplace(IObjectPtr&& obj) {
    values.emplace_back(std::move(obj));
}

inline void Array::emplace(StringType&& name, IObjectPtr&& obj) {
    throw AggregateTypeError();
}

inline void Array::serialize(StringType&& indentation, OstreamT& os) const {
    os << Literals::beginArray << Literals::space;
    for (auto i = values.begin(); i != values.end();) {
        (*i)->serialize(std::move(indentation), os);
        ++i;
        if (i != values.end())
            os << Literals::space << Literals::valueSeparator
                    << Literals::space;
    }
    os << Literals::space << Literals::endArray;
}

inline void Array::serialize(OstreamT& os) const {
    os << Literals::beginArray;
    for (auto i = values.begin(); i != values.end();) {
        (*i)->serialize(os);
        ++i;
        if (i != values.end())
            os << Literals::valueSeparator;
    }
    os << Literals::endArray;
}

inline bool Array::compare(const Array& a) const noexcept {
    return a.values.size() == values.size()
            && std::equal(a.values.begin(), a.values.end(), values.begin(),
                [](const auto& lhs, const auto& rhs) {return *lhs == *rhs;});
}

inline ArrayEntry::ArrayEntry(It it_, const Array& parent_) :
        it { it_ }, parent { parent_ } {
}

inline const ArrayEntry::StringType& ArrayEntry::getValue() const {
    return (*it)->getValue();
}

inline void ArrayEntry::serialize(OstreamT& os) const {
    (*it)->serialize(os);
}

inline bool ArrayEntry::compare(const ArrayEntry& a) const noexcept {
    return a.index() == index() && a.obj() == obj();
}

inline ArrayEntry::index_t ArrayEntry::index() const noexcept {
    return it - parent.getValues().begin();
}

inline const IObject& ArrayEntry::obj() const noexcept {
    return **it;
}

inline const BuiltIn::StringType& BuiltIn::getValue() const {
    return value;
}

inline void BuiltIn::serialize(StringType&&, OstreamT& os) const {
    os << value;
}
inline void BuiltIn::serialize(OstreamT& os) const {
    os << value;
}

inline BuiltIn::BuiltIn() :
        value { } {
}
inline BuiltIn::BuiltIn(const StringType& s) :
        value { s } {
}

inline BuiltIn::BuiltIn(StringType&& s) :
        value { std::move(s) } {
}

inline bool Bool::getNativeValue() const noexcept {
    return nativeValue;
}

inline Bool::Bool(bool b) :
    Base(impl::to_string(b)), nativeValue { b } {
}

inline Bool::Bool(const StringType& s) :
    Base(impl::Validator<bool> { }(s)) {
}

inline Bool::Bool(StringType&& s) :
    Base(impl::Validator<bool> { }(std::move(s))) {
}

inline Bool::Bool(const char * s) :
        Bool(StringType(s)) {
}

inline bool Bool::compare(const Bool& b) const noexcept {
    return b.nativeValue == nativeValue;
}

inline True::True() :
    Base(Literals::ValueTrue()) {
}

inline True::True(const StringType& s) :
    Base(s) {
}

inline True::True(StringType&& s) :
    Base(std::move(s)) {
}

inline const char* True::Literal() {
    return Literals::ValueTrue();
}

inline False::False() :
    Base(Literals::ValueFalse()) {
}
inline False::False(const StringType& s) :
    Base(s) {
}
inline False::False(StringType&& s) :
    Base(std::move(s)) {
}
inline const char* False::Literal() {
    return Literals::ValueFalse();
}

inline String::String() :
    Base(StringType { }) {
}

inline String::String(const StringType& t) :
    Base(t) {
}
inline String::String(StringType&& t) :
    Base(std::move(t)) {
}

inline void String::serialize(StringType&&, OstreamT& os) const {
    serialize(os);
}

inline void String::serialize(OstreamT& os) const {
    os << Literals::quotationMark << this->getValue()
            << Literals::quotationMark;
}

inline bool String::compare(const String& s) const noexcept {
    return s.getValue() == getValue();
}

inline Number::Number(double d) :
        nativeValue { d } {
    std::stringstream ss { };
    ss << std::scientific << d;
    this->value = ss.str();
}

inline Number::Number(const StringType& s) :
    Base(impl::Validator<double> { }(s)) {
}

inline Number::Number(StringType&& s) :
    Base(impl::Validator<double> { }(std::move(s))) {
}

inline double Number::getNativeValue() const noexcept {
    return nativeValue;
}

inline bool Number::compare(const Number& n) const noexcept {
    return n.nativeValue == nativeValue;
}

inline Null::Null() :
    Base(StringType(Literals::ValueNull())) {
}

inline Null::Null(const StringType& s) :
    Base(impl::Validator<std::nullptr_t> { }(s)) {
}

inline Null::Null(StringType&& s) :
    Base(impl::Validator<nullptr_t> { }(std::move(s))) {
}

inline nullptr_t Null::getNativeValue() const noexcept {
    return nullptr;
}

inline const char* Null::Literal() {
    return Literals::ValueNull();
}

inline bool Null::compare(const Null&) const noexcept {
    return true;
}

}  // namespace JSON

#endif  // JSONOBJECTS_H_
