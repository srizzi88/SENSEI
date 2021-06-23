//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_VirtualObjectTransferShareWithControl_h
#define svtk_m_cont_internal_VirtualObjectTransferShareWithControl_h

#include <svtkm/StaticAssert.h>
#include <svtkm/VirtualObjectBase.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename VirtualDerivedType>
struct VirtualObjectTransferShareWithControl
{
  SVTKM_CONT VirtualObjectTransferShareWithControl(const VirtualDerivedType* virtualObject)
    : VirtualObject(virtualObject)
  {
  }

  SVTKM_CONT const VirtualDerivedType* PrepareForExecution(bool svtkmNotUsed(updateData))
  {
    return this->VirtualObject;
  }

  SVTKM_CONT void ReleaseResources() {}

private:
  const VirtualDerivedType* VirtualObject;
};
}
}
} // svtkm::cont::internal

#endif // svtk_m_cont_internal_VirtualObjectTransferShareWithControl_h
