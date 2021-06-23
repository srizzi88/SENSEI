//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define svtk_m_cont_CellSetExplicit_cxx

#include <svtkm/cont/CellSetExplicit.h>

namespace svtkm
{
namespace cont
{

template class SVTKM_CONT_EXPORT CellSetExplicit<>; // default
template class SVTKM_CONT_EXPORT
  CellSetExplicit<typename svtkm::cont::ArrayHandleConstant<svtkm::UInt8>::StorageTag,
                  SVTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
                  typename svtkm::cont::ArrayHandleCounting<svtkm::Id>::StorageTag>;
}
}
