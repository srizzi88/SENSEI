//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define svtkm_filter_ExtractStructured_cxx
#include <svtkm/filter/ExtractStructured.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ExtractStructured::ExtractStructured()
  : svtkm::filter::FilterDataSet<ExtractStructured>()
  , VOI(svtkm::RangeId3(0, -1, 0, -1, 0, -1))
  , SampleRate(svtkm::Id3(1, 1, 1))
  , IncludeBoundary(false)
  , IncludeOffset(false)
  , Worklet()
{
}


//-----------------------------------------------------------------------------
SVTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(ExtractStructured);
}
}
