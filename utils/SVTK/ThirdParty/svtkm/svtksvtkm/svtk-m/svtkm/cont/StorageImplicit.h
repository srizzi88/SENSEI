//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_StorageImplicit
#define svtk_m_cont_StorageImplicit

#include <svtkm/Types.h>

#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Storage.h>

#include <svtkm/cont/internal/ArrayTransfer.h>

namespace svtkm
{
namespace cont
{

/// \brief An implementation for read-only implicit arrays.
///
/// It is sometimes the case that you want SVTK-m to operate on an array of
/// implicit values. That is, rather than store the data in an actual array, it
/// is gerenated on the fly by a function. This is handled in SVTK-m by creating
/// an ArrayHandle in SVTK-m with a StorageTagImplicit type of \c Storage. This
/// tag itself is templated to specify an ArrayPortal that generates the
/// desired values. An ArrayHandle created with this tag will raise an error on
/// any operation that tries to modify it.
///
template <class ArrayPortalType>
struct SVTKM_ALWAYS_EXPORT StorageTagImplicit
{
  using PortalType = ArrayPortalType;
};

namespace internal
{

template <class ArrayPortalType>
class SVTKM_ALWAYS_EXPORT
  Storage<typename ArrayPortalType::ValueType, StorageTagImplicit<ArrayPortalType>>
{
  using ClassType =
    Storage<typename ArrayPortalType::ValueType, StorageTagImplicit<ArrayPortalType>>;

public:
  using ValueType = typename ArrayPortalType::ValueType;
  using PortalConstType = ArrayPortalType;

  // Note that this portal is likely to be read-only, so you will probably get an error
  // if you try to write to it.
  using PortalType = ArrayPortalType;

  SVTKM_CONT
  Storage(const PortalConstType& portal = PortalConstType())
    : Portal(portal)
    , NumberOfValues(portal.GetNumberOfValues())
  {
  }

  SVTKM_CONT Storage(const ClassType&) = default;
  SVTKM_CONT Storage(ClassType&&) = default;
  SVTKM_CONT ClassType& operator=(const ClassType&) = default;
  SVTKM_CONT ClassType& operator=(ClassType&&) = default;

  // All these methods do nothing but raise errors.
  SVTKM_CONT
  PortalType GetPortal() { throw svtkm::cont::ErrorBadValue("Implicit arrays are read-only."); }
  SVTKM_CONT
  PortalConstType GetPortalConst() const { return this->Portal; }
  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }
  SVTKM_CONT
  void Allocate(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(numberOfValues <= this->Portal.GetNumberOfValues());
    this->NumberOfValues = numberOfValues;
  }
  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues)
  {
    SVTKM_ASSERT(numberOfValues <= this->Portal.GetNumberOfValues());
    this->NumberOfValues = numberOfValues;
  }
  SVTKM_CONT
  void ReleaseResources() {}

private:
  PortalConstType Portal;
  svtkm::Id NumberOfValues;
};

template <typename T, class ArrayPortalType, class DeviceAdapterTag>
class ArrayTransfer<T, StorageTagImplicit<ArrayPortalType>, DeviceAdapterTag>
{
public:
  using StorageTag = StorageTagImplicit<ArrayPortalType>;
  using StorageType = svtkm::cont::internal::Storage<T, StorageTag>;

  using ValueType = T;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = PortalControl;
  using PortalConstExecution = PortalConstControl;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return this->Storage->GetPortalConst();
  }

#if defined(SVTKM_GCC) && defined(SVTKM_ENABLE_OPENMP) && (__GNUC__ == 6 && __GNUC_MINOR__ == 1)
// When using GCC 6.1 with OpenMP enabled we cause a compiler ICE that is
// an identified compiler regression (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71210)
// The easiest way to work around this is to make sure we aren't building with >= O2
#define NO_OPTIMIZE_FUNC_ATTRIBUTE __attribute__((optimize(1)))
#else // gcc 6.1 openmp compiler ICE workaround
#define NO_OPTIMIZE_FUNC_ATTRIBUTE
#endif

  SVTKM_CONT
  NO_OPTIMIZE_FUNC_ATTRIBUTE
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    throw svtkm::cont::ErrorBadValue("Implicit arrays cannot be used for output or in place.");
  }

  SVTKM_CONT
  NO_OPTIMIZE_FUNC_ATTRIBUTE
  PortalExecution PrepareForOutput(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadValue("Implicit arrays cannot be used for output.");
  }

#undef NO_OPTIMIZE_FUNC_ATTRIBUTE

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(controlArray)) const
  {
    throw svtkm::cont::ErrorBadValue("Implicit arrays cannot be used for output.");
  }

  SVTKM_CONT
  void Shrink(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadValue("Implicit arrays cannot be resized.");
  }

  SVTKM_CONT
  void ReleaseResources() {}

private:
  StorageType* Storage;
};

} // namespace internal
}
} // namespace svtkm::cont

#endif //svtk_m_cont_StorageImplicit
