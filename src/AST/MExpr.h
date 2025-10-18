#pragma once

#include "Embeddable.h"
#include "Expr.h"

#include "AST/MExprEnvironment.h"

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

	virtual size_t length() const = 0;

	virtual std::shared_ptr<MExpr> getHead() const = 0;

	bool hasHead(std::shared_ptr<MExpr> headMExpr) const;

	bool hasHead(const Expr& headExpr) const;

	bool hasHead(const char* headName) const;

	virtual bool sameQ(std::shared_ptr<MExpr> other) const = 0;

	std::string toString() const;

	static Expr toExpr(std::shared_ptr<MExpr> expr);

	static std::shared_ptr<MExpr> construct(const Expr& e);

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
		, _head(std::move(head))
		, _children(std::move(children))
	{
	}

	/// @brief Create a new MExprNormal from an Expr by recursively constructing MExprs for the head and children.
	static std::shared_ptr<MExpr> create(const Expr& expr);

	Expr getExpr() const override { return _expr; }

	bool sameQ(std::shared_ptr<MExpr> other) const override;

	/// @brief Get the head of the normal expression.
	std::shared_ptr<MExpr> getHead() const override;

	/// @brief Get the number of children of the normal expression.
	size_t length() const override;

	const std::vector<std::shared_ptr<MExpr>>& getChildren() const { return _children; }

	/// @brief Get the i-th child (1-based index).
	/// @param i The index of the child to get (1-based).
	/// @return The i-th child MExpr.
	std::shared_ptr<MExpr> part(mint i) const;

	/// @brief Initialize embedding methods for the MExprNormal instance.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName) override;

private:
	Expr _expr;
	std::shared_ptr<MExpr> _head;
	std::vector<std::shared_ptr<MExpr>> _children;
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
		, system_protected(prot)
	{
	}

	/// @brief  Create a new MExprSymbol from an Expr.
	/// @details This creates a new MExprSymbol from the given Expr.
	/// It extracts the context, source name, and protection status of the symbol.
	/// @param expr The Expr representing the symbol.
	/// @return The created MExprSymbol instance.
	static std::shared_ptr<MExpr> create(const Expr& expr);

	Expr getExpr() const override { return _expr; }

	size_t length() const override { return 0; }

	std::shared_ptr<MExpr> getHead() const override;

	bool sameQ(std::shared_ptr<MExpr> other) const override;

	const std::string& getContext() const { return context; }

	const std::string& getSourceName() const { return sourceName; }

	const std::string& getName() const { return name; }

	const std::string& getLexicalName() const { return sourceName; } // TODO: change if we add renaming

	bool isSystemProtected() const { return system_protected; }

	void rename(const std::string& newName)
	{
		if (!system_protected)
			name = newName;
	}

	void initializeEmbedMethods(const char* embedName) override;

private:
	Expr _expr;
	bool system_protected;
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

	/// @brief  Create a new MExprLiteral from an Expr.
	/// @details This creates a new MExprLiteral from the given Expr.
	/// @param expr The Expr representing the literal.
	/// @return The created MExprLiteral instance.
	static std::shared_ptr<MExpr> create(const Expr& expr);

	Expr getExpr() const override { return _expr; }

	size_t length() const override { return 0; }

	std::shared_ptr<MExpr> getHead() const override;

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
