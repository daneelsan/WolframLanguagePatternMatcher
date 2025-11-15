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

	/// @brief Get the unique ID of this MExpr.
	/// @return The unique ID.
	mint getID() const { return _id; }

	/// @brief Get the kind of this MExpr.
	/// @return The kind.
	Kind getKind() const { return _kind; }

	/// @brief Get the number of children of the expression.
	/// @return The number of children.
	virtual size_t length() const { return 0; } // Default: no children

	/// @brief Get the head of the expression.
	/// @return The head MExpr.
	virtual std::shared_ptr<MExpr> getHead() const = 0;

	/// @brief Get the expression represented by this MExpr.
	/// @return The expression represented by this MExpr.
	virtual Expr getExpr() const = 0; // force subclasses to expose Expr

	/// @brief Get the held expression (i.e., HoldComplete[expr]).
	/// @return The held expression.
	Expr toHeldExpr() const;

	///	@brief Get the held form expression (i.e., HoldCompleteForm[expr]).
	/// @return The held form expression.
	Expr toHeldFormExpr() const;

	/// @brief Compare this MExpr with another for structural equality.
	/// @param other The other MExpr to compare with.
	/// @return true if they are structurally equal, false otherwise.
	virtual bool sameQ(std::shared_ptr<MExpr> other) const = 0;

	/// @brief Check if this MExpr has a specific head.
	/// @param headMExpr The head MExpr to check for.
	/// @return true if this MExpr has the specified head, false otherwise.
	bool hasHead(std::shared_ptr<MExpr> headMExpr) const;

	/// @brief Check if this MExpr has a specific head.
	/// @param headExpr The head Expr to check for.
	/// @return true if this MExpr has the specified head, false otherwise.
	bool hasHead(const Expr& headExpr) const;

	/// @brief Check if this MExpr has a specific head.
	/// @param headName The name of the head to check for.
	/// @return true if this MExpr has the specified head, false otherwise.
	bool hasHead(const char* headName) const;

	/// @brief Convert this MExpr to its string representation.
	/// @return The string representation of this MExpr.
	std::string toString() const;

	/// @brief Convert an MExpr to an Expr.
	/// @param expr The MExpr to convert.
	/// @return The converted Expr.
	[[nodiscard("Memory leak if ignored - Expr must be stored")]]
	static Expr toExpr(std::shared_ptr<MExpr> expr);

	/// @brief Construct an MExpr from an Expr.
	/// @param e The Expr to construct the MExpr from.
	/// @return The constructed MExpr.
	[[nodiscard("Memory leak if ignored - MExpr must be stored")]]
	static std::shared_ptr<MExpr> construct(const Expr& e);

	// Type checking convenience methods
	[[nodiscard]] bool literalQ() const { return _kind == Kind::Literal; }
	[[nodiscard]] bool normalQ() const { return _kind == Kind::Normal; }
	[[nodiscard]] bool symbolQ() const { return _kind == Kind::Symbol; }

	/// @brief Initialize embedding methods for the MExpr instance.
	/// @param embedName The name to use for embedding.
	virtual void initializeEmbedMethods(const char* embedName) = 0;

	/// @brief Common initialization for embedding methods.
	/// @tparam T The type of the embedding.
	/// @param embedName The name to use for embedding.
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
	MExprNormal(std::shared_ptr<MExpr> head, std::vector<std::shared_ptr<MExpr>> children)
		: MExpr(Kind::Normal)
		, _head(std::move(head))
		, _children(std::move(children))
	{
	}

	/// @brief Create a new MExprNormal from an Expr by recursively constructing MExprs for the head and children.
	/// @details This creates a new MExprNormal from the given Expr. It recursively constructs MExprs for the head and
	/// children of the expression.
	/// @param expr The Expr representing the normal expression.
	/// @return The created MExprNormal instance.
	[[nodiscard("Memory leak if ignored - MExprNormal must be stored")]]
	static std::shared_ptr<MExpr> create(const Expr& expr);

	/// @brief Get the expression represented by the normal expression.
	/// @return The expression represented by the normal expression.
	Expr getExpr() const override;

	/// @brief Compare this MExprNormal with another for structural equality.
	/// @param other The other MExpr to compare with.
	/// @return true if they are structurally equal, false otherwise.
	bool sameQ(std::shared_ptr<MExpr> other) const override;

	/// @brief Get the number of children of the normal expression.
	size_t length() const override;

	/// @brief Get the head of the normal expression.
	std::shared_ptr<MExpr> getHead() const override;

	/// @brief Get the arguments (children) of the normal expression.
	/// @return A vector of shared pointers to the argument MExprs.
	const std::vector<std::shared_ptr<MExpr>>& getChildren() const { return _children; }

	/// @brief Get the i-th child (1-based index).
	/// @param i The index of the child to get (1-based).
	/// @return The i-th child MExpr.
	std::shared_ptr<MExpr> part(mint i) const;

	/// @brief Initialize embedding methods for the MExprNormal instance.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName) override;

