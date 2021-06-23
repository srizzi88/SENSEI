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
#include "PolyDataConverter.h"

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
// convert an polydata type
svtkm::cont::DataSet Convert(svtkPolyData* input, FieldsFlag fields)
{
  // the poly data is an interesting issue with the fact that the
  // svtk datastructure can contain multiple types.
  // we should look at querying the cell types, so we can use single cell
  // set where possible
  svtkm::cont::DataSet dataset;

  // first step convert the points over to an array handle
  svtkm::cont::CoordinateSystem coords = Convert(input->GetPoints());
  dataset.AddCoordinateSystem(coords);

  // first check if we only have polys/lines/verts
  bool filled = false;
  const bool onlyPolys = (input->GetNumberOfCells() == input->GetNumberOfPolys());
  const bool onlyLines = (input->GetNumberOfCells() == input->GetNumberOfLines());
  const bool onlyVerts = (input->GetNumberOfCells() == input->GetNumberOfVerts());

  const svtkIdType numPoints = input->GetNumberOfPoints();
  if (onlyPolys)
  {
    svtkCellArray* cells = input->GetPolys();
    const svtkIdType homoSize = cells->IsHomogeneous();
    if (homoSize == 3)
    {
      // We are all triangles
      svtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, SVTK_TRIANGLE, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else if (homoSize == 4)
    {
      // We are all quads
      svtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, SVTK_QUAD, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else
    {
      // we have mixture of polygins/quads/triangles, we don't support that
      // currently
      svtkErrorWithObjectMacro(input,
        "SVTK-m currently only handles svtkPolyData "
        "with only triangles or only quads.");
    }
  }
  else if (onlyLines)
  {
    svtkCellArray* cells = input->GetLines();
    const svtkIdType homoSize = cells->IsHomogeneous();
    if (homoSize == 2)
    {
      // We are all lines
      svtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, SVTK_LINE, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else
    {
      svtkErrorWithObjectMacro(input,
        "SVTK-m does not currently support "
        "PolyLine cells.");
    }
  }
  else if (onlyVerts)
  {
    svtkCellArray* cells = input->GetVerts();
    const svtkIdType homoSize = cells->IsHomogeneous();
    if (homoSize == 1)
    {
      // We are all single vertex
      svtkm::cont::DynamicCellSet dcells = ConvertSingleType(cells, SVTK_VERTEX, numPoints);
      dataset.SetCellSet(dcells);
      filled = true;
    }
    else
    {
      svtkErrorWithObjectMacro(input,
        "SVTK-m does not currently support "
        "PolyVertex cells.");
    }
  }
  else
  {
    svtkErrorWithObjectMacro(input,
      "SVTK-m does not currently support mixed "
      "cell types or triangle strips in "
      "svtkPolyData.");
  }

  if (!filled)
  {
    // todo: we need to convert a mixed type which
  }

  ProcessFields(input, dataset, fields);

  return dataset;
}

} // namespace tosvtkm

namespace fromsvtkm
{

//------------------------------------------------------------------------------
bool Convert(const svtkm::cont::DataSet& voutput, svtkPolyData* output, svtkDataSet* input)
{
  svtkPoints* points = fromsvtkm::Convert(voutput.GetCoordinateSystem());
  output->SetPoints(points);
  points->FastDelete();

  // this should be fairly easy as the cells are all a single cell type
  // so we just need to determine what cell type it is and copy the results
  // into a new array
  svtkm::cont::DynamicCellSet outCells = voutput.GetCellSet();
  svtkNew<svtkCellArray> cells;
  const bool cellsConverted = fromsvtkm::Convert(outCells, cells.GetPointer());
  if (!cellsConverted)
  {
    return false;
  }

  output->SetPolys(cells.GetPointer());

  // next we need to convert any extra fields from svtkm over to svtk
  bool arraysConverted = ConvertArrays(voutput, output);

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return arraysConverted;
}

} // namespace fromsvtkm
