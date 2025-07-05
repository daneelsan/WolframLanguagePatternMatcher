#define WLR_HEADER_VERSION WLR_VERSION_1

#define WLR_START_RUNTIME(licenseType, layoutDirectory, configuration) \
    wlr_StartRuntime(WLR_HEADER_VERSION, licenseType, layoutDirectory, configuration)

#define RUNTIME_NOT_STARTED 1000

#define WLR_SIGNATURE_SIZE 256

#define wlr_E(...) WLR_INTERNAL_CALL_VARIADIC_FUNCTION(wlr_E, __VA_ARGS__)
#define wlr_List(...) WLR_INTERNAL_CALL_VARIADIC_FUNCTION(wlr_List, __VA_ARGS__)
#define wlr_Association(...) WLR_INTERNAL_CALL_VARIADIC_FUNCTION(wlr_Association, __VA_ARGS__)

/* Expression API types */

typedef void *wlr_expr;

typedef void *wlr_exprbag;

typedef void (*wlr_stdout_handler_t)(char *, mint, void *);

typedef void (*wlr_message_handler_t)(wlr_expr, wlr_expr, wlr_expr, void *);

typedef unsigned char wlr_signature_t[WLR_SIGNATURE_SIZE];

typedef enum wlr_number_type
{
    WLR_MACHINE_INTEGER = 0,
    WLR_BIG_INTEGER = 1,
    WLR_MACHINE_REAL = 2,
    WLR_BIG_REAL = 3,
    WLR_COMPLEX = 4,
    WLR_RATIONAL = 5,
    WLR_OVERFLOW = 6,
    WLR_UNDERFLOW = 7,
    WLR_NOT_A_NUMBER = 8
} wlr_num_t;

typedef enum wlr_expression_type
{
    WLR_NUMBER = 0,
    WLR_STRING = 1,
    WLR_SYMBOL = 2,
    WLR_NORMAL = 3,
    WLR_ERROR = 4,
    WLR_PACKED_ARRAY = 5,
    WLR_NUMERIC_ARRAY = 6,
    WLR_BOOLEAN_FUNCTION = 7,
    WLR_GRAPH = 8,
    WLR_ASSOCIATION = 9,
    WLR_DISPATCH = 10,
    WLR_REGION = 11,
    WLR_OTHER = 12
} wlr_expr_t;

typedef enum wlr_error_type
{
    WLR_SUCCESS = 0,
    WLR_ALLOCATION_ERROR = 1,
    WLR_UNEXPECTED_TYPE = 2,
    WLR_ERROR_EXPRESSION = 3,
    WLR_MISCELLANEOUS_ERROR = 4,
    WLR_OUT_OF_BOUNDS = 5,
    WLR_SIGNING_ERROR = 6,
    WLR_UNSAFE_EXPRESSION = 7,
    WLR_MALFORMED = 8,
    WLR_RUNTIME_NOT_STARTED = 9
} wlr_err_t;

typedef enum wlr_license_type
{
    WLR_SIGNED_CODE_MODE = 0,
    WLR_LICENSE_OR_SIGNED_CODE_MODE = 1
} wlr_license_t;

typedef enum wlr_version_type
{
    WLR_VERSION_1 = 0
} wlr_version_t;

typedef enum wlr_signing_configuration_type
{
    WLR_ENABLE_CODE_SIGNING = 0,
    WLR_ENABLE_CODE_SIGNING_EXCEPT_EXPRESSION_API = 1,
    WLR_DISABLE_CODE_SIGNING = 2
} wlr_signing_conf_t;

typedef enum wlr_containment_type
{
    WLR_CONTAINED = 0,
    WLR_UNCONTAINED = 1
} wlr_containment_t;

typedef struct wlr_runtime_configuration
{
    mint argumentCount;
    char **arguments;
    wlr_containment_t containmentSetting;
} wlr_runtime_conf;

void wlr_InitializeRuntimeConfiguration(wlr_runtime_conf *configuration)
{
    if (configuration == NULL)
    {
        return;
    }

    configuration->argumentCount = 0;
    configuration->arguments = NULL;
    configuration->containmentSetting = WLR_CONTAINED;
}

wlr_err_t wlr_StartRuntime(wlr_version_t version, wlr_license_t licenseType, char *layoutDirectory, wlr_runtime_conf *configuration)
{
    /* The expression API and code signing are only enabled on certain platforms. On unsupported platforms, this function will
        fail and return WLR_RUNTIME_NOT_STARTED. This function will also fail and return WLR_RUNTIME_NOT_STARTED if the
        kernel is being built as a monolithic executable instead of a dynamic library. */
#if defined(EXPRESSION_API_AND_CODE_SIGNING_SUPPORTED) && (defined(NT) || defined(MATHKERNEL_DLL))
    wlr_runtime_conf *processedConfiguration;
    wlr_runtime_conf defaultConfiguration;

    /* fail if the expression API abort system is not supported */
    if (!checkAPIAbortSupported())
    {
        return WLR_RUNTIME_NOT_STARTED;
    }

    /* fail if the runtime has been closed */
    if (expressionAPIClosed)
    {
        return WLR_RUNTIME_NOT_STARTED;
    }

    /* if the runtime and expression API have already been successfully started up, return WLR_SUCCESS */
    if (expressionAPIMode)
    {
        return WLR_SUCCESS;
    }

    processedConfiguration = configuration;
    if (processedConfiguration == NULL)
    {
        (void)wlr_InitializeRuntimeConfiguration(&defaultConfiguration);
        processedConfiguration = &defaultConfiguration;
    }

    if (version != WLR_VERSION_1)
    {
        return WLR_RUNTIME_NOT_STARTED;
    }

    if (
        (layoutDirectory == NULL) ||
        (strlen(layoutDirectory) == 0))
    {
        return WLR_RUNTIME_NOT_STARTED;
    }

    if (processedConfiguration->argumentCount < 0)
    {
        return WLR_RUNTIME_NOT_STARTED;
    }

    if (
        (processedConfiguration->argumentCount > 0) &&
        (processedConfiguration->arguments == NULL))
    {
        return WLR_RUNTIME_NOT_STARTED;
    }

    /* on Unix operating systems, these functions are usually called by the kernel loader, which is not present in the
        expression API context, so we call them ourselves */
#ifndef NT
    /* initialization for $RootDirectory */
    (void)setRootDirectory("/");
    /* initialization for $TemporaryDirectory */
    (void)setDefaultTemporaryDirectory("/tmp");
#endif

    /* Enable expression API mode. This affects behavior throughout the system, but primarily it will cause the kernel main
        function to return rather than infinitely loop. Expression API functions will fail and return an error (if possible)
        if this variable is not set to TRUE. */
    expressionAPIMode = TRUE;

    /* in certain situations, it makes sense to force the kernel to use code signing (by setting codeSigningEnabled to TRUE),
        so that an end user's license is not consumed */
    if (licenseType == WLR_SIGNED_CODE_MODE)
    {
        codeSigningEnabled = TRUE;
    }

    /* override the default behavior for determining $TopDirectory and $InstallationDirectory (so that they are set to
        layoutDirectory instead) by setting topDirectory */
    (void)strncpy(topDirectory, layoutDirectory, TOP_DIRECTORY_SIZE - 1);
    topDirectory[TOP_DIRECTORY_SIZE - 1] = '\0';

    /* if containmentSetting is WLR_CONTAINED, disable initialization from $BaseDirectory and $UserBaseDirectory, and
        cause the paclet manager to enter -layoutpaclets mode so that only paclets inside the layout are used */
    if (processedConfiguration->containmentSetting == WLR_CONTAINED)
    {
        noInitialization = TRUE;
        layoutPaclets = TRUE;
    }

#ifdef NT
    /* on Windows, various callback functions are expected to be set by the kernel loader */
    /* if the kernel needs to be started using wlr_StartRuntime, there is no kernel loader,
        so we need to set the callback functions ourself */
    (void)defaultInitializeCallbacks();
#endif

    /* start the kernel */
    (void)kernel_dll_main((int)processedConfiguration->argumentCount, processedConfiguration->arguments);

    if (codeSigningEnabled)
    {
        /* fail if we are in signed code mode, but the OpenSSL library libcrypto was not successfully loaded */
        if (libcryptoObject == NULL)
        {
            /* the argument TRUE tells closeRuntime to allow the runtime to be restarted */
            (void)closeRuntime(TRUE);
            return WLR_RUNTIME_NOT_STARTED;
        }

        /* register a basic set of symbols that can be evaluated in the expression API when there is no license present,
            without the user having to register use them */
        if (wlr_RegisterSymbols(basicSymbolsSignature, sizeof(basicSymbols) / sizeof(char *), basicSymbols) != WLR_SUCCESS)
        {
            (void)closeRuntime(TRUE);
            return WLR_RUNTIME_NOT_STARTED;
        }
    }

    /* The PageWidth and PageHeight for the "stdout" stream should be Infinity by default. Generally, these values make
        the most sense for the expression API use case, but users can still change them if they wish to do so. In
        addition, calling SetOptions on this stream will automatically set the value of its CharacterEncoding option to
        "Unicode." The implementation of SetOptions is hardcoded to behave this way for any stream that the expression API
        will intercept output to. This is necessary because the "Unicode" encoding tells the kernel to leave output in
        UTF-8M, which is the encoding the expression API expects when it intercepts output to the stream. */
    apiEvaluate(
        E(
            Symbol("SetOptions"),
            String("stdout"),
            E(steRule, stePageWidth, steInfinity),
            E(steRule, stePageHeight, steInfinity)));

    return WLR_SUCCESS;
#else
    return WLR_RUNTIME_NOT_STARTED;
#endif
}

