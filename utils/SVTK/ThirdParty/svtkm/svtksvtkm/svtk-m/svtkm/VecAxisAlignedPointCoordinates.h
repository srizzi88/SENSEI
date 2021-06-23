//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_VecAxisAlignedPointCoordinates_h
#define svtk_m_VecAxisAlignedPointCoordinates_h

#include <svtkm/Math.h>
#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>
#include <svtkm/internal/ExportMacros.h>

namespace svtkm
{

namespace detail
{

/// Specifies the size of VecAxisAlignedPointCoordinates for the given
/// dimension.
///
template <svtkm::IdComponent NumDimensions>
struct VecAxisAlignedPointCoordinatesNumComponents;

template <>
struct VecAxisAlignedPointCoordinatesNumComponents<1>
{
  static constexpr svtkm::IdComponent NUM_COMPONENTS = 2;
};

template <>
struct VecAxisAlignedPointCoordinatesNumComponents<2>
{
  static constexpr svtkm::IdComponent NUM_COMPONENTS = 4;
};

template <>
struct VecAxisAlignedPointCoordinatesNumComponents<3>
{
  static constexpr svtkm::IdComponent NUM_COMPONENTS = 8;
};

struct VecAxisAlignedPointCoordinatesOffsetTable
{
  SVTKM_EXEC_CONT svtkm::FloatDefault Get(svtkm::Int32 i, svtkm::Int32 j) const
  {
    SVTKM_STATIC_CONSTEXPR_ARRAY svtkm::FloatDefault offsetTable[8][3] = {
      { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
      { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f }
    };
    return offsetTable[i][j];
  }
};

} // namespace detail

/// \brief An implicit vector for point coordinates in axis aligned cells. For
/// internal use only.
///
/// The \c VecAxisAlignedPointCoordinates class is a Vec-like class that holds
/// the point coordinates for a axis aligned cell. The class is templated on the
/// dimensions of the cell, which can be 1 (for a line).
///
/// This is an internal class used to represent coordinates for uniform datasets
/// in an execution environment when executing a WorkletMapPointToCell. Users
/// should not directly construct this class under any circumstances. Use the
/// related ArrayPortalUniformPointCoordinates and
/// ArrayHandleUniformPointCoordinates classes instead.
///
template <svtkm::IdComponent NumDimensions>
class VecAxisAlignedPointCoordinates
{
public:
  using ComponentType = svtkm::Vec3f;

  static constexpr svtkm::IdComponent NUM_COMPONENTS =
    detail::VecAxisAlignedPointCoordinatesNumComponents<NumDimensions>::NUM_COMPONENTS;

  SVTKM_EXEC_CONT
  VecAxisAlignedPointCoordinates(ComponentType origin = ComponentType(0, 0, 0),
                                 ComponentType spacing = ComponentType(1, 1, 1))
    : Origin(origin)
    , Spacing(spacing)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfComponents() const { return NUM_COMPONENTS; }

  template <svtkm::IdComponent DestSize>
  SVTKM_EXEC_CONT void CopyInto(svtkm::Vec<ComponentType, DestSize>& dest) const
  {
    svtkm::IdComponent numComponents = svtkm::Min(DestSize, this->GetNumberOfComponents());
    for (svtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  SVTKM_EXEC_CONT
  ComponentType operator[](svtkm::IdComponent index) const
  {
    detail::VecAxisAlignedPointCoordinatesOffsetTable table;
    return ComponentType(this->Origin[0] + table.Get(index, 0) * this->Spacing[0],
                         this->Origin[1] + table.Get(index, 1) * this->Spacing[1],
                         this->Origin[2] + table.Get(index, 2) * this->Spacing[2]);
  }

  SVTKM_EXEC_CONT
  const ComponentType& GetOrigin() const { return this->Origin; }

  SVTKM_EXEC_CONT
  const ComponentType& GetSpacing() const { return this->Spacing; }

private:
  // Position of lower left point.
  ComponentType Origin;

  // Spacing in the x, y, and z directions.
  ComponentType Spacing;
};

template <svtkm::IdComponent NumDimensions>
struct TypeTraits<svtkm::VecAxisAlignedPointCoordinates<NumDimensions>>
{
  using NumericTag = svtkm::TypeTraitsRealTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  SVTKM_EXEC_CONT
  static svtkm::VecAxisAlignedPointCoordinates<NumDimensions> ZeroInitialization()
  {
    return svtkm::VecAxisAlignedPointCoordinates<NumDimensions>(svtkm::Vec3f(0, 0, 0),
                                                               svtkm::Vec3f(0, 0, 0));
  }
};

template <svtkm::IdComponent NumDimensions>
struct VecTraits<svtkm::VecAxisAlignedPointCoordinates<NumDimensions>>
{
  using VecType = svtkm::VecAxisAlignedPointCoordinates<NumDimensions>;

  using ComponentType = svtkm::Vec3f;
  using BaseComponentType = svtkm::FloatDefault;
  using HasMultipleComponents = svtkm::VecTraitsTagMultipleComponents;
  using IsSizeStatic = svtkm::VecTraitsTagSizeStatic;

  static constexpr svtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  SVTKM_EXEC_CONT
  static svtkm::IdComponent GetNumberOfComponents(const VecType&) { return NUM_COMPONENTS; }

  SVTKM_EXEC_CONT
  static ComponentType GetComponent(const VecType& vector, svtkm::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  // These are a bit of a hack since VecAxisAlignedPointCoordinates only supports one component
  // type. Using these might not work as expected.
  template <typename NewComponentType>
  using ReplaceComponentType = svtkm::Vec<NewComponentType, NUM_COMPONENTS>;
  template <typename NewComponentType>
  using ReplaceBaseComponenttype = svtkm::Vec<svtkm::Vec<NewComponentType, 3>, NUM_COMPONENTS>;

  template <svtkm::IdComponent destSize>
  SVTKM_EXEC_CONT static void CopyInto(const VecType& src, svtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace svtkm

#endif //svtk_m_VecAxisAlignedPointCoordinates_h
