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

#ifndef svtkmlib_CellSetConverters_h
#define svtkmlib_CellSetConverters_h

#include "svtkAcceleratorsSVTKmModule.h"
#include "svtkmConfig.h" //required for general svtkm setup

#include <svtkType.h>
#include <svtkm/cont/DynamicCellSet.h>

class svtkCellArray;
class svtkUnsignedCharArray;
class svtkIdTypeArray;

namespace tosvtkm
{
SVTKACCELERATORSSVTKM_EXPORT
svtkm::cont::DynamicCellSet ConvertSingleType(
  svtkCellArray* cells, int cellType, svtkIdType numberOfPoints);

SVTKACCELERATORSSVTKM_EXPORT
svtkm::cont::DynamicCellSet Convert(
  svtkUnsignedCharArray* types, svtkCellArray* cells, svtkIdType numberOfPoints);
}

namespace fromsvtkm
{

SVTKACCELERATORSSVTKM_EXPORT
bool Convert(const svtkm::cont::DynamicCellSet& toConvert, svtkCellArray* cells,
  svtkUnsignedCharArray* types = nullptr);
}

#endif // svtkmlib_CellSetConverters_h
