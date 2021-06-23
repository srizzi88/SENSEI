/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPointCloudPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractPointCloudPiece.h"

#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
// #include "svtkPolyData.h"
#include "svtkDoubleArray.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkExtractPointCloudPiece);

//-----------------------------------------------------------------------------
svtkExtractPointCloudPiece::svtkExtractPointCloudPiece()
{
  this->ModuloOrdering = true;
}

//-----------------------------------------------------------------------------
int svtkExtractPointCloudPiece::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // get the info object
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//-----------------------------------------------------------------------------
int svtkExtractPointCloudPiece::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // handle field data
  svtkFieldData* fd = input->GetFieldData();
  svtkFieldData* outFD = output->GetFieldData();
  svtkDataArray* offsets = fd->GetArray("BinOffsets");
  // we wipe the field data as the early viesion of the binner
  // was producing some huge field data that was killing file IO times
  outFD->Initialize();

  // Pipeline update piece will tell us what to generate.
  int piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  svtkIdType startIndex, endIndex;
  if (svtkIntArray::SafeDownCast(offsets))
  {
    svtkIntArray* ioffs = svtkIntArray::SafeDownCast(offsets);
    startIndex = ioffs->GetValue(piece);
    endIndex = ioffs->GetValue(piece + 1);
  }
  else
  {
    svtkIdTypeArray* ioffs = svtkIdTypeArray::SafeDownCast(offsets);
    startIndex = ioffs->GetValue(piece);
    endIndex = ioffs->GetValue(piece + 1);
  }

  svtkIdType numPts = endIndex - startIndex;
  svtkPointData* pd = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(pd, numPts);

  svtkNew<svtkPoints> newPoints;
  newPoints->Allocate(numPts);

  newPoints->SetNumberOfPoints(numPts);

  if (this->ModuloOrdering)
  {
    // loop over points copying them to the output
    // we do it in an mod 11 approach to add some randomization to the order
    svtkIdType inIdx = 0;
    svtkIdType nextStart = 1;
    for (svtkIdType i = 0; i < numPts; i++)
    {
      newPoints->SetPoint(i, input->GetPoint(inIdx + startIndex));
      outPD->CopyData(pd, inIdx + startIndex, i);
      inIdx += 11;
      if (inIdx >= numPts)
      {
        inIdx = nextStart;
        nextStart++;
      }
    }
  }
  else // otherwise no reordering
  {
    // copy the points
    newPoints->InsertPoints(0, numPts, startIndex, input->GetPoints());
    // copy point data
    outPD->CopyData(pd, 0, numPts, startIndex);
  }

  output->SetPoints(newPoints);

  return 1;
}

//-----------------------------------------------------------------------------
void svtkExtractPointCloudPiece::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ModuloOrdering: " << this->ModuloOrdering << "\n";
}
