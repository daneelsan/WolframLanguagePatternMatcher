#pragma once

#include "WolframLibrary.h"
#include "ClassSupport.h"
#include "Expr.h"
#include "TypeTraits.h" // for is_shared_ptr_v

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

/*
 * Deal with embedding object instances into WL expressions.
 *
 * Most of the functionality is present here, but some especially that which deals with calling the WL engine
 * is in the Expr API.
 *
 * This tends to use the name embed for example as in embedName rather than the Kernel usage of Class
 * (which would suggest className). This is because class seems confusing in language C++ which already
 * has classes.
 *
 * Classes opting into this need to provide EmbedName and initializeEmbedMethods.
 * For more discussion with practical examples look in TypeEnvironment.cpp
 *
 */

namespace PatternMatcher
{

/* =======================================================================
 *  Delete Method
 * ======================================================================= */

/// @brief Generic deleter for an object embedded in a WL expression.
/// @tparam TargetT The exact type to delete (e.g., T or std::shared_ptr<T>).
/// @param rawInstance The raw ExprStruct containing the object pointer.
/// @param /*extArg*/ Unused external argument.
/// @note
/// This function is called from the WL engine when an expression passes out of scope.
/// `T` might be a class, such as `TypeEnvironment` or it might be a `shared_ptr` as in `shared_ptr<TypeConstructor>`.
/// In the latter case, the pointer was allocated in `EmbedObjectShared`.
template <typename TargetT>
static void defaultEmbeddedDeleter(ExprStruct rawInstance, void* /*extArg*/)
{
	delete reinterpret_cast<TargetT*>(rawInstance);
}

/// @brief Registers a deleter for an embedded WL object instance.
/// @tparam EmbeddedT The type of the instance for which to set the delete method.
/// @param customDeleter Optional deleter function. If null, a default one is selected based on `EmbeddedT`.
/// @note For `shared_ptr<T>`, this deletes the heap-allocated shared pointer.
///       For raw `T*`, this deletes the object directly.
template <typename EmbeddedT>
static void RegisterEmbeddedDeleter(void (*customDeleter)(ExprStruct, void*) = nullptr)
{
	static_assert(is_shared_ptr_v<EmbeddedT> || std::is_pointer_v<EmbeddedT>, "EmbeddedT must be shared_ptr<T> or T*");

	const char* embedName = EmbedName<EmbeddedT>();
	// Using a conditional type to determine the appropriate delete type
	using DeleteT = std::conditional_t<is_shared_ptr_v<EmbeddedT>,
									   EmbeddedT, // Delete the shared_ptr itself
									   element_type_t<EmbeddedT> // Delete the raw object
									   >;

	if (!customDeleter)
		customDeleter = &defaultEmbeddedDeleter<DeleteT>;

	SetClassRawMethod(embedName, "releaseInstance", reinterpret_cast<void*>(customDeleter));
}

/* =======================================================================
 *  Setup Embedding
 * ======================================================================= */
/*
 * Setup embedding in the WL engine.
 *
 * EmbedName is provided by ClassSupport for each C++ class.
 * initializeEmbedMethods is implemented for each class.
 *
 * T is different for all different classes (ie it is not a parent),
 * so initializedQ will be different for each class.
 * T might be a pointer to a class or it might be a shared_ptr to a class.
 */

/// @brief Setup embedding for a type `T`.
/// @tparam T The type for which embedding is being set up.
/// @param inst The instance of the type to be embedded.
/// @param customDeleter Optional deleter function. If null, a default one is selected based on `EmbeddedT`.
/// @note Called when a type is to be embedded into an expression.
template <typename EmbeddedT>
void SetupEmbed(EmbeddedT inst, void (*customDeleter)(ExprStruct, void*) = nullptr)
{
	static_assert(is_shared_ptr_v<EmbeddedT> || std::is_pointer_v<EmbeddedT>, "EmbeddedT must be shared_ptr<T> or T*");

	static bool initializedQ = false;
	if (!initializedQ)
	{
		initializedQ = true;
		const char* embedName = EmbedName<EmbeddedT>();
		InitializeCompilerClass_Export(embedName);
		RegisterEmbeddedDeleter<EmbeddedT>(customDeleter);
		inst->initializeEmbedMethods(embedName);
		FinalizeCompilerClass_Export(embedName);
	}
}

/* =======================================================================
 *  Object Embedding
 * ======================================================================= */

namespace detail
{
	// Raw-pointer-like embed
	template <typename T>
	Expr EmbedObjectRaw(T* inst, void (*customDeleter)(ExprStruct, void*) = nullptr)
	{
		const char* embedName = EmbedName<T>();
		SetupEmbed<T*>(inst, customDeleter);

		ExprStruct val = reinterpret_cast<ExprStruct>(inst);
		Expr head = Expr::ToExpression(embedName);
		return Expr::embedObjectInstance(val, embedName, head);
	}

