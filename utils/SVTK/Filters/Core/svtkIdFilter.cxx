/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIdFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkIdFilter.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkIdFilter);

// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
svtkIdFilter::svtkIdFilter()
{
  this->PointIds = 1;
  this->CellIds = 1;
  this->FieldData = 0;
  this->PointIdsArrayName = nullptr;
  this->CellIdsArrayName = nullptr;

  // these names are set to the same name for backwards compatibility.
  this->SetPointIdsArrayName("svtkIdFilter_Ids");
  this->SetCellIdsArrayName("svtkIdFilter_Ids");
}

svtkIdFilter::~svtkIdFilter()
{
  delete[] this->PointIdsArrayName;
  delete[] this->CellIdsArrayName;
}

//
// Map ids into attribute data
//
int svtkIdFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPts, numCells, id;
  svtkIdTypeArray* ptIds;
  svtkIdTypeArray* cellIds;
  svtkPointData *inPD = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *inCD = input->GetCellData(), *outCD = output->GetCellData();

  // Initialize
  //
  svtkDebugMacro(<< "Generating ids!");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  // Loop over points (if requested) and generate ids
  //
  if (this->PointIds && numPts > 0)
  {
    ptIds = svtkIdTypeArray::New();
    ptIds->SetNumberOfValues(numPts);

    for (id = 0; id < numPts; id++)
    {
      ptIds->SetValue(id, id);
    }

    ptIds->SetName(this->PointIdsArrayName);
    if (!this->FieldData)
    {
      int idx = outPD->AddArray(ptIds);
      outPD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
      outPD->CopyScalarsOff();
    }
    else
    {
      outPD->AddArray(ptIds);
      outPD->CopyFieldOff(this->PointIdsArrayName);
    }
    ptIds->Delete();
  }

  // Loop over cells (if requested) and generate ids
  //
  if (this->CellIds && numCells > 0)
  {
    cellIds = svtkIdTypeArray::New();
    cellIds->SetNumberOfValues(numCells);

    for (id = 0; id < numCells; id++)
    {
      cellIds->SetValue(id, id);
    }

    cellIds->SetName(this->CellIdsArrayName);
    if (!this->FieldData)
    {
      int idx = outCD->AddArray(cellIds);
      outCD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
      outCD->CopyScalarsOff();
    }
    else
    {
      outCD->AddArray(cellIds);
      outCD->CopyFieldOff(this->CellIdsArrayName);
    }
    cellIds->Delete();
  }

  outPD->PassData(inPD);
  outCD->PassData(inCD);

  return 1;
}

void svtkIdFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point Ids: " << (this->PointIds ? "On\n" : "Off\n");
  os << indent << "Cell Ids: " << (this->CellIds ? "On\n" : "Off\n");
  os << indent << "Field Data: " << (this->FieldData ? "On\n" : "Off\n");
  os << indent
     << "PointIdsArrayName: " << (this->PointIdsArrayName ? this->PointIdsArrayName : "(none)")
     << "\n";
  os << indent
     << "CellIdsArrayName: " << (this->CellIdsArrayName ? this->CellIdsArrayName : "(none)")
     << "\n";
}

#if !defined(SVTK_LEGACY_REMOVE)
void svtkIdFilter::SetIdsArrayName(const char* name)
{
  SVTK_LEGACY_REPLACED_BODY(svtkIdFilter::SetIdsArrayName, "SVTK 9.0",
    svtkIdFilter::SetPointIdsArrayName or svtkIdFilter::SetCellIdsArrayName);
  this->SetPointIdsArrayName(name);
  this->SetCellIdsArrayName(name);
}
#endif

#if !defined(SVTK_LEGACY_REMOVE)
const char* svtkIdFilter::GetIdsArrayName()
{
  SVTK_LEGACY_REPLACED_BODY(svtkIdFilter::GetIdsArrayName, "SVTK 9.0",
    svtkIdFilter::GetPointIdsArrayName or svtkIdFilter::GetCellIdsArrayName);
  return this->GetPointIdsArrayName();
}
#endif
