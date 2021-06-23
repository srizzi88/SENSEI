/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericClip.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkExecutive.h"
#include "svtkGenericAdaptorCell.h"
#include "svtkGenericAttribute.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericCell.h"
#include "svtkGenericCellIterator.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericDataSet.h"
#include "svtkGenericPointIterator.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImplicitFunction.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include <cmath>

svtkStandardNewMacro(svtkGenericClip);
svtkCxxSetObjectMacro(svtkGenericClip, ClipFunction, svtkImplicitFunction);
svtkCxxSetObjectMacro(svtkGenericClip, Locator, svtkIncrementalPointLocator);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
svtkGenericClip::svtkGenericClip(svtkImplicitFunction* cf)
{
  this->ClipFunction = cf;
  this->InsideOut = 0;
  this->Locator = nullptr;
  this->Value = 0.0;
  this->GenerateClipScalars = 0;

  this->GenerateClippedOutput = 0;
  this->MergeTolerance = 0.01;

  this->SetNumberOfOutputPorts(2);
  svtkUnstructuredGrid* output2 = svtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();

  this->InputScalarsSelection = nullptr;

  this->InternalPD = svtkPointData::New();
  this->SecondaryPD = svtkPointData::New();
  this->SecondaryCD = svtkCellData::New();
}

//----------------------------------------------------------------------------
svtkGenericClip::~svtkGenericClip()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  this->SetClipFunction(nullptr);
  this->SetInputScalarsSelection(nullptr);
  this->InternalPD->Delete();
  this->SecondaryPD->Delete();
  this->SecondaryCD->Delete();
}

//----------------------------------------------------------------------------
// Do not say we have two outputs unless we are generating the clipped output.
int svtkGenericClip::GetNumberOfOutputs()
{
  if (this->GenerateClippedOutput)
  {
    return 2;
  }
  return 1;
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
svtkMTimeType svtkGenericClip::GetMTime()
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

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkGenericClip::GetClippedOutput()
{
  if (!this->GenerateClippedOutput)
  {
    return nullptr;
  }
  return svtkUnstructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//----------------------------------------------------------------------------
//
// Clip through data generating surface.
//
int svtkGenericClip::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGenericDataSet* input =
    svtkGenericDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input == nullptr)
  {
    return 1;
  }

  svtkUnstructuredGrid* clippedOutput = this->GetClippedOutput();

  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* outCD[2];
  svtkIdType npts = 0;
  const svtkIdType* pts;
  int cellType = 0;
  int j;
  svtkIdType estimatedSize;
  svtkUnsignedCharArray* types[2];
  int numOutputs = 1;
  svtkGenericAdaptorCell* cell;

  outCD[0] = nullptr;
  outCD[1] = nullptr;

  svtkDebugMacro(<< "Clipping dataset");

  // Initialize self; create output objects
  //
  if (numPts < 1)
  {
    svtkErrorMacro(<< "No data to clip");
    return 1;
  }

  if (!this->ClipFunction && this->GenerateClipScalars)
  {
    svtkErrorMacro(<< "Cannot generate clip scalars if no clip function defined");
    return 1;
  }

  // allocate the output and associated helper classes
  estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  svtkPoints* newPoints = svtkPoints::New();
  newPoints->Allocate(numPts, numPts / 2);

  svtkCellArray* conn[2];
  conn[0] = svtkCellArray::New();
  conn[0]->AllocateEstimate(estimatedSize, 1);
  conn[0]->InitTraversal();
  types[0] = svtkUnsignedCharArray::New();
  types[0]->Allocate(estimatedSize, estimatedSize / 2);

  if (this->GenerateClippedOutput)
  {
    numOutputs = 2;
    conn[1] = svtkCellArray::New();
    conn[1]->AllocateEstimate(estimatedSize, 1);
    conn[1]->InitTraversal();
    types[1] = svtkUnsignedCharArray::New();
    types[1]->Allocate(estimatedSize, estimatedSize / 2);
  }

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPoints, input->GetBounds());

  // prepare the output attributes
  svtkGenericAttributeCollection* attributes = input->GetAttributes();
  svtkGenericAttribute* attribute;
  svtkDataArray* attributeArray;

  int c = attributes->GetNumberOfAttributes();
  svtkDataSetAttributes* secondaryAttributes;

  int attributeType;

  svtkIdType i;
  for (i = 0; i < c; ++i)
  {
    attribute = attributes->GetAttribute(i);
    attributeType = attribute->GetType();
    if (attribute->GetCentering() == svtkPointCentered)
    {
      secondaryAttributes = this->SecondaryPD;

      attributeArray = svtkDataArray::CreateDataArray(attribute->GetComponentType());
      attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
      attributeArray->SetName(attribute->GetName());
      this->InternalPD->AddArray(attributeArray);
      attributeArray->Delete();
      if (this->InternalPD->GetAttribute(attributeType) == nullptr)
      {
        this->InternalPD->SetActiveAttribute(
          this->InternalPD->GetNumberOfArrays() - 1, attributeType);
      }
    }
    else // svtkCellCentered
    {
      secondaryAttributes = this->SecondaryCD;
    }

    attributeArray = svtkDataArray::CreateDataArray(attribute->GetComponentType());
    attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
    attributeArray->SetName(attribute->GetName());
    secondaryAttributes->AddArray(attributeArray);
    attributeArray->Delete();

    if (secondaryAttributes->GetAttribute(attributeType) == nullptr)
    {
      secondaryAttributes->SetActiveAttribute(
        secondaryAttributes->GetNumberOfArrays() - 1, attributeType);
    }
  }
  outPD->InterpolateAllocate(this->SecondaryPD, estimatedSize, estimatedSize / 2);

  outCD[0] = output->GetCellData();
  outCD[0]->CopyAllocate(this->SecondaryCD, estimatedSize, estimatedSize / 2);
  if (this->GenerateClippedOutput)
  {
    outCD[1] = clippedOutput->GetCellData();
    outCD[1]->CopyAllocate(this->SecondaryCD, estimatedSize, estimatedSize / 2);
  }

  // svtkGenericPointIterator *pointIt = input->GetPoints();
  svtkGenericCellIterator* cellIt = input->NewCellIterator(); // explicit cell could be 2D or 3D

  // Process all cells and clip each in turn
  //
  int abort = 0;
  svtkIdType updateTime = numCells / 20 + 1; // update roughly every 5%

  int num[2];
  num[0] = num[1] = 0;
  int numNew[2];
  numNew[0] = numNew[1] = 0;
  svtkIdType cellId;

  input->GetTessellator()->InitErrorMetrics(input);

  for (cellId = 0, cellIt->Begin(); !cellIt->IsAtEnd() && !abort; cellId++, cellIt->Next())
  {
    cell = cellIt->GetCell();
    if (!(cellId % updateTime))
    {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    // perform the clipping

    cell->Clip(this->Value, this->ClipFunction, input->GetAttributes(), input->GetTessellator(),
      this->InsideOut, this->Locator, conn[0], outPD, outCD[0], this->InternalPD, this->SecondaryPD,
      this->SecondaryCD);
    numNew[0] = conn[0]->GetNumberOfCells() - num[0];
    num[0] = conn[0]->GetNumberOfCells();

    if (this->GenerateClippedOutput)
    {
      cell->Clip(this->Value, this->ClipFunction, input->GetAttributes(), input->GetTessellator(),
        this->InsideOut, this->Locator, conn[1], outPD, outCD[1], this->InternalPD,
        this->SecondaryPD, this->SecondaryCD);

      numNew[1] = conn[1]->GetNumberOfCells() - num[1];
      num[1] = conn[1]->GetNumberOfCells();
    }

    for (i = 0; i < numOutputs; i++) // for both outputs
    {
      for (j = 0; j < numNew[i]; j++)
      {
        conn[i]->GetNextCell(npts, pts);

        // For each new cell added, got to set the type of the cell
        switch (cell->GetDimension())
        {
          case 0: // points are generated--------------------------------
            cellType = (npts > 1 ? SVTK_POLY_VERTEX : SVTK_VERTEX);
            break;

          case 1: // lines are generated---------------------------------
            cellType = (npts > 2 ? SVTK_POLY_LINE : SVTK_LINE);
            break;

          case 2: // polygons are generated------------------------------
            cellType = (npts == 3 ? SVTK_TRIANGLE : (npts == 4 ? SVTK_QUAD : SVTK_POLYGON));
            break;

          case 3: // tetrahedra or wedges are generated------------------
            cellType = (npts == 4 ? SVTK_TETRA : SVTK_WEDGE);
            break;
        } // switch

        types[i]->InsertNextValue(cellType);
      } // for each new cell
    }   // for both outputs
  }     // for each cell
  cellIt->Delete();

  output->SetPoints(newPoints);
  output->SetCells(types[0], conn[0]);
  conn[0]->Delete();
  types[0]->Delete();

  if (this->GenerateClippedOutput)
  {
    clippedOutput->SetPoints(newPoints);
    clippedOutput->SetCells(types[1], conn[1]);
    conn[1]->Delete();
    types[1]->Delete();
  }

  newPoints->Delete();
  this->Locator->Initialize(); // release any extra memory
  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkGenericClip::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkGenericClip::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
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

  if (this->InputScalarsSelection)
  {
    os << indent << "InputScalarsSelection: " << this->InputScalarsSelection << endl;
  }
}
//----------------------------------------------------------------------------
int svtkGenericClip::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  return 1;
}
