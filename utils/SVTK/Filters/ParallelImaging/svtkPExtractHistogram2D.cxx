/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkPExtractHistogram2D.cxx

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
#include "svtkPExtractHistogram2D.h"

#include "svtkDataArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkPExtractHistogram2D);
svtkCxxSetObjectMacro(svtkPExtractHistogram2D, Controller, svtkMultiProcessController);
//------------------------------------------------------------------------------
svtkPExtractHistogram2D::svtkPExtractHistogram2D()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}
//------------------------------------------------------------------------------
svtkPExtractHistogram2D::~svtkPExtractHistogram2D()
{
  this->SetController(nullptr);
}
//------------------------------------------------------------------------------
void svtkPExtractHistogram2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
//------------------------------------------------------------------------------
void svtkPExtractHistogram2D::Learn(
  svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta)
{
  svtkTable* primaryTab = svtkTable::SafeDownCast(outMeta->GetBlock(0));
  if (!primaryTab)
  {
    return;
  }

  svtkImageData* outImage =
    svtkImageData::SafeDownCast(this->GetOutputDataObject(svtkPExtractHistogram2D::HISTOGRAM_IMAGE));

  // have all of the nodes compute their histograms
  this->Superclass::Learn(inData, inParameters, outMeta);

  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
  {
    // Nothing to do for single process.
    return;
  }

  // Now we need to collect and reduce data from all nodes on the root.
  svtkCommunicator* comm = this->Controller->GetCommunicator();
  if (!comm)
  {
    svtkErrorMacro("svtkCommunicator is needed.");
    return;
  }

  int myid = this->Controller->GetLocalProcessId();

  svtkImageData* reducedOutImage = svtkImageData::New();
  reducedOutImage->DeepCopy(outImage);

  svtkDataArray* myArray = outImage->GetPointData()->GetScalars();
  svtkDataArray* recvArray = reducedOutImage->GetPointData()->GetScalars();

  // this sums up all of the images and distributes them to
  // every node
  if (!comm->AllReduce(myArray, recvArray, svtkCommunicator::SUM_OP))
  {
    svtkErrorMacro(<< myid << ": Reduce failed!");
    reducedOutImage->Delete();
    return;
  }

  outImage->DeepCopy(reducedOutImage);

  // update the maximum bin count
  for (int i = 0; i < recvArray->GetNumberOfTuples(); i++)
  {
    if (this->MaximumBinCount < recvArray->GetTuple1(i))
      this->MaximumBinCount = (long unsigned)recvArray->GetTuple1(i);
  }

  reducedOutImage->Delete();

  primaryTab->Initialize();
  primaryTab->AddColumn(outImage->GetPointData()->GetScalars());
}

int svtkPExtractHistogram2D::ComputeBinExtents(svtkDataArray* col1, svtkDataArray* col2)
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1 ||
    this->UseCustomHistogramExtents)
  {
    // Nothing extra to do for single process.
    return this->Superclass::ComputeBinExtents(col1, col2);
  }

  svtkCommunicator* comm = this->Controller->GetCommunicator();
  if (!comm)
  {
    svtkErrorMacro("svtkCommunicator is needed.");
    return false;
  }

  // have everyone compute their own bin extents
  double myRange[4] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN, SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN };
  double allRange[4] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN, SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN };
  if (this->Superclass::ComputeBinExtents(col1, col2))
  {
    double* r = this->GetHistogramExtents();
    myRange[0] = r[0];
    myRange[1] = r[1];
    myRange[2] = r[2];
    myRange[3] = r[3];
  }

  int myid = this->Controller->GetLocalProcessId();
  double* r = this->GetHistogramExtents();
  if (!comm->AllReduce(myRange, allRange, 1, svtkCommunicator::MIN_OP) ||
    !comm->AllReduce(myRange + 1, allRange + 1, 1, svtkCommunicator::MAX_OP) ||
    !comm->AllReduce(myRange + 2, allRange + 2, 1, svtkCommunicator::MIN_OP) ||
    !comm->AllReduce(myRange + 3, allRange + 3, 1, svtkCommunicator::MAX_OP))
  {
    svtkErrorMacro(<< myid << ": Reduce failed!");
    return 0;
  }

  r[0] = allRange[0];
  r[1] = allRange[1];
  r[2] = allRange[2];
  r[3] = allRange[3];
  return 1;
}
