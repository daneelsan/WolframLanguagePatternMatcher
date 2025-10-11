#pragma once

#include "TypeTraits.h" // for element_type_t

namespace PatternMatcher
{

/// @brief Get the embed name for a type `T`.
/// @tparam T The type for which to get the embed name.
/// @return The embed name as a string literal.
/// @note If `T` is a pointer or smart pointer, it resolves to the underlying type's embed name.
template <typename T>
inline const char* EmbedName()
{
	using ElemT = element_type_t<T>;
	return EmbedName<ElemT>();
}

}; // namespace PatternMatcher
