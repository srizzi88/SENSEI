//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_Logging_h
#define svtk_m_cont_Logging_h

#include <svtkm/internal/Configure.h>
#include <svtkm/internal/ExportMacros.h>

#include <svtkm/Types.h>

#include <svtkm/cont/svtkm_cont_export.h>

#ifdef SVTKM_ENABLE_LOGGING

// disable MSVC warnings in loguru.hpp
#ifdef SVTKM_MSVC
#pragma warning(push)
#pragma warning(disable : 4722)
#endif // SVTKM_MSVC

#define LOGURU_EXPORT SVTKM_CONT_EXPORT
#define LOGURU_WITH_STREAMS 1
#define LOGURU_SCOPE_TIME_PRECISION 6
#include <svtkm/thirdparty/loguru/svtkmloguru/loguru.hpp>

#ifdef SVTKM_MSVC
#pragma warning(pop)
#endif // SVTKM_MSVC

#else // SVTKM_ENABLE_LOGGING
#include <iostream>
#endif // SVTKM_ENABLE_LOGGING

#include <string>
#include <typeinfo>

/// \file Logging.h
/// \brief Logging utilities.
///
/// This file includes the logging system for SVTK-m. There are a variety of
/// macros to print log messages using C++ stream or printf syntax. Nested
/// scopes may be created in the log output, and there are several helper
/// functions to help format common types of log data such as byte counts and
/// type names.
///
/// Logging is enabled via the CMake option SVTKm_ENABLE_LOGGING by default.
/// The default log level is set to only log Warn and Error messages; Fatal
/// levels are printed to stderr by default. The logging system will need
/// to be initialized through a call to either svtkm::cont::Initialize or
/// svtkm::cont::InitLogging.
///
/// Additional logging features are enabled by calling svtkm::cont::InitLogging
/// (or preferably, svtkm::cont::Initialize) in an executable. This will:
/// - Set human-readable names for the log levels in the output.
/// - Allow the stderr logging level to be set at runtime by passing a
///   '-v [level]' argument to the executable.
/// - Name the main thread.
/// - Print a preamble with details of the program's startup (args, etc).
/// - Install signal handlers to automatically print stacktraces and error
///   contexts (linux only) on crashes.
///
/// The main logging entry points are the macros SVTKM_LOG_S and SVTKM_LOG_F,
/// which using C++ stream and printf syntax, repectively. Other variants exist,
/// including conditional logging and special-purpose logs for writing specific
/// events, such as DynamicObject cast results and TryExecute failures.
///
/// The logging backend supports the concept of "Scopes". By creating a new
/// scope with the macros SVTKM_LOG_SCOPE or SVTKM_LOG_SCOPE_FUNCTION, a new
/// "logging scope" is opened within the C++ scope the macro is called from. New
/// messages will be indented in the log until the scope ends, at which point
/// a message is logged with the elapsed time that the scope was active. Scopes
/// may be nested to arbitrary depths.
///
/// The logging implementation is thread-safe. When working in a multithreaded
/// environment, each thread may be assigned a human-readable name using
/// svtkm::cont::SetThreadName. This will appear in the log output so that
/// per-thread messages can be easily tracked.
///
/// By default, only Warn, Error, and Fatal messages are printed to
/// stderr. This can be changed at runtime by passing the '-v' flag to an
/// executable that calls svtkm::cont::InitLogging. Alternatively, the
/// application can explicitly call svtkm::cont::SetStderrLogLevel to change the
/// verbosity. When specifying a verbosity, all log levels with enum values
/// less-than-or-equal-to the requested level are printed.
/// svtkm::cont::LogLevel::Off (or "-v Off") may be used to silence the log
/// completely.
///
/// The helper functions svtkm::cont::GetHumanReadableSize and
/// svtkm::cont::GetSizeString assist in formating byte sizes to a more readable
/// format. Similarly, the svtkm::cont::TypeToString template functions provide RTTI
/// based type-name information. When logging is enabled, these use the logging
/// backend to demangle symbol names on supported platforms.
///
/// The more verbose SVTK-m log levels are:
/// - Perf: Logs performance information, using the scopes feature to track
///   execution time of filters, worklets, and device algorithms with
///   microsecond resolution.
/// - MemCont / MemExec: These levels log memory allocations in the control and
///   execution environments, respectively.
/// - MemTransfer: This level logs memory transfers between the control and host
///   environments.
/// - KernelLaunches: This level logs details about each device side kernel launch
///   such as the CUDA PTX, Warps, and Grids used.
/// - Cast: Logs details of dynamic object resolution.
///
/// The log may be shared and extended by applications that use SVTK-m. There
/// are two log level ranges left available for applications: User and
/// UserVerbose. The User levels may be enabled without showing any of the
/// verbose SVTK-m levels, while UserVerbose levels will also enable all SVTK-m
/// levels.

