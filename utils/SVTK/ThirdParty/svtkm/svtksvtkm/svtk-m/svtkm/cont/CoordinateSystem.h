//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CoordinateSystem_h
#define svtk_m_cont_CoordinateSystem_h

#include <svtkm/Bounds.h>

#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <svtkm/cont/CastAndCall.h>
#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace cont
{
class SVTKM_CONT_EXPORT CoordinateSystem : public svtkm::cont::Field
{
  using Superclass = svtkm::cont::Field;
  using CoordinatesTypeList = svtkm::List<svtkm::cont::ArrayHandleVirtualCoordinates::ValueType>;

public:
  SVTKM_CONT
  CoordinateSystem();

  SVTKM_CONT CoordinateSystem(std::string name,
                             const svtkm::cont::ArrayHandleVirtual<svtkm::Vec3f>& data);

  template <typename TypeList>
  SVTKM_CONT CoordinateSystem(std::string name,
                             const svtkm::cont::VariantArrayHandleBase<TypeList>& data);

  template <typename T, typename Storage>
  SVTKM_CONT CoordinateSystem(std::string name, const ArrayHandle<T, Storage>& data);

  /// This constructor of coordinate system sets up a regular grid of points.
  ///
  SVTKM_CONT
  CoordinateSystem(std::string name,
                   svtkm::Id3 dimensions,
                   svtkm::Vec3f origin = svtkm::Vec3f(0.0f, 0.0f, 0.0f),
                   svtkm::Vec3f spacing = svtkm::Vec3f(1.0f, 1.0f, 1.0f));

  SVTKM_CONT
  svtkm::Id GetNumberOfPoints() const { return this->GetNumberOfValues(); }

  SVTKM_CONT
  svtkm::cont::ArrayHandleVirtualCoordinates GetData() const;

  SVTKM_CONT void SetData(const svtkm::cont::ArrayHandleVirtual<svtkm::Vec3f>& newdata);

  template <typename T, typename Storage>
  SVTKM_CONT void SetData(const svtkm::cont::ArrayHandle<T, Storage>& newdata);

  SVTKM_CONT
  template <typename TypeList>
  void SetData(const svtkm::cont::VariantArrayHandleBase<TypeList>& newdata);

  SVTKM_CONT
  void GetRange(svtkm::Range* range) const
  {
    this->Superclass::GetRange(range, CoordinatesTypeList());
  }

  SVTKM_CONT
  svtkm::Vec<svtkm::Range, 3> GetRange() const
  {
    svtkm::Vec<svtkm::Range, 3> range;
    this->GetRange(&range[0]);
    return range;
  }

  SVTKM_CONT
  svtkm::cont::ArrayHandle<svtkm::Range> GetRangeAsArrayHandle() const
  {
    return this->Superclass::GetRange(CoordinatesTypeList());
  }

  SVTKM_CONT
  svtkm::Bounds GetBounds() const
  {
    svtkm::Range ranges[3];
    this->GetRange(ranges);
    return svtkm::Bounds(ranges[0], ranges[1], ranges[2]);
  }

  virtual void PrintSummary(std::ostream& out) const override;

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  SVTKM_CONT void ReleaseResourcesExecution() override
  {
    this->Superclass::ReleaseResourcesExecution();
    this->GetData().ReleaseResourcesExecution();
  }
};

template <typename Functor, typename... Args>
void CastAndCall(const svtkm::cont::CoordinateSystem& coords, Functor&& f, Args&&... args)
{
  CastAndCall(coords.GetData(), std::forward<Functor>(f), std::forward<Args>(args)...);
}

template <typename T>
svtkm::cont::CoordinateSystem make_CoordinateSystem(std::string name,
                                                   const std::vector<T>& data,
                                                   svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  return svtkm::cont::CoordinateSystem(name, svtkm::cont::make_ArrayHandle(data, copy));
}

template <typename T>
svtkm::cont::CoordinateSystem make_CoordinateSystem(std::string name,
                                                   const T* data,
                                                   svtkm::Id numberOfValues,
                                                   svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  return svtkm::cont::CoordinateSystem(name,
                                      svtkm::cont::make_ArrayHandle(data, numberOfValues, copy));
}

namespace internal
{

template <>
struct DynamicTransformTraits<svtkm::cont::CoordinateSystem>
{
  using DynamicTag = svtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal
} // namespace cont
} // namespace svtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

template <>
struct Serialization<svtkm::cont::CoordinateSystem>
{
  static SVTKM_CONT void save(BinaryBuffer& bb, const svtkm::cont::CoordinateSystem& cs)
  {
    svtkmdiy::save(bb, cs.GetName());
    svtkmdiy::save(bb, cs.GetData());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, svtkm::cont::CoordinateSystem& cs)
  {
    std::string name;
    svtkmdiy::load(bb, name);
    svtkm::cont::ArrayHandleVirtualCoordinates array;
    svtkmdiy::load(bb, array);
    cs = svtkm::cont::CoordinateSystem(name, array);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_CoordinateSystem_h
