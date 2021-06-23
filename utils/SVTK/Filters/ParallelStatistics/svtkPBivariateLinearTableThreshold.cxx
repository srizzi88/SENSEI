/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkPBivariateLinearTableThreshold.cxx

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
#include "svtkPBivariateLinearTableThreshold.h"

#include "svtkDataArrayCollection.h"
#include "svtkDataObject.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"

#include <map>

svtkStandardNewMacro(svtkPBivariateLinearTableThreshold);
svtkCxxSetObjectMacro(svtkPBivariateLinearTableThreshold, Controller, svtkMultiProcessController);

svtkPBivariateLinearTableThreshold::svtkPBivariateLinearTableThreshold()
{
  this->Controller = 0;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

svtkPBivariateLinearTableThreshold::~svtkPBivariateLinearTableThreshold()
{
  this->SetController(0);
}

void svtkPBivariateLinearTableThreshold::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

int svtkPBivariateLinearTableThreshold::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->Superclass::RequestData(request, inputVector, outputVector);

  // single process?
  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
  {
    return 1;
  }

  svtkCommunicator* comm = this->Controller->GetCommunicator();
  if (!comm)
  {
    svtkErrorMacro("Need a communicator.");
    return 0;
  }

  svtkTable* outRowDataTable = svtkTable::GetData(outputVector, OUTPUT_ROW_DATA);

  int numProcesses = this->Controller->GetNumberOfProcesses();

  // 2) gather the selected data together
  // for each column, make a new one and add it to a new table
  svtkSmartPointer<svtkTable> gatheredTable = svtkSmartPointer<svtkTable>::New();
  for (int i = 0; i < outRowDataTable->GetNumberOfColumns(); i++)
  {
    svtkAbstractArray* col = svtkArrayDownCast<svtkAbstractArray>(outRowDataTable->GetColumn(i));
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

  outRowDataTable->ShallowCopy(gatheredTable);

  return 1;
}
