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

#ifndef svtkmlib_DataSetConverters_h
#define svtkmlib_DataSetConverters_h

#include "svtkAcceleratorsSVTKmModule.h"

#include "ArrayConverters.h" // for FieldsFlag

#include "svtkmConfig.h" //required for general svtkm setup

#include <svtkm/cont/DataSet.h>

class svtkDataSet;
class svtkDataSetAttributes;
class svtkImageData;
class svtkPoints;
class svtkRectilinearGrid;
class svtkStructuredGrid;

namespace tosvtkm
{

// convert a svtkPoints array into a coordinate system
SVTKACCELERATORSSVTKM_EXPORT
svtkm::cont::CoordinateSystem Convert(svtkPoints* points);

// convert an structured grid type
SVTKACCELERATORSSVTKM_EXPORT
svtkm::cont::DataSet Convert(svtkStructuredGrid* input, FieldsFlag fields = FieldsFlag::None);

// determine the type and call the proper Convert routine
SVTKACCELERATORSSVTKM_EXPORT
svtkm::cont::DataSet Convert(svtkDataSet* input, FieldsFlag fields = FieldsFlag::None);
}

namespace fromsvtkm
{

SVTKACCELERATORSSVTKM_EXPORT
void PassAttributesInformation(svtkDataSetAttributes* input, svtkDataSetAttributes* output);

SVTKACCELERATORSSVTKM_EXPORT
bool Convert(const svtkm::cont::DataSet& svtkmOut, svtkRectilinearGrid* output, svtkDataSet* input);

SVTKACCELERATORSSVTKM_EXPORT
bool Convert(const svtkm::cont::DataSet& svtkmOut, svtkStructuredGrid* output, svtkDataSet* input);

}

#endif // svtkmlib_DataSetConverters_h
