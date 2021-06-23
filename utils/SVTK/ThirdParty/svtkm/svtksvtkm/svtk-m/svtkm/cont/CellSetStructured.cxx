//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define svtkm_cont_CellSetStructured_cxx
#include <svtkm/cont/CellSetStructured.h>

namespace svtkm
{
namespace cont
{

template class SVTKM_CONT_EXPORT CellSetStructured<1>;
template class SVTKM_CONT_EXPORT CellSetStructured<2>;
template class SVTKM_CONT_EXPORT CellSetStructured<3>;
}
}
