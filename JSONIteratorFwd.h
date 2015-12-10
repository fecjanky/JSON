#ifndef JSONITERATORFWD_H_INCLUDED__
#define JSONITERATORFWD_H_INCLUDED__

#include <stack>

#include "JSONFwd.h"
#include "JSON.h"


namespace JSON {
namespace impl {

template<typename Cloneable, typename D = std::default_delete<Cloneable>>
class CloneableUniquePtr : public std::unique_ptr<Cloneable, D> {
public:
    using Base = std::unique_ptr<Cloneable, D>;
    using Base::Base;
            
    CloneableUniquePtr(const CloneableUniquePtr&  other) : Base(nullptr) 
    {
        if (other)
            this->Base::operator=(other->clone());
    }
            
    CloneableUniquePtr(Base&& other) : Base(std::move(other)) 
    {
    }
            
    CloneableUniquePtr& operator=(const CloneableUniquePtr&  other) 
    {
        if (other)
            this->Base::operator=(other->clone());
        else
            this->reset();
        return *this;
    }
            
    CloneableUniquePtr& operator=(Base&&  other) 
    {
        this->Base::operator=(std::move(other));
        return *this;
    }
            
    bool operator==(const CloneableUniquePtr& rhs)const 
    {
        if (!*this && !rhs)return true;
        else if (!*this && rhs || *this && !rhs) return false;
        else return **this == *rhs;
    }
            
    bool operator!=(const CloneableUniquePtr& rhs)const 
    {
        return !(*this == rhs);
    }

};

struct ObjectIteratorState;
struct ArrayIteratorState;
struct IteratorState {
    virtual std::unique_ptr<IteratorState> clone() const = 0;
    virtual bool operator==(const IteratorState&)const = 0;
    virtual ~IteratorState() = default;
    virtual bool compare(const ObjectIteratorState&)const;
    virtual bool compare(const ArrayIteratorState&)const;
};

} //namespace impl

class Iterator : public IObject::IVisitor {
public:
    enum end_t{
        end
    };

    Iterator(const IObject& o,end_t);
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
    bool operator==(const Iterator& rhs)const;
    bool operator!=(const Iterator& rhs)const;
private:
    using StatePtr = impl::CloneableUniquePtr<impl::IteratorState>;
    using State = std::pair<const IObject*, StatePtr>;

    void visit(const Object&) override;
    void visit(const Array&) override;
    void visit(const True&) override;
    void visit(const False&) override;
    void visit(const Null&) override;
    void visit(const Number&) override;
    void visit(const String&) override;
        
    void next_elem();
    template<typename State, typename Obj>
    void stateful_visit(const Obj&);
    
    const IObject* root;
    std::stack<State> objStack;
};

} //namespace JSON
#endif //JSONITERATORFWD_H_INCLUDED__
