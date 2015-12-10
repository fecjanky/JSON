#ifndef JSONOBJECTS_H_INCLUDED__
#define JSONOBJECTS_H_INCLUDED__

#include <unordered_map>
#include <vector>
#include <type_traits>
#include <utility>
#include <sstream>
#include <algorithm> //std::equal

#include "JSONLiterals.h"
#include "JSONObjectsFwd.h"
#include "JSONIteratorFwd.h"

namespace JSON {
    
namespace impl {
        
template<typename IF>
Iterator IteratorIFImpl<IF>::begin() const {
    return Iterator(*this);
}

template<typename IF>
Iterator IteratorIFImpl<IF>::end() const {
    return Iterator{*this,Iterator::end};
}

template<typename T>
struct Validator;

template<>
struct Validator<double> {
    const std::string& operator()(
        const std::string& rep) const
    {
        return validate(rep);
    }

            
    std::string&& operator()(std::string&& rep) const
    {
        return validate(std::move(rep));
    }
private:
    template<typename StringType>
    StringType&& validate(StringType&& str) const
    {
        double temp{};
        std::stringstream ss(str);
        ss << std::scientific;
        ss >> temp;
        if (ss.bad() || ss.fail()
            || ss.peek() != std::char_traits<char>::eof())
            throw ValueError();
        return std::forward<StringType>(str);
    }
};

template<>
struct Validator<nullptr_t> {
            
    const std::string& operator()(
        const std::string& rep) const
    {
        return validate(rep);
    }

            
    std::string&& operator()(std::string&& rep) const
    {
        return validate(std::move(rep));
    }
private:
    template<typename StringType>
    StringType&& validate(StringType&& str) const
    {
        if (str
            != std::remove_reference_t<StringType>(
                Literals::value_null()))
            throw ValueError();
        return std::forward<StringType>(str);
    }
};

template<>
struct Validator<bool> {
            
