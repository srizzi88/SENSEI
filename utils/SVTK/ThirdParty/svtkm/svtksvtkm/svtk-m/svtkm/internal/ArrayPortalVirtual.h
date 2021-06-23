//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_internal_ArrayPortalVirtual_h
#define svtk_m_internal_ArrayPortalVirtual_h


#include <svtkm/VecTraits.h>
#include <svtkm/VirtualObjectBase.h>

#include <svtkm/internal/ArrayPortalHelpers.h>
#include <svtkm/internal/ExportMacros.h>

namespace svtkm
{
namespace internal
{

class SVTKM_ALWAYS_EXPORT PortalVirtualBase
{
public:
  SVTKM_EXEC_CONT PortalVirtualBase() noexcept {}

  SVTKM_EXEC_CONT virtual ~PortalVirtualBase() noexcept {
    //we implement this as we need a destructor with cuda markup.
    //Using =default causes cuda free errors inside VirtualObjectTransferCuda
  };
};

} // namespace internal

template <typename T>
class SVTKM_ALWAYS_EXPORT ArrayPortalVirtual : public internal::PortalVirtualBase
{
public:
  using ValueType = T;

  //use parents constructor
  using PortalVirtualBase::PortalVirtualBase;

  SVTKM_EXEC_CONT virtual ~ArrayPortalVirtual<T>(){};

  SVTKM_EXEC_CONT virtual T Get(svtkm::Id index) const noexcept = 0;

  SVTKM_EXEC_CONT virtual void Set(svtkm::Id, const T&) const noexcept {}
};


template <typename PortalT>
class SVTKM_ALWAYS_EXPORT ArrayPortalWrapper final
  : public svtkm::ArrayPortalVirtual<typename PortalT::ValueType>
{
  using T = typename PortalT::ValueType;

public:
  ArrayPortalWrapper(const PortalT& p) noexcept : ArrayPortalVirtual<T>(), Portal(p) {}

  SVTKM_EXEC
  T Get(svtkm::Id index) const noexcept
  {
    using call_supported_t = typename internal::PortalSupportsGets<PortalT>::type;
    return this->Get(call_supported_t(), index);
  }

  SVTKM_EXEC
  void Set(svtkm::Id index, const T& value) const noexcept
  {
    using call_supported_t = typename internal::PortalSupportsSets<PortalT>::type;
    this->Set(call_supported_t(), index, value);
  }

private:
  // clang-format off
  SVTKM_EXEC inline T Get(std::true_type, svtkm::Id index) const noexcept { return this->Portal.Get(index); }
  SVTKM_EXEC inline T Get(std::false_type, svtkm::Id) const noexcept { return T{}; }
  SVTKM_EXEC inline void Set(std::true_type, svtkm::Id index, const T& value) const noexcept { this->Portal.Set(index, value); }
  SVTKM_EXEC inline void Set(std::false_type, svtkm::Id, const T&) const noexcept {}
  // clang-format on


  PortalT Portal;
};


template <typename T>
class SVTKM_ALWAYS_EXPORT ArrayPortalRef
{
public:
  using ValueType = T;

  ArrayPortalRef() noexcept : Portal(nullptr), NumberOfValues(0) {}

  ArrayPortalRef(const ArrayPortalVirtual<T>* portal, svtkm::Id numValues) noexcept
    : Portal(portal),
      NumberOfValues(numValues)
  {
  }

  //Currently this needs to be valid on both the host and device for cuda, so we can't
  //call the underlying portal as that uses device virtuals and the method will fail.
  //We need to seriously look at the interaction of portals and iterators for device
  //adapters and determine a better approach as iterators<Portal> are really fat
  SVTKM_EXEC_CONT inline svtkm::Id GetNumberOfValues() const noexcept { return this->NumberOfValues; }

  //This isn't valid on the host for cuda
  SVTKM_EXEC_CONT inline T Get(svtkm::Id index) const noexcept { return this->Portal->Get(index); }

  //This isn't valid on the host for
  SVTKM_EXEC_CONT inline void Set(svtkm::Id index, const T& t) const noexcept
  {
    this->Portal->Set(index, t);
  }

  const ArrayPortalVirtual<T>* Portal;
  svtkm::Id NumberOfValues;
};

template <typename T>
inline ArrayPortalRef<T> make_ArrayPortalRef(const ArrayPortalVirtual<T>* portal,
                                             svtkm::Id numValues) noexcept
{
  return ArrayPortalRef<T>(portal, numValues);
}


} // namespace svtkm

#endif
