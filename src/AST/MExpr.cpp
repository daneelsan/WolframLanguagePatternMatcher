#include "AST/MExpr.h"
#include "AST/MExprEnvironment.h"

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

Expr MExpr::getHeldExpr() const
{
	return Expr::construct("HoldComplete", getExpr());
}

Expr MExpr::getHeldFormExpr() const
{
	return Expr::construct("HoldCompleteForm", getExpr());
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

std::shared_ptr<MExpr> MExpr::construct(const Expr& e)
{
	if (e.symbolQ())
	{
		// Improve test above
		return MExprSymbol::create(e);
	}
	else if (e.depth() > 1)
	{
		// Improve test above
		return MExprNormal::create(e);
	}
	else
	{
		// Fallback to Literal
		return MExprLiteral::create(e);
	}
}

bool MExpr::hasHead(std::shared_ptr<MExpr> headMExpr) const
{
	return getHead()->sameQ(headMExpr);
}

bool MExpr::hasHead(const Expr& headExpr) const
{
	auto headMExpr = MExpr::construct(headExpr);
	return hasHead(headMExpr);
}

bool MExpr::hasHead(const char* headName) const
{
	return hasHead(Expr::ToExpression(headName));
}

namespace MethodInterface
{
	template <typename T>
	Expr getExpr(std::shared_ptr<T> mexpr)
	{
		return mexpr->getExpr();
	}

	template <typename T>
	Expr getHeldExpr(std::shared_ptr<T> mexpr)
	{
		return mexpr->getHeldExpr();
	}

	template <typename T>
	Expr getHeldFormExpr(std::shared_ptr<T> mexpr)
	{
		return mexpr->getHeldFormExpr();
	}

	template <typename T>
	Expr getHead(std::shared_ptr<T> mexpr)
	{
		auto headMExpr = mexpr->getHead();
		return MExpr::toExpr(headMExpr);
	}

	template <typename T>
	Expr getID(std::shared_ptr<T> mexpr)
	{
		return Expr(mexpr->getID());
	}

	template <typename T>
	Expr hasHead(std::shared_ptr<T> mexpr, Expr headExpr)
	{
		bool res = false;
		auto headMExprOpt = headExpr.as<std::shared_ptr<MExpr>>();
		if (headMExprOpt)
		{
			res = mexpr->hasHead(*headMExprOpt);
		}
		else
		{
			res = mexpr->hasHead(headExpr);
		}
		return toExpr(res);
	}

	template <typename T>
	Expr length(std::shared_ptr<T> mexpr)
	{
		return Expr(mexpr->length());
	}

	template <typename T>
	Expr normalQ(std::shared_ptr<T> mexpr)
	{
		return toExpr(mexpr->getKind() == MExpr::Kind::Normal);
	}

	template <typename T>
	Expr sameQ(std::shared_ptr<T> mexpr, Expr other)
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
	Expr symbolQ(std::shared_ptr<T> mexpr)
	{
		return toExpr(mexpr->getKind() == MExpr::Kind::Symbol);
	}

	template <typename T>
	Expr toString(std::shared_ptr<T> mexpr)
	{
		return Expr(mexpr->toString());
	}
}; // namespace MethodInterface

template <typename T>
void MExpr::initializeEmbedMethodsCommon(const char* embedName)
{
	using SharedT = std::shared_ptr<T>;
	RegisterMethod<SharedT, MethodInterface::getExpr<T>>(embedName, "getExpr");
	RegisterMethod<SharedT, MethodInterface::getHeldExpr<T>>(embedName, "getHeldExpr");
	RegisterMethod<SharedT, MethodInterface::getHeldFormExpr<T>>(embedName, "getHeldFormExpr");
	RegisterMethod<SharedT, MethodInterface::getHead<T>>(embedName, "getHead");
	RegisterMethod<SharedT, MethodInterface::getID<T>>(embedName, "getID");
	RegisterMethod<SharedT, MethodInterface::hasHead<T>>(embedName, "hasHead");
	RegisterMethod<SharedT, MethodInterface::length<T>>(embedName, "length");
	RegisterMethod<SharedT, MethodInterface::normalQ<T>>(embedName, "normalQ");
	RegisterMethod<SharedT, MethodInterface::toString<T>>(embedName, "toString");
	RegisterMethod<SharedT, MethodInterface::sameQ<T>>(embedName, "sameQ");
	RegisterMethod<SharedT, MethodInterface::symbolQ<T>>(embedName, "symbolQ");
}

// Explicit instantiations:
template void MExpr::initializeEmbedMethodsCommon<MExprLiteral>(const char*);
template void MExpr::initializeEmbedMethodsCommon<MExprNormal>(const char*);
template void MExpr::initializeEmbedMethodsCommon<MExprSymbol>(const char*);
}; // namespace PatternMatcher