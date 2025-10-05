#include "AST/MExpr.h"

#include "Logger.h"

namespace PatternMatcher
{
void MExprNormal::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprNormal>(embedName);
}
}; // namespace PatternMatcher