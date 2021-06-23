/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkPComputeHistogram2DOutliers.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkPComputeHistogram2DOutliers.h"
//------------------------------------------------------------------------------
#include "svtkCollection.h"
#include "svtkCommunicator.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImageGradientMagnitude.h"
#include "svtkImageMedian3D.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkPComputeHistogram2DOutliers);
svtkCxxSetObjectMacro(svtkPComputeHistogram2DOutliers, Controller, svtkMultiProcessController);
//------------------------------------------------------------------------------
svtkPComputeHistogram2DOutliers::svtkPComputeHistogram2DOutliers()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}
//------------------------------------------------------------------------------
svtkPComputeHistogram2DOutliers::~svtkPComputeHistogram2DOutliers()
{
  this->SetController(nullptr);
}
//------------------------------------------------------------------------------
void svtkPComputeHistogram2DOutliers::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
//------------------------------------------------------------------------------
int svtkPComputeHistogram2DOutliers::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    return 0;

  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
  {
    // Nothing to do for single process.
    return 1;
  }

  svtkCommunicator* comm = this->Controller->GetCommunicator();
  if (!comm)
  {
    svtkErrorMacro("Need a communicator.");
    return 0;
  }

  // get the output
  svtkInformation* outTableInfo = outputVector->GetInformationObject(OUTPUT_SELECTED_TABLE_DATA);
  svtkTable* outputTable = svtkTable::SafeDownCast(outTableInfo->Get(svtkDataObject::DATA_OBJECT()));

  int numProcesses = this->Controller->GetNumberOfProcesses();
  // int myId = this->Controller->GetLocalProcessId();

  // 1) leave the selected rows alone, since they don't make sense for multiple nodes
  //

  // 2) gather the selected data together
  // for each column, make a new one and add it to a new table
  svtkSmartPointer<svtkTable> gatheredTable = svtkSmartPointer<svtkTable>::New();
  for (int i = 0; i < outputTable->GetNumberOfColumns(); i++)
  {
    svtkAbstractArray* col = svtkArrayDownCast<svtkAbstractArray>(outputTable->GetColumn(i));
    if (!col)
      continue;

    svtkIdType myLength = col->GetNumberOfTuples();
    svtkIdType totalLength = 0;
    std::vector<svtkIdType> recvLengths(numProcesses, 0);
    std::vector<svtkIdType> recvOffsets(numProcesses, 0);

    // gathers all of the array lengths together
    comm->AllGather(&myLength, &recvLengths[0], 1);

    // compute the displacements
    svtkIdType typeSize = col->GetDataTypeSize();
    for (int j = 0; j < numProcesses; j++)
    {
      recvOffsets[j] = totalLength * typeSize;
      totalLength += recvLengths[j];
      recvLengths[j] *= typeSize;
    }

    // communicating this as a byte array :/
    svtkAbstractArray* received = svtkAbstractArray::CreateArray(col->GetDataType());
    received->SetNumberOfTuples(totalLength);

    char* sendBuf = (char*)col->GetVoidPointer(0);
    char* recvBuf = (char*)received->GetVoidPointer(0);

    comm->AllGatherV(sendBuf, recvBuf, myLength * typeSize, &recvLengths[0], &recvOffsets[0]);

    gatheredTable->AddColumn(received);
    received->Delete();
  }

  outputTable->ShallowCopy(gatheredTable);

  return 1;
}
