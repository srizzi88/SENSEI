/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlankStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBlankStructuredGrid.h"

#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStructuredGrid.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkBlankStructuredGrid);

// Construct object to extract all of the input data.
svtkBlankStructuredGrid::svtkBlankStructuredGrid()
{
  this->MinBlankingValue = SVTK_FLOAT_MAX;
  this->MaxBlankingValue = SVTK_FLOAT_MAX;
  this->ArrayName = nullptr;
  this->ArrayId = -1;
  this->Component = 0;
}

svtkBlankStructuredGrid::~svtkBlankStructuredGrid()
{
  delete[] this->ArrayName;
  this->ArrayName = nullptr;
}

template <class T>
void svtkBlankStructuredGridExecute(svtkBlankStructuredGrid* svtkNotUsed(self), T* dptr, int numPts,
  int numComp, int comp, double min, double max, svtkUnsignedCharArray* ghosts)
{
  T compValue;
  dptr += comp;

  for (int ptId = 0; ptId < numPts; ptId++, dptr += numComp)
  {
    compValue = *dptr;
    unsigned char value = 0;
    if (compValue >= min && compValue <= max)
    {
      value |= svtkDataSetAttributes::HIDDENPOINT;
    }
    ghosts->SetValue(ptId, value);
  }
}

int svtkBlankStructuredGrid::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkStructuredGrid* input =
    svtkStructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkStructuredGrid* output =
    svtkStructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* outCD = output->GetCellData();
  int numPts = input->GetNumberOfPoints();
  svtkDataArray* dataArray = nullptr;
  int numComp;

  svtkDebugMacro(<< "Blanking Grid");

  // Pass input to output
  //
  output->CopyStructure(input);
  outPD->PassData(pd);
  outCD->PassData(cd);

  // Get the appropriate data array
  //
  if (this->ArrayName != nullptr)
  {
    dataArray = pd->GetArray(this->ArrayName);
  }
  else if (this->ArrayId >= 0)
  {
    dataArray = pd->GetArray(this->ArrayId);
  }

  if (!dataArray || (numComp = dataArray->GetNumberOfComponents()) <= this->Component)
  {
    svtkWarningMacro(<< "Data array not found");
    return 1;
  }
  void* dptr = dataArray->GetVoidPointer(0);

  // Loop over the data array setting anything within the data range specified
  // to be blanked.
  //
  svtkUnsignedCharArray* ghosts = svtkUnsignedCharArray::New();
  ghosts->SetNumberOfValues(numPts);
  ghosts->SetName(svtkDataSetAttributes::GhostArrayName());
  switch (dataArray->GetDataType())
  {
    svtkTemplateMacro(svtkBlankStructuredGridExecute(this, static_cast<SVTK_TT*>(dptr), numPts,
      numComp, this->Component, this->MinBlankingValue, this->MaxBlankingValue, ghosts));
    default:
      break;
  }
  output->GetPointData()->AddArray(ghosts);
  ghosts->Delete();

  return 1;
}

void svtkBlankStructuredGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Min Blanking Value: " << this->MinBlankingValue << "\n";
  os << indent << "Max Blanking Value: " << this->MaxBlankingValue << "\n";
  os << indent << "Array Name: ";
  if (this->ArrayName)
  {
    os << this->ArrayName << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Array ID: " << this->ArrayId << "\n";
  os << indent << "Component: " << this->Component << "\n";
}
