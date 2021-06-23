//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSet_h
#define svtk_m_cont_CellSet_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/StaticAssert.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/VariantArrayHandle.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT CellSet
{
public:
  CellSet() = default;
  CellSet(const CellSet& src) = default;
  CellSet(CellSet&& src) noexcept = default;

  CellSet& operator=(const CellSet& src) = default;
  CellSet& operator=(CellSet&& src) noexcept = default;

  virtual ~CellSet();

  virtual svtkm::Id GetNumberOfCells() const = 0;

  virtual svtkm::Id GetNumberOfFaces() const = 0;

  virtual svtkm::Id GetNumberOfEdges() const = 0;

  virtual svtkm::Id GetNumberOfPoints() const = 0;

  virtual svtkm::UInt8 GetCellShape(svtkm::Id id) const = 0;
  virtual svtkm::IdComponent GetNumberOfPointsInCell(svtkm::Id id) const = 0;
  virtual void GetCellPointIds(svtkm::Id id, svtkm::Id* ptids) const = 0;

  virtual std::shared_ptr<CellSet> NewInstance() const = 0;
  virtual void DeepCopy(const CellSet* src) = 0;

  virtual void PrintSummary(std::ostream&) const = 0;

  virtual void ReleaseResourcesExecution() = 0;
};

namespace internal
{

/// Checks to see if the given object is a cell set. It contains a
/// typedef named \c type that is either std::true_type or
/// std::false_type. Both of these have a typedef named value with the
/// respective boolean value.
///
template <typename T>
struct CellSetCheck
{
  using U = typename std::remove_pointer<T>::type;
  using type = typename std::is_base_of<svtkm::cont::CellSet, U>;
};

#define SVTKM_IS_CELL_SET(T) SVTKM_STATIC_ASSERT(::svtkm::cont::internal::CellSetCheck<T>::type::value)

} // namespace internal
}
} // namespace svtkm::cont

#endif //svtk_m_cont_CellSet_h