/// \def SVTKM_LOG_S(level, ...)
/// \brief Writes a message using stream syntax to the indicated log \a level.
///
/// The ellipsis may be replaced with the log message as if constructing a C++
/// stream, e.g:
///
/// \code
/// SVTKM_LOG_S(svtkm::cont::LogLevel::Perf,
///            "Executed functor " << svtkm::cont::TypeToString(functor)
///             << " on device " << deviceId.GetName());
/// \endcode

/// \def SVTKM_LOG_F(level, ...)
/// \brief Writes a message using printf syntax to the indicated log \a level.
///
/// The ellipsis may be replaced with the log message as if constructing a
/// printf call, e.g:
///
/// \code
/// SVTKM_LOG_F(svtkm::cont::LogLevel::Perf,
///            "Executed functor %s on device %s",
///            svtkm::cont::TypeToString(functor).c_str(),
///            deviceId.GetName().c_str());
/// \endcode

/// \def SVTKM_LOG_IF_S(level, cond, ...)
/// Same as SVTKM_LOG_S, but only logs if \a cond is true.

/// \def SVTKM_LOG_IF_F(level, cond, ...)
/// Same as SVTKM_LOG_F, but only logs if \a cond is true.

/// \def SVTKM_LOG_SCOPE(level, ...)
/// Creates a new scope at the requested \a level. The log scope ends when the
/// code scope ends. The ellipses form the scope name using printf syntax.
///
/// \code
/// {
///   SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf,
///                  "Executing filter %s",
///                  svtkm::cont::TypeToString(myFilter).c_str());
///   myFilter.Execute();
/// }
/// \endcode

/// \def SVTKM_LOG_SCOPE_FUNCTION(level)
/// Equivalent to SVTKM_LOG_SCOPE(__func__);

/// \def SVTKM_LOG_ALWAYS_S(level, ...)
/// This ostream-style log message is always emitted, even when logging is
/// disabled at compile time.

/// \def SVTKM_LOG_CAST_SUCC(inObj, outObj)
/// \brief Convenience macro for logging the successful cast of dynamic object.
/// \param inObj The dynamic object.
/// \param outObj The resulting downcasted object.

/// \def SVTKM_LOG_CAST_FAIL(inObj, outType)
/// \brief Convenience macro for logging a failed cast of dynamic object.
/// \param inObj The dynamic object.
/// \param outObj The candidate type (or typelist) that was unsuccessful.

/// \def SVTKM_LOG_TRYEXECUTE_FAIL(errorMessage, functorName, deviceId)
/// \brief Convenience macro for logging a TryExecute failure to the Error level.
/// If logging is disabled, a message is still printed to stderr.
/// \param errorMessage The error message detailing the failure.
/// \param functorName The name of the functor (see svtkm::cont::TypeToString)
/// \param deviceId The device tag / id for the device on which the functor
/// failed.

/// \def SVTKM_LOG_TRYEXECUTE_DISABLE(errorMessage, functorName, deviceId)
/// \brief Similar to SVTKM_LOG_TRYEXECUTE_FAIL, but also informs the user
/// that the device has been disable for future TryExecute calls.
/// \param errorMessage The error message detailing the failure.
/// \param functorName The name of the functor (see svtkm::cont::TypeToString)
/// \param deviceId The device tag / id for the device on which the functor
/// failed.

