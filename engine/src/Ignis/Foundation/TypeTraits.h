#pragma once

#include <type_traits>

namespace Ignis
{
    // --- Type Transformations (Aliases) ---
    
    template <typename T>
    using RemoveReference = std::remove_reference_t<T>;

    template <typename T>
    using RemoveConst = std::remove_const_t<T>;

    template <typename T>
    using RemoveVolatile = std::remove_volatile_t<T>;

    template <typename T>
    using RemoveCv = std::remove_cv_t<T>;

    template <typename T>
    using RemoveCvRef = std::remove_cvref_t<T>;

    template <typename T>
    using RemovePointer = std::remove_pointer_t<T>;

    template <typename T>
    using AddPointer = std::add_pointer_t<T>;

    template <bool B, typename T, typename F>
    using Conditional = std::conditional_t<B, T, F>;

    template <bool B, typename T = void>
    using EnableIf = std::enable_if_t<B, T>;


    // --- Type Properties & Relations (Constants) ---

    template <typename T, typename U>
    inline constexpr bool IsSame = std::is_same_v<T, U>;

    template <typename Base, typename Derived>
    inline constexpr bool IsBaseOf = std::is_base_of_v<Base, Derived>;

    template <typename From, typename To>
    inline constexpr bool IsConvertible = std::is_convertible_v<From, To>;

    template <typename T, typename... Args>
    inline constexpr bool IsConstructible = std::is_constructible_v<T, Args...>;


    // --- Primary Type Categories ---

    template <typename T>
    inline constexpr bool IsPointer = std::is_pointer_v<T>;

    template <typename T>
    inline constexpr bool IsReference = std::is_reference_v<T>;

    template <typename T>
    inline constexpr bool IsFloatingPoint = std::is_floating_point_v<T>;

    template <typename T>
    inline constexpr bool IsIntegral = std::is_integral_v<T>;

    template <typename T>
    inline constexpr bool IsEnum = std::is_enum_v<T>;

    template <typename T>
    inline constexpr bool IsClass = std::is_class_v<T>;
}