//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_source_Source_h
#define svtk_m_source_Source_h

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/source/svtkm_source_export.h>
namespace svtkm
{
namespace source
{

class SVTKM_SOURCE_EXPORT Source
{
public:
  SVTKM_CONT
  Source();

  SVTKM_CONT
  virtual ~Source();

  virtual svtkm::cont::DataSet Execute() const = 0;

protected:
  svtkm::cont::Invoker Invoke;
};

} // namespace source
} // namespace svtkm

#endif // svtk_m_source_Source_h