/// \def SVTKM_DEFINE_USER_LOG_LEVEL(name, offset)
/// \brief Convenience macro for creating a custom log level that is usable
/// in the other macros.  If logging is disabled this macro does nothing.
/// \param name The name to give the new log level
/// \param offset The offset from the svtkm::cont::LogLevel::UserFirst value
/// from the LogLevel enum.  Additionally moduloed against the
/// svtkm::cont::LogLevel::UserLast value
/// \note This macro is to be used for quickly setting log levels.  For a
/// more maintainable solution it is recommended to create a custom enum class
/// and then cast appropriately, as described here:
/// https://gitlab.kitware.com/svtk/svtk-m/issues/358#note_550157

#if defined(SVTKM_ENABLE_LOGGING)

#define SVTKM_LOG_S(level, ...) VLOG_S(static_cast<loguru::Verbosity>(level)) << __VA_ARGS__
#define SVTKM_LOG_F(level, ...) VLOG_F(static_cast<loguru::Verbosity>(level), __VA_ARGS__)
#define SVTKM_LOG_IF_S(level, cond, ...)                                                            \
  VLOG_IF_S(static_cast<loguru::Verbosity>(level), cond) << __VA_ARGS__
#define SVTKM_LOG_IF_F(level, cond, ...)                                                            \
  VLOG_IF_F(static_cast<loguru::Verbosity>(level), cond, __VA_ARGS__)
#define SVTKM_LOG_SCOPE(level, ...) VLOG_SCOPE_F(static_cast<loguru::Verbosity>(level), __VA_ARGS__)
#define SVTKM_LOG_SCOPE_FUNCTION(level)                                                             \
  SVTKM_LOG_SCOPE(static_cast<loguru::Verbosity>(level), __func__)
#define SVTKM_LOG_ERROR_CONTEXT(desc, data) ERROR_CONTEXT(desc, data)
#define SVTKM_LOG_ALWAYS_S(level, ...) SVTKM_LOG_S(level, __VA_ARGS__)

// Convenience macros:

// Cast success:
#define SVTKM_LOG_CAST_SUCC(inObj, outObj)                                                          \
  SVTKM_LOG_F(svtkm::cont::LogLevel::Cast,                                                           \
             "Cast succeeded: %s (%p) --> %s (%p)",                                                \
             svtkm::cont::TypeToString(inObj).c_str(),                                              \
             &inObj,                                                                               \
             svtkm::cont::TypeToString(outObj).c_str(),                                             \
             &outObj)

// Cast failure:
#define SVTKM_LOG_CAST_FAIL(inObj, outType)                                                         \
  SVTKM_LOG_F(svtkm::cont::LogLevel::Cast,                                                           \
             "Cast failed: %s (%p) --> %s",                                                        \
             svtkm::cont::TypeToString(inObj).c_str(),                                              \
             &inObj,                                                                               \
             svtkm::cont::TypeToString<outType>().c_str())

// TryExecute failure
#define SVTKM_LOG_TRYEXECUTE_FAIL(errorMessage, functorName, deviceId)                              \
  SVTKM_LOG_S(svtkm::cont::LogLevel::Error, "TryExecute encountered an error: " << errorMessage);    \
  SVTKM_LOG_S(svtkm::cont::LogLevel::Error, "Failing functor: " << functorName);                     \
  SVTKM_LOG_S(svtkm::cont::LogLevel::Error, "Failing device: " << deviceId.GetName())

// Same, but disabling device:
#define SVTKM_LOG_TRYEXECUTE_DISABLE(errorMessage, functorName, deviceId)                           \
  SVTKM_LOG_S(svtkm::cont::LogLevel::Error, "TryExecute encountered an error: " << errorMessage);    \
  SVTKM_LOG_S(svtkm::cont::LogLevel::Error, "Failing functor: " << functorName);                     \
  SVTKM_LOG_S(svtkm::cont::LogLevel::Error, "Failing device: " << deviceId.GetName());               \
  SVTKM_LOG_S(svtkm::cont::LogLevel::Error, "The failing device has been disabled.")

// Custom log level
#define SVTKM_DEFINE_USER_LOG_LEVEL(name, offset)                                                   \
  static constexpr svtkm::cont::LogLevel name = static_cast<svtkm::cont::LogLevel>(                  \
    static_cast<typename std::underlying_type<svtkm::cont::LogLevel>::type>(                        \
      svtkm::cont::LogLevel::UserFirst) +                                                           \
    offset % static_cast<typename std::underlying_type<svtkm::cont::LogLevel>::type>(               \
               svtkm::cont::LogLevel::UserLast))

