

#include "ObjectFactory.h"
#include "WolframLibrary.h"

#include "wstp.h"

#include "Expr.h"
#include "Logger.h"

#include "AST/MExprEnvironment.h"

using namespace PatternMatcher;

typedef ExprStruct (*callFunction)(ExprStruct);

ExprStruct InstantiateObject(ExprStruct arg)
{
	Expr val(arg, true);

	if (val.length() < 2)
	{
		PM_ERROR("No argument passed to \"InstantiateObject\".");
		return Expr::throwError("No argument passed to InstantiateObject.");
	}

	Expr part = val.part(2);
	if (part.sameQ("\"MExprEnvironment\""))
	{
		return MExprEnvironmentExpr();
	}

	return Expr::throwError("It is not known how to instantiate `1`.", part);
}

mint writeRule(MLINK mlp, const char* name, callFunction value)
{
	mint err;

	err = !WSPutFunction(mlp, "Rule", 2);
	if (err)
	{
		return err;
	}
	err = !WSPutString(mlp, name);
	if (err)
	{
		return err;
	}
	err = !WSPutLongInteger(mlp, (mint) value);
	if (err)
	{
		return err;
	}
	return 0;
}

mint getObjectFactoryMethods(MLINK mlp)
{
	mint err;
	int len;

	err = !WSTestHead(mlp, "List", &len);
	if (err)
		return err;
	if (len != 0)
		return 55;

	err = !WSNewPacket(mlp);
	if (err)
	{
		return err;
	}
	err = !WSPutFunction(mlp, "List", 1);
	if (err)
	{
		return err;
	}
	err = writeRule(mlp, "InstantiateObject", InstantiateObject);
	if (err)
	{
		return err;
	}
	return 0;
}