    const std::string& operator()(
        const std::string& rep) const
    {
        return validate(rep);
    }

            
    std::string&& operator()(std::string&& rep) const
    {
        return validate(std::move(rep));
    }
private:
    template<typename StringType>
    StringType&& validate(StringType&& str) const
    {
        if (str
            != std::basic_string < char
            >(Literals::value_false()) && str
            != std::string(Literals::value_true()))
            throw ValueError();
        return std::forward<StringType>(str);
    }
};

std::string to_string(bool b)
{
    if (b)
        return std::string(Literals::value_true());
    else
        return std::string(Literals::value_false());
}

} //namespace impl
        
       

inline Object::Object()
{
}

inline IObject& Object::operator[](const StringType& key)
{
    auto obj = values.find(key);
    if (obj == values.end())
        throw AttributeMissing();
    return *obj->second;
}

inline const IObject& Object::operator[](const StringType& key) const
{
    auto obj = values.find(key);
    if (obj == values.end())
        throw AttributeMissing();
    return *obj->second;
}

inline IObject& Object::operator[](size_t index)
{
    throw TypeError();
}

inline const IObject& Object::operator[](size_t index) const
{
    throw TypeError();
}

inline  const Object::StringType& Object::getValue() const 
{
    throw TypeError();
}

inline const Object::Container& Object::getValues() const
{
    return values;
}


inline void Object::emplace(IObjectPtr && obj)
{
    throw AggregateTypeError();
}

inline void Object::emplace(StringType&& name, IObjectPtr && obj)
{
    if (values.find(name) != values.end())
        throw AttributeNotUnique();
    values.emplace(std::move(name), std::move(obj));
}

inline void Object::serialize(StringType&& indentation, OstreamT& os) const
{
    os << Literals::newline << indentation << Literals::begin_object
        << Literals::newline;
    indentation.push_back(Literals::space);
    indentation.push_back(Literals::space);
    for (auto i = values.begin(); i != values.end();) {
        os << indentation << Literals::quotation_mark << i->first
            << Literals::quotation_mark << Literals::space
            << Literals::name_separator << Literals::space;

        i->second->serialize(std::move(indentation), os);
        ++i;
        if (i != values.end())
            os << Literals::space << Literals::value_separator
            << Literals::newline;
        else
            os << Literals::newline;

    }
    indentation.pop_back();
    indentation.pop_back();
    os << indentation << Literals::end_object;
}

inline void Object::serialize(OstreamT& os) const 
{
    os << Literals::begin_object;
    for (auto i = values.begin(); i != values.end();) {
        os << Literals::quotation_mark << i->first
            << Literals::quotation_mark << Literals::name_separator;
        i->second->serialize(os);
        ++i;
        if (i != values.end())
            os << Literals::value_separator;
    }
    os << Literals::end_object;
}

inline void Object::accept(IVisitor& v)
{
    v.visit(*this);
}

inline void Object::accept(IVisitor& v)const
{
    v.visit(*this);
}

inline bool Object::operator==(const IObject& rhs) const 
{
    return rhs.compare(*this);
}

inline bool Object::compare(const Object& o)  const 
{
    if (o.values.size() != values.size())return false;
    for (auto& x : o.values) {
        auto it = values.find(x.first);
        if (it == values.end())return false;
        if (*x.second != *it->second)return false;
    }
    return true;
}


inline Array::Array()
{
}

inline IObject& Array::operator[](const StringType& key)
{
    throw TypeError();
}

inline const IObject& Array::operator[](const StringType& key) const
{
    throw TypeError();
}

inline IObject& Array::operator[](size_t index)
{
    if (index >= values.size())
        throw OutOfRange();
    return *values[index];
}

inline const IObject& Array::operator[](size_t index) const
{
    if (index >= values.size())
        throw OutOfRange();
    return *values[index];
}

inline const Array::StringType& Array::getValue() const
{
    throw TypeError();
}

inline Array::Container& Array::getValues() 
{
    return values;
}

inline const Array::Container& Array::getValues() const 
{
    return values;
}

inline void Array::emplace(IObjectPtr&& obj) 
{
    values.emplace_back(std::move(obj));
}

inline void Array::emplace(StringType&& name, IObjectPtr&& obj) 
{
    throw AggregateTypeError();
}

inline void Array::serialize(StringType&& indentation, OstreamT& os) const 
{
    os << Literals::begin_array << Literals::space;
    for (auto i = values.begin(); i != values.end();) {
        (*i)->serialize(std::move(indentation), os);
        ++i;
        if (i != values.end())
            os << Literals::space <<
            Literals::value_separator << Literals::space;
    }
    os << Literals::space << Literals::end_array;
}

inline void Array::serialize(OstreamT& os) const 
{
    os << Literals::begin_array;
    for (auto i = values.begin(); i != values.end();) {
        (*i)->serialize(os);
        ++i;
        if (i != values.end()) os << Literals::value_separator;
    }
    os << Literals::end_array;
}

inline void Array::accept(IVisitor& v) 
{
    v.visit(*this);
}

inline void Array::accept(IVisitor& v)const
{
    v.visit(*this);
}

inline bool Array::operator==(const IObject& o) const
{
    return o.compare(*this);
}

inline bool Array::compare(const Array& a)  const
{
    return a.values.size() == values.size() && std::equal(a.values.begin(), a.values.end(), values.begin(), 
        [](const auto& lhs, const auto& rhs) {return *lhs == *rhs; });
}

inline IObject& BuiltIn::operator[](const StringType& key)
{
    throw TypeError();
}

inline const IObject& BuiltIn::operator[](const StringType& key) const
{
    throw TypeError();
}

inline IObject& BuiltIn::operator[](size_t index)
{
    throw TypeError();
}

inline const IObject& BuiltIn::operator[](size_t index) const
{
    throw TypeError();
}

inline const BuiltIn::StringType& BuiltIn::getValue() const
{
    return value;
}

inline void BuiltIn::serialize(StringType&&, OstreamT& os) const
{
    os << value;
}
inline void BuiltIn::serialize(OstreamT& os) const 
{
    os << value;
}

inline BuiltIn::BuiltIn() : value{}
{
}
inline BuiltIn::BuiltIn(const StringType& s) : value{ s }
{
}
    
inline BuiltIn::BuiltIn(StringType&& s) : value{ std::move(s) }
{
}
    
inline bool Bool::getNativeValue() const noexcept
{
    return nativeValue;
}
inline Bool::Bool(bool b) : BuiltIn(impl::to_string(b)), nativeValue{ b }
{
}
inline Bool::Bool(const StringType& s) : BuiltIn(impl::Validator<bool> { }(s))
{
}
inline Bool::Bool(StringType&& s) : BuiltIn(impl::Validator<bool> { }(std::move(s)))
{
}
inline Bool::Bool(const char * s) : Bool(StringType(s))
{
}

inline bool Bool::operator==(const IObject& o) const
{
    return o.compare(*this);
}

inline bool Bool::compare(const Bool& b)  const
{
    return b.nativeValue == nativeValue;
}


inline True::True() : Bool(Literals::value_true())
{
}
    
inline True::True(const StringType& s) : Bool(s)
{
}

inline True::True(StringType&& s) : Bool(std::move(s))
{
}
    
inline const char* True::Literal()
{
    return Literals::value_true();
}

inline void True::accept(IVisitor& v)
{
    v.visit(*this);
}

inline void True::accept(IVisitor& v)const
{
    v.visit(*this);
}


inline False::False() : Bool(Literals::value_false())
{
}
inline False::False(const StringType& s) : Bool(s)
{
}
inline False::False(StringType&& s) : Bool(std::move(s))
{
}
inline const char* False::Literal()
{
    return Literals::value_false();
}

inline void False::accept(IVisitor& v)
{
    v.visit(*this);
}

inline void False::accept(IVisitor& v)const
{
    v.visit(*this);
}


inline String::String() : BuiltIn(StringType{})
{
}
    
inline String::String(const StringType& t) : BuiltIn(t)
{
}
inline String::String(StringType&& t) : BuiltIn(std::move(t))
{
}

inline void String::serialize(StringType&&, OstreamT& os) const
{
    serialize(os);
}

inline void String::serialize(OstreamT& os) const
{
    os << Literals::quotation_mark << this->getValue()
        << Literals::quotation_mark;
}

inline void String::accept(IVisitor& v)
{
    v.visit(*this);
}

inline void String::accept(IVisitor& v)const
{
    v.visit(*this);
}

inline bool String::operator==(const IObject& o) const
{
    return o.compare(*this);
}

inline bool String::compare(const String& s)  const
{
    return s.getValue() == getValue();
}


inline Number::Number(double d) : nativeValue{ d }
{
    std::stringstream ss{};
    ss << std::scientific << d;
    this->value = ss.str();
}
    
inline Number::Number(const StringType& s) : BuiltIn(impl::Validator<double> { }(s))
{
}
inline Number::Number(StringType&& s) : BuiltIn(impl::Validator<double> { }(std::move(s)))
{
}

inline double Number::getNativeValue() const noexcept
{
    return nativeValue;
}

inline void Number::accept(IVisitor& v)
{
    v.visit(*this);
}

inline void Number::accept(IVisitor& v)const
{
    v.visit(*this);
}

inline bool Number::operator==(const IObject& o) const
{
    return o.compare(*this);
}

inline bool Number::compare(const Number& n)  const
{
    return n.nativeValue == nativeValue;
}


inline Null::Null() : BuiltIn(StringType(Literals::value_null()))
{
}
inline Null::Null(const StringType& s) : BuiltIn(impl::Validator<nullptr_t> { }(s))
{
}
inline Null::Null(StringType&& s) : BuiltIn(impl::Validator<nullptr_t> { }(std::move(s)))
{
}
inline nullptr_t Null::getNativeValue() const noexcept
{
    return nullptr;
}
inline const char* Null::Literal()
{
    return Literals::value_null();
}

inline void Null::accept(IVisitor& v)
{
    v.visit(*this);
}

inline void Null::accept(IVisitor& v)const
{
    v.visit(*this);
}

inline bool Null::operator==(const IObject& o) const
{
    return o.compare(*this);
}

inline bool Null::compare(const Null&)  const
{
    return true;
}


} //namespace JSON

#endif //JSONOBJECTS_H_INCLUDED__
