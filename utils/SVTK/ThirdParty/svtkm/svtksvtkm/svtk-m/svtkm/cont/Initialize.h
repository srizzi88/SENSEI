//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_Initialize_h
#define svtk_m_cont_Initialize_h

#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/svtkm_cont_export.h>
#include <svtkm/internal/ExportMacros.h>

#include <string>
#include <type_traits>
#include <vector>

namespace svtkm
{
namespace cont
{

struct InitializeResult
{
  /// Device passed into -d, or undefined
  DeviceAdapterId Device = DeviceAdapterTagUndefined{};

  /// Usage statement for arguments parsed by SVTK-m
  std::string Usage;
};

enum class InitializeOptions
{
  None = 0x00,

  /// Issue an error if the device argument is not specified.
  RequireDevice = 0x01,

  /// If no device is specified, treat it as if the user gave --device=Any. This means that
  /// DeviceAdapterTagUndefined will never be return in the result.
  DefaultAnyDevice = 0x02,

  /// Add a help argument. If -h or --help is provided, prints a usage statement. Of course,
  /// the usage statement will only print out arguments processed by SVTK-m.
  AddHelp = 0x04,

  /// If an unknown option is encountered, the program terminates with an error and a usage
  /// statement is printed. If this option is not provided, any unknown options are returned
  /// in argv. If this option is used, it is a good idea to use AddHelp as well.
  ErrorOnBadOption = 0x08,

  /// If an extra argument is encountered, the program terminates with an error and a usage
  /// statement is printed. If this option is not provided, any unknown arguments are returned
  /// in argv.
  ErrorOnBadArgument = 0x10,

  /// If supplied, Initialize treats its own arguments as the only ones supported by the
  /// application and provides an error if not followed exactly. This is a convenience
  /// option that is a combination of ErrorOnBadOption, ErrorOnBadArgument, and AddHelp.
  Strict = ErrorOnBadOption | ErrorOnBadArgument | AddHelp
};

// Allow options to be used as a bitfield
inline InitializeOptions operator|(const InitializeOptions& lhs, const InitializeOptions& rhs)
{
  using T = std::underlying_type<InitializeOptions>::type;
  return static_cast<InitializeOptions>(static_cast<T>(lhs) | static_cast<T>(rhs));
}
inline InitializeOptions operator&(const InitializeOptions& lhs, const InitializeOptions& rhs)
{
  using T = std::underlying_type<InitializeOptions>::type;
  return static_cast<InitializeOptions>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

/**
 * Initialize the SVTKm library, parsing arguments when provided:
 * - Sets log level names when logging is configured.
 * - Sets the calling thread as the main thread for logging purposes.
 * - Sets the default log level to the argument provided to -v.
 * - Forces usage of the device name passed to -d or --device.
 * - Prints usage when -h is passed.
 *
 * The parameterless version only sets up log level names.
 *
 * Additional options may be supplied via the @a opts argument, such as
 * requiring the -d option.
 *
 * Results are available in the returned InitializeResult.
 *
 * @note This method may call exit() on parse error.
 * @{
 */
SVTKM_CONT_EXPORT
SVTKM_CONT
InitializeResult Initialize(int& argc,
                            char* argv[],
                            InitializeOptions opts = InitializeOptions::None);
SVTKM_CONT_EXPORT
SVTKM_CONT
InitializeResult Initialize();
/**@}*/
}
} // end namespace svtkm::cont


#endif // svtk_m_cont_Initialize_h
