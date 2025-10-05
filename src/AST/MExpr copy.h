#pragma once

#include "Embeddable.h"
#include "Expr.h"

#include <memory>
#include <optional>
#include <string>

namespace PatternMatcher
{

class MExpr : public std::enable_shared_from_this<MExpr>
{
public:
	enum class MExprKind
	{
		MExprNormal,
		MExprSymbol,
		MExprLiteral
	};

	MExpr(Expr expr)
	{
		static mint _baseID = 0;
		_id = _baseID++;
		// TODO: True/False should be MExprLiteral
		if (expr.symbolQ())
		{
			_kind = MExprKind::MExprSymbol;
		}
		else if (expr.depth() > 0)
		{
			_kind = MExprKind::MExprNormal;
		}
		else
		{
			_kind = MExprKind::MExprLiteral;
		}
		_expr = expr;
	}

	mint getID() { return _id; }

	MExprKind getKind() const { return _kind; }

	Expr getExpr() const { return _expr; }

	bool sameQ(std::shared_ptr<MExpr> other)
	{
		if (_id == other->_id)
		{
			return true;
		}
		if (_kind == other->_kind)
		{
			return _expr.sameQ(other->_expr);
		}
		return false;
	}

private:
	mint _id;
	MExprKind _kind;
	Expr _expr;
};

class MExprNormal : public MExpr
{
public:
	std::shared_ptr<MExpr> getHead() const { return head; }

	const std::vector<std::shared_ptr<MExpr>>& getChildren() const { return children; }

	size_t length() const { return children.size(); }

private:
	std::shared_ptr<MExpr> head;
	std::vector<std::shared_ptr<MExpr>> children;
};

class MExprSymbol : public MExpr
{
public:
	bool isProtected() const { return protectedQ; }

	const std::string& getContext() const { return context; }

	const std::string& getSourceName() const { return sourceName; }

	const std::string& getName() const { return name; }

	void rename(const std::string& newName)
	{
		if (!protectedQ)
		{
			name = newName;
		}
	}

private:
	bool protectedQ = false;
	std::string context;
	std::string sourceName; // name of the symbol as it appears in the source
	std::string name; // this may get rewritten to be unique for each binding environment unless protected is true
};

class MExprLiteral : public MExpr
{
public:
	// TODO
};

template <>
inline const char* EmbedName<MExprNormal>()
{
	return "PatternMatcherLibrary`AST`MExprNormal";
}

template <>
inline const char* EmbedName<MExprSymbol>()
{
	return "PatternMatcherLibrary`AST`MExprSymbol";
}

template <>
inline const char* EmbedName<MExprLiteral>()
{
	return "PatternMatcherLibrary`AST`MExprLiteral";
}
}; // namespace PatternMatcher