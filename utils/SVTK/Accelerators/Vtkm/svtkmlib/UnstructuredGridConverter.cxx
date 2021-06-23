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

#include "UnstructuredGridConverter.h"

#include "ArrayConverters.h"
#include "CellSetConverters.h"
#include "DataSetConverters.h"

// datasets we support
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkDataObject.h"
#include "svtkDataObjectTypes.h"
#include "svtkDataSetAttributes.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStructuredGrid.h"
#include "svtkUniformGrid.h"
#include "svtkUnstructuredGrid.h"

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/Field.h>

namespace tosvtkm
{

//------------------------------------------------------------------------------
// convert an unstructured grid type
svtkm::cont::DataSet Convert(svtkUnstructuredGrid* input, FieldsFlag fields)
{
  // This will need to use the custom storage and portals so that
  // we can efficiently map between SVTK and SVTKm
  svtkm::cont::DataSet dataset;

  // first step convert the points over to an array handle
  svtkm::cont::CoordinateSystem coords = Convert(input->GetPoints());
  dataset.AddCoordinateSystem(coords);
  // last

  // Use our custom explicit cell set to do the conversion
  const svtkIdType numPoints = input->GetNumberOfPoints();
  if (input->IsHomogeneous())
  {
    int cellType = input->GetCellType(0); // get the celltype
    svtkm::cont::DynamicCellSet cells = ConvertSingleType(input->GetCells(), cellType, numPoints);
    dataset.SetCellSet(cells);
  }
  else
  {
    svtkm::cont::DynamicCellSet cells =
      Convert(input->GetCellTypesArray(), input->GetCells(), numPoints);
    dataset.SetCellSet(cells);
  }

  ProcessFields(input, dataset, fields);

  return dataset;
}

} // namespace tosvtkm

namespace fromsvtkm
{

//------------------------------------------------------------------------------
bool Convert(const svtkm::cont::DataSet& voutput, svtkUnstructuredGrid* output, svtkDataSet* input)
{
  svtkPoints* points = fromsvtkm::Convert(voutput.GetCoordinateSystem());
  // If this fails, it's likely a missing entry in tosvtkm::PointListOutSVTK:
  if (!points)
  {
    return false;
  }
  output->SetPoints(points);
  points->FastDelete();

  // With unstructured grids we need to actually convert 3 arrays from
  // svtkm to svtk
  svtkNew<svtkCellArray> cells;
  svtkNew<svtkUnsignedCharArray> types;
  svtkm::cont::DynamicCellSet outCells = voutput.GetCellSet();

  const bool cellsConverted = fromsvtkm::Convert(outCells, cells.GetPointer(), types.GetPointer());

  if (!cellsConverted)
  {
    return false;
  }

  output->SetCells(types.GetPointer(), cells.GetPointer());

  // now have to set this info back to the unstructured grid

  // Next we need to convert any extra fields from svtkm over to svtk
  const bool arraysConverted = fromsvtkm::ConvertArrays(voutput, output);

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return arraysConverted;
}

} // namespace fromsvtkm
