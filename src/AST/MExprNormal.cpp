#include "AST/MExpr.h"

#include "Logger.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace PatternMatcher
{
std::shared_ptr<MExpr> MExprNormal::create(const Expr& e)
{
	// Normal expressions: head + children
	mint len = e.length();
	Expr headExpr = e.head();
	auto headMExpr = MExpr::construct(headExpr);
	std::vector<std::shared_ptr<MExpr>> children;
	children.reserve(len);
	for (mint i = 1; i <= len; i += 1)
	{
		Expr child = e.part(i);
		children.push_back(MExpr::construct(child));
	}
	return std::make_shared<MExprNormal>(std::move(headMExpr), std::move(children));
}

size_t MExprNormal::length() const
{
	return _children.size();
}

std::shared_ptr<MExpr> MExprNormal::getHead() const
{
	return _head;
}

Expr MExprNormal::getExpr() const
{
	std::vector<Expr> argExprs;
	mint len = length();
	argExprs.reserve(len);
	for (const auto& child : _children)
	{
		argExprs.push_back(child->getExpr());
	}
	Expr headExpr = _head->getExpr();
	Expr normalExpr = Expr::createNormal(len, headExpr);
	for (mint i = 1; i <= len; ++i)
	{
		normalExpr.setPart(i, argExprs[i - 1]);
	}
	return normalExpr;
}

bool MExprNormal::sameQ(std::shared_ptr<MExpr> other) const
{
	if (other->getKind() != Kind::Normal)
		return false;
	auto o = std::static_pointer_cast<MExprNormal>(other);
	if (this->getID() == o->getID())
		return true;
	if (this->length() != o->length())
		return false;
	if (!_head->sameQ(o->_head))
		return false;
	return std::equal(_children.begin(), _children.end(), o->_children.begin(),
					  [](const auto& a, const auto& b) { return a->sameQ(b); });
}

std::shared_ptr<MExpr> MExprNormal::part(mint i) const
{
	if (i < 0 || i > _children.size())
	{
		PM_ERROR("Index out of bounds in MExprNormal::part: ", i, " (length: ", _children.size(), ")");
		return nullptr;
	}
	if (i == 0)
	{
		return _head;
	}
	else
	{
		return _children[i - 1];
	}
}

namespace MExprNormalInterface
{
	Expr arguments(std::shared_ptr<MExprNormal> obj)
	{
		const auto& children = obj->getChildren();
		// TODO: Implement Expr::createList
		auto list = Expr::createNormal(children.size(), "List");
		for (size_t i = 0; i < children.size(); ++i)
		{
			list.setPart(i + 1, MExpr::toExpr(children[i]));
		}
		return list;
	}

	Expr part(std::shared_ptr<MExprNormal> mexpr, mint i)
	{
		auto childMExpr = mexpr->part(i);
		return MExpr::toExpr(childMExpr);
	}

	Expr toBoxes(Expr objExpr, Expr fmt)
	{
		return Expr::construct("DanielS`PatternMatcher`AST`Private`toMExprNormalBoxes", objExpr, fmt).eval();
	}
}; // namespace MExprNormalInterface

void MExprNormal::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprNormal>(embedName);
	RegisterMethod<std::shared_ptr<MExprNormal>, MExprNormalInterface::arguments>(embedName, "arguments");
	RegisterMethod<std::shared_ptr<MExprNormal>, MExprNormalInterface::part>(embedName, "part");
	RegisterMethod<std::shared_ptr<MExprNormal>, MExprNormalInterface::toBoxes>(embedName, "toBoxes");
}
}; // namespace PatternMatcher