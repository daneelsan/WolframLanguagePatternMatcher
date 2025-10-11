#pragma once

#include <memory>
#include <type_traits>

namespace PatternMatcher
{

/* =======================================================================
 *  element_type<t>
 * ======================================================================= */

// This template recursively strips away:
//   - const / volatile qualifiers
//   - references (&, &&)
//   - raw pointer layers (*)
//   - std::shared_ptr layers
//
// The goal: given something like `const std::shared_ptr<MyClass*>&`,
// element_type_t<T> will resolve to just `MyClass`.

// Base case: remove const/volatile and reference from `T`, leave as-is.
template <typename T>
struct element_type
{
	using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

// Specialization: if `T` is a raw pointer (`T*`), peel off the pointer
// and apply `element_type` recursively to the pointee type.
template <typename T>
struct element_type<T*>
{
	using type = typename element_type<T>::type;
};

// Specialization: if `T` is a `shared_ptr<U>`, peel off the smart pointer
// and apply `element_type` recursively to the contained type.
template <typename T>
struct element_type<std::shared_ptr<T>>
{
	using type = typename element_type<T>::type;
};

// Alias to get the unwrapped type directly
template <typename T>
using element_type_t = typename element_type<T>::type;

/* =======================================================================
 *  Pointer Traits
 * ======================================================================= */

template <class T>
struct is_shared_ptr : std::false_type
{
};

template <class T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

/* =======================================================================
 * RTTI
 * ======================================================================= */
/*
 * This is modelled on the LLVM mechanism.
 * https://llvm.org/docs/HowToSetUpLLVMStyleRTTI.html
 * also at Casting.h.
 */
template <typename To, typename From>
struct isa_impl
{
	static inline bool doit(const From& Val) { return To::classof(&Val); }
};

template <typename To, typename From>
inline bool isa(const From& Val)
{
	return isa_impl<To, const From>::doit(Val);
}

} // namespace PatternMatcher