void wlr_CloseRuntime(void)
{
    /* the argument FALSE tells closeRuntime to NOT allow the runtime to be restarted */
    (void)closeRuntime(FALSE);
}

wlr_err_t wlr_RegisterSignature(wlr_signature_t signature)
{
    struct hash_data encrypted_hash;

    (void)memcpy(encrypted_hash.data, signature, sizeof(unsigned char) * WLR_SIGNATURE_SIZE);

    encrypted_hash.size = WLR_SIGNATURE_SIZE;

    if (loadEncryptedHash(&encrypted_hash))
    {
        return WLR_SUCCESS;
    }
    else
    {
        return WLR_MISCELLANEOUS_ERROR;
    }
}

wlr_err_t wlr_RegisterSignatureFile(char *fileName)
{
    if (loadSignatureFile(fileName))
    {
        return WLR_SUCCESS;
    }
    else
    {
        return WLR_MISCELLANEOUS_ERROR;
    }
}

wlr_err_t wlr_RegisterSymbols(wlr_signature_t signature, mint symbolNameCount, char **symbolNames)
{
    wlr_expr symbol;

    char *symbolsString;

    wlr_err_t error;

    mint index;

    mbool symbolsStringVerified;

    if (symbolNameCount <= 0)
    {
        return WLR_MISCELLANEOUS_ERROR;
    }

    for (index = 0; index < symbolNameCount; index++)
    {
        if (!isValidFullyQualifiedSymbolName(symbolNames[index]))
        {
            return WLR_MISCELLANEOUS_ERROR;
        }
    }

    /* register the provided signature so that we can check if the symbols are appropriately signed */
    error = wlr_RegisterSignature(signature);
    if (error != WLR_SUCCESS)
    {
        return error;
    }

    /* the signature for a set of symbols is based off a string formatted from their symbol names,
        so generate that string now */
    symbolsString = getSymbolsString(symbolNameCount, symbolNames);
    if (symbolsString == NULL)
    {
        return WLR_MISCELLANEOUS_ERROR;
    }

    /* check if the symbols string is appropriately signed */
    /* the second argument IGNORE_SIGNING_MODE tells verifyString to attempt to verify the string even if the
        kernel is not in signed code mode */
    symbolsStringVerified = verifyString(symbolsString, IGNORE_SIGNING_MODE);

    (void)Free(symbolsString);

    if (!symbolsStringVerified)
    {
        return WLR_MISCELLANEOUS_ERROR;
    }

    /* add the provided symbols to the table of symbols that can be evaluated using the expression API */
    for (index = 0; index < symbolNameCount; index++)
    {
        symbol = wlr_Symbol(symbolNames[index]);
        if (!SymbolQ(symbol))
        {
            return WLR_MISCELLANEOUS_ERROR;
        }

        (void)HashTable_GetOrAdd(apiSymbols, VoidPointer(symbol), NULL);
    }

    return WLR_SUCCESS;
}

void wlr_ConfigureCodeSigning(wlr_signing_conf_t signingConfiguration)
{
#ifndef ENABLE_CODE_SIGNING_SECURE_MODE
    switch (signingConfiguration)
    {
    case WLR_ENABLE_CODE_SIGNING:
        codeSigningEnabled = TRUE;
        apiProtectionMode = TRUE;
        break;
    case WLR_ENABLE_CODE_SIGNING_EXCEPT_EXPRESSION_API:
        codeSigningEnabled = TRUE;
        apiProtectionMode = FALSE;
        break;
    case WLR_DISABLE_CODE_SIGNING:
        codeSigningEnabled = FALSE;
        apiProtectionMode = FALSE;
        break;
    }

    (void)LoadOpenSSL();

    (void)wlr_RegisterSymbols(basicSymbolsSignature, sizeof(basicSymbols) / sizeof(char *), basicSymbols);
#endif
}

mint wlr_MemoryInUse(void)
{
    struct expression_pool *iterator;

    mint result;

    result = MemoryInUse();

    for (
        iterator = currentExpressionPool;
        iterator != NULL;
        iterator = iterator->parentExpressionPool)
    {
        result -= HashTable_ByteCount(iterator->expressions, NULL);
#ifdef DEBUG
        /* hacky fix to try to account for some of the memory used by the hash table that is not represented in
            HashTable_ByteCount in debug builds */
        result -= HashTable_Length(iterator->expressions) * 48;
#endif
    }

    return result;
}

wlr_expr wlr_Error(wlr_err_t errorType)
{
    wlr_expr result;

    result = E(steExpressionAPIError, MInteger(errorType));
    if (!wlr_ErrorQ(result))
    {
        return wlr_Error(WLR_MALFORMED);
    }

    return result;
}

mbool wlr_ErrorQ(wlr_expr expression)
{
    wlr_expr errorTypeExpr;

    mint errorType;

    if (Head(expression) != steExpressionAPIError)
    {
        return FALSE;
    }

    /* for the moment, error expressions can only contain one element */
    /* however, they may contain more metadata in the future, such as the name of the API function that
        produced the error expression */
    if (Length(expression) != 1)
    {
        return FALSE;
    }

    errorTypeExpr = First(expression);
    if (!MIntegerQ(errorTypeExpr))
    {
        return FALSE;
    }

    /* WLR_SUCCESS is the minimum error type, and WLR_RUNTIME_NOT_STARTED is the maximum error type,
        so the error expression is malformed if its error type is not in this interval */
    errorType = MIntegerData(errorTypeExpr);
    if (
        (errorType < WLR_SUCCESS) ||
        (errorType > WLR_RUNTIME_NOT_STARTED))
    {
        return FALSE;
    }

    return TRUE;
}

wlr_err_t wlr_ErrorType(wlr_expr errorExpression)
{
    if (!wlr_ErrorQ(errorExpression))
    {
        return WLR_MALFORMED;
    }

    return (wlr_err_t)MIntegerData(First(errorExpression));
}

