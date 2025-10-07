
/*
 * Needs to worry about memory management.
 * Really there should be a destructor  which decrements the ref count.
 * The extract method should increment the ref count.
 * There should be a constructor for arguments which increments, since arguments are not counted.
 */

#include "WolframLibrary.h"
#include "Expr.h"

#include <cstring>
#include <optional>
#include <string>

namespace PatternMatcher
{
/* =======================================================================
 * Expr to T conversions
 * ======================================================================= */

// std::string specialization
template <>
std::optional<std::string> Expr::as<std::string>() const
{
	const char* bytes;
	mint len;
	if (!StringExpressionToUTF8Bytes(instance, &bytes, &len))
	{
		return std::nullopt;
	}
	std::string res(bytes, len);
	return res;
}

// mint specialization
template <>
std::optional<mint> Expr::as<mint>() const
{
	mint res;
	if (TestGet_Integer(instance, 64, true, reinterpret_cast<ExprStruct>(&res)))
	{
		return res;
	}
	return std::nullopt;
}

// bool specialization
template <>
std::optional<bool> Expr::as<bool>() const
{
	// TODO: TestGet_Boolean or something
	if (Expr::construct("BooleanQ", *this).eval().sameQ("True")) {
		return sameQ("True");
	}
	return std::nullopt;
}

// Expr specialization (identity)
template <>
std::optional<Expr> Expr::as<Expr>() const
{
	return *this;
}

/* =======================================================================
 * Expr
 * ======================================================================= */
/*
 * Why can't this call the Ctor for instances?
 * Same for const char* Ctor
 */
Expr::Expr(mint x)
{
	instance = CreateIntegerExpr(reinterpret_cast<ExprStruct>(&x), 64, true);
}

Expr::Expr(const char* txt)
{
	mint len = strlen(txt);
	instance = UTF8BytesAndLengthToStringExpression(txt, len, len);
}

Expr::Expr(std::string txt)
	: Expr(txt.c_str())
{
}

Expr Expr::eval()
{
	return Expr(Evaluate_E_E(instance));
}

mint Expr::length() const
{
	return Length_Expression_Integer(instance);
}

mint Expr::depth() const
{
	return Depth_Expression_Integer(instance);
}

Expr Expr::part(mint i) const
{
	return Expr(Part_E_I_E(instance, i));
}

Expr Expr::head() const
{
	return Expr(Part_E_I_E(instance, 0));
}

void Expr::setPart(mint i, Expr val)
{
	SetElement_EIE_E(instance, i, val.instance);
}

mint Expr::print()
{
	return Print_E_I(instance);
}

bool Expr::sameQ(Expr other) const
{
	return SameQ_E_E_Boolean(instance, other.instance);
}

bool Expr::sameQ(const char* txt) const
{
	return SameQ_E_E_Boolean(instance, Expr::ToExpression(txt).instance);
}

// Return a string from a String Expr.
std::string Expr::toString() const
{
	Expr toStrExpr = Expr::construct("ToString", *this).eval();
	return toStrExpr.as<std::string>().value_or("");
}

/*
 * Return true if an expr string.
 * Doesn't need to free the bytes.
 */
bool Expr::stringQ() const
{
	const char* bytes;
	return TestGet_CString(instance, &bytes);
}

// Return true if an expr list.
bool Expr::listQ() const
{
	return head().sameQ(Expr::ToExpression("List"));
}

// Return true if an expr rule.
bool Expr::ruleQ() const
{
	return length() == 2 && head().sameQ(Expr::ToExpression("Rule"));
}

bool Expr::symbolQ() const
{
	return length() == 0 && head().sameQ(Expr::ToExpression("Symbol"));
}

/*
 * Static methods
 */
Expr Expr::createNormal(mint len, Expr head)
{
	return Expr(CreateHeaded_IE_E(len, head.instance));
}

Expr Expr::createNormal(mint len, const char* head)
{
	return Expr::createNormal(len, Expr::ToExpression(head));
}

/*
 * Embed an instance in an expression
 */
Expr Expr::embedObjectInstance(ExprStruct val, const char* name, Expr head)
{
	int init;
	ExprStruct instanceN = Create_ObjectInstanceByNameInitWithHead(val, name, &init, head.instance);
	Expr expr(instanceN);
	return expr;
}

std::optional<ExprStruct> Expr::unembedObjectInstance(Expr self, const char* className)
{
	ExprStruct obj;
	if (TestGet_ObjectInstanceByName(self.instance, className, &obj))
	{
		return obj;
	}
	return std::nullopt;
}

Expr Expr::ToExpression(const char* txt)
{
	return Expr(CreateGeneralExpr(txt));
}

Expr Expr::failure()
{
	return Expr::ToExpression("$Failed");
}

Expr Expr::throwError(const char* txt)
{
	return Expr::throwError(std::string(txt));
}

Expr Expr::throwError(std::string txt)
{
	Expr message = Expr(txt);
	Expr eThrow = Expr::construct("DanielS`PatternMatcher`ErrorHandling`ThrowLibraryError", message);
	return eThrow;
}

Expr Expr::throwError(const char* txt, Expr arg1)
{
	Expr message = Expr(txt);
	Expr params = Expr::construct("List", arg1);
	Expr eThrow = Expr::construct("DanielS`PatternMatcher`ErrorHandling`ThrowLibraryError", message, params);
	return eThrow;
}

}; // namespace PatternMatcher
