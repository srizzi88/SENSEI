/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExodusIIWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkPExodusIIWriter.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataObject.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkModelMetadata.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkThreshold.h"
#include "svtkUnstructuredGrid.h"

#include "svtkMultiProcessController.h"

#include "svtk_exodusII.h"
#include <cctype>
#include <ctime>

svtkStandardNewMacro(svtkPExodusIIWriter);

//----------------------------------------------------------------------------

svtkPExodusIIWriter::svtkPExodusIIWriter() {}

svtkPExodusIIWriter::~svtkPExodusIIWriter() {}

void svtkPExodusIIWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkPExodusIIWriter::CheckParameters()
{
  svtkMultiProcessController* c = svtkMultiProcessController::GetGlobalController();
  int numberOfProcesses = c ? c->GetNumberOfProcesses() : 1;
  int myRank = c ? c->GetLocalProcessId() : 0;

  if (this->GhostLevel > 0)
  {
    svtkWarningMacro(<< "ExodusIIWriter ignores ghost level request");
  }

  return this->Superclass::CheckParametersInternal(numberOfProcesses, myRank);
}

//----------------------------------------------------------------------------
int svtkPExodusIIWriter::RequestUpdateExtent(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);
  svtkMultiProcessController* c = svtkMultiProcessController::GetGlobalController();
  if (c)
  {
    int numberOfProcesses = c->GetNumberOfProcesses();
    int myRank = c->GetLocalProcessId();

    svtkInformation* info = inputVector[0]->GetInformationObject(0);
    info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), myRank);
    info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numberOfProcesses);
  }

  return 1;
}

void svtkPExodusIIWriter::CheckBlockInfoMap()
{
  // if we're multiprocess we need to make sure the block info map matches
  if (this->NumberOfProcesses > 1)
  {
    int maxId = -1;
    std::map<int, Block>::const_iterator iter;
    for (iter = this->BlockInfoMap.begin(); iter != this->BlockInfoMap.end(); ++iter)
    {
      if (iter->first > maxId)
      {
        maxId = iter->first;
      }
    }
    svtkMultiProcessController* c = svtkMultiProcessController::GetGlobalController();
    int globalMaxId;
    c->AllReduce(&maxId, &globalMaxId, 1, svtkCommunicator::MAX_OP);
    maxId = globalMaxId;
    for (int i = 1; i <= maxId; i++)
    {
      Block& b = this->BlockInfoMap[i]; // ctor called (init all to 0/-1) if not preset
      int globalType;
      c->AllReduce(&b.Type, &globalType, 1, svtkCommunicator::MAX_OP);
      if (b.Type != 0 && b.Type != globalType)
      {
        svtkWarningMacro(<< "The type associated with ID's across processors doesn't match");
      }
      else
      {
        b.Type = globalType;
      }
      int globalNodes;
      c->AllReduce(&b.NodesPerElement, &globalNodes, 1, svtkCommunicator::MAX_OP);
      if (b.NodesPerElement != globalNodes &&
        // on a processor with no data, b.NodesPerElement == 0.
        b.NodesPerElement != 0)
      {
        svtkWarningMacro(<< "NodesPerElement associated with ID's across "
                           "processors doesn't match: "
                        << b.NodesPerElement << " != " << globalNodes);
      }
      else
      {
        b.NodesPerElement = globalNodes;
      }
    }
  }
}

//----------------------------------------------------------------------------
int svtkPExodusIIWriter::GlobalContinueExecuting(int localContinue)
{
  svtkMultiProcessController* c = svtkMultiProcessController::GetGlobalController();
  int globalContinue = localContinue;
  if (c)
  {
    c->AllReduce(&localContinue, &globalContinue, 1, svtkCommunicator::MIN_OP);
  }
  return globalContinue;
}

//----------------------------------------------------------------------------
unsigned int svtkPExodusIIWriter::GetMaxNameLength()
{
  unsigned int maxName = this->Superclass::GetMaxNameLength();

  svtkMultiProcessController* c = svtkMultiProcessController::GetGlobalController();
  unsigned int globalMaxName = 0;
  c->AllReduce(&maxName, &globalMaxName, 1, svtkCommunicator::MAX_OP);
  return maxName;
}