	// shared_ptr embed
	template <typename T>
	Expr EmbedObjectShared(std::shared_ptr<T> obj, void (*customDeleter)(ExprStruct, void*) = nullptr)
	{
		const char* embedName = EmbedName<T>();
		SetupEmbed<std::shared_ptr<T>>(obj, customDeleter);

		// This heap-allocated shared_ptr<T> will be freed by deleteMethod<std::shared_ptr<T>>() when the WL expression
		// is released. NOTE: Maybe this is inefficient.
		auto* inst = new std::shared_ptr<T>(obj);
		ExprStruct val = reinterpret_cast<ExprStruct>(inst);
		Expr head = Expr::ToExpression(embedName);
		return Expr::embedObjectInstance(val, embedName, head);
	}

	// Raw pointer unembed
	template <typename T>
	std::optional<T*> UnembedObjectRaw(Expr self)
	{
		if (auto obj = Expr::unembedObjectInstance(self, EmbedName<T>()))
		{
			return reinterpret_cast<T*>(*obj);
		}
		return std::nullopt;
	}

	// shared_ptr unembed
	template <typename T>
	std::optional<std::shared_ptr<T>> UnembedObjectShared(Expr self)
	{
		if (auto obj = Expr::unembedObjectInstance(self, EmbedName<T>()))
		{
			// We stored the shared_ptr<T> itself on the heap in EmbedObjectShared
			auto inst_ptr = reinterpret_cast<std::shared_ptr<T>*>(*obj);
			return *inst_ptr;
		}
		return std::nullopt;
	}
} // namespace detail

/// @brief Embed an object instance into an `Expr`.
/// @details This function automatically determines whether to embed a raw pointer or a shared pointer.
/// @tparam EmbeddedT The type of the instance to embed.
/// @param inst The instance to embed, which can be a raw pointer or a shared pointer.
/// @param customDeleter Optional deleter function. If null, a default one is selected based on `EmbeddedT`.
/// @return An `Expr` containing the embedded instance.
/// @note If `T` is a shared pointer, it will allocate memory for the shared pointer on the heap.
/// If `T` is a raw pointer, it will embed the pointer value directly.
/// The caller is responsible for managing the lifetime of the instance unless a delete method is set.
/// @warning This function does not take ownership of the instance unless a delete method is set via `SetDeleteMethod`.
template <typename EmbeddedT>
Expr EmbedObject(const EmbeddedT& inst, void (*customDeleter)(ExprStruct, void*) = nullptr)
{
	static_assert(is_shared_ptr_v<EmbeddedT> || std::is_pointer_v<EmbeddedT>, "EmbeddedT must be shared_ptr<T> or T*");

	using BaseT = element_type_t<EmbeddedT>;
	if constexpr (is_shared_ptr_v<EmbeddedT>)
	{
		return detail::EmbedObjectShared<BaseT>(inst, customDeleter);
	}
	else
	{
		return detail::EmbedObjectRaw<BaseT>(inst, customDeleter);
	}
}

/// @brief Unembed an object from an `Expr`, auto-detecting raw or shared pointer.
/// @tparam EmbeddedT The type of the object to unembed. Can be `U*` or `std::shared_ptr<U>`.
/// @param self The expression containing the embedded instance.
/// @return An optional containing the unembedded instance, or `nullopt` if it fails.
template <typename EmbeddedT>
std::optional<EmbeddedT> UnembedObject(Expr self)
{
	static_assert(is_shared_ptr_v<EmbeddedT> || std::is_pointer_v<EmbeddedT>, "EmbeddedT must be shared_ptr<T> or T*");

	using BaseT = element_type_t<EmbeddedT>;
	if constexpr (is_shared_ptr_v<EmbeddedT>)
	{
		return detail::UnembedObjectShared<BaseT>(self);
	}
	else
	{
		return detail::UnembedObjectRaw<BaseT>(self);
	}
}

/* =======================================================================
 *  Error Helpers
 * ======================================================================= */

/// @brief  Method error argument count for a specific type.
/// @tparam T The type for which the method was called.
/// @param exp The expected number of arguments.
/// @param rec The received number of arguments.
/// @return An Expr representing the error exception.
template <typename T>
Expr methodErrorArgumentCount(mint exp, mint rec)
{
	std::string msg = EmbedName<T>();
	msg += " method received " + std::to_string(rec);
	msg += (rec == 1) ? " argument but expected " : " arguments but expected ";
	msg += std::to_string(exp) + ".";
	return Expr::throwError(msg);
}

template <typename T>
Expr methodErrorObject(Expr arg, const char* expName, const char* methodName)
{
	std::string msg = EmbedName<T>();
	msg.append(" method ");
	msg.append(methodName);
	msg.append(" expected ");
	msg.append(expName);
	msg.append(".");
	return Expr::throwError(msg);
}

/// @brief  Method error failure for a specific type.
/// @details This creates an error exception indicating that a method for the specified type failed.
/// @tparam T The type for which the method failed.
/// @return An Expr representing the error exception.
template <typename T>
Expr methodErrorFailure()
{
	std::string msg = EmbedName<T>();
	msg += " method failed.";
	return Expr::throwError(msg);
}

/* =======================================================================
 *  MethodTraits
 * ======================================================================= */
// Trait to detect method signature
template <typename>
struct MethodTraits;

// Specialization for raw pointer methods
template <typename R, typename T, typename... Args>
struct MethodTraits<R (*)(T*, Args...)>
{
	using ReturnType = R;
	using InstanceType = T*;
	using ArgsTuple = std::tuple<Args...>;
	static constexpr size_t ArgCount = sizeof...(Args);
	static constexpr bool NeedsUnembed = true;
};

// Specialization for shared_ptr methods
template <typename R, typename T, typename... Args>
struct MethodTraits<R (*)(std::shared_ptr<T>, Args...)>
{
	using ReturnType = R;
	using InstanceType = std::shared_ptr<T>;
	using ArgsTuple = std::tuple<Args...>;
	static constexpr size_t ArgCount = sizeof...(Args);
	static constexpr bool NeedsUnembed = true;
};

// Specialization for Expr-first methods (instance passed as Expr, needs unembedding)
template <typename R, typename... Args>
struct MethodTraits<R (*)(Expr, Args...)>
{
	using ReturnType = R;
	using InstanceType = Expr; // Instance comes as Expr
	using ArgsTuple = std::tuple<Args...>;
	static constexpr size_t ArgCount = sizeof...(Args);
	static constexpr bool NeedsUnembed = false; // Don't unembed, pass Expr directly
};

/* =======================================================================
 *  Method Dispatcher
 * ======================================================================= */
/// @brief Dispatcher to call an embedded object method from WL.
/// @tparam EmbeddedT The type of the embedded object (e.g., `T*` or `std::shared_ptr<T>`).
/// @tparam method The method to call, specified as a function pointer.
/// @param raw_expr The raw `ExprStruct` containing the expression with the method call.
/// @return An `ExprStruct` representing the result of the method call, or an error.
template <typename EmbeddedT, auto method>
ExprStruct embeddedObjectMethod(ExprStruct raw_expr)
{
	using Traits = MethodTraits<decltype(method)>;
	using BaseT = element_type_t<EmbeddedT>;

	Expr expr(raw_expr, true);
	constexpr size_t expectedArgs = Traits::ArgCount;

	// Traditional method: needs to unembed instance first
	if (expr.length() != expectedArgs + 1)
	{
		return methodErrorArgumentCount<BaseT>(expectedArgs, expr.length());
	}

	// Check if this is an Expr-first method (instance passed as Expr)
	if constexpr (std::is_same_v<typename Traits::InstanceType, Expr>)
	{
		// Method signature: Expr (*)(Expr, Args...)
		// The first Expr contains the embedded instance. No need to unembed here.
		Expr selfExpr = expr.head();
		// Pass selfExpr as-is, plus other arguments
		return invokeExprFirstMethod<method>(expr, selfExpr, std::make_index_sequence<expectedArgs> {});
	}
	else
	{
		// Method signature: R (*)(T*, Args...) or R (*)(std::shared_ptr<T>, Args...)
		// Need to unembed the instance first.
		Expr selfExpr = expr.head();
		std::optional<EmbeddedT> selfOpt = UnembedObject<EmbeddedT>(selfExpr);
		if (!selfOpt)
		{
			return methodErrorFailure<BaseT>();
		}

		return invokeMethod<EmbeddedT, method>(expr, *selfOpt, std::make_index_sequence<expectedArgs> {});
	}
}

// Helper to extract and convert arguments
template <typename T>
std::optional<T> extractArg(Expr expr, size_t index)
{
	return expr.part(index + 2).as<T>(); // +2 because part(1) is head, part(2) is first arg
}

/// @brief Invoke a method with the first argument as an Expr (instance passed as Expr)
/// @tparam method The method to invoke.
/// @tparam ...Is Index sequence for unpacking arguments.
/// @param expr The expression containing the method call.
/// @param selfExpr The Expr representing the instance.
/// @param index_sequence The index sequence for unpacking arguments.
/// @return An ExprStruct representing the result of the method call.
template <auto method, size_t... Is>
ExprStruct invokeExprFirstMethod(Expr expr, Expr selfExpr, std::index_sequence<Is...>)
{
	using Traits = MethodTraits<decltype(method)>;
	using ArgsTuple = typename Traits::ArgsTuple;
	using BaseT = element_type_t<typename Traits::InstanceType>;

	// Extract remaining arguments (not the first Expr which is selfExpr)
	auto argsTuple = std::make_tuple(extractArg<std::tuple_element_t<Is, ArgsTuple>>(expr, Is)...);

	// Check all arguments are valid
	if (!(std::get<Is>(argsTuple).has_value() && ...))
	{
		return methodErrorFailure<BaseT>();
	}

	// Call method with selfExpr as first arg, then unpacked additional args
	return method(selfExpr, *std::get<Is>(argsTuple)...);
}

/// @brief Invoke a method with the instance as the first parameter
/// @tparam EmbeddedT The type of the embedded object (e.g., `T*` or `std::shared_ptr<T>`).
/// @tparam method The method to invoke.
/// @tparam ...Is Index sequence for unpacking arguments.
/// @param expr The expression containing the method call.
/// @param self The embedded object instance.
/// @param index_sequence The index sequence for unpacking arguments.
/// @return An ExprStruct representing the result of the method call.
template <typename EmbeddedT, auto method, size_t... Is>
ExprStruct invokeMethod(Expr expr, EmbeddedT& self, std::index_sequence<Is...>)
{
	using Traits = MethodTraits<decltype(method)>;
	using BaseT = element_type_t<EmbeddedT>;
	using ArgsTuple = typename Traits::ArgsTuple;

	// Extract all arguments
	auto argsTuple = std::make_tuple(extractArg<std::tuple_element_t<Is, ArgsTuple>>(expr, Is)...);

	// Check all arguments are valid
	if (!(std::get<Is>(argsTuple).has_value() && ...))
	{
		return methodErrorFailure<BaseT>();
	}

	// Get instance pointer
	auto* instance = [&]() {
		if constexpr (is_shared_ptr_v<EmbeddedT>)
		{
			return self.get();
		}
		else
		{
			return self;
		}
	}();

	// Call the method
	return method(instance, *std::get<Is>(argsTuple)...);
}

/* =======================================================================
 *  Method Registration
 * ======================================================================= */
/// @brief Register a method for an embedded object type.
/// @tparam EmbeddedT The type of the embedded object (e.g., `T*` or `std::shared_ptr<T>`).
/// @tparam method The method to register, specified as a function pointer.
/// @param embedName The name used for embedding the object type.
template <typename EmbeddedT, auto method>
void RegisterMethod(const char* embedName, const char* methodName)
{
	AddCompilerClassMethod_Export(embedName, methodName, reinterpret_cast<void*>(&embeddedObjectMethod<EmbeddedT, method>));
}
}; // namespace PatternMatcher
