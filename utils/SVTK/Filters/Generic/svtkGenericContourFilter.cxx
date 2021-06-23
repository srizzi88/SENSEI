/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericContourFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericContourFilter.h"
#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkContourGrid.h"
#include "svtkContourValues.h"
#include "svtkDoubleArray.h"
#include "svtkGenericAdaptorCell.h"
#include "svtkGenericAttribute.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericCellIterator.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericDataSet.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLine.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkTimerLog.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkGenericContourFilter);

svtkCxxSetObjectMacro(svtkGenericContourFilter, Locator, svtkIncrementalPointLocator);

// Construct object with initial range (0,1) and single contour value
// of 0.0.
svtkGenericContourFilter::svtkGenericContourFilter()
{
  this->ContourValues = svtkContourValues::New();

  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->Locator = nullptr;

  this->InputScalarsSelection = nullptr;

  this->InternalPD = svtkPointData::New();
  this->SecondaryPD = svtkPointData::New();
  this->SecondaryCD = svtkCellData::New();
}

//-----------------------------------------------------------------------------
svtkGenericContourFilter::~svtkGenericContourFilter()
{
  this->ContourValues->Delete();
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  this->SetInputScalarsSelection(nullptr);
  this->InternalPD->Delete();
  this->SecondaryPD->Delete();
  this->SecondaryCD->Delete();
}

//-----------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
svtkMTimeType svtkGenericContourFilter::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->ContourValues)
  {
    time = this->ContourValues->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->Locator)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  // mTime should also take into account the fact that tessellator is view
  // dependent

  return mTime;
}

//-----------------------------------------------------------------------------
// General contouring filter.  Handles arbitrary input.
int svtkGenericContourFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGenericDataSet* input =
    svtkGenericDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Executing contour filter");

  if (!input)
  {
    svtkErrorMacro("No input specified");
    return 1;
  }
  svtkPointData* outPd = output->GetPointData();
  svtkCellData* outCd = output->GetCellData();

  // Create objects to hold output of contour operation. First estimate
  // allocation size.
  svtkIdType numCells = input->GetNumberOfCells();
  svtkIdType estimatedSize = input->GetEstimatedSize();
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
      if (this->InternalPD->GetAttribute(attributeType))
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

  if (this->InputScalarsSelection)
  {
    int attrib = input->GetAttributes()->FindAttribute(this->InputScalarsSelection);
    if (attrib != -1)
    {
      svtkGenericAttribute* a = input->GetAttributes()->GetAttribute(attrib);
      if (a->GetNumberOfComponents() == 1)
      {
        input->GetAttributes()->SetActiveAttribute(attrib, 0);
      }
    }
  }

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
    cell->Contour(this->ContourValues, nullptr, input->GetAttributes(), input->GetTessellator(),
      this->Locator, newVerts, newLines, newPolys, outPd, outCd, this->InternalPD,
      this->SecondaryPD, this->SecondaryCD);
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

//-----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkGenericContourFilter::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//-----------------------------------------------------------------------------
void svtkGenericContourFilter::SelectInputScalars(const char* fieldName)
{
  this->SetInputScalarsSelection(fieldName);
}

//-----------------------------------------------------------------------------
void svtkGenericContourFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->InputScalarsSelection)
  {
    os << indent << "InputScalarsSelection: " << this->InputScalarsSelection << endl;
  }

  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}

//-----------------------------------------------------------------------------
// Description:
// Set a particular contour value at contour number i. The index i ranges
// between 0<=i<NumberOfContours.
void svtkGenericContourFilter::SetValue(int i, float value)
{
  this->ContourValues->SetValue(i, value);
}

//----------------------------------------------------------------------------
// Description:
// Get the ith contour value.
double svtkGenericContourFilter::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

//----------------------------------------------------------------------------
// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
double* svtkGenericContourFilter::GetValues()
{
  return this->ContourValues->GetValues();
}

//----------------------------------------------------------------------------
// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
void svtkGenericContourFilter::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

//----------------------------------------------------------------------------
// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
void svtkGenericContourFilter::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

//----------------------------------------------------------------------------
// Description:
// Get the number of contours in the list of contour values.
svtkIdType svtkGenericContourFilter::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

//----------------------------------------------------------------------------
// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void svtkGenericContourFilter::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

//----------------------------------------------------------------------------
// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void svtkGenericContourFilter::GenerateValues(int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

//----------------------------------------------------------------------------
int svtkGenericContourFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  return 1;
}
