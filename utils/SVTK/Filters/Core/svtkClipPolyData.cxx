/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClipPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkClipPolyData.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkGenericCell.h"
#include "svtkImplicitFunction.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLine.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkTriangle.h"

#include <cmath>

svtkStandardNewMacro(svtkClipPolyData);
svtkCxxSetObjectMacro(svtkClipPolyData, ClipFunction, svtkImplicitFunction);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
svtkClipPolyData::svtkClipPolyData(svtkImplicitFunction* cf)
{
  this->ClipFunction = cf;
  this->InsideOut = 0;
  this->Locator = nullptr;
  this->Value = 0.0;
  this->GenerateClipScalars = 0;
  this->GenerateClippedOutput = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->SetNumberOfOutputPorts(2);

  svtkPolyData* output2 = svtkPolyData::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();
}

//----------------------------------------------------------------------------
svtkClipPolyData::~svtkClipPolyData()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  this->SetClipFunction(nullptr);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
svtkMTimeType svtkClipPolyData::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->ClipFunction != nullptr)
  {
    time = this->ClipFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

svtkPolyData* svtkClipPolyData::GetClippedOutput()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//----------------------------------------------------------------------------
//
// Clip through data generating surface.
//
int svtkClipPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType cellId, i, updateTime;
  svtkPoints* cellPts;
  svtkDataArray* clipScalars;
  svtkFloatArray* cellScalars;
  svtkGenericCell* cell;
  svtkCellArray *newVerts, *newLines, *newPolys, *connList = nullptr;
  svtkCellArray *clippedVerts = nullptr, *clippedLines = nullptr;
  svtkCellArray *clippedPolys = nullptr, *clippedList = nullptr;
  svtkPoints* newPoints;
  svtkIdList* cellIds;
  double s;
  svtkIdType estimatedSize, numCells = input->GetNumberOfCells();
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkPoints* inPts = input->GetPoints();
  int numberOfPoints;
  svtkPointData *inPD = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *inCD = input->GetCellData(), *outCD = output->GetCellData();
  svtkCellData* outClippedCD = nullptr;

  svtkDebugMacro(<< "Clipping polygonal data");

  // Initialize self; create output objects
  //
  if (numPts < 1 || inPts == nullptr)
  {
    svtkDebugMacro(<< "No data to clip");
    return 1;
  }

  if (!this->ClipFunction && this->GenerateClipScalars)
  {
    svtkErrorMacro(<< "Cannot generate clip scalars if no clip function defined");
    return 1;
  }

  // Determine whether we're clipping with input scalars or a clip function
  // and to necessary setup.
  if (this->ClipFunction)
  {
    svtkFloatArray* tmpScalars = svtkFloatArray::New();
    tmpScalars->SetNumberOfTuples(numPts);
    inPD = svtkPointData::New();
    inPD->ShallowCopy(input->GetPointData()); // copies original
    if (this->GenerateClipScalars)
    {
      inPD->SetScalars(tmpScalars);
    }
    for (i = 0; i < numPts; i++)
    {
      s = this->ClipFunction->FunctionValue(inPts->GetPoint(i));
      tmpScalars->SetComponent(i, 0, s);
    }
    clipScalars = tmpScalars;
  }
  else // using input scalars
  {
    clipScalars = inPD->GetScalars();
    if (!clipScalars)
    {
      svtkErrorMacro(<< "Cannot clip without clip function or input scalars");
      return 1;
    }
  }

  // Create objects to hold output of clip operation
  //
  estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  newPoints = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    newPoints->SetDataType(input->GetPoints()->GetDataType());
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_DOUBLE);
  }

  newPoints->Allocate(numPts, numPts / 2);
  newVerts = svtkCellArray::New();
  newVerts->AllocateEstimate(estimatedSize, 1);
  newLines = svtkCellArray::New();
  newLines->AllocateEstimate(estimatedSize, 2);
  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(estimatedSize, 4);

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPoints, input->GetBounds());

  if (!this->GenerateClipScalars && !input->GetPointData()->GetScalars())
  {
    outPD->CopyScalarsOff();
  }
  else
  {
    outPD->CopyScalarsOn();
  }
  outPD->InterpolateAllocate(inPD, estimatedSize, estimatedSize / 2);
  outCD->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);

  // If generating second output, setup clipped output
  if (this->GenerateClippedOutput)
  {
    this->GetClippedOutput()->Initialize();
    outClippedCD = this->GetClippedOutput()->GetCellData();
    outClippedCD->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);
    clippedVerts = svtkCellArray::New();
    clippedVerts->AllocateEstimate(estimatedSize, 1);
    clippedLines = svtkCellArray::New();
    clippedLines->AllocateEstimate(estimatedSize, 2);
    clippedPolys = svtkCellArray::New();
    clippedPolys->AllocateEstimate(estimatedSize, 4);
  }

  cellScalars = svtkFloatArray::New();
  cellScalars->Allocate(SVTK_CELL_SIZE);

  // perform clipping on cells
  int abort = 0;
  updateTime = numCells / 20 + 1; // update roughly every 5%
  cell = svtkGenericCell::New();
  for (cellId = 0; cellId < numCells && !abort; cellId++)
  {
    input->GetCell(cellId, cell);
    cellPts = cell->GetPoints();
    cellIds = cell->GetPointIds();
    numberOfPoints = cellPts->GetNumberOfPoints();

    // evaluate implicit cutting function
    for (i = 0; i < numberOfPoints; i++)
    {
      s = clipScalars->GetComponent(cellIds->GetId(i), 0);
      cellScalars->InsertTuple(i, &s);
    }

    switch (cell->GetCellDimension())
    {
      case 0: // points are generated-------------------------------
        connList = newVerts;
        clippedList = clippedVerts;
        break;

      case 1: // lines are generated----------------------------------
        connList = newLines;
        clippedList = clippedLines;
        break;

      case 2: // triangles are generated------------------------------
        connList = newPolys;
        clippedList = clippedPolys;
        break;

    } // switch

    cell->Clip(this->Value, cellScalars, this->Locator, connList, inPD, outPD, inCD, cellId, outCD,
      this->InsideOut);

    if (this->GenerateClippedOutput)
    {
      cell->Clip(this->Value, cellScalars, this->Locator, clippedList, inPD, outPD, inCD, cellId,
        outClippedCD, !this->InsideOut);
    }

    if (!(cellId % updateTime))
    {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }
  } // for each cell
  cell->Delete();

  svtkDebugMacro(<< "Created: " << newPoints->GetNumberOfPoints() << " points, "
                << newVerts->GetNumberOfCells() << " verts, " << newLines->GetNumberOfCells()
                << " lines, " << newPolys->GetNumberOfCells() << " polys");

  if (this->GenerateClippedOutput)
  {
    svtkDebugMacro(<< "Created (clipped output): " << clippedVerts->GetNumberOfCells() << " verts, "
                  << clippedLines->GetNumberOfCells() << " lines, "
                  << clippedPolys->GetNumberOfCells() << " triangles");
  }

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory.
  //
  if (this->ClipFunction)
  {
    clipScalars->Delete();
    inPD->Delete();
  }

  if (newVerts->GetNumberOfCells())
  {
    output->SetVerts(newVerts);
  }
  newVerts->Delete();

  if (newLines->GetNumberOfCells())
  {
    output->SetLines(newLines);
  }
  newLines->Delete();

  if (newPolys->GetNumberOfCells())
  {
    output->SetPolys(newPolys);
  }
  newPolys->Delete();

  if (this->GenerateClippedOutput)
  {
    this->GetClippedOutput()->SetPoints(newPoints);

    if (clippedVerts->GetNumberOfCells())
    {
      this->GetClippedOutput()->SetVerts(clippedVerts);
    }
    clippedVerts->Delete();

    if (clippedLines->GetNumberOfCells())
    {
      this->GetClippedOutput()->SetLines(clippedLines);
    }
    clippedLines->Delete();

    if (clippedPolys->GetNumberOfCells())
    {
      this->GetClippedOutput()->SetPolys(clippedPolys);
    }
    clippedPolys->Delete();

    this->GetClippedOutput()->GetPointData()->PassData(outPD);
    this->GetClippedOutput()->Squeeze();
  }

  output->SetPoints(newPoints);
  newPoints->Delete();
  cellScalars->Delete();

  this->Locator->Initialize(); // release any extra memory
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkClipPolyData::SetLocator(svtkIncrementalPointLocator* locator)
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
void svtkClipPolyData::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

//----------------------------------------------------------------------------
void svtkClipPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->ClipFunction)
  {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
  }
  else
  {
    os << indent << "Clip Function: (none)\n";
  }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Generate Clip Scalars: " << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: " << (this->GenerateClippedOutput ? "On\n" : "Off\n");

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