private:
	std::shared_ptr<MExpr> _head;
	std::vector<std::shared_ptr<MExpr>> _children;
};

// ---------------- Symbol ----------------
class MExprSymbol : public MExpr
{
public:
	MExprSymbol(Expr expr, std::string context, std::string sourceName, bool prot)
		: MExpr(Kind::Symbol)
		, _expr(expr)
		, context(std::move(context))
		, sourceName(sourceName)
		, name(std::move(sourceName))
		, system_protected(prot)
	{
	}

	/// @brief  Create a new MExprSymbol from an Expr.
	/// @details This creates a new MExprSymbol from the given Expr.
	/// It extracts the context, source name, and protection status of the symbol.
	/// @param expr The Expr representing the symbol.
	/// @return The created MExprSymbol instance.
	[[nodiscard("Memory leak if ignored - MExprSymbol must be stored")]]
	static std::shared_ptr<MExpr> create(const Expr& expr);

	/// @brief Get the expression represented by the symbol.
	/// @return The expression represented by the symbol.
	Expr getExpr() const override;

	/// @brief Get the head of the symbol expression (always Symbol).
	/// @return The head MExpr (Symbol).
	std::shared_ptr<MExpr> getHead() const override;

	/// @brief Compare this MExprSymbol with another for structural equality.
	/// @param other The other MExpr to compare with.
	/// @return true if they are structurally equal, false otherwise.
	bool sameQ(std::shared_ptr<MExpr> other) const override;

	/// @brief Get the context of the symbol.
	/// @return The context of the symbol.
	const std::string& getContext() const { return context; }

	/// @brief Get the source name of the symbol.
	/// @return The source name of the symbol.
	const std::string& getSourceName() const { return sourceName; }

	/// @brief Get the name of the symbol (without context).
	/// @return The name of the symbol.
	const std::string& getName() const { return name; }

	/// @brief Get the lexical name of the symbol (including context).
	/// @return The lexical name of the symbol.
	std::string getLexicalName() const;

	/// @brief Check if the symbol is system protected.
	/// @return true if the symbol is system protected, false otherwise.
	bool isSystemProtected() const { return system_protected; }

	/// @brief Rename the symbol if it is not system protected.
	/// @param newName The new name for the symbol.
	/// @return true if the name was updated, false if it was system protected.
	bool updateName(const std::string& newName);

	/// @brief Initialize embedding methods for the MExprSymbol instance.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName) override;

private:
	const Expr _expr;
	const bool system_protected;
	const std::string context;
	const std::string sourceName;
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
	[[nodiscard("Memory leak if ignored - MExprLiteral must be stored")]]
	static std::shared_ptr<MExpr> create(const Expr& expr);

	/// @brief Get the expression represented by the literal.
	/// @return The expression represented by the literal.
	Expr getExpr() const override { return _expr; }

	/// @brief Get the head of the literal expression (always itself).
	/// @return The head MExpr (itself).
	std::shared_ptr<MExpr> getHead() const override;

	/// @brief Compare this MExprLiteral with another for structural equality.
	/// @param other The other MExpr to compare with.
	/// @return true if they are structurally equal, false otherwise.
	bool sameQ(std::shared_ptr<MExpr> other) const override;

	/// @brief Initialize embedding methods for the MExprLiteral instance.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName) override;

private:
	const Expr _expr;
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
