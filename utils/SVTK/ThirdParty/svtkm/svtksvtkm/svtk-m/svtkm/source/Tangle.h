//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_source_Tangle_h
#define svtk_m_source_Tangle_h

#include <svtkm/source/Source.h>

namespace svtkm
{
namespace source
{
/**
 * @brief The Tangle source creates a uniform dataset.
 *
 * This class generates a predictable uniform grid dataset with an
 * interesting set of point and cell scalar arrays, which is useful
 * for testing and benchmarking.
 *
 * The Execute method creates a complete structured dataset that have a
 * point field named 'nodevar', and a cell field named 'cellvar'.
 *
**/
class SVTKM_SOURCE_EXPORT Tangle final : public svtkm::source::Source
{
public:
  ///Construct a Tangle with Cell Dimensions
  SVTKM_CONT
  Tangle(svtkm::Id3 dims)
    : Dims(dims)
  {
  }

  svtkm::cont::DataSet Execute() const;

private:
  svtkm::Id3 Dims;
};
} //namespace source
} //namespace svtkm

#endif //svtk_m_source_Tangle_h
