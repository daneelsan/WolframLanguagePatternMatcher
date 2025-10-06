#pragma once

#include "WolframLibrary.h"

#include <optional>
#include <string>
#include <map>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace PatternMatcher
{

/*
 *  ExprStruct is a type to hold an opaque pointer to the actual expr struct
 *  returned from the Wolfram Language.  It could be void* but having a specific
 *  type seems a better.
 *
 *  The Expr class holds an instance of ExprStruct.  It has an automatic
 *  conversion function to go from Expr to ExprStruct.
 */
struct st_ExprStruct
{
	int dummy;
};

using ExprStruct = struct st_ExprStruct*;

extern "C" ExprStruct Evaluate_E_E(ExprStruct arg);

extern "C" mint Length_Expression_Integer(ExprStruct);
extern "C" mint Depth_Expression_Integer(ExprStruct);
extern "C" ExprStruct Part_E_I_E(ExprStruct, mint);

extern "C" ExprStruct Expression_SetPart_Export(ExprStruct, ExprStruct, ExprStruct, bool*);
extern "C" void SetElement_EIE_E(ExprStruct base, mint pos, ExprStruct elem);

extern "C" mint Expression_Acquire_Export(ExprStruct);
extern "C" mint Expression_Release_Export(ExprStruct);
extern "C" mint Print_E_I(ExprStruct);
extern "C" ExprStruct CreateGeneralExpr(const char*);
extern "C" ExprStruct CreateHeaded_IE_E(mint, ExprStruct);

extern "C" bool SameQ_E_E_Boolean(ExprStruct, ExprStruct);
extern "C" ExprStruct UTF8BytesAndLengthToStringExpression(const char*, mint, mint);
extern "C" ExprStruct CreateIntegerExpr(ExprStruct, mint, bool);

extern "C" mint InitializeCompilerClass_Export(const char* name);
extern "C" mint AddCompilerClassMethod_Export(const char* className, const char* methodName, void* fun);
extern "C" mint FinalizeCompilerClass_Export(const char* className);
extern "C" umint SetClassRawMethod(const char* className, const char* methodName, void* fun);

extern "C" ExprStruct Create_ObjectInstanceByNameInitWithHead(ExprStruct inst, const char* className, int* init,
															  ExprStruct vhead);
extern "C" bool TestGet_ObjectInstanceByName(ExprStruct expr, const char* className, ExprStruct* ptr);
extern "C" bool StringExpressionToUTF8Bytes(ExprStruct arg, const char** dataP, mint* lenP);
extern "C" bool TestGet_CString(ExprStruct arg, const char** dataP);
extern "C" bool TestGet_Integer(ExprStruct arg, const uint32_t size, const bool signedQ, ExprStruct res);

extern "C" bool CompiledObjectInstanceQ_Export(ExprStruct arg, const char* className);

class Expr
{

public:
	template <typename T>
	std::optional<T> as() const;

	// Create from an instance, if count is true then acquire
	// This would be eg if the call was from an argument
	Expr(ExprStruct instanceIn, bool count = false)
	{
		instance = instanceIn;
		if (count)
		{
			acquire();
		}
	}

	// Create from values
	Expr(mint x);

	Expr(const char* txt);

	Expr(std::string txt);

	Expr(const Expr& other)
	{
		instance = other.instance;
		acquire();
	}

	//  this = other
	Expr& operator=(Expr& rhs)
	{
		release();
		instance = rhs.instance;
		acquire();
		return *this;
	}

	~Expr() { Expression_Release_Export(instance); }

	Expr eval();

	mint length();

	mint depth();

	Expr part(mint i);

	Expr head() const;

	mint print();

	void setPart(mint i, Expr val);

	void setPart(Expr i, Expr val);

	/*
	 * Automatically convert an Expr to an ExprStruct.
	 * Useful when returning an Expr to a Wolfram Language
	 * function that expects a ExprStruct.  It should only be used
	 * with a return value, not when calling an argument.
	 */
	operator ExprStruct() const
	{
		const_cast<Expr*>(this)->acquire();
		return instance;
	}

	bool sameQ(Expr other) const;

	bool sameQ(const char* other) const;

	static Expr failure();

	static Expr throwError(const char* txt);

	static Expr throwError(const char* txt, Expr);

	static Expr throwError(std::string txt);

	static Expr throwError(std::string txt, Expr);

	static Expr inertExpression(const char* txt);

	static Expr createNormal(mint len, Expr head);

	static Expr createNormal(mint len, const char* head);

	/// @brief Construct a new expression.
	/// @tparam ...TArgs The type of the arguments (these should be all `Expr`s).
	/// @param head The head of the expression.
	/// @param ...args The arguments to construct the expression.
	template <typename... TArgs>
	static Expr construct(Expr head, TArgs... args)
	{
		constexpr mint len = sizeof...(TArgs);
		Expr res = Expr(CreateHeaded_IE_E(len, head.instance));
		int partIndex = 1;
		(res.setPart(partIndex++, std::forward<TArgs>(args)), ...);
		return res;
	}

	template <typename... TArgs>
	static Expr construct(const char* headStr, TArgs... args)
	{
		Expr head = Expr::inertExpression(headStr);
		return Expr::construct(head, std::forward<TArgs>(args)...);
	}

	/*
	 * Embedding methods, some class and some static.
	 * For a further discussion look in Embeddable.h
	 */
	static Expr embedObjectInstance(ExprStruct val, const char* name, Expr head);

	static std::optional<ExprStruct> unembedObjectInstance(Expr, const char*);

	bool stringQ();

	bool ruleQ();

	bool listQ();

	bool symbolQ();

	std::string toString();

private:
	ExprStruct instance;

	mint acquire() { return Expression_Acquire_Export(instance); }

	mint release() { return Expression_Release_Export(instance); }
};

/* =======================================================================
 * T to Expr conversions
 * ======================================================================= */
inline Expr toExpr(const Expr& arg)
{
	return arg;
}

inline Expr toExpr(mint arg)
{
	return Expr(arg);
}

inline Expr toExpr(bool arg)
{
	return Expr::inertExpression(arg ? "True" : "False");
}

// fallback: disabled unless someone defines it
template <typename T>
Expr toExpr(const T&) = delete;

}; // namespace PatternMatcher
