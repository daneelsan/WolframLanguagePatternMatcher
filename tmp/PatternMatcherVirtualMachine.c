#include "WolframLanguageRuntimeV1SDK.h"

// EXTERN_C DLLEXPORT mint WolframLibrary_getVersion(void)
// {
// 	return WolframLibraryVersion;
// }

// EXTERN_C DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData)
// {
// 	wlr_err_t err0 = WLR_SDK_START_RUNTIME(WLR_DYNAMIC_LIBRARY, WLR_SIGNED_CODE_MODE, "/Applications/Wolfram.app/Contents", NULL);
// 	if (err0 != WLR_SUCCESS)
// 	{
// 		// printf("%s (error: %d)\n", "SDK failed to start correctly", err0);
// 		return 1;
// 	}
// 	return 0;
// }
EXTERN_C DLLEXPORT int sdk_initialize()
{
	wlr_err_t err0 = WLR_SDK_START_RUNTIME(WLR_DYNAMIC_LIBRARY, WLR_LICENSE_OR_SIGNED_CODE_MODE, "/Applications/Wolfram.app/Contents", NULL);
	if (err0 != WLR_SUCCESS)
	{
		// printf("%s (error: %d)\n", "SDK failed to start correctly", err0);
		return 1;
	}
	return 0;
}

EXTERN_C DLLEXPORT int transliterate(char *arg, void *res)
{
	wlr_expr head = wlr_Symbol("Transliterate");
	wlr_expr arg1 = wlr_String(arg);
	wlr_expr normal = wlr_E(head, arg1);
	wlr_expr *result = (wlr_expr *)res;
	*result = wlr_Clone(wlr_Eval(normal));
	// wlr_err_t err = wlr_StringData(res, &result, NULL);
	//  if (err != WLR_SUCCESS) {
	//  	return 1;
	//  }
	return 0;
}

// EXTERN_C DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData)
// {
// 	wlr_ReleaseAll();
// 	return;
// }

EXTERN_C DLLEXPORT void sdk_uninitialize()
{
	wlr_ReleaseAll();
	return;
}
