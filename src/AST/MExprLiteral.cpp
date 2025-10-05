#include "AST/MExpr.h"

#include "Logger.h"

namespace PatternMatcher
{
void MExprLiteral::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprLiteral>(embedName);
}
}; // namespace PatternMatcher