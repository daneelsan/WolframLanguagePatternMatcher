#include "AST/MExpr.h"

#include "Expr.h"
#include "Logger.h"
#include "TypeTraits.h"

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace PatternMatcher
{
// Expr -> MExpr unembedding
template <typename T>
std::optional<std::shared_ptr<T>> unembedMExpr(const Expr& e)
{
	if constexpr (std::is_same_v<T, MExpr>)
	{
		if (auto norm = unembedMExpr<MExprNormal>(e))
			return norm;
		if (auto sym = unembedMExpr<MExprSymbol>(e))
			return sym;
		if (auto lit = unembedMExpr<MExprLiteral>(e))
			return lit;
		return std::nullopt;
	}
	return UnembedObject<std::shared_ptr<T>>(e);
}

template <>
std::optional<std::shared_ptr<MExpr>> Expr::as<std::shared_ptr<MExpr>>() const
{
	return unembedMExpr<MExpr>(*this);
}

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
	Expr getID(T* mexpr)
	{
		return Expr(mexpr->getID());
	}

	template <typename T>
	Expr sameQ(T* mexpr, Expr other)
	{
		bool res = false;
		auto otherOpt = other.as<std::shared_ptr<MExpr>>();
		if (otherOpt)
		{
			res = mexpr->sameQ(*otherOpt);
		}
		return toExpr(res);
	}

	template <typename T>
	Expr toString(T* mexpr)
	{
		return Expr(mexpr->toString());
	}
}; // namespace MethodInterface

template <typename T>
void MExpr::initializeEmbedMethodsCommon(const char* embedName)
{
	using SharedT = std::shared_ptr<T>;
	AddCompilerClassMethod_Export(
		embedName, "getID", reinterpret_cast<void*>(&embeddedObjectNullaryMethod<SharedT, MethodInterface::getID<T>>));
	AddCompilerClassMethod_Export(
		embedName, "toString", reinterpret_cast<void*>(&embeddedObjectNullaryMethod<SharedT, MethodInterface::toString<T>>));
	AddCompilerClassMethod_Export(
		embedName, "sameQ", reinterpret_cast<void*>(&embeddedObjectUnaryMethod<SharedT, Expr, MethodInterface::sameQ<T>>));
}

// Explicit instantiations:
template void MExpr::initializeEmbedMethodsCommon<MExprLiteral>(const char*);
template void MExpr::initializeEmbedMethodsCommon<MExprNormal>(const char*);
template void MExpr::initializeEmbedMethodsCommon<MExprSymbol>(const char*);
}; // namespace PatternMatcher