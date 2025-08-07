#include "WolframLibrary.h"
#include "wstp.h"

typedef void* (*callFunction)(void*);

mint writePair(MLINK mlp, const char* name, callFunction value)
{
	mint err;

	err = !WSPutFunction(mlp, "List", 2);
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

extern void* InstantiateObject(void* arg);

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
	err = writePair(mlp, "InstantiateObject", InstantiateObject);
	if (err)
	{
		return err;
	}
	return 0;
}

// Library Link

EXTERN_C DLLEXPORT mint WolframLibrary_getVersion()
{
	return WolframLibraryVersion;
}

EXTERN_C DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData)
{
	return LIBRARY_NO_ERROR;
}

EXTERN_C DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) { }

EXTERN_C DLLEXPORT int PatternMatcherVirtualMachineLibrary_ObjectFactory(WolframLibraryData libData, MLINK mlp)
{
	mint res = getObjectFactoryMethods(mlp);
	return res;
}
