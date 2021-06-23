/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointConnectivityFilter.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkUnsignedIntArray.h"

#include "svtkNew.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkPointConnectivityFilter);

//----------------------------------------------------------------------------
svtkPointConnectivityFilter::svtkPointConnectivityFilter() = default;

//----------------------------------------------------------------------------
svtkPointConnectivityFilter::~svtkPointConnectivityFilter() = default;

//----------------------------------------------------------------------------
namespace
{

// This class is general purpose for all dataset types.
struct UpdateConnectivityCount
{
  svtkDataSet* Input;
  unsigned int* ConnCount;
  svtkSMPThreadLocalObject<svtkIdList> CellIds;

  UpdateConnectivityCount(svtkDataSet* input, unsigned int* connPtr)
    : Input(input)
    , ConnCount(connPtr)
  {
  }

  void Initialize()
  {
    svtkIdList*& cellIds = this->CellIds.Local();
    cellIds->Allocate(128); // allocate some memory
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    svtkIdList*& cellIds = this->CellIds.Local();
    for (; ptId < endPtId; ++ptId)
    {
      this->Input->GetPointCells(ptId, cellIds);
      this->ConnCount[ptId] = cellIds->GetNumberOfIds();
    }
  }

  void Reduce() {}
};

} // end anon namespace

//----------------------------------------------------------------------------
// This is the generic non-optimized method
int svtkPointConnectivityFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkSmartPointer<svtkDataSet> input = svtkDataSet::GetData(inputVector[0]);
  svtkDataSet* output = svtkDataSet::GetData(outputVector);

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Check input
  svtkIdType numPts;
  if (input == nullptr || (numPts = input->GetNumberOfPoints()) < 1)
  {
    return 1;
  }

  // Create integral array and populate it
  svtkNew<svtkUnsignedIntArray> connCount;
  connCount->SetNumberOfTuples(numPts);
  connCount->SetName("Point Connectivity Count");
  unsigned int* connPtr = connCount->GetPointer(0);

  // Loop over all points, retrieving connectivity count
  // The first GetPointCells() primes the pump (builds internal structures, etc.)
  svtkNew<svtkIdList> cellIds;
  input->GetPointCells(0, cellIds);
  UpdateConnectivityCount updateCount(input, connPtr);
  svtkSMPTools::For(0, numPts, updateCount);

  // Pass array to the output
  output->GetPointData()->AddArray(connCount);

  return 1;
}

//----------------------------------------------------------------------------
void svtkPointConnectivityFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