mbool wlr_NumberQ(wlr_expr expression)
{
    return NumberQ(expression);
}

wlr_num_t wlr_NumberType(wlr_expr numberExpression)
{
    if (OverflowQ(numberExpression))
    {
        return WLR_OVERFLOW;
    }
    else if (UnderflowQ(numberExpression))
    {
        return WLR_UNDERFLOW;
    }

    switch (Type(numberExpression))
    {
    case TMINTEGER:
        return WLR_MACHINE_INTEGER;
    case TINTEGER:
        return WLR_BIG_INTEGER;
    case TMREAL:
        return WLR_MACHINE_REAL;
    case TREAL:
        return WLR_BIG_REAL;
    case TCOMPLEX:
        return WLR_COMPLEX;
    case TRATIONAL:
        return WLR_RATIONAL;
    default:
        return WLR_NOT_A_NUMBER;
    }
}

wlr_expr wlr_Integer(mint value)
{
    wlr_expr result;

    result = MInteger(value);

    if (!MIntegerQ(result))
    {
        return wlr_Error(WLR_ALLOCATION_ERROR);
    }

    return result;
}

wlr_err_t wlr_IntegerConvert(wlr_expr numberExpression, mint *result)
{
    wlr_expr integerExpr;

    if (wlr_ErrorQ(numberExpression))
    {
        return WLR_ERROR_EXPRESSION;
    }

    integerExpr = toMachineInteger(numberExpression);
    if (!MIntegerQ(integerExpr))
    {
        SAFE_POINTER_SET(result, -1);

        if (wlr_ErrorQ(integerExpr))
        {
            return wlr_ErrorType(integerExpr);
        }
        else
        {
            return WLR_MISCELLANEOUS_ERROR;
        }
    }

    SAFE_POINTER_SET(result, MIntegerData(integerExpr));

    return WLR_SUCCESS;
}

wlr_expr wlr_Real(mreal value)
{
    wlr_expr realExpr;

    /* test if the value is infinite or NaN */
    if (TESTFPOV(value))
    {
        if (isNan(value))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
        else if (isPlusInf(value))
        {
            return sInfinity;
        }
        else if (isMinusInf(value))
        {
            return sMInfinity;
        }
        else
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
    }

    realExpr = MReal(value);
    if (!MRealQ(realExpr))
    {
        return wlr_Error(WLR_ALLOCATION_ERROR);
    }

    return realExpr;
}

/*
    Get a real value from a number-type expression
    @param numberExpression The number-type expression or the expressions DirectedInfinity[1] or DirectedInfinity[-1]
    @param[out] result Pointer to write the result to
    @returns Whether the function succeeded
    @retval WLR_SUCCESS The function succeeded
    @retval WLR_UNEXPECTED_TYPE The expression @p numberExpression was not a number or error expression
    @retval WLR_ERROR_EXPRESSION The expression @p numberExpression was an error expression
    @retval WLR_MISCELLANEOUS_ERROR A miscellaneous error occurred
    @retval WLR_ALLOCATION_ERROR An allocation error occurred
    @retval WLR_RUNTIME_NOT_STARTED The runtime has not been started using wlr_StartRuntime
    @post If the function is not successful, the result is -1
    @post If the expression is a machine integer, the result is its integer value cast to a machine real
    @post If the expression is a big integer, the result is its value rounded to the nearest machine real
    @post If the expression is a machine real, the result is its real value
    @post If the expression is a big real, the result is its value rounded to the nearest machine real
    @post If the expression is a rational, the result is its value rounded to the nearest machine real
    @post If the expression is DirectedInfinity[1], the result is the real value Inf
    @post If the expression is DirectedInfinity[-1], the result is the real value -Inf
    @remarks If the expression is a complex number, we consider its real part
*/
wlr_err_t wlr_RealConvert(wlr_expr numberExpression, mreal *result)
{
    wlr_expr realExpr;

    if (wlr_ErrorQ(numberExpression))
    {
        return WLR_ERROR_EXPRESSION;
    }

    realExpr = toMachineReal(numberExpression);

    if (SameQ(realExpr, sInfinity))
    {
        SAFE_POINTER_SET(result, dInfinity);
        return WLR_SUCCESS;
    }
    else if (SameQ(realExpr, sMInfinity))
    {
        SAFE_POINTER_SET(result, -dInfinity);
        return WLR_SUCCESS;
    }

    if (!MRealQ(realExpr))
    {
        SAFE_POINTER_SET(result, -1);
        if (wlr_ErrorQ(realExpr))
        {
            /* return the error type returned by toMachineReal (see its documentation for more information) */
            return wlr_ErrorType(realExpr);
        }
        else
        {
            return WLR_MISCELLANEOUS_ERROR;
        }
    }

    SAFE_POINTER_SET(result, MRealData(realExpr));

    return WLR_SUCCESS;
}

wlr_expr wlr_Complex(wlr_expr realPart, wlr_expr imaginaryPart)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(realPart);
    RETURN_IF_ERROR_EXPRESSION(imaginaryPart);

    if (!RealValuedNumberQ(realPart))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    if (!RealValuedNumberQ(imaginaryPart))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = Complex(realPart, imaginaryPart);
    if (!ComplexQ(result))
    {
        return wlr_Error(WLR_ALLOCATION_ERROR);
    }

    return result;
}

