#pragma once

#include "Embeddable.h"
#include "Expr.h"

#include <memory>
#include <string>
#include <vector>

namespace PatternMatcher
{

class MExpr : public std::enable_shared_from_this<MExpr>
{
public:
	enum class Kind
	{
		Normal,
		Symbol,
		Literal
	};

	MExpr(Kind kind)
		: _id(_baseID++)
		, _kind(kind)
	{
	}
	virtual ~MExpr() = default;

	mint getID() const { return _id; }

	Kind getKind() const { return _kind; }

	virtual Expr getExpr() const = 0; // force subclasses to expose Expr

	virtual bool sameQ(std::shared_ptr<MExpr> other) const = 0;

	std::string toString() const;

	static Expr toExpr(std::shared_ptr<MExpr> expr);

	static std::shared_ptr<MExpr> construct(Expr e);

	virtual void initializeEmbedMethods(const char*) = 0;

	template <typename T>
	void initializeEmbedMethodsCommon(const char* embedName);

protected:
	mint _id;
	Kind _kind;

private:
	inline static mint _baseID = 0; // static counter
};

// ---------------- Normal ----------------
class MExprNormal : public MExpr
{
public:
	MExprNormal(Expr expr, std::shared_ptr<MExpr> head, std::vector<std::shared_ptr<MExpr>> children)
		: MExpr(Kind::Normal)
		, _expr(expr)
		, head(std::move(head))
		, children(std::move(children))
	{
	}

	Expr getExpr() const override { return _expr; }

	bool sameQ(std::shared_ptr<MExpr> other) const override;

	std::shared_ptr<MExpr> getHead() const { return head; }

	const std::vector<std::shared_ptr<MExpr>>& getChildren() const { return children; }

	size_t length() const { return children.size(); }

	void initializeEmbedMethods(const char* embedName) override;

private:
	Expr _expr;
	std::shared_ptr<MExpr> head;
	std::vector<std::shared_ptr<MExpr>> children;
};

// ---------------- Symbol ----------------
class MExprSymbol : public MExpr
{
public:
	MExprSymbol(Expr expr, std::string context, std::string sourceName, std::string name, bool prot = false)
		: MExpr(Kind::Symbol)
		, _expr(expr)
		, context(std::move(context))
		, sourceName(std::move(sourceName))
		, name(std::move(name))
		, protectedQ(prot)
	{
	}

	Expr getExpr() const override { return _expr; }

	bool sameQ(std::shared_ptr<MExpr> other) const override;

	bool isProtected() const { return protectedQ; }

	const std::string& getContext() const { return context; }

	const std::string& getSourceName() const { return sourceName; }

	const std::string& getName() const { return name; }

	void rename(const std::string& newName)
	{
		if (!protectedQ)
			name = newName;
	}

	void initializeEmbedMethods(const char* embedName) override;

private:
	Expr _expr;
	bool protectedQ;
	std::string context;
	std::string sourceName;
	std::string name;
};

// ---------------- Literal ----------------
class MExprLiteral : public MExpr
{
public:
	explicit MExprLiteral(Expr expr)
		: MExpr(Kind::Literal)
		, _expr(expr)
	{
	}

	Expr getExpr() const override { return _expr; }

	bool sameQ(std::shared_ptr<MExpr> other) const override;

	void initializeEmbedMethods(const char* embedName) override;

private:
	Expr _expr;
};

// ---------------- Embedding ----------------
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

} // namespace PatternMatcher
