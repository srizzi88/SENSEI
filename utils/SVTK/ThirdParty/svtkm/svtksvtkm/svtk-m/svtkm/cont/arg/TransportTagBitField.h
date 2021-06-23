//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagBitField_h
#define svtk_m_cont_arg_TransportTagBitField_h

#include <svtkm/cont/arg/Transport.h>

#include <svtkm/cont/BitField.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

struct TransportTagBitFieldIn
{
};
struct TransportTagBitFieldOut
{
};
struct TransportTagBitFieldInOut
{
};

template <typename Device>
struct Transport<svtkm::cont::arg::TransportTagBitFieldIn, svtkm::cont::BitField, Device>
{
  using ExecObjectType =
    typename svtkm::cont::BitField::template ExecutionTypes<Device>::PortalConst;

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType
  operator()(svtkm::cont::BitField& field, const InputDomainType&, svtkm::Id, svtkm::Id) const
  {
    return field.PrepareForInput(Device{});
  }
};

template <typename Device>
struct Transport<svtkm::cont::arg::TransportTagBitFieldOut, svtkm::cont::BitField, Device>
{
  using ExecObjectType = typename svtkm::cont::BitField::template ExecutionTypes<Device>::Portal;

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType
  operator()(svtkm::cont::BitField& field, const InputDomainType&, svtkm::Id, svtkm::Id) const
  {
    // This behaves similarly to WholeArray tags, where "Out" maps to InPlace
    // since we don't want to reallocate or enforce size restrictions.
    return field.PrepareForInPlace(Device{});
  }
};

template <typename Device>
struct Transport<svtkm::cont::arg::TransportTagBitFieldInOut, svtkm::cont::BitField, Device>
{
  using ExecObjectType = typename svtkm::cont::BitField::template ExecutionTypes<Device>::Portal;

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType
  operator()(svtkm::cont::BitField& field, const InputDomainType&, svtkm::Id, svtkm::Id) const
  {
    return field.PrepareForInPlace(Device{});
  }
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagBitField_h
