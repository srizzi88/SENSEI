//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_Loggable_h
#define svtk_m_rendering_raytracing_Loggable_h

#include <sstream>
#include <stack>

#include <svtkm/Types.h>
#include <svtkm/rendering/svtkm_rendering_export.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class SVTKM_RENDERING_EXPORT Logger
{
public:
  ~Logger();
  static Logger* GetInstance();
  void OpenLogEntry(const std::string& entryName);
  void CloseLogEntry(const svtkm::Float64& entryTime);
  void Clear();
  template <typename T>
  void AddLogData(const std::string key, const T& value)
  {
    this->Stream << key << " " << value << "\n";
  }

  std::stringstream& GetStream();

protected:
  Logger();
  Logger(Logger const&);
  std::stringstream Stream;
  static class Logger* Instance;
  std::stack<std::string> Entries;
};
}
}
} // namespace svtkm::rendering::raytracing
#endif