#else // SVTKM_ENABLE_LOGGING

#define SVTKM_LOG_S(level, ...)
#define SVTKM_LOG_F(level, ...)
#define SVTKM_LOG_IF_S(level, cond, ...)
#define SVTKM_LOG_IF_F(level, cond, ...)
#define SVTKM_LOG_SCOPE(level, ...)
#define SVTKM_LOG_SCOPE_FUNCTION(level)
#define SVTKM_LOG_ERROR_CONTEXT(desc, data)
#define SVTKM_LOG_CAST_SUCC(inObj, outObj)
#define SVTKM_LOG_CAST_FAIL(inObj, outType)
#define SVTKM_DEFINE_USER_LOG_LEVEL(name, offset)

// Always emitted. When logging is disabled, std::cerr is used.

#define SVTKM_LOG_ALWAYS_S(level, ...)                                                              \
  (static_cast<int>(level) < 0 ? std::cerr : std::cout) << svtkm::cont::GetLogLevelName(level)      \
                                                        << ": " << __VA_ARGS__ << "\n"

// TryExecute failures are still important enough to log, but we just write to
// std::cerr when logging is disabled.
#define SVTKM_LOG_TRYEXECUTE_FAIL(errorMessage, functorName, deviceId)                              \
  std::cerr << "Error: TryExecute encountered an error: " << errorMessage << "\n"                  \
            << "\t- Failing functor: " << functorName << "\n"                                      \
            << "\t- Failing device: " << deviceId.GetName() << "\n\n"
#define SVTKM_LOG_TRYEXECUTE_DISABLE(errorMessage, functorName, deviceId)                           \
  std::cerr << "Error: TryExecute encountered an error: " << errorMessage << "\n"                  \
            << "\t- Failing functor: " << functorName << "\n"                                      \
            << "\t- Failing device: " << deviceId.GetName() << "\n"                                \
            << "The failing device has been disabled.\n\n"

#endif // SVTKM_ENABLE_LOGGING

namespace svtkm
{
namespace cont
{

/// Log levels for use with the logging macros.
enum class LogLevel
{
  /// Used with SetStderrLogLevel to silence the log. Do not actually log to
  /// this level.
  Off = -9, //loguru::Verbosity_OFF,

  /// Fatal errors that should abort execution.
  Fatal = -3, // loguru::Verbosity_FATAL,

  /// Important but non-fatal errors, such as device fail-over.
  Error = -2, // loguru::Verbosity_ERROR,

  /// Less important user errors, such as out-of-bounds parameters.
  Warn = -1, // loguru::Verbosity_WARNING,

  /// Information messages (detected hardware, etc) and temporary debugging
  /// output.
  Info = 0, //loguru::Verbosity_INFO,

  /// The range 1-255 are reserved to application use.
  UserFirst = 1,
  /// The range 1-255 are reserved to application use.
  UserLast = 255,

  /// Information about which devices are enabled/disabled
  DevicesEnabled,

  /// General timing data and algorithm flow information, such as filter
  /// execution, worklet dispatches, and device algorithm calls.
  Perf,

  /// Host-side resource allocations/frees (e.g. ArrayHandle control buffers)
  MemCont,

  /// Device-side resource allocations/frees (e.g ArrayHandle device buffers)
  MemExec,

  /// Host->device / device->host data copies
  MemTransfer,

  /// Details on Device-side Kernel Launches
  KernelLaunches,

  /// When a dynamic object is (or isn't) resolved via CastAndCall, etc.
  Cast,

