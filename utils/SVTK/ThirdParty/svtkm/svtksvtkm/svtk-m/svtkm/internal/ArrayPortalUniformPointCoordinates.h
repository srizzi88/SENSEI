//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_internal_ArrayPortalUniformPointCoordinates_h
#define svtk_m_internal_ArrayPortalUniformPointCoordinates_h

#include <svtkm/Assert.h>
#include <svtkm/Types.h>

namespace svtkm
{
namespace internal
{

/// \brief An implicit array port that computes point coordinates for a uniform grid.
///
class SVTKM_ALWAYS_EXPORT ArrayPortalUniformPointCoordinates
{
public:
  using ValueType = svtkm::Vec3f;

  SVTKM_EXEC_CONT
  ArrayPortalUniformPointCoordinates()
    : Dimensions(0)
    , NumberOfValues(0)
    , Origin(0, 0, 0)
    , Spacing(1, 1, 1)
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalUniformPointCoordinates(svtkm::Id3 dimensions, ValueType origin, ValueType spacing)
    : Dimensions(dimensions)
    , NumberOfValues(dimensions[0] * dimensions[1] * dimensions[2])
    , Origin(origin)
    , Spacing(spacing)
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalUniformPointCoordinates(const ArrayPortalUniformPointCoordinates& src)
    : Dimensions(src.Dimensions)
    , NumberOfValues(src.NumberOfValues)
    , Origin(src.Origin)
    , Spacing(src.Spacing)
  {
  }

  SVTKM_EXEC_CONT
  ArrayPortalUniformPointCoordinates& operator=(const ArrayPortalUniformPointCoordinates& src)
  {
    this->Dimensions = src.Dimensions;
    this->NumberOfValues = src.NumberOfValues;
    this->Origin = src.Origin;
    this->Spacing = src.Spacing;
    return *this;
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const
  {
    SVTKM_ASSERT(index >= 0);
    SVTKM_ASSERT(index < this->GetNumberOfValues());
    return this->Get(svtkm::Id3(index % this->Dimensions[0],
                               (index / this->Dimensions[0]) % this->Dimensions[1],
                               index / (this->Dimensions[0] * this->Dimensions[1])));
  }

  SVTKM_EXEC_CONT
  svtkm::Id3 GetRange3() const { return this->Dimensions; }

  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id3 index) const
  {
    SVTKM_ASSERT((index[0] >= 0) && (index[1] >= 0) && (index[2] >= 0));
    SVTKM_ASSERT((index[0] < this->Dimensions[0]) && (index[1] < this->Dimensions[1]) &&
                (index[2] < this->Dimensions[2]));
    return ValueType(this->Origin[0] + this->Spacing[0] * static_cast<svtkm::FloatDefault>(index[0]),
                     this->Origin[1] + this->Spacing[1] * static_cast<svtkm::FloatDefault>(index[1]),
                     this->Origin[2] +
                       this->Spacing[2] * static_cast<svtkm::FloatDefault>(index[2]));
  }

  SVTKM_EXEC_CONT
  const svtkm::Id3& GetDimensions() const { return this->Dimensions; }

  SVTKM_EXEC_CONT
  const ValueType& GetOrigin() const { return this->Origin; }

  SVTKM_EXEC_CONT
  const ValueType& GetSpacing() const { return this->Spacing; }

private:
  svtkm::Id3 Dimensions = { 0, 0, 0 };
  svtkm::Id NumberOfValues = 0;
  ValueType Origin = { 0.0f, 0.0f, 0.0f };
  ValueType Spacing = { 0.0f, 0.0f, 0.0f };
};
}
} // namespace svtkm::internal

#endif //svtk_m_internal_ArrayPortalUniformPointCoordinates_h
