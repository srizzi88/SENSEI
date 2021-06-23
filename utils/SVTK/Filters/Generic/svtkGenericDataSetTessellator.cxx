/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericDataSetTessellator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericDataSetTessellator.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkGenericAdaptorCell.h"
#include "svtkGenericAttribute.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericCellIterator.h"
#include "svtkGenericCellTessellator.h"
#include "svtkGenericDataSet.h"
#include "svtkIdTypeArray.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTetra.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkGenericDataSetTessellator);

svtkCxxSetObjectMacro(svtkGenericDataSetTessellator, Locator, svtkIncrementalPointLocator);
//----------------------------------------------------------------------------
//
svtkGenericDataSetTessellator::svtkGenericDataSetTessellator()
{
  this->InternalPD = svtkPointData::New();
  this->KeepCellIds = 1;

  this->Merging = 1;
  this->Locator = nullptr;
}

//----------------------------------------------------------------------------
svtkGenericDataSetTessellator::~svtkGenericDataSetTessellator()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  this->InternalPD->Delete();
}

//----------------------------------------------------------------------------
//
int svtkGenericDataSetTessellator::RequestData(svtkInformation* svtkNotUsed(request),
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

  svtkDebugMacro(<< "Executing svtkGenericDataSetTessellator...");

  //  svtkGenericDataSet *input = this->GetInput();
  //  svtkUnstructuredGrid *output = this->GetOutput();
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkGenericAdaptorCell* cell;
  svtkIdType numInserted = 0, numNew, i;
  int abortExecute = 0;

  // Copy original points and point data
  svtkPoints* newPts = svtkPoints::New();
  newPts->Allocate(2 * numPts, numPts);

  // loop over region
  svtkUnsignedCharArray* types = svtkUnsignedCharArray::New();
  types->Allocate(numCells);
  svtkCellArray* conn = svtkCellArray::New();
  conn->AllocateEstimate(numCells, 1);

  // prepare the output attributes
  svtkGenericAttributeCollection* attributes = input->GetAttributes();
  svtkGenericAttribute* attribute;
  svtkDataArray* attributeArray;

  int c = attributes->GetNumberOfAttributes();
  svtkDataSetAttributes* dsAttributes;

  int attributeType;

  i = 0;
  while (i < c)
  {
    attribute = attributes->GetAttribute(i);
    attributeType = attribute->GetType();
    if (attribute->GetCentering() == svtkPointCentered)
    {
      dsAttributes = outputPD;

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
      dsAttributes = outputCD;
    }
    attributeArray = svtkDataArray::CreateDataArray(attribute->GetComponentType());
    attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
    attributeArray->SetName(attribute->GetName());
    dsAttributes->AddArray(attributeArray);
    attributeArray->Delete();

    if (dsAttributes->GetAttribute(attributeType) == nullptr)
    {
      dsAttributes->SetActiveAttribute(dsAttributes->GetNumberOfArrays() - 1, attributeType);
    }
    ++i;
  }

  svtkIdTypeArray* cellIdArray = nullptr;

  if (this->KeepCellIds)
  {
    cellIdArray = svtkIdTypeArray::New();
    cellIdArray->SetName("OriginalIds");
  }

  svtkGenericCellIterator* cellIt = input->NewCellIterator();
  svtkIdType updateCount = numCells / 20 + 1; // update roughly every 5%
  svtkIdType count = 0;

  input->GetTessellator()->InitErrorMetrics(input);

  svtkIncrementalPointLocator* locator = nullptr;
  if (this->Merging)
  {
    if (this->Locator == nullptr)
    {
      this->CreateDefaultLocator();
    }
    this->Locator->InitPointInsertion(newPts, input->GetBounds());
    locator = this->Locator;
  }

  for (cellIt->Begin(); !cellIt->IsAtEnd() && !abortExecute; cellIt->Next(), count++)
  {
    if (!(count % updateCount))
    {
      this->UpdateProgress(static_cast<double>(count) / numCells);
      abortExecute = this->GetAbortExecute();
    }

    cell = cellIt->GetCell();
    cell->Tessellate(input->GetAttributes(), input->GetTessellator(), newPts, locator, conn,
      this->InternalPD, outputPD, outputCD, types);
    numNew = conn->GetNumberOfCells() - numInserted;
    numInserted = conn->GetNumberOfCells();

    svtkIdType cellId = cell->GetId();

    if (this->KeepCellIds)
    {
      for (i = 0; i < numNew; i++)
      {
        cellIdArray->InsertNextValue(cellId);
      }
    }
  } // for all cells
  cellIt->Delete();

  // Send to the output
  if (this->KeepCellIds)
  {
    outputCD->AddArray(cellIdArray);
    cellIdArray->Delete();
  }

  output->SetPoints(newPts);
  output->SetCells(types, conn);

  if (!this->Merging && this->Locator)
  {
    this->Locator->Initialize();
  }

  svtkDebugMacro(<< "Subdivided " << numCells << " cells to produce " << conn->GetNumberOfCells()
                << "new cells");

  newPts->Delete();
  types->Delete();
  conn->Delete();

  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
int svtkGenericDataSetTessellator::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  return 1;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of svtkMergePoints is used.
void svtkGenericDataSetTessellator::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

//----------------------------------------------------------------------------
void svtkGenericDataSetTessellator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "keep cells ids=";
  if (this->KeepCellIds)
  {
    os << "true" << endl;
  }
  else
  {
    os << "false" << endl;
  }

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkGenericDataSetTessellator::GetMTime()
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
