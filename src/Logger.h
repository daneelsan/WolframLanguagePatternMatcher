#pragma once

#include "Expr.h"

#include <string>
#include <utility> // for std::forward

/**
 * @def PM_LOG_DEBUG
 * Define PM_LOG_DEBUG to enable all log levels
 */
#ifdef PM_LOG_DEBUG
#define PM_LOG_LEVEL_DEBUG
#define PM_LOG_WARNING
#endif

/**
 * @def PM_LOG_WARNING
 * Define PM_LOG_WARNING to enable warning and error logs. Debug logs will be ignored.
 */
#ifdef PM_LOG_WARNING
#define PM_LOG_LEVEL_WARNING
#define PM_LOG_ERROR
#endif

/**
 * @def PM_LOG_ERROR
 * Define PM_LOG_ERROR to enable only error logs. Debug and warning logs will be ignored.
 */
#ifdef PM_LOG_ERROR
#define PM_LOG_LEVEL_ERROR
#endif

#ifdef PM_LOG_LEVEL_DEBUG
/**
 * Log a message as debug information, the log will consist of the line number, file name,
 * function name and user-provided args.
 * Formatting can be customized on the Wolfram Language side.
 * @note This macro is only active with LOG_DEBUG compilation flag.
 */
#define PM_DEBUG(...) \
	PatternMatcher::Logger::log<PatternMatcher::Logger::Level::Debug>(__LINE__, __FILE__, __func__, __VA_ARGS__)
#else
#define PM_DEBUG(...) ((void) 0)
#endif

#ifdef PM_LOG_LEVEL_WARNING
/**
 * Log a message (arbitrary sequence of arguments that can be passed via WSTP) as warning, the log will consist of the
 * line number, file name, function name and user-provided args. Formatting can be customized on the Wolfram Language side.
 * @note This macro is only active with PM_LOG_DEBUG or PM_LOG_WARNING compilation flag.
 */
#define PM_WARNING(...) \
	PatternMatcher::Logger::log<PatternMatcher::Logger::Level::Warning>(__LINE__, __FILE__, __func__, __VA_ARGS__)
#else
#define PM_WARNING(...) ((void) 0)
#endif

#ifdef PM_LOG_LEVEL_ERROR
/**
 * Log a message (arbitrary sequence of arguments that can be passed via WSTP) as error, the log will consist of the line
 * number, file name, function name and user-provided args. Formatting can be customized on the Wolfram Language side.
 * @note This macro is only active with PM_LOG_DEBUG, PM_LOG_WARNING or PM_LOG_ERROR compilation flag.
 */
#define PM_ERROR(...) \
	PatternMatcher::Logger::log<PatternMatcher::Logger::Level::Error>(__LINE__, __FILE__, __func__, __VA_ARGS__)
#else
#define PM_ERROR(...) ((void) 0)
#endif

#ifdef PM_LOG_LEVEL_ERROR
/// @def PM_ASSERT
/// Assert that a condition is true, otherwise log an error message and abort the program.
/// @param cond The condition to check.
/// @param ... Additional arguments to log if the assertion fails.
/// @note This macro is only active with PM_LOG_DEBUG, PM_LOG_WARNING or PM_LOG_ERROR compilation flag.
#define PM_ASSERT(cond, ...)                                           \
	do                                                                 \
	{                                                                  \
		if (!(cond))                                                   \
		{                                                              \
			PM_ERROR("Assertion failed: ", #cond, " | ", __VA_ARGS__); \
			abort();                                                   \
		}                                                              \
	} while (0)
#else
#define PM_ASSERT(cond, ...) ((void) 0)
#endif

#ifdef PM_LOG_LEVEL_TRACE
#define PM_TRACE(...) \
	PatternMatcher::Logger::trace<PatternMatcher::Logger::Level::Trace>(__LINE__, __FILE__, __func__, __VA_ARGS__)
#else
#define PM_TRACE(...) ((void) 0)
#endif

namespace PatternMatcher
{
class Logger
{
public:
	enum class Level
	{
		Trace,
		Debug,
		Warning,
		Error
	};

	// Runtime control for tracing
	static inline bool traceEnabled = false;

	static const char* to_string(Level l)
	{
		switch (l)
		{
			case Level::Trace:
				return "Trace";
			case Level::Debug:
				return "Debug";
			case Level::Warning:
				return "Warning";
			case Level::Error:
				return "Error";
			default:
				return "Unknown";
		}
	}

	template <Level logLevel, typename... TArgs>
	static void log(int line, const char* file, const char* function, TArgs&&... args)
	{
		logOrTrace<logLevel>(logHandlerName, line, file, function, std::forward<TArgs>(args)...);
	}

	template <Level logLevel, typename... TArgs>
	static void trace(int line, const char* file, const char* function, TArgs&&... args)
	{
		// Check if tracing is enabled at runtime
		if (!traceEnabled)
			return;
		
		logOrTrace<logLevel>(traceHandlerName, line, file, function, std::forward<TArgs>(args)...);
	}

	// Enable/disable tracing at runtime
	static void setTraceEnabled(bool enabled) { traceEnabled = enabled; }
	static bool isTraceEnabled() { return traceEnabled; }

private:
	template <Level logLevel, typename... TArgs>
	static void logOrTrace(const char* logHandlerName, int line, const char* file, const char* function, TArgs&&... args)
	{
		Expr logExpr = Expr::createNormal(4 + sizeof...(TArgs), logHandlerName);
		logExpr.setPart(1, Expr(to_string(logLevel)));
		logExpr.setPart(2, Expr(static_cast<mint>(line)));
		logExpr.setPart(3, Expr(file));
		logExpr.setPart(4, Expr(function));
		// Iterate over args and add each to the correct part of logExpr
		int partIndex = 5;
		(logExpr.setPart(partIndex++, Expr(std::forward<TArgs>(args))), ...);
		logExpr.eval();
	}

	static constexpr const char* logHandlerName = "DanielS`PatternMatcher`Utilities`Logger`LogHandler";
	static constexpr const char* traceHandlerName = "DanielS`PatternMatcher`Utilities`Logger`TraceHandler";
}; // namespace Logger
} // namespace PatternMatcher
