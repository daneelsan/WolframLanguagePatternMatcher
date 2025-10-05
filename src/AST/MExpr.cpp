#include "AST/MExpr.h"

#include "Expr.h"
#include "Logger.h"
#include "TypeTraits.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace PatternMatcher
{
std::string MExpr::toString() const
{
	return getExpr().toString();
}

Expr MExpr::toExpr(std::shared_ptr<MExpr> mexpr)
{
	if (auto l = std::dynamic_pointer_cast<MExprLiteral>(mexpr))
		return EmbedObject(l);
	if (auto s = std::dynamic_pointer_cast<MExprSymbol>(mexpr))
		return EmbedObject(s);
	if (auto n = std::dynamic_pointer_cast<MExprNormal>(mexpr))
		return EmbedObject(n);

	return Expr::throwError("Unexpected MExpr subclass in toExpr.");
}

std::shared_ptr<MExpr> MExpr::construct(Expr e)
{
	if (e.symbolQ())
	{
		// Symbol
		auto nameOpt = e.as<std::string>();
		std::string name = nameOpt.value_or("<UnknownSymbol>");
		return std::make_shared<MExprSymbol>(e, "" /*context*/, name /*sourceName*/, name /*name*/);
	}
	else if (e.depth() > 1)
	{
		// Normal expressions: head + children
		Expr headExpr = e.head();
		auto headMExpr = MExpr::construct(headExpr);
		std::vector<std::shared_ptr<MExpr>> children;
		children.reserve(e.length());
		for (mint i = 1; i <= e.length(); i += 1)
		{
			Expr child = e.part(i);
			children.push_back(MExpr::construct(child));
		}
		auto res = std::make_shared<MExprNormal>(e, headMExpr, std::move(children));
		return res;
	}
	else
	{
		// Fallback to Literal
		return std::make_shared<MExprLiteral>(e);
	}
}

namespace MethodInterface
{
	template <typename T>
	Expr getIDFunction(T* expr)
	{
		return Expr(expr->getID());
	}

	template <typename T>
	Expr toStringFunction(T* expr)
	{
		return Expr(expr->toString());
	}
}; // namespace MethodInterface

template <typename T>
void MExpr::initializeEmbedMethodsCommon(const char* embedName)
{
	using SharedT = std::shared_ptr<T>;
	PM_DEBUG("Initializing embed methods for ", embedName);
	AddCompilerClassMethod_Export(
		embedName, "getID",
		reinterpret_cast<void*>(&embeddedObjectNullaryMethod<SharedT, MethodInterface::getIDFunction<T>>));
	AddCompilerClassMethod_Export(
		embedName, "toString",
		reinterpret_cast<void*>(&embeddedObjectNullaryMethod<SharedT, MethodInterface::toStringFunction<T>>));
}

// Explicit instantiations:
template void MExpr::initializeEmbedMethodsCommon<MExprLiteral>(const char*);
template void MExpr::initializeEmbedMethodsCommon<MExprNormal>(const char*);
template void MExpr::initializeEmbedMethodsCommon<MExprSymbol>(const char*);
}; // namespace PatternMatcher