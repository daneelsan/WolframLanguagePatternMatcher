#include "WolframLibrary.h"
#include "ObjectFactory.h"

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
