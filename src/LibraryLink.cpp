#include "WolframLibrary.h"
#include "ObjectFactory.h"
#include "Logger.h"

EXTERN_C DLLEXPORT mint WolframLibrary_getVersion()
{
	return WolframLibraryVersion;
}

EXTERN_C DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData)
{
	return LIBRARY_NO_ERROR;
}

EXTERN_C DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) { }

EXTERN_C DLLEXPORT int PatternMatcherLibrary_ObjectFactoryMethods(WolframLibraryData libData, MLINK mlp)
{
	mint res = getObjectFactoryMethods(mlp);
	return static_cast<int>(res);
}

// Global trace control functions (don't require VM instance)
EXTERN_C DLLEXPORT int PatternMatcherLibrary_SetTraceEnabled(WolframLibraryData libData, mint argc, MArgument* args, MArgument res)
{
	bool enabled = MArgument_getBoolean(args[0]);
	PatternMatcher::Logger::setTraceEnabled(enabled);
	MArgument_setBoolean(res, enabled);
	return LIBRARY_NO_ERROR;
}

EXTERN_C DLLEXPORT int PatternMatcherLibrary_TraceEnabledQ(WolframLibraryData libData, mint argc, MArgument* args, MArgument res)
{
	bool enabled = PatternMatcher::Logger::isTraceEnabled();
	MArgument_setBoolean(res, enabled);
	return LIBRARY_NO_ERROR;
}
