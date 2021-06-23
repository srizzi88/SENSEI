/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearToQuadraticCellsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLinearToQuadraticCellsFilter.h"

#include "svtkAlgorithm.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkGenericCell.h"
#include "svtkHexahedron.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLine.h"
#include "svtkMergePoints.h"
#include "svtkPointData.h"
#include "svtkPointLocator.h"
#include "svtkPolygon.h"
#include "svtkPyramid.h"
#include "svtkQuad.h"
#include "svtkQuadraticEdge.h"
#include "svtkQuadraticHexahedron.h"
#include "svtkQuadraticPolygon.h"
#include "svtkQuadraticPyramid.h"
#include "svtkQuadraticQuad.h"
#include "svtkQuadraticTetra.h"
#include "svtkQuadraticTriangle.h"
#include "svtkQuadraticWedge.h"
#include "svtkTetra.h"
#include "svtkTriangle.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkWedge.h"

svtkStandardNewMacro(svtkLinearToQuadraticCellsFilter);

namespace
{
void DegreeElevate(svtkCell* lowerOrderCell, svtkIncrementalPointLocator* pointLocator,
  svtkUnsignedCharArray* types, svtkCellArray* cells, svtkPointData* inPd, svtkPointData* outPd,
  svtkCellData* inCd, svtkIdType cellId, svtkCellData* outCd)
{
  double lowerOrderCoeffs[SVTK_CELL_SIZE];

  svtkCell* higherOrderCell = nullptr;

  switch (lowerOrderCell->GetCellType())
  {

#define DegreeElevateCase(LowerOrderCellType, HigherOrderCell)                                     \
  case LowerOrderCellType:                                                                         \
    higherOrderCell = HigherOrderCell::New();                                                      \
    break

    DegreeElevateCase(SVTK_LINE, svtkQuadraticEdge);
    DegreeElevateCase(SVTK_TRIANGLE, svtkQuadraticTriangle);
    DegreeElevateCase(SVTK_QUAD, svtkQuadraticQuad);
    DegreeElevateCase(SVTK_POLYGON, svtkQuadraticPolygon);
    DegreeElevateCase(SVTK_TETRA, svtkQuadraticTetra);
    DegreeElevateCase(SVTK_HEXAHEDRON, svtkQuadraticHexahedron);
    DegreeElevateCase(SVTK_WEDGE, svtkQuadraticWedge);
    DegreeElevateCase(SVTK_PYRAMID, svtkQuadraticPyramid);

#undef DegreeElevateMacro

    default:
      svtkGenericWarningMacro(
        << "svtkLinearToQuadraticCellsFilter does not currently support degree elevating cell type "
        << lowerOrderCell->GetCellType() << ".");
      break;
  }

  if (higherOrderCell == nullptr)
  {
    return;
  }

  double* higherOrderPCoords = higherOrderCell->GetParametricCoords();
  for (svtkIdType hp = 0; hp < higherOrderCell->GetNumberOfPoints(); hp++)
  {
    lowerOrderCell->InterpolateFunctions(higherOrderPCoords + (hp * 3), lowerOrderCoeffs);

    double higherOrderPoint[3] = { 0., 0., 0. };
    double lowerOrderPoint[3];
    for (svtkIdType lp = 0; lp < lowerOrderCell->GetNumberOfPoints(); lp++)
    {
      // NB: svtkGenericCell creates a local copy of the cell's points, so we
      //     must use local indexing here (i.e. <lp> instead of
      //     <lowerOrderCell->GetPointIds()->GetId(lp)>).
      lowerOrderCell->GetPoints()->GetPoint(lp, lowerOrderPoint);
      ;
      for (int i = 0; i < 3; i++)
      {
        higherOrderPoint[i] += lowerOrderPoint[i] * lowerOrderCoeffs[lp];
      }
    }

    svtkIdType pId;
    pointLocator->InsertUniquePoint(higherOrderPoint, pId);
    higherOrderCell->GetPointIds()->SetId(hp, pId);

    outPd->InterpolatePoint(inPd, pId, lowerOrderCell->GetPointIds(), lowerOrderCoeffs);
  }

  svtkIdType newCellId = cells->InsertNextCell(higherOrderCell);
  types->InsertNextValue(higherOrderCell->GetCellType());
  outCd->CopyData(inCd, cellId, newCellId);

  higherOrderCell->Delete();
}

}

//----------------------------------------------------------------------------
svtkLinearToQuadraticCellsFilter::svtkLinearToQuadraticCellsFilter()
{
  this->Locator = nullptr;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
svtkLinearToQuadraticCellsFilter::~svtkLinearToQuadraticCellsFilter()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkLinearToQuadraticCellsFilter::SetLocator(svtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }

  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkLinearToQuadraticCellsFilter::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

//----------------------------------------------------------------------------
// Overload standard modified time function.
svtkMTimeType svtkLinearToQuadraticCellsFilter::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int svtkLinearToQuadraticCellsFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkUnstructuredGrid* input =
    svtkUnstructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkNew<svtkUnsignedCharArray> outputCellTypes;
  svtkNew<svtkCellArray> outputCellConnectivities;

  output->SetPoints(svtkNew<svtkPoints>());

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    output->GetPoints()->SetDataType(input->GetPoints()->GetDataType());
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    output->GetPoints()->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    output->GetPoints()->SetDataType(SVTK_DOUBLE);
  }

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(output->GetPoints(), input->GetBounds());

  svtkIdType estimatedSize = input->GetNumberOfCells();
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  output->GetPointData()->InterpolateAllocate(
    input->GetPointData(), estimatedSize, estimatedSize / 2);
  output->GetCellData()->CopyAllocate(input->GetCellData(), estimatedSize, estimatedSize / 2);

  svtkGenericCell* cell = svtkGenericCell::New();
  svtkCellIterator* it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    it->GetCell(cell);
    DegreeElevate(cell, this->Locator, outputCellTypes, outputCellConnectivities,
      input->GetPointData(), output->GetPointData(), input->GetCellData(), it->GetCellId(),
      output->GetCellData());
  }
  it->Delete();
  cell->Delete();

  output->SetCells(outputCellTypes, outputCellConnectivities);

  this->Locator->Initialize(); // release any extra memory
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void svtkLinearToQuadraticCellsFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