wlr_expr wlr_RealPart(wlr_expr complexExpression)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(complexExpression);

    if (!ComplexQ(complexExpression))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = Re(complexExpression);
    if (!RealValuedNumberQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_ImaginaryPart(wlr_expr complexExpression)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(complexExpression);

    if (!ComplexQ(complexExpression))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = Im(complexExpression);
    if (!RealValuedNumberQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_Rational(wlr_expr numerator, wlr_expr denominator)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(numerator);
    RETURN_IF_ERROR_EXPRESSION(denominator);

    if (!IntegerQ(numerator))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    if (!IntegerQ(denominator))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    if (SameQ(denominator, sZero))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    result = Rational(numerator, denominator);
    if (
        (!RationalQ(result)) &&
        (!IntegerQ(result)))
    {
        return wlr_Error(WLR_ALLOCATION_ERROR);
    }

    return result;
}

wlr_expr wlr_Numerator(wlr_expr rationalExpression)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(rationalExpression);

    if (IntegerQ(rationalExpression))
    {
        return rationalExpression;
    }

    if (!RationalQ(rationalExpression))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = Num(rationalExpression);
    if (!IntegerQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_Denominator(wlr_expr rationalExpression)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(rationalExpression);

    if (IntegerQ(rationalExpression))
    {
        return sOne;
    }

    if (!RationalQ(rationalExpression))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = Den(rationalExpression);
    if (!IntegerQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_NumberFromString(char *numberString)
{
    wlr_expr result;

    char *trimmedString;

    char *unsignedString;

    mbool isNegative;

    if (numberString == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    trimmedString = trimString(numberString);
    if (trimmedString == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    /* check if the number string is negative, and remove the leading minus sign if it is because
        Token_To_Expr (called next) will fail otherwise */
    /* NOTE: the byte '-' cannot lead a UTF8 multibyte character, so we do not have to worry about
        creating some kind of invalid string */
    unsignedString = trimmedString;
    isNegative = (unsignedString[0] == '-');
    if (isNegative)
    {
        unsignedString++;
    }

    /* check that the string is valid before passing it to Token_To_Expr */
    if (!StringQ(wlr_RawString(unsignedString)))
    {
        (void)Free(trimmedString);
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    /* use Token_To_Expr to create a integer or real expression from a number string */
    result = Token_To_Expr(unsignedString);

    (void)Free(trimmedString);

    if (!IntegerOrRealQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    /* if the number string was negative, make the new expression negative */
    if (isNegative)
    {
        result = Minus(result);
        if (!IntegerOrRealQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
    }

    return result;
}

wlr_err_t wlr_StringFromNumber(wlr_expr numberExpression, char **result)
{
    wlr_expr processedNumberExpr;

    wlr_expr stringExpr;

    if (wlr_ErrorQ(numberExpression))
    {
        return WLR_ERROR_EXPRESSION;
    }

    if (ComplexQ(numberExpression))
    {
        processedNumberExpr = Re(numberExpression);
    }
    else
    {
        processedNumberExpr = numberExpression;
    }

    if (
        (!RealValuedNumberQ(processedNumberExpr)) ||
        (AnyflowQ(processedNumberExpr)))
    {
        SAFE_POINTER_SET(result, NULL);
        return WLR_UNEXPECTED_TYPE;
    }

    /* the third argument is set to FALSE to tell ExprToString to use the current encoding */
    stringExpr = ExprToString(E(steDecimalForm, processedNumberExpr), FALSE);
    if (!StringQ(stringExpr))
    {
        SAFE_POINTER_SET(result, NULL);
        return WLR_MISCELLANEOUS_ERROR;
    }

    if (wlr_StringData(stringExpr, result, NULL) != WLR_SUCCESS)
    {
        SAFE_POINTER_SET(result, NULL);
        return WLR_MISCELLANEOUS_ERROR;
    }

    return WLR_SUCCESS;
}

wlr_expr wlr_Rule(wlr_expr leftHandSide, wlr_expr rightHandSide)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(leftHandSide);
    RETURN_IF_ERROR_EXPRESSION(rightHandSide);

    result = Rule(leftHandSide, rightHandSide);
    if (!RuleQ(result))
    {
        return wlr_Error(WLR_ALLOCATION_ERROR);
    }

    return result;
}

mbool wlr_RuleQ(wlr_expr expression)
{
    return RuleQ(expression);
}

mbool wlr_ListQ(wlr_expr expression)
{
    return aListQ(expression);
}

mbool wlr_AssociationQ(wlr_expr expression)
{
    return AssociationQ(expression);
}

mint wlr_Length(wlr_expr expression)
{
    if (wlr_ErrorQ(expression))
    {
        return 0;
    }

    return aLength(expression, 0);
}

wlr_expr wlr_Part(wlr_expr expression, mint index)
{
    wlr_expr partExpression;

    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(expression);

    if (index == 0)
    {
        return TopLevelHead(expression);
    }

    if (
        (index < 0) ||
        (index > wlr_Length(expression)))
    {
        return wlr_Error(WLR_OUT_OF_BOUNDS);
    }

    partExpression = E(stePart, expression, MInteger(index));

    result = oPart(partExpression);
    if (SameQ(result, partExpression))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_First(wlr_expr expression)
{
    return wlr_Part(expression, 1);
}

wlr_expr wlr_Last(wlr_expr expression)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(expression);

    if (wlr_Length(expression) == 0)
    {
        return wlr_Error(WLR_OUT_OF_BOUNDS);
    }

    return wlr_Part(expression, wlr_Length(expression));
}

wlr_expr wlr_Rest(wlr_expr expression)
{
    wlr_expr partExpression;

    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(expression);

    if (wlr_Length(expression) == 0)
    {
        return wlr_Error(WLR_OUT_OF_BOUNDS);
    }

    partExpression = E(stePart, expression, E(steSpan, MInteger(2), steAll));

    result = oPart(partExpression);
    if (SameQ(result, partExpression))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_Head(wlr_expr expression)
{
    return wlr_Part(expression, 0);
}

wlr_expr wlr_ReplacePart(wlr_expr expression, mint index, wlr_expr newPart)
{
    wlr_expr replacePartExpression;

    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(expression);
    RETURN_IF_ERROR_EXPRESSION(newPart);

    if (ByteOrNumericArrayQ(expression))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    if (
        (index < 0) ||
        (index > wlr_Length(expression)))
    {
        return wlr_Error(WLR_OUT_OF_BOUNDS);
    }

    if (
        (!NormalQ(expression)) &&
        (index == 0))
    {
        return wlr_Error(WLR_OUT_OF_BOUNDS);
    }

    replacePartExpression = E(steReplacePart, expression, Rule(MInteger(index), newPart));

    result = oReplacePart(replacePartExpression);
    if (SameQ(result, replacePartExpression))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_Symbol(char *symbolName)
{
    struct context_mark_tests contextMarkTests;

    wlr_expr symbolNameExpr;

    wlr_expr result;

    if (!testContextMarks(&contextMarkTests, symbolName))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    /* strtosym doesn't care if there is no base symbol name, but we do, so check that now */
    if (contextMarkTests.endsWithContextMark)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    symbolNameExpr = wlr_String(symbolName);
    if (!StringQ(symbolNameExpr))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    result = strtosym(StringData(symbolNameExpr), TRUE, IGNORE_CONTEXT_ALIASES);
    if (result == EFAIL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_GlobalSymbol(char *baseSymbolName)
{
    return wlr_ContextSymbol(globalContextStringData, baseSymbolName);
}

wlr_expr wlr_SystemSymbol(char *baseSymbolName)
{
    return wlr_ContextSymbol(systemContextStringData, baseSymbolName);
}

wlr_expr wlr_ContextSymbol(char *symbolContext, char *baseSymbolName)
{
    wlr_expr result;

    struct context_mark_tests contextMarkTests;

    char *fullName;

    if (!testContextMarks(&contextMarkTests, symbolContext))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    /* return an error if symbolContext is relative or does not end with a context mark */
    if (
        (contextMarkTests.beginsWithContextMark) ||
        (!contextMarkTests.endsWithContextMark))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    if (!testContextMarks(&contextMarkTests, baseSymbolName))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    /* return an error if baseSymbolName contains a context mark */
    if (contextMarkTests.containsContextMark)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    fullName = (char *)Malloc(sizeof(char) * (strlen(symbolContext) + strlen(baseSymbolName) + 1));
    if (fullName == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    (void)strcpy(fullName, symbolContext);
    (void)strcat(fullName, baseSymbolName);

    result = wlr_Symbol(fullName);
    if (!SymbolQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    (void)Free(fullName);

    return result;
}

wlr_expr wlr_SymbolName(wlr_expr symbol)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(symbol);

    if (!SymbolQ(symbol))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = SName(symbol);
    if (!StringQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_SymbolContext(wlr_expr symbol)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(symbol);

    if (!SymbolQ(symbol))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = SContext(symbol);
    if (!StringQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_exprbag wlr_ExpressionBag(void)
{
    return ExprBagInit();
}

wlr_err_t wlr_AddExpression(wlr_exprbag expressionBag, wlr_expr expression)
{
    if (expressionBag == NULL)
    {
        return WLR_MISCELLANEOUS_ERROR;
    }

    if (wlr_ErrorQ(expression))
    {
        return WLR_ERROR_EXPRESSION;
    }

    ExprBagStuff((exprbag)expressionBag, expression);

    return WLR_SUCCESS;
}

mint wlr_ExpressionBagLength(wlr_exprbag expressionBag)
{
    if (expressionBag == NULL)
    {
        return 0;
    }

    return ExprBagLength((exprbag)expressionBag);
}

wlr_expr wlr_ExpressionBagToExpression(wlr_exprbag expressionBag, wlr_expr expressionHead)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(expressionHead);

    if (expressionBag == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    result = ExprBagToExpr((exprbag)expressionBag, expressionHead);
    if (!NormalQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

void wlr_ReleaseExpressionBag(wlr_exprbag expressionBag)
{
    if (expressionBag == NULL)
    {
        return;
    }

    ExprBagFree((exprbag)expressionBag);
}

wlr_expr wlr_String(char *string)
{
    if (string == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return wlr_StringFromData(string, strlen(string));
}

wlr_expr wlr_StringFromData(char *utf8Data, mint utf8DataLength)
{
    return stringFromData(utf8Data, utf8DataLength, FALSE);
}

wlr_expr wlr_RawString(char *string)
{
    if (string == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return wlr_RawStringFromData(string, strlen(string));
}

wlr_expr wlr_RawStringFromData(char *utf8Data, mint utf8DataLength)
{
    return stringFromData(utf8Data, utf8DataLength, TRUE);
}

wlr_err_t wlr_StringData(wlr_expr expression, char **resultData, mint *resultLength)
{
    utf8str stringData;

    char *newString;

    if (!StringQ(expression))
    {
        SAFE_POINTER_SET(resultData, NULL);
        SAFE_POINTER_SET(resultLength, -1);
        if (wlr_ErrorQ(expression))
        {
            return WLR_ERROR_EXPRESSION;
        }
        else
        {
            return WLR_UNEXPECTED_TYPE;
        }
    }

    stringData = String_to_utf8str(expression);

    if (stringData == NULL)
    {
        SAFE_POINTER_SET(resultData, NULL);
        SAFE_POINTER_SET(resultLength, -1);
        return WLR_ALLOCATION_ERROR;
    }

    if (stringData->nbytes < 0)
    {
        (void)utf8str_free(stringData);
        SAFE_POINTER_SET(resultData, NULL);
        SAFE_POINTER_SET(resultLength, -1);
        return WLR_ALLOCATION_ERROR;
    }

    if (
        (resultLength == NULL) &&
        (strlen((char *)stringData->string) < (size_t)stringData->nbytes))
    {
        (void)utf8str_free(stringData);
        SAFE_POINTER_SET(resultData, NULL);
        SAFE_POINTER_SET(resultLength, -1);
        return WLR_MISCELLANEOUS_ERROR;
    }

    newString = (char *)Malloc(sizeof(char) * (stringData->nbytes + 1));
    if (newString == NULL)
    {
        (void)utf8str_free(stringData);
        SAFE_POINTER_SET(resultData, NULL);
        SAFE_POINTER_SET(resultLength, -1);
        return WLR_ALLOCATION_ERROR;
    }

    if (stringData->nbytes > 0)
    {
        (void)memcpy(newString, stringData->string, sizeof(char) * stringData->nbytes);
    }

    newString[stringData->nbytes] = '\0';

    if (!StringQ(wlr_StringFromData(newString, stringData->nbytes)))
    {
        (void)Free(newString);
        (void)utf8str_free(stringData);
        SAFE_POINTER_SET(resultData, NULL);
        SAFE_POINTER_SET(resultLength, -1);
        return WLR_ALLOCATION_ERROR;
    }

    SAFE_POINTER_SET(resultData, newString);
    SAFE_POINTER_SET(resultLength, stringData->nbytes);

    (void)utf8str_free(stringData);

    return WLR_SUCCESS;
}

wlr_expr wlr_ExpressionFromNumericArray(MNumericArray numericArray, wlr_expr expressionHead)
{
    wlr_expr result;

    MNumericArray numericArrayClone;

    RETURN_IF_ERROR_EXPRESSION(expressionHead);

    if (numericArray == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    /* get a copy of numericArray in numericArrayClone if the new expression will depend on provided
        MNumericArray data */
    if (
        (expressionHead == steNumericArray) ||
        (expressionHead == steByteArray))
    {
        numericArrayClone = NULL;
        if (MNumericArray_clone(numericArray, &numericArrayClone) != A_OK)
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
    }

    if (expressionHead == steNumericArray)
    {
        result = CreateNumericArrayUsingData(steNumericArray, numericArrayClone);
        if (!NumericArrayQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
    }
    else if (expressionHead == steByteArray)
    {
        result = CreateByteArrayUsingData(numericArrayClone);
        if (!ByteArrayQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
    }
    else if (expressionHead == steList)
    {
        /* this check is done because we do not want to generate a list expression that cannot be converted into a numeric array expression */
        /* alternatively, we could use CreateNumericArrayUsingData, which also does these checks */
        if (
            (MNumericArray_getRank(numericArray) == 0) ||
            (MNumericArray_getNumberOfElements(numericArray) == 0))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }

        /* this function turns MNumericArray data into a normal expression list or packed array */
        result = MNumericArrayNormal(numericArray);

        /* we use wlr_ListQ because it returns TRUE for both normal expression lists and packed arrays */
        if (!wlr_ListQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
    }
    else
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_err_t wlr_NumericArrayData(wlr_expr expression, MNumericArray *result)
{
    wlr_expr numericArrayExpr;

    MNumericArray numericArray;

    MNumericArray numericArrayClone;

    numericArrayExpr = expression;

    if (wlr_ErrorQ(expression))
    {
        SAFE_POINTER_SET(result, NULL);
        return WLR_ERROR_EXPRESSION;
    }

    if (wlr_ListQ(numericArrayExpr))
    {
        numericArrayExpr = listToNumericArrayExpression(expression);
        if (!NumericArrayQ(numericArrayExpr))
        {
            SAFE_POINTER_SET(result, NULL);
            return WLR_UNEXPECTED_TYPE;
        }
    }

    if (!ByteOrNumericArrayQ(numericArrayExpr))
    {
        SAFE_POINTER_SET(result, NULL);
        return WLR_UNEXPECTED_TYPE;
    }

    numericArray = NumericArray_getData(numericArrayExpr);
    numericArrayClone = NULL;
    if (MNumericArray_clone(numericArray, &numericArrayClone) != A_OK)
    {
        SAFE_POINTER_SET(result, NULL);
        return WLR_ALLOCATION_ERROR;
    }

    SAFE_POINTER_SET(result, numericArrayClone);

    return WLR_SUCCESS;
}

wlr_expr_t wlr_ExpressionType(wlr_expr expression)
{
    if (wlr_ErrorQ(expression))
    {
        return WLR_ERROR;
    }

    switch (Type(expression))
    {
    case TMINTEGER:
    case TINTEGER:
    case TMREAL:
    case TREAL:
    case TCOMPLEX:
    case TRATIONAL:
        return WLR_NUMBER;
    case TNORMAL:
        return WLR_NORMAL;
    case TSYMBOL:
        return WLR_SYMBOL;
    case TSTRING:
        return WLR_STRING;
    case TRAW:
        switch (RawType(expression))
        {
        case RPACKED:
            return WLR_PACKED_ARRAY;
        case RNUMERICARRAY:
            return WLR_NUMERIC_ARRAY;
        case RBDDINSTANCE:
            return WLR_BOOLEAN_FUNCTION;
        case RGRAPHCOMP:
            return WLR_GRAPH;
        case RASSOCIATION:
            return WLR_ASSOCIATION;
        case RDISPATCHTABLE:
            return WLR_DISPATCH;
        case RREGION:
            return WLR_REGION;
        default:
            return WLR_OTHER;
        }
    default:
        return WLR_OTHER;
    }
}

wlr_expr wlr_Normalize(wlr_expr expression)
{
    wlr_expr result;

    wlr_expr head;

    wlr_expr part;

    mint length;

    mint index;

    RETURN_IF_ERROR_EXPRESSION(expression);

    switch (Type(expression))
    {
    case TMINTEGER:
    case TINTEGER:
    case TMREAL:
    case TREAL:
    case TSTRING:
    case TSYMBOL:
        result = expression;
        break;

    case TNORMAL:
        head = wlr_Normalize(NormalHead(expression));
        if (wlr_ErrorQ(head))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }

        length = Length(expression);

        result = iCreateHeaded(length, head);
        if (!NormalQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }

        for (index = 1; index <= length; index++)
        {
            part = wlr_Normalize(Elem(expression, index));
            if (wlr_ErrorQ(part))
            {
                return wlr_Error(WLR_MISCELLANEOUS_ERROR);
            }

            SetElem(result, index, part);
        }

        break;

    case TRATIONAL:
        result = ToNormalRational(expression);
        if (!NormalQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
        break;

    case TCOMPLEX:
        result = ToNormalComplex(expression);
        if (!NormalQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
        break;

    case TRAW:
        /* RawExprSerialization turns raw expressions into non-raw expressions */
        result = RawExprSerialization(expression, FALSE);
        if (RawQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }

        /* call wlr_Normalize so that the other branches of this switch statement can normalize the
            now non-raw expression */
        result = wlr_Normalize(result);
        break;

    default:
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_ExpressionFromIntegerArray(mint arrayLength, mint *array, wlr_expr expressionHead)
{
    wlr_expr result;

    MNumericArray numericArray;

    mint *integerArray;

    mint dimensions;

    dimensions = arrayLength;

    if (dimensions == 0)
    {
        if (!SameQ(expressionHead, steByteArray))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }

        result = CreateByteArrayUsingDataArray(0, NULL);
        if (!ByteArrayQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }

        return result;
    }

    if (array == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    integerArray = (mint *)Malloc(sizeof(mint) * dimensions);
    if (integerArray == NULL)
    {
        return wlr_Error(WLR_ALLOCATION_ERROR);
    }

    (void)memcpy(integerArray, array, sizeof(mint) * dimensions);

    numericArray = MNumericArray_newUsingDataArray(MINT_NUMERIC_ARRAY_TYPE, 1, &dimensions, integerArray);
    if (numericArray == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    if (SameQ(expressionHead, steByteArray))
    {
        /* convert numericArray into a list in order to dispose of the MNumericArray type */
        result = wlr_ExpressionFromNumericArray(numericArray, steList);
        if (!wlr_ListQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }

        /* if the list does not only contain integers 0-255, listToByteArray will not return a byte array expression */
        result = listToByteArray(result, dimensions);
        if (!ByteArrayQ(result))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }
    }
    else
    {
        result = wlr_ExpressionFromNumericArray(numericArray, expressionHead);
    }

    /* this is safe because wlr_ExpressionFromNumericArray uses a copy of the MNumericArray for numeric array expressions */
    (void)MNumericArray_delete(numericArray);

    return result;
}

wlr_expr wlr_ExpressionFromRealArray(mint arrayLength, mreal *array, wlr_expr expressionHead)
{
    wlr_expr result;

    MNumericArray numericArray;

    mreal *realArray;

    mint dimensions;

    dimensions = arrayLength;

    if (dimensions == 0)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    if (array == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    realArray = (mreal *)Malloc(sizeof(mreal) * dimensions);
    if (realArray == NULL)
    {
        return wlr_Error(WLR_ALLOCATION_ERROR);
    }

    (void)memcpy(realArray, array, sizeof(mreal) * dimensions);

    numericArray = MNumericArray_newUsingDataArray(MREAL_NUMERIC_ARRAY_TYPE, 1, &dimensions, realArray);
    if (numericArray == NULL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    result = wlr_ExpressionFromNumericArray(numericArray, expressionHead);

    /* this is safe because wlr_ExpressionFromNumericArray uses a copy of the MNumericArray for numeric array expressions */
    (void)MNumericArray_delete(numericArray);

    return result;
}

wlr_err_t wlr_IntegerArrayData(wlr_expr expression, mint *resultLength, mint **resultArray)
{
    MNumericArray numericArray;

    type_t numericArrayType;

    wlr_err_t error;

    void *numericArrayData;

    mint *integerArray;

    mint *dimensions;

    mint index;

    if (wlr_ErrorQ(expression))
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_ERROR_EXPRESSION;
    }

    error = wlr_NumericArrayData(expression, &numericArray);
    if (error != WLR_SUCCESS)
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return error;
    }

    if (MNumericArray_getRank(numericArray) != 1)
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_UNEXPECTED_TYPE;
    }

    dimensions = MNumericArray_getDimensions(numericArray);
    if (dimensions == NULL)
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_MISCELLANEOUS_ERROR;
    }

    /* if the length of the MNumericData is zero, just return WLR_SUCCESS now */
    /* this should only happen if expression is a byte array expression */
    if (*dimensions == 0)
    {
        SAFE_POINTER_SET(resultLength, 0);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_SUCCESS;
    }

    /* reject non-integer numeric array types */
    numericArrayType = MNumericArray_getType(numericArray);
    switch (numericArrayType)
    {
    case MNumericArray_Type_Bit8:
    case MNumericArray_Type_UBit8:
    case MNumericArray_Type_Bit16:
    case MNumericArray_Type_UBit16:
    case MNumericArray_Type_Bit32:
    case MNumericArray_Type_UBit32:
    case MNumericArray_Type_Bit64:
        break;
    default:
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_UNEXPECTED_TYPE;
    }

    integerArray = (mint *)Malloc(sizeof(mint) * *dimensions);
    if (integerArray == NULL)
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_ALLOCATION_ERROR;
    }

    numericArrayData = MNumericArray_getDataPointer(numericArray);
    if (
        (numericArrayData == NULL) &&
        (*dimensions != 0))
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_MISCELLANEOUS_ERROR;
    }

    for (index = 0; index < *dimensions; index++)
    {
        switch (numericArrayType)
        {
        case MNumericArray_Type_Bit8:
            integerArray[index] = ((int8_t *)numericArrayData)[index];
            break;
        case MNumericArray_Type_UBit8:
            integerArray[index] = ((uint8_t *)numericArrayData)[index];
            break;
        case MNumericArray_Type_Bit16:
            integerArray[index] = ((int16_t *)numericArrayData)[index];
            break;
        case MNumericArray_Type_UBit16:
            integerArray[index] = ((uint16_t *)numericArrayData)[index];
            break;
        case MNumericArray_Type_Bit32:
            integerArray[index] = ((int32_t *)numericArrayData)[index];
            break;
        case MNumericArray_Type_UBit32:
            integerArray[index] = ((uint32_t *)numericArrayData)[index];
            break;
        case MNumericArray_Type_Bit64:
            integerArray[index] = ((int64_t *)numericArrayData)[index];
            break;
        }
    }

    SAFE_POINTER_SET(resultLength, *dimensions);
    SAFE_POINTER_SET(resultArray, integerArray);

    return WLR_SUCCESS;
}

wlr_err_t wlr_RealArrayData(wlr_expr expression, mint *resultLength, mreal **resultArray)
{
    MNumericArray numericArray;

    type_t numericArrayType;

    wlr_err_t error;

    void *numericArrayData;

    mreal *realArray;

    mint *dimensions;

    mint index;

    if (wlr_ErrorQ(expression))
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_ERROR_EXPRESSION;
    }

    error = wlr_NumericArrayData(expression, &numericArray);
    if (error != WLR_SUCCESS)
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return error;
    }

    if (MNumericArray_getRank(numericArray) != 1)
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_UNEXPECTED_TYPE;
    }

    dimensions = MNumericArray_getDimensions(numericArray);
    if (dimensions == NULL)
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_MISCELLANEOUS_ERROR;
    }

    /* this should only occur if the expression is a byte array, which wlr_RealArrayData rejects */
    if (*dimensions == 0)
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_UNEXPECTED_TYPE;
    }

    /* reject non-real numeric array types */
    numericArrayType = MNumericArray_getType(numericArray);
    switch (numericArrayType)
    {
    case MNumericArray_Type_Real32:
    case MNumericArray_Type_Real64:
        break;
    default:
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_UNEXPECTED_TYPE;
    }

    realArray = (mreal *)Malloc(sizeof(mreal) * *dimensions);
    if (realArray == NULL)
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_ALLOCATION_ERROR;
    }

    numericArrayData = MNumericArray_getDataPointer(numericArray);
    if (
        (numericArrayData == NULL) &&
        (*dimensions != 0))
    {
        SAFE_POINTER_SET(resultLength, -1);
        SAFE_POINTER_SET(resultArray, NULL);
        return WLR_MISCELLANEOUS_ERROR;
    }

    for (index = 0; index < *dimensions; index++)
    {
        switch (numericArrayType)
        {
        case MNumericArray_Type_Real32:
            realArray[index] = ((float *)numericArrayData)[index];
            break;
        case MNumericArray_Type_Real64:
            realArray[index] = ((double *)numericArrayData)[index];
            break;
        }
    }

    SAFE_POINTER_SET(resultLength, *dimensions);
    SAFE_POINTER_SET(resultArray, realArray);

    return WLR_SUCCESS;
}

mbool wlr_TrueQ(wlr_expr expression)
{
    return SameQ(expression, steTrue);
}

mbool wlr_SameQ(wlr_expr firstExpression, wlr_expr secondExpression)
{
    return SameQ(firstExpression, secondExpression);
}

wlr_expr wlr_GetValueFromKey(wlr_expr association, wlr_expr key)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(association);
    RETURN_IF_ERROR_EXPRESSION(key);

    if (!AssociationQ(association))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = Association_getValueByKey(association, key, NULL);
    if (result == EFAIL)
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_GetKeys(wlr_expr association)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(association);

    if (!AssociationQ(association))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = Association_getKeys(association, steList, KEYVAL_DEFAULT);
    if (!ListQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

wlr_expr wlr_GetValues(wlr_expr association)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(association);

    if (!AssociationQ(association))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = Association_getValues(association, steList, KEYVAL_DEFAULT);
    if (!ListQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    return result;
}

void wlr_CreateExpressionPool(void)
{
    currentExpressionPool = createExpressionPool(currentExpressionPool);
}

void wlr_ReleaseExpressionPool(void)
{
    struct expression_pool *newCurrentExpressionPool;

    /* only release the current expression pool if the pool has been created by the user (i.e., the current pool is not the root pool) */
    if (currentExpressionPool != rootExpressionPool)
    {
        newCurrentExpressionPool = currentExpressionPool->parentExpressionPool;

        (void)releaseExpressionPool(currentExpressionPool);

        currentExpressionPool = newCurrentExpressionPool;
    }
}

void wlr_ReleaseExpression(wlr_expr detachedExpression)
{
    (void)releaseExpression(rootExpressionPool, detachedExpression);
}

void wlr_MoveExpressionToParentPool(wlr_expr expression)
{
    if (currentExpressionPool->parentExpressionPool != NULL)
    {
        (void)moveExpression(currentExpressionPool->parentExpressionPool, expression);
    }
}

void wlr_DetachExpression(wlr_expr expression)
{
    if (currentExpressionPool != rootExpressionPool)
    {
        (void)moveExpression(rootExpressionPool, expression);
    }
}

wlr_err_t wlr_AddStdoutHandler(wlr_stdout_handler_t handlerFunction, void *contextData)
{
    return addHandler(apiOutputStringHandlers, (function_pointer_hash_t)handlerFunction, contextData);
}

wlr_err_t wlr_AddMessageHandler(wlr_message_handler_t handlerFunction, void *contextData)
{
    return addHandler(apiOutputMessageHandlers, (function_pointer_hash_t)handlerFunction, contextData);
}

void wlr_RemoveStdoutHandler(wlr_stdout_handler_t handlerFunction)
{
    (void)removeHandler(apiOutputStringHandlers, (function_pointer_hash_t)handlerFunction);
}

void wlr_RemoveMessageHandler(wlr_message_handler_t handlerFunction)
{
    (void)removeHandler(apiOutputMessageHandlers, (function_pointer_hash_t)handlerFunction);
}

void wlr_Abort(void)
{
    (void)triggerAPIAbort();
}

void wlr_ClearAbort(void)
{
    (void)clearAPIAbort();
}

wlr_expr wlr_Clone(wlr_expr expression)
{
    return expression;
}

void wlr_ReleaseAll(void)
{
    while (currentExpressionPool != rootExpressionPool)
    {
        (void)wlr_ReleaseExpressionPool();
    }

    Empty_HashTable(rootExpressionPool->expressions);
}

wlr_expr wlr_ParseExpression(wlr_expr inputStringExpression)
{
    wlr_expr result;

    RETURN_IF_ERROR_EXPRESSION(inputStringExpression);

    if (!StringQ(inputStringExpression))
    {
        return wlr_Error(WLR_UNEXPECTED_TYPE);
    }

    result = oToExpression(E(steToExpression, inputStringExpression, steInputForm, steList));
    if (!ListQ(result))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    if (Length(result) == 1)
    {
        result = First(result);
    }

    return result;
}

wlr_expr wlr_Get(char *fileName)
{
    wlr_expr fileNameExpr;

    wlr_expr result;

    /* should there be another version of this function with an argument for an Encode key? */

    fileNameExpr = wlr_String(fileName);
    if (!StringQ(fileNameExpr))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    signingViolationInGet = FALSE;

    suppressSigningViolationMessageInGet = TRUE;
    result = oGet(E(steGet, fileNameExpr));

    if (Head(result) == steDumpGet)
    {
        suppressSigningViolationMessageInGet = TRUE;
        result = apiEvaluate(result);
    }

    if (signingViolationInGet)
    {
        return wlr_Error(WLR_SIGNING_ERROR);
    }

    return result;
}

wlr_err_t wlr_Serialize(char *fileName, wlr_expr expression)
{
    wlr_expr fileNameExpr;

    wlr_expr writeWXFFileResult;

    if (wlr_ErrorQ(expression))
    {
        return WLR_ERROR_EXPRESSION;
    }

    fileNameExpr = wlr_String(fileName);
    if (!StringQ(fileNameExpr))
    {
        return WLR_MISCELLANEOUS_ERROR;
    }

    writeWXFFileResult = oWriteWXFFile(E(steWriteWXFFile, fileNameExpr, expression));
    if (aFailureQ(writeWXFFileResult))
    {
        return WLR_MISCELLANEOUS_ERROR;
    }

    return WLR_SUCCESS;
}

wlr_expr wlr_Deserialize(char *fileName)
{
    wlr_expr fileNameExpr;

    wlr_expr resultHead;
    wlr_expr result;

    wlr_expr normalizedResult;

    fileNameExpr = wlr_String(fileName);
    if (!StringQ(fileNameExpr))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    resultHead = ContextSymbol("ExpressionAPIGetResult", "Internal`");
    if (!SymbolQ(resultHead))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    result = oReadWXFFile(E(steReadWXFFile, fileNameExpr, resultHead));
    if (
        (!HasHead(result, resultHead)) ||
        /* I am not sure if oReadWXFFile can produce a result containing multiple elements, but fail if it happens */
        (Length(result) != 1))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    if (codeSigningEnabled)
    {
        normalizedResult = wlr_Normalize(result);
        if (wlr_ErrorQ(normalizedResult))
        {
            return wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }

        if (!verifyExpression(normalizedResult, IGNORE_API_PROTECTION_MODE))
        {
            return wlr_Error(WLR_SIGNING_ERROR);
        }
    }

    return First(result);
}

wlr_err_t wlr_IntegerData(wlr_expr machineIntegerExpression, mint *result)
{
    if (wlr_ErrorQ(machineIntegerExpression))
    {
        SAFE_POINTER_SET(result, -1);
        return WLR_ERROR_EXPRESSION;
    }

    if (!MIntegerQ(machineIntegerExpression))
    {
        SAFE_POINTER_SET(result, -1);
        return WLR_UNEXPECTED_TYPE;
    }

    return wlr_IntegerConvert(machineIntegerExpression, result);
}

wlr_err_t wlr_RealData(wlr_expr machineRealExpression, mreal *result)
{
    if (wlr_ErrorQ(machineRealExpression))
    {
        SAFE_POINTER_SET(result, -1);
        return WLR_ERROR_EXPRESSION;
    }

    if (
        !(
            (is_real_infinity(machineRealExpression)) ||
            (SameQ(machineRealExpression, minusInfinity)) ||
            (MRealQ(machineRealExpression))))
    {
        SAFE_POINTER_SET(result, -1);
        return WLR_UNEXPECTED_TYPE;
    }

    return wlr_RealConvert(machineRealExpression, result);
}

mbool wlr_SignedCodeMode(void)
{
    return codeSigningEnabled;
}

wlr_expr wlr_Eval(wlr_expr expression)
{
    RETURN_IF_ERROR_EXPRESSION(expression);

    if (!verifyExpression(expression, USE_API_PROTECTION_MODE))
    {
        return wlr_Error(WLR_SIGNING_ERROR);
    }

    return apiEvaluate(expression);
}

wlr_expr wlr_EvalData(wlr_expr expression)
{
    wlr_expr evaluationResult;

    wlr_expr result;

    struct evaluation_data *oldEvaluationData;

    struct evaluation_data evaluationData;

    RETURN_IF_ERROR_EXPRESSION(expression);

    if (!verifyExpression(expression, USE_API_PROTECTION_MODE))
    {
        return wlr_Error(WLR_SIGNING_ERROR);
    }

    if (!initializeEvaluationData(&evaluationData))
    {
        return wlr_Error(WLR_MISCELLANEOUS_ERROR);
    }

    /* record evaluation data in evaluationData, and prevent evaluation data from being recorded anywhere else */
    oldEvaluationData = apiEvaluationData;
    apiEvaluationData = &evaluationData;

    evaluationResult = topeval(expression);

    /* restore the original evaluation data recording behavior */
    apiEvaluationData = oldEvaluationData;

    result = evaluationDataToAssociation(&evaluationData, evaluationResult);

    (void)deinitializeEvaluationData(&evaluationData);

    return result;
}

wlr_expr wlr_EvalString(wlr_expr inputStringExpression)
{
    wlr_expr parsedExpression;

    (void)new_message_stack();

    parsedExpression = wlr_ParseExpression(inputStringExpression);

    (void)clear_message_stack();

    return wlr_Eval(parsedExpression);
}

void wlr_Release(void *data)
{
    if (data == NULL)
    {
        return;
    }

    (void)Free(data);
}

wlr_expr wlr_VariadicE(wlr_expr expressionHead, mint childElementNumber, ...)
{
    va_list childElements;

    wlr_expr result;

    if (!expressionAPIMode)
    {
        return NULL;
    }

    (void)va_start(childElements, childElementNumber);

    result = wlr_iVariadicE(expressionHead, childElementNumber, childElements);

    (void)va_end(childElements);

    return result;
}

/* The following function is the equivalent of wlr_VariadicE, but it takes a va_list instead of variadic
    arguments. Higher-level bindings written on top of the expression API would use this function instead of
    wlr_VariadicE. The caller is responsible for calling va_start and va_end. */
wlr_expr wlr_iVariadicE(wlr_expr expressionHead, mint childElementNumber, va_list childElements)
{
    if (!expressionAPIMode)
    {
        return NULL;
    }

    {
        wlr_expr head;
        wlr_expr result;

        /* ignore wlr_Abort aborts and suppress messages */
        (void)stageAPIFunction();

        head = prepareExpressionAPIArgument(expressionHead);

        result = variadicE(head, childElementNumber, childElements, FALSE);

        /* restore abort and messaging behavior */
        (void)unstageAPIFunction();

        return prepareExpressionAPIResult(result);
    }
}

wlr_expr wlr_VariadicList(mint childElementNumber, ...)
{
    va_list childElements;

    wlr_expr result;

    if (!expressionAPIMode)
    {
        return NULL;
    }

    (void)va_start(childElements, childElementNumber);

    result = wlr_iVariadicList(childElementNumber, childElements);

    (void)va_end(childElements);

    return result;
}

/* The following function is the equivalent of wlr_VariadicList, but it takes a va_list instead of variadic
    arguments. Higher-level bindings written on top of the expression API would use this function instead of
    wlr_VariadicList. The caller is responsible for calling va_start and va_end. */
wlr_expr wlr_iVariadicList(mint childElementNumber, va_list childElements)
{
    if (!expressionAPIMode)
    {
        return NULL;
    }

    {
        wlr_expr result;

        /* ignore wlr_Abort aborts and suppress messages */
        (void)stageAPIFunction();

        result = variadicE(steList, childElementNumber, childElements, FALSE);

        /* restore abort and messaging behavior */
        (void)unstageAPIFunction();

        return prepareExpressionAPIResult(result);
    }
}

wlr_expr wlr_VariadicAssociation(mint keyValuePairNumber, ...)
{
    va_list keyValuePairs;

    wlr_expr result;

    if (!expressionAPIMode)
    {
        return NULL;
    }

    (void)va_start(keyValuePairs, keyValuePairNumber);

    result = wlr_iVariadicAssociation(keyValuePairNumber, keyValuePairs);

    (void)va_end(keyValuePairs);

    return result;
}

/* The following function is the equivalent of wlr_VariadicAssociation, but it takes a va_list instead of variadic
    arguments. Higher-level bindings written on top of the expression API would use this function instead of
    wlr_VariadicAssociation. The caller is responsible for calling va_start and va_end. */
wlr_expr wlr_iVariadicAssociation(mint keyValuePairNumber, va_list keyValuePairs)
{
    if (!expressionAPIMode)
    {
        return NULL;
    }

    {
        wlr_expr result;

        /* ignore wlr_Abort aborts and suppress messages */
        (void)stageAPIFunction();

        result = variadicE(steList, keyValuePairNumber, keyValuePairs, TRUE);

        if (wlr_ErrorQ(result))
        {
            (void)unstageAPIFunction();
            return prepareExpressionAPIResult(result);
        }

        /* @note List and Rule will always be approved symbols */
        if (!verifyExpression(result, USE_API_PROTECTION_MODE))
        {
            (void)unstageAPIFunction();
            return prepareExpressionAPIResult(wlr_Error(WLR_SIGNING_ERROR));
        }

        /* the second argument being TRUE tells Association_createNewStructure to create an ordered association */
        result = Association_createNewStructure(result, TRUE);
        if (!AssociationQ(result))
        {
            result = wlr_Error(WLR_MISCELLANEOUS_ERROR);
        }

        /* restore abort and messaging behavior */
        (void)unstageAPIFunction();

        return prepareExpressionAPIResult(result);
    }
}

errcode_t wlr_MNumericArray_new(const numericarray_data_t type, const mint rank, const mint *dims,
                                MNumericArray *res);
errcode_t wlr_MNumericArray_clone(const MNumericArray from, MNumericArray *to);
void wlr_MNumericArray_free(MNumericArray narray);
void wlr_MNumericArray_disown(MNumericArray narray);
void wlr_MNumericArray_disownAll(MNumericArray narray);
mint wlr_MNumericArray_shareCount(const MNumericArray narray);
numericarray_data_t wlr_MNumericArray_getType(const MNumericArray narray);
mint wlr_MNumericArray_getRank(const MNumericArray narray);
mint wlr_MNumericArray_getFlattenedLength(const MNumericArray narray);
errcode_t wlr_MNumericArray_convertType(MNumericArray *outP, const MNumericArray narray,
                                        const numericarray_data_t result_type,
                                        const numericarray_convert_method_t method, const mreal tolerance);
mint *wlr_MNumericArray_getDimensions(const MNumericArray narray);
void *wlr_MNumericArray_getData(const MNumericArray narray);
Generated by doxygen 1.9.1