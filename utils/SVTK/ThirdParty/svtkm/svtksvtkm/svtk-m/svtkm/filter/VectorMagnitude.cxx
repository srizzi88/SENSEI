//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define svtkm_filter_VectorMagnitude_cxx

#include <svtkm/filter/VectorMagnitude.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
VectorMagnitude::VectorMagnitude()
  : svtkm::filter::FilterField<VectorMagnitude>()
  , Worklet()
{
  this->SetOutputFieldName("magnitude");
}

//-----------------------------------------------------------------------------
SVTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(VectorMagnitude);
}
}
