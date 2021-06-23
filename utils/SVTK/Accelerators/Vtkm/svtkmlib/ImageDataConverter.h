/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageDataConverter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkmlib_ImageDataConverter_h
#define svtkmlib_ImageDataConverter_h

#include "svtkAcceleratorsSVTKmModule.h"

#include "ArrayConverters.h" // for FieldsFlag

#include "svtkmConfig.h" //required for general svtkm setup

#include <svtkm/cont/DataSet.h>

class svtkImageData;
class svtkDataSet;

namespace tosvtkm
{

SVTKACCELERATORSSVTKM_EXPORT
svtkm::cont::DataSet Convert(svtkImageData* input, FieldsFlag fields = FieldsFlag::None);

}

namespace fromsvtkm
{

SVTKACCELERATORSSVTKM_EXPORT
bool Convert(const svtkm::cont::DataSet& voutput, svtkImageData* output, svtkDataSet* input);

SVTKACCELERATORSSVTKM_EXPORT
bool Convert(
  const svtkm::cont::DataSet& voutput, int extents[6], svtkImageData* output, svtkDataSet* input);

}
#endif // svtkmlib_ImageDataConverter_h
