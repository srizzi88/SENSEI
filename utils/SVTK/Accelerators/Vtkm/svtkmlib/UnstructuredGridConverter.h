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

#ifndef svtkmlib_UnstructuredGridConverter_h
#define svtkmlib_UnstructuredGridConverter_h

#include "svtkAcceleratorsSVTKmModule.h"

#include "ArrayConverters.h" // For FieldsFlag

#include "svtkmConfig.h" //required for general svtkm setup

#include <svtkm/cont/DataSet.h>

class svtkUnstructuredGrid;
class svtkDataSet;

namespace tosvtkm
{

// convert an unstructured grid type
SVTKACCELERATORSSVTKM_EXPORT
svtkm::cont::DataSet Convert(svtkUnstructuredGrid* input, FieldsFlag fields = FieldsFlag::None);
}

namespace fromsvtkm
{
SVTKACCELERATORSSVTKM_EXPORT
bool Convert(const svtkm::cont::DataSet& voutput, svtkUnstructuredGrid* output, svtkDataSet* input);
}
#endif // svtkmlib_UnstructuredGridConverter_h
