#include "AST/MExprEnvironment.h"

#include "AST/MExpr.h"

#include "Expr.h"
#include "Logger.h"

#include <memory>
#include <optional>

namespace PatternMatcher
{

std::shared_ptr<MExpr> MExprEnvironment::createMExpr(Expr e)
{
	return MExpr::construct(e);
}

namespace MethodInterface
{
	Expr createMExpr(MExprEnvironment* env, Expr e)
	{
		return MExpr::toExpr(env->createMExpr(e));
	}
}; // namespace MethodInterface

/// @brief Initialize the embedding in an expression.
/// @param embedName The name to use for embedding.
void MExprEnvironment::initializeEmbedMethods(const char* embedName)
{
	AddCompilerClassMethod_Export(
		embedName, "createMExpr",
		reinterpret_cast<void*>(&embeddedObjectUnaryMethod<MExprEnvironment*, Expr, MethodInterface::createMExpr>));
};

/// @brief This function allows a MExprEnvironment to be instantiated from Wolfram Language
Expr MExprEnvironmentExpr()
{
	MExprEnvironment* env = new MExprEnvironment();
	Expr expr = EmbedObject(env);
	return expr;
}

}; // namespace PatternMatcher