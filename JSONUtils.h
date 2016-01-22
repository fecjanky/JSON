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

#ifndef JSONUTILS_H_
#define JSONUTILS_H_

#include <memory>  // unique_ptr
#include <type_traits>
#include <utility>

namespace JSON {
namespace Utils{

template<typename... T>
struct TypeTester;

template<bool P, typename T1, typename T2>
struct If {
    using type = T2;
};

template<typename T1, typename T2>
struct If<true, T1, T2> {
    using type = T1;
};

template<bool P, typename T1, typename T2>
using If_t = typename If<P, T1, T2>::type;

template<typename Cloneable, typename D = std::default_delete<Cloneable>>
class CloneableUniquePtr : public std::unique_ptr<Cloneable, D> {
 private:
    template<typename T>
    struct PointerCheck {
        static constexpr bool value = std::is_polymorphic<Cloneable>{} ?
            std::is_convertible<T*, Cloneable*>::value :
            std::is_same<Cloneable, T>::value;
    };

    template<typename T, typename Check>
    struct FullCheck {
        static constexpr bool value = PointerCheck<T>::value && Check::value;
    };

    struct NoExceptComparable {
        static constexpr bool value = 
            noexcept(std::declval<Cloneable>().operator==(std::declval<Cloneable>()));
    };

    template<typename DT>
    using NECopyC = std::is_nothrow_copy_constructible<DT>;

    template<typename DT>
    using NECopyA = std::is_nothrow_copy_assignable<DT>;

    template<typename DT>
    using NEMoveC = std::is_nothrow_move_constructible<DT>;

    template<typename DT>
    using NEMoveA = std::is_nothrow_move_assignable<DT>;

    template<typename DT>
    struct DNonRefCheck {
        static constexpr bool value = 
            !std::is_reference<D>::value && std::is_same<DT, D>::value;
    };

    template<typename DT>
    struct DURefCheck {
        static constexpr bool value =
            (std::is_reference<D>::value &&
                std::is_same<
                std::remove_reference_t<DT>,
                std::remove_reference_t<D>
                >::value) ||
            (!std::is_reference<D>::value && 
              std::is_same<std::decay_t<DT>, D>::value);
    };

    template<typename T>
    struct HasClone {
        template<typename U>
        static std::enable_if_t<
            std::is_same<
            std::unique_ptr<T, D>,
            decltype(std::declval<U>().clone())>::value
            , std::true_type
        > test(decltype(std::declval<U>().clone())*);
        template<typename U>
        static std::false_type test(...);
        static constexpr bool value =
            std::is_same<decltype(test<T>(nullptr)), std::true_type>::value;
    };
    static_assert(HasClone<Cloneable>::value, "Type is not Cloneable");

 public:
    using Base = std::unique_ptr<Cloneable, D>;

    explicit CloneableUniquePtr(Cloneable* ptr = nullptr) : Base{ ptr } {}

    constexpr CloneableUniquePtr(decltype(nullptr) n) : Base{ n } {};

    template<typename T, typename = std::enable_if_t<PointerCheck<T>::value>>
    explicit CloneableUniquePtr(T* ptr) : Base{ ptr } {};

    CloneableUniquePtr(const CloneableUniquePtr& other) :
        Base(other ? other->clone() : nullptr) {
    }

    CloneableUniquePtr(CloneableUniquePtr&& other) noexcept(NEMoveC<D>::value) :
        Base(std::unique_ptr<Cloneable, D>(other.release(), std::move(other.get_deleter()))) {
    }

    CloneableUniquePtr& operator=(const CloneableUniquePtr& other) {
        if (other)
            this->Base::operator=(other->clone());
        else
            this->reset();
        return *this;
    }

    CloneableUniquePtr& operator=(CloneableUniquePtr&& other) noexcept(NEMoveA<D>::value) {
        this->Base::operator=(std::unique_ptr<Cloneable>(other.release(), std::move(other.get_deleter())));
        return *this;
    }

    template<typename T, typename DT, typename = std::enable_if_t<FullCheck<T, DURefCheck<DT>>::value>>
    CloneableUniquePtr(T* ptr, DT&& d) noexcept :
    Base{ ptr,std::forward<DT>(d) } {
        static_assert(NEMoveC<D>::value, 
            "Deleter is not no-throw copy constructible");
    };

    template<typename T, typename DT, typename = std::enable_if_t<FullCheck<T, DNonRefCheck<DT>>::value>>
    CloneableUniquePtr(T* ptr, const DT& d) noexcept :
    Base{ ptr,d } {
        static_assert(NECopyC<D>::value, 
            "Deleter is not no-throw moveconstructible");
    };

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr(const CloneableUniquePtr<T, DT>& other) :
        Base(other ? other->clone() : nullptr) {
    }

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr(CloneableUniquePtr<T, DT>&& other)
        noexcept(NEMoveC<D>::value && NEMoveC<DT>::value) :
        Base(std::unique_ptr<T, DT>(other.release(), std::move(other.get_deleter()))) {
    }

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr(const std::unique_ptr<T, DT>& other) :
        Base(other ? other->clone() : nullptr) {
    }

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr(std::unique_ptr<T, DT>&& other) noexcept(NEMoveC<D>::value) :
        Base(std::move(other)) {
    }

    template<typename T, typename DT, 
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr& operator=(const CloneableUniquePtr<T, DT>& other) {
        if (other)
            this->Base::operator=(other->clone());
        else
            this->reset();
        return *this;
    }

    template<typename T, typename DT,
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr& operator=(CloneableUniquePtr<T, DT>&& other)
        noexcept(NEMoveC<DT>::value && NEMoveA<D>::value) {
        this->Base::operator=(std::unique_ptr<T, DT>(other.release(), std::move(other.get_deleter())));
        return *this;
    }

    template<typename T, typename DT,
        typename = std::enable_if_t<PointerCheck<T>::value>>
    CloneableUniquePtr& operator=(std::unique_ptr<T, DT>&& other)
        noexcept(NEMoveA<D>::value) {
        this->Base::operator=(std::move(other));
        return *this;
    }

    bool operator==(const CloneableUniquePtr& rhs) const
        noexcept(NoExceptComparable::value) {
        if (!*this && !rhs)
            return true;
        else if ((!*this && rhs) || (*this && !rhs))
            return false;
        else
            return **this == *rhs;
    }

    bool operator!=(const CloneableUniquePtr& rhs) const
        noexcept(NoExceptComparable::value)
    {
        return !(*this == rhs);
    }
};

}  // namespace Utils
}  // namespace JSON


#endif  // JSONUTILS_H_
