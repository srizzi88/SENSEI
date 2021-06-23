//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef svtkmlib_PolyDataConverter_h
#define svtkmlib_PolyDataConverter_h

#include "svtkAcceleratorsSVTKmModule.h"

#include "ArrayConverters.h" // for FieldsFlag

#include "svtkmConfig.h" //required for general svtkm setup

#include <svtkm/cont/DataSet.h>

class svtkPolyData;
class svtkDataSet;

namespace tosvtkm
{
// convert an polydata type
SVTKACCELERATORSSVTKM_EXPORT
svtkm::cont::DataSet Convert(svtkPolyData* input, FieldsFlag fields = FieldsFlag::None);
}

namespace fromsvtkm
{
SVTKACCELERATORSSVTKM_EXPORT
bool Convert(const svtkm::cont::DataSet& voutput, svtkPolyData* output, svtkDataSet* input);
}
#endif // svtkmlib_PolyDataConverter_h
