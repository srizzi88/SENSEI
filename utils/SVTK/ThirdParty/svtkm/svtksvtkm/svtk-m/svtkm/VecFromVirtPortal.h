//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_VecFromVirtPortal_h
#define svtk_m_VecFromVirtPortal_h

#include <svtkm/Types.h>

#include <svtkm/VecFromPortal.h>
#include <svtkm/internal/ArrayPortalVirtual.h>

namespace svtkm
{

/// \brief A short variable-length array from a window in an ArrayPortal.
///
/// The \c VecFromPortal class is a Vec-like class that holds an array portal
/// and exposes a small window of that portal as if it were a \c Vec.
///
template <typename T>
class SVTKM_ALWAYS_EXPORT VecFromVirtPortal
{
  using RefType = svtkm::internal::ArrayPortalValueReference<svtkm::ArrayPortalRef<T>>;

public:
  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  VecFromVirtPortal(const svtkm::ArrayPortalRef<T>* portal,
                    svtkm::IdComponent numComponents,
                    svtkm::Id offset)
    : Portal(portal)
    , NumComponents(numComponents)
    , Offset(offset)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfComponents() const { return this->NumComponents; }

  template <svtkm::IdComponent DestSize>
  SVTKM_EXEC_CONT void CopyInto(svtkm::Vec<T, DestSize>& dest) const
  {
    svtkm::IdComponent numComponents = svtkm::Min(DestSize, this->NumComponents);
    for (svtkm::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = this->Portal->Get(index + this->Offset);
    }
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  RefType operator[](svtkm::IdComponent index) const
  {
    return RefType(*this->Portal, index + this->Offset);
  }

private:
  const svtkm::ArrayPortalRef<T>* Portal = nullptr;
  svtkm::IdComponent NumComponents = 0;
  svtkm::Id Offset = 0;
};
}
#endif
