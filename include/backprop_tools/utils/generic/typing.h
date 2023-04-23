#ifndef BACKPROP_TOOLS_UTILS_GENERIC_TYPING_H
#define BACKPROP_TOOLS_UTILS_GENERIC_TYPING_H

namespace backprop_tools::utils::typing {
    template<class T, T v>
    struct integral_constant {
        static constexpr T value = v;
        using value_type = T;
        using type = integral_constant; // using injected-class-name
        constexpr operator value_type() const noexcept { return value; }
        constexpr value_type operator()() const noexcept { return value; } // since c++14
    };
    using false_type = integral_constant<bool, false>;
    using true_type = integral_constant<bool, true>;

    template<class T, class U>
    struct is_same : false_type {};

    template<class T>
    struct is_same<T, T> : true_type {};

    template< class T, class U >
    inline constexpr bool is_same_v = is_same<T, U>::value;

    template<class T>
    struct remove_reference{
        typedef T type;
    };
    template<class T>
    struct remove_reference<T&>{
        typedef T type;
    };

    template< class T > struct remove_pointer                    {typedef T type;};
    template< class T > struct remove_pointer<T*>                {typedef T type;};
    template< class T > struct remove_pointer<T* const>          {typedef T type;};
    template< class T > struct remove_pointer<T* volatile>       {typedef T type;};
    template< class T > struct remove_pointer<T* const volatile> {typedef T type;};

    template<bool B, class T = void>
    struct enable_if {};

    template<class T>
    struct enable_if<true, T> { typedef T type; };

    template< bool B, class T = void >
    using enable_if_t = typename enable_if<B,T>::type;

    template< class T > struct remove_cv                   { typedef T type; };
    template< class T > struct remove_cv<const T>          { typedef T type; };
    template< class T > struct remove_cv<volatile T>       { typedef T type; };
    template< class T > struct remove_cv<const volatile T> { typedef T type; };

    template< class T > struct remove_const                { typedef T type; };
    template< class T > struct remove_const<const T>       { typedef T type; };

    template< class T > struct remove_volatile             { typedef T type; };
    template< class T > struct remove_volatile<volatile T> { typedef T type; };

    template< class T >
    using remove_cv_t = typename remove_cv<T>::type;

    template<bool condition>
    struct warn_if{};

    template<> struct [[deprecated]] warn_if<false>{constexpr warn_if() = default;};

    template<bool B, class T, class F>
    struct conditional { using type = T; };

    template<class T, class F>
    struct conditional<false, T, F> { using type = F; };

}

#define backprop_tools_static_warn(x, ...) ((void) warn_if<x>())

#endif