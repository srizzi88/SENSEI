/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericCutter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkContourValues.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkGenericAdaptorCell.h"
#include "svtkGenericAttribute.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericCell.h"
#include "svtkGenericCellIterator.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericDataSet.h"
#include "svtkGenericPointIterator.h"
#include "svtkImplicitFunction.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkUnstructuredGrid.h"

#include <cmath>

svtkStandardNewMacro(svtkGenericCutter);
svtkCxxSetObjectMacro(svtkGenericCutter, CutFunction, svtkImplicitFunction);
svtkCxxSetObjectMacro(svtkGenericCutter, Locator, svtkIncrementalPointLocator);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; initial value of 0.0; and
// generating cut scalars turned off.
//
svtkGenericCutter::svtkGenericCutter(svtkImplicitFunction* cf)
{
  this->ContourValues = svtkContourValues::New();
  this->CutFunction = cf;
  this->GenerateCutScalars = 0;
  this->Locator = nullptr;

  this->InternalPD = svtkPointData::New();
  this->SecondaryPD = svtkPointData::New();
  this->SecondaryCD = svtkCellData::New();
}

//----------------------------------------------------------------------------
svtkGenericCutter::~svtkGenericCutter()
{
  this->ContourValues->Delete();
  this->SetCutFunction(nullptr);
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  this->InternalPD->Delete();
  this->SecondaryPD->Delete();
  this->SecondaryCD->Delete();
}

//----------------------------------------------------------------------------
// Description:
// Set a particular contour value at contour number i. The index i ranges
// between 0<=i<NumberOfContours.
void svtkGenericCutter::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

//-----------------------------------------------------------------------------
// Description:
// Get the ith contour value.
double svtkGenericCutter::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

//-----------------------------------------------------------------------------
// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
double* svtkGenericCutter::GetValues()
{
  return this->ContourValues->GetValues();
}

//-----------------------------------------------------------------------------
// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
void svtkGenericCutter::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

//-----------------------------------------------------------------------------
// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
void svtkGenericCutter::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

//-----------------------------------------------------------------------------
// Description:
// Get the number of contours in the list of contour values.
svtkIdType svtkGenericCutter::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

//-----------------------------------------------------------------------------
// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void svtkGenericCutter::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void svtkGenericCutter::GenerateValues(int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If cut functions is modified,
// or contour values modified, then this object is modified as well.
//
svtkMTimeType svtkGenericCutter::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType contourValuesMTime = this->ContourValues->GetMTime();
  svtkMTimeType time;

  mTime = (contourValuesMTime > mTime ? contourValuesMTime : mTime);

  if (this->CutFunction != nullptr)
  {
    time = this->CutFunction->GetMTime();
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
// Cut through data generating surface.
//
int svtkGenericCutter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGenericDataSet* input =
    svtkGenericDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Executing cutter");

  if (input == nullptr)
  {
    svtkErrorMacro("No input specified");
    return 1;
  }

  if (this->CutFunction == nullptr)
  {
    svtkErrorMacro("No cut function specified");
    return 1;
  }

  if (input->GetNumberOfPoints() < 1)
  {
    svtkErrorMacro("Input data set is empty");
    return 1;
  }

  svtkPointData* outPd = output->GetPointData();
  svtkCellData* outCd = output->GetCellData();

  // Create objects to hold output of contour operation
  //
  svtkIdType numCells = input->GetNumberOfCells();
  svtkIdType numContours = this->ContourValues->GetNumberOfContours();

  svtkIdType estimatedSize =
    static_cast<svtkIdType>(pow(static_cast<double>(numCells), .75)) * numContours;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  svtkPoints* newPts = svtkPoints::New();
  newPts->Allocate(estimatedSize, estimatedSize);
  svtkCellArray* newVerts = svtkCellArray::New();
  newVerts->AllocateExact(estimatedSize, estimatedSize);
  svtkCellArray* newLines = svtkCellArray::New();
  newLines->AllocateExact(estimatedSize, estimatedSize);
  svtkCellArray* newPolys = svtkCellArray::New();
  newPolys->AllocateExact(estimatedSize, estimatedSize);

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPts, input->GetBounds(), estimatedSize);

  // prepare the output attributes
  svtkGenericAttributeCollection* attributes = input->GetAttributes();
  svtkGenericAttribute* attribute;
  svtkDataArray* attributeArray;

  int c = attributes->GetNumberOfAttributes();
  svtkDataSetAttributes* secondaryAttributes;

  int attributeType;

  for (svtkIdType i = 0; i < c; ++i)
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

  outPd->InterpolateAllocate(this->SecondaryPD, estimatedSize, estimatedSize);
  outCd->CopyAllocate(this->SecondaryCD, estimatedSize, estimatedSize);

  svtkGenericAdaptorCell* cell;

  //----------- Begin of contouring algorithm --------------------//
  svtkGenericCellIterator* cellIt = input->NewCellIterator();

  svtkIdType updateCount = numCells / 20 + 1; // update roughly every 5%
  svtkIdType count = 0;
  int abortExecute = 0;

  input->GetTessellator()->InitErrorMetrics(input);

  for (cellIt->Begin(); !cellIt->IsAtEnd() && !abortExecute; cellIt->Next())
  {
    if (!(count % updateCount))
    {
      this->UpdateProgress(static_cast<double>(count) / numCells);
      abortExecute = this->GetAbortExecute();
    }

    cell = cellIt->GetCell();
    cell->Contour(this->ContourValues, this->CutFunction, input->GetAttributes(),
      input->GetTessellator(), this->Locator, newVerts, newLines, newPolys, outPd, outCd,
      this->InternalPD, this->SecondaryPD, this->SecondaryCD);
    ++count;
  } // for each cell
  cellIt->Delete();

  svtkDebugMacro(<< "Created: " << newPts->GetNumberOfPoints() << " points, "
                << newVerts->GetNumberOfCells() << " verts, " << newLines->GetNumberOfCells()
                << " lines, " << newPolys->GetNumberOfCells() << " triangles");

  //----------- End of contouring algorithm ----------------------//

  // Update ourselves.  Because we don't know up front how many verts, lines,
  // polys we've created, take care to reclaim memory.
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newVerts->GetNumberOfCells() > 0)
  {
    output->SetVerts(newVerts);
  }
  newVerts->Delete();

  if (newLines->GetNumberOfCells() > 0)
  {
    output->SetLines(newLines);
  }
  newLines->Delete();

  if (newPolys->GetNumberOfCells() > 0)
  {
    output->SetPolys(newPolys);
  }
  newPolys->Delete();

  this->Locator->Initialize(); // releases leftover memory
  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkGenericCutter::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkGenericCutter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cut Function: " << this->CutFunction << "\n";

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Generate Cut Scalars: " << (this->GenerateCutScalars ? "On\n" : "Off\n");
}
//----------------------------------------------------------------------------
int svtkGenericCutter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  return 1;
}