  /// 1024-2047 are reserved for application usage.
  UserVerboseFirst = 1024,
  /// 1024-2047 are reserved for application usage.
  UserVerboseLast = 2047
};

/**
 * This shouldn't be called directly -- prefer calling svtkm::cont::Initialize,
 * which takes care of logging as well as other initializations.
 *
 * Initializes logging. Sets up custom log level and thread names. Parses any
 * "-v [LogLevel]" arguments to set the stderr log level. This argument may
 * be either numeric, or the 4-character string printed in the output. Note that
 * loguru will consume the "-v [LogLevel]" argument and shrink the arg list.
 *
 * If the parameterless overload is used, the `-v` parsing is not used, but
 * other functionality should still work.
 *
 * @note This function is not threadsafe and should only be called from a single
 * thread (ideally the main thread).
 * @{
 */
SVTKM_CONT_EXPORT
SVTKM_CONT
void InitLogging(int& argc, char* argv[]);
SVTKM_CONT_EXPORT
SVTKM_CONT
void InitLogging();
/**@}*/

/**
 * Set the range of log levels that will be printed to stderr. All levels
 * with an enum value less-than-or-equal-to \a level will be printed.
 */
SVTKM_CONT_EXPORT
SVTKM_CONT
void SetStderrLogLevel(svtkm::cont::LogLevel level);

/**
 * Get the active highest log level that will be printed to stderr.
 */
SVTKM_CONT_EXPORT
SVTKM_CONT
svtkm::cont::LogLevel GetStderrLogLevel();

/**
 * Register a custom name to identify a log level. The name will be truncated
 * to 4 characters internally.
 *
 * Must not be called after InitLogging. Such calls will fail and log an error.
 *
 * There is no need to call this for the default svtkm::cont::LogLevels. They
 * are populated in InitLogging and will be overwritten.
 */
SVTKM_CONT_EXPORT
SVTKM_CONT
void SetLogLevelName(svtkm::cont::LogLevel level, const std::string& name);

/**
 * Get a human readable name for the log level. If a name has not been
 * registered via InitLogging or SetLogLevelName, the returned string just
 * contains the integer representation of the level.
 */
SVTKM_CONT_EXPORT
SVTKM_CONT
std::string GetLogLevelName(svtkm::cont::LogLevel level);

/**
 * The name to identify the current thread in the log output.
 * @{
 */
SVTKM_CONT_EXPORT
SVTKM_CONT
void SetLogThreadName(const std::string& name);
SVTKM_CONT_EXPORT
SVTKM_CONT
std::string GetLogThreadName();
/**@}*/

// Per-thread error context, not currently used, undocumented....
SVTKM_CONT_EXPORT
SVTKM_CONT
std::string GetLogErrorContext();

/**
 * Returns a stacktrace on supported platforms.
 * Argument is the number of frames to skip (GetStackTrace and below are already
 * skipped).
 */
SVTKM_CONT_EXPORT
SVTKM_CONT
std::string GetStackTrace(svtkm::Int32 skip = 0);

//@{
/// Convert a size in bytes to a human readable string (e.g. "64 bytes",
/// "1.44 MiB", "128 GiB", etc). @a prec controls the fixed point precision
/// of the stringified number.
SVTKM_CONT_EXPORT
SVTKM_CONT
std::string GetHumanReadableSize(svtkm::UInt64 bytes, int prec = 2);

template <typename T>
SVTKM_CONT inline std::string GetHumanReadableSize(T&& bytes, int prec = 2)
{
  return GetHumanReadableSize(static_cast<svtkm::UInt64>(std::forward<T>(bytes)), prec);
}
//@}

//@{
/// Returns "%1 (%2 bytes)" where %1 is the result from GetHumanReadableSize
/// and two is the exact number of bytes.
SVTKM_CONT_EXPORT
SVTKM_CONT
std::string GetSizeString(svtkm::UInt64 bytes, int prec = 2);

template <typename T>
SVTKM_CONT inline std::string GetSizeString(T&& bytes, int prec = 2)
{
  return GetSizeString(static_cast<svtkm::UInt64>(std::forward<T>(bytes)), prec);
}
//@}

/**
 * Use RTTI information to retrieve the name of the type T. If logging is
 * enabled and the platform supports it, the type name will also be demangled.
 * @{
 */
inline SVTKM_CONT std::string TypeToString(const std::type_info& t)
{
#ifdef SVTKM_ENABLE_LOGGING
  return loguru::demangle(t.name()).c_str();
#else  // SVTKM_ENABLE_LOGGING
  return t.name();
#endif // SVTKM_ENABLE_LOGGING
}
template <typename T>
inline SVTKM_CONT std::string TypeToString()
{
  return TypeToString(typeid(T));
}
template <typename T>
inline SVTKM_CONT std::string TypeToString(const T&)
{
  return TypeToString(typeid(T));
}
/**@}*/
}
} // end namespace svtkm::cont

#endif // svtk_m_cont_Logging_h
