/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMaskPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMaskPolyData.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkMaskPolyData);

svtkMaskPolyData::svtkMaskPolyData()
{
  this->OnRatio = 11;
  this->Offset = 0;
}

// Down sample polygonal data.  Don't down sample points (that is, use the
// original points, since usually not worth it.
//
int svtkMaskPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType id;
  svtkPointData* pd;
  svtkIdType numCells;
  const svtkIdType* pts = nullptr;
  svtkIdType npts = 0;
  int abortExecute = 0;

  // Check input / pass data through
  //
  numCells = input->GetNumberOfCells();

  if (numCells < 1)
  {
    svtkErrorMacro(<< "No PolyData to mask!");
    return 1;
  }

  output->AllocateCopy(input);
  input->BuildCells();

  // Traverse topological lists and traverse
  //
  svtkIdType tenth = numCells / 10 + 1;
  for (id = this->Offset; id < numCells && !abortExecute; id += this->OnRatio)
  {
    if (!(id % tenth))
    {
      this->UpdateProgress((float)id / numCells);
      abortExecute = this->GetAbortExecute();
    }
    input->GetCellPoints(id, npts, pts);
    output->InsertNextCell(input->GetCellType(id), npts, pts);
  }

  // Update ourselves and release memory
  //
  output->SetPoints(input->GetPoints());
  pd = input->GetPointData();
  output->GetPointData()->PassData(pd);

  output->Squeeze();

  return 1;
}

void svtkMaskPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
}
