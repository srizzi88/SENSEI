/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractGeometry.h"

#include "svtk3DLinearGridCrinkleExtractor.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkEventForwarderCommand.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLogger.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkExtractGeometry);
svtkCxxSetObjectMacro(svtkExtractGeometry, ImplicitFunction, svtkImplicitFunction);

//----------------------------------------------------------------------------
// Construct object with ExtractInside turned on.
svtkExtractGeometry::svtkExtractGeometry(svtkImplicitFunction* f)
{
  this->ImplicitFunction = f;
  if (this->ImplicitFunction)
  {
    this->ImplicitFunction->Register(this);
  }

  this->ExtractInside = 1;
  this->ExtractBoundaryCells = 0;
  this->ExtractOnlyBoundaryCells = 0;
}

//----------------------------------------------------------------------------
svtkExtractGeometry::~svtkExtractGeometry()
{
  this->SetImplicitFunction(nullptr);
}

// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
svtkMTimeType svtkExtractGeometry::GetMTime()
{
  svtkMTimeType mTime = this->MTime.GetMTime();
  svtkMTimeType impFuncMTime;

  if (this->ImplicitFunction != nullptr)
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int svtkExtractGeometry::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // May be nullptr, check before dereferencing.
  svtkUnstructuredGrid* gridInput = svtkUnstructuredGrid::SafeDownCast(input);

  if (!this->GetExtractInside() && this->GetExtractOnlyBoundaryCells() &&
    this->GetExtractBoundaryCells() &&
    svtk3DLinearGridCrinkleExtractor::CanFullyProcessDataObject(input))
  {
    svtkNew<svtk3DLinearGridCrinkleExtractor> linear3DExtractor;
    linear3DExtractor->SetImplicitFunction(this->GetImplicitFunction());
    linear3DExtractor->SetCopyPointData(true);
    linear3DExtractor->SetCopyCellData(true);

    svtkNew<svtkEventForwarderCommand> progressForwarder;
    progressForwarder->SetTarget(this);
    linear3DExtractor->AddObserver(svtkCommand::ProgressEvent, progressForwarder);

    int retval = linear3DExtractor->ProcessRequest(request, inputVector, outputVector);

    return retval;
  }

  svtkIdType ptId, numPts, numCells, i, newCellId, newId, *pointMap;
  svtkSmartPointer<svtkCellIterator> cellIter =
    svtkSmartPointer<svtkCellIterator>::Take(input->NewCellIterator());
  svtkIdList* pointIdList;
  int cellType;
  svtkIdType numCellPts;
  double x[3];
  double multiplier;
  svtkPoints* newPts;
  svtkIdList* newCellPts;
  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  int npts;

  svtkDebugMacro(<< "Extracting geometry");

  if (!this->ImplicitFunction)
  {
    svtkErrorMacro(<< "No implicit function specified");
    return 1;
  }

  // As this filter is doing a subsetting operation, set the Copy Tuple flag
  // for GlobalIds array so that, if present, it will be copied to the output.
  outputPD->CopyGlobalIdsOn();
  outputCD->CopyGlobalIdsOn();

  newCellPts = svtkIdList::New();
  newCellPts->Allocate(SVTK_CELL_SIZE);

  if (this->ExtractInside)
  {
    multiplier = 1.0;
  }
  else
  {
    multiplier = -1.0;
  }

  // Loop over all points determining whether they are inside the
  // implicit function. Copy the points and point data if they are.
  //
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  pointMap = new svtkIdType[numPts]; // maps old point ids into new
  for (i = 0; i < numPts; i++)
  {
    pointMap[i] = -1;
  }

  output->Allocate(numCells / 4); // allocate storage for geometry/topology
  newPts = svtkPoints::New();
  newPts->Allocate(numPts / 4, numPts);
  outputPD->CopyAllocate(pd);
  outputCD->CopyAllocate(cd);
  svtkFloatArray* newScalars = nullptr;

  if (!this->ExtractBoundaryCells)
  {
    for (ptId = 0; ptId < numPts; ptId++)
    {
      input->GetPoint(ptId, x);
      if ((this->ImplicitFunction->FunctionValue(x) * multiplier) < 0.0)
      {
        newId = newPts->InsertNextPoint(x);
        pointMap[ptId] = newId;
        outputPD->CopyData(pd, ptId, newId);
      }
    }
  }
  else
  {
    // To extract boundary cells, we have to create supplemental information
    double val;
    newScalars = svtkFloatArray::New();
    newScalars->SetNumberOfValues(numPts);

    for (ptId = 0; ptId < numPts; ptId++)
    {
      input->GetPoint(ptId, x);
      val = this->ImplicitFunction->FunctionValue(x) * multiplier;
      newScalars->SetValue(ptId, val);
    }
  }

  // Now loop over all cells to see whether they are inside implicit
  // function (or on boundary if ExtractBoundaryCells is on).
  //
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    cellType = cellIter->GetCellType();
    numCellPts = cellIter->GetNumberOfPoints();
    pointIdList = cellIter->GetPointIds();

    newCellPts->Reset();
    if (!this->ExtractBoundaryCells) // requires less work
    {
      for (npts = 0, i = 0; i < numCellPts; i++, npts++)
      {
        ptId = pointIdList->GetId(i);
        if (pointMap[ptId] < 0)
        {
          break; // this cell won't be inserted
        }
        else
        {
          newCellPts->InsertId(i, pointMap[ptId]);
        }
      }
    } // if don't want to extract boundary cells

    else // want boundary cells
    {
      for (npts = 0, i = 0; i < numCellPts; i++)
      {
        ptId = pointIdList->GetId(i);
        if (newScalars->GetValue(ptId) <= 0.0)
        {
          npts++;
        }
      }
      int extraction_condition = 0;
      if (this->ExtractOnlyBoundaryCells)
      {
        if ((npts > 0) && (npts != numCellPts))
        {
          extraction_condition = 1;
        }
      }
      else
      {
        if (npts > 0)
        {
          extraction_condition = 1;
        }
      }
      if (extraction_condition)
      {
        for (i = 0; i < numCellPts; i++)
        {
          ptId = pointIdList->GetId(i);
          if (pointMap[ptId] < 0)
          {
            input->GetPoint(ptId, x);
            newId = newPts->InsertNextPoint(x);
            pointMap[ptId] = newId;
            outputPD->CopyData(pd, ptId, newId);
          }
          newCellPts->InsertId(i, pointMap[ptId]);
        }
      } // a boundary or interior cell
    }   // if mapping boundary cells

    int extraction_condition = 0;
    if (this->ExtractOnlyBoundaryCells)
    {
      if (npts != numCellPts && (this->ExtractBoundaryCells && npts > 0))
      {
        extraction_condition = 1;
      }
    }
    else
    {
      if (npts >= numCellPts || (this->ExtractBoundaryCells && npts > 0))
      {
        extraction_condition = 1;
      }
    }
    if (extraction_condition)
    {
      // special handling for polyhedron cells
      if (gridInput && cellType == SVTK_POLYHEDRON)
      {
        newCellPts->Reset();
        gridInput->GetFaceStream(cellIter->GetCellId(), newCellPts);
        svtkUnstructuredGrid::ConvertFaceStreamPointIds(newCellPts, pointMap);
      }
      newCellId = output->InsertNextCell(cellType, newCellPts);
      outputCD->CopyData(cd, cellIter->GetCellId(), newCellId);
    }
  } // for all cells

  // Update ourselves and release memory
  //
  delete[] pointMap;
  newCellPts->Delete();
  output->SetPoints(newPts);
  newPts->Delete();

  if (this->ExtractBoundaryCells)
  {
    newScalars->Delete();
  }

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractGeometry::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractGeometry::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Implicit Function: " << static_cast<void*>(this->ImplicitFunction) << "\n";
  os << indent << "Extract Inside: " << (this->ExtractInside ? "On\n" : "Off\n");
  os << indent << "Extract Boundary Cells: " << (this->ExtractBoundaryCells ? "On\n" : "Off\n");
  os << indent
     << "Extract Only Boundary Cells: " << (this->ExtractOnlyBoundaryCells ? "On\n" : "Off\n");
}
