/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollectGraph.cxx

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
#include "svtkCollectGraph.h"

#include "svtkCellData.h"
#include "svtkEdgeListIterator.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMultiProcessController.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"
#include "svtkSocketController.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkVariant.h"

#include <map>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkCollectGraph);

svtkCxxSetObjectMacro(svtkCollectGraph, Controller, svtkMultiProcessController);
svtkCxxSetObjectMacro(svtkCollectGraph, SocketController, svtkSocketController);

//----------------------------------------------------------------------------
svtkCollectGraph::svtkCollectGraph()
{
  this->PassThrough = 0;
  this->SocketController = nullptr;

  // Default vertex id array.
  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, "id");

  // Controller keeps a reference to this object as well.
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());

  this->OutputType = USE_INPUT_TYPE;
}

//----------------------------------------------------------------------------
svtkCollectGraph::~svtkCollectGraph()
{
  this->SetController(nullptr);
  this->SetSocketController(nullptr);
}

//--------------------------------------------------------------------------
int svtkCollectGraph::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//--------------------------------------------------------------------------
int svtkCollectGraph::RequestDataObject(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->OutputType == USE_INPUT_TYPE)
  {
    return Superclass::RequestDataObject(request, inputVector, outputVector);
  }

  svtkGraph* output = nullptr;
  if (this->OutputType == DIRECTED_OUTPUT)
  {
    output = svtkDirectedGraph::New();
  }
  else if (this->OutputType == UNDIRECTED_OUTPUT)
  {
    output = svtkUndirectedGraph::New();
  }
  else
  {
    svtkErrorMacro(<< "Invalid output type setting.");
    return 0;
  }
  svtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(svtkDataObject::DATA_OBJECT(), output);
  output->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkCollectGraph::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkGraph* output = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int numProcs, myId;
  int idx;

  if (this->Controller == nullptr && this->SocketController == nullptr)
  { // Running as a single process.
    output->ShallowCopy(input);
    return 1;
  }

  if (this->Controller == nullptr && this->SocketController != nullptr)
  { // This is a client.  We assume no data on client for input.
    if (!this->PassThrough)
    {
      if (this->OutputType != DIRECTED_OUTPUT && this->OutputType != UNDIRECTED_OUTPUT)
      {
        svtkErrorMacro(
          << "OutputType must be set to DIRECTED_OUTPUT or UNDIRECTED_OUTPUT on the client.");
        return 0;
      }
      svtkGraph* g = nullptr;
      if (this->OutputType == DIRECTED_OUTPUT)
      {
        g = svtkDirectedGraph::New();
      }
      else
      {
        g = svtkUndirectedGraph::New();
      }
      this->SocketController->Receive(g, 1, 121767);
      output->ShallowCopy(g);
      g->Delete();
      g = nullptr;
      return 1;
    }
    // If not collected, output will be empty from initialization.
    return 0;
  }

  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

  if (this->PassThrough)
  {
    // Just copy and return (no collection).
    output->ShallowCopy(input);
    return 1;
  }

  // Collect.
  if (myId == 0)
  {
    svtkSmartPointer<svtkMutableDirectedGraph> dirBuilder =
      svtkSmartPointer<svtkMutableDirectedGraph>::New();
    svtkSmartPointer<svtkMutableUndirectedGraph> undirBuilder =
      svtkSmartPointer<svtkMutableUndirectedGraph>::New();

    bool directed = (svtkDirectedGraph::SafeDownCast(input) != nullptr);

    svtkGraph* builder = nullptr;
    if (directed)
    {
      builder = dirBuilder;
    }
    else
    {
      builder = undirBuilder;
    }

    svtkDataSetAttributes* wholePointData = builder->GetVertexData();
    svtkPoints* wholePoints = builder->GetPoints();
    wholePointData->CopyAllocate(input->GetVertexData());

    // Get the name of the ID array.
    svtkAbstractArray* ids = this->GetInputAbstractArrayToProcess(0, inputVector);

    if (ids == nullptr)
    {
      svtkErrorMacro("The ID array is undefined.");
      return 0;
    }

    if (!ids->IsA("svtkIntArray") && !ids->IsA("svtkStringArray"))
    {
      svtkErrorMacro(
        "The ID array must be an integer or string array but is a " << ids->GetClassName());
      return 0;
    }

    char* idFieldName = ids->GetName();

    // Map from global ids (owner, ownerId pairs) to wholeGraph ids.
    std::map<int, svtkIdType> globalIdMapInt;
    std::map<svtkStdString, svtkIdType> globalIdMapStr;

    // Map from curGraph ids to wholeGraph ids.
    std::vector<svtkIdType> localIdVec;

    // Edge iterator.
    svtkSmartPointer<svtkEdgeListIterator> edges = svtkSmartPointer<svtkEdgeListIterator>::New();

    for (idx = 0; idx < numProcs; ++idx)
    {
      svtkGraph* curGraph;
      if (idx == 0)
      {
        curGraph = input;
      }
      else
      {
        if (directed)
        {
          curGraph = svtkDirectedGraph::New();
        }
        else
        {
          curGraph = svtkUndirectedGraph::New();
        }
        this->Controller->Receive(curGraph, idx, 121767);

        // Resize the point data arrays to fit the new data.
        svtkIdType numVertices =
          directed ? dirBuilder->GetNumberOfVertices() : undirBuilder->GetNumberOfVertices();
        svtkIdType newSize = numVertices + curGraph->GetNumberOfVertices();
        for (svtkIdType i = 0; i < wholePointData->GetNumberOfArrays(); i++)
        {
          svtkAbstractArray* arr = wholePointData->GetAbstractArray(i);
          arr->Resize(newSize);
        }
      }

      svtkAbstractArray* idArr = curGraph->GetVertexData()->GetAbstractArray(idFieldName);
      svtkStringArray* idArrStr = svtkArrayDownCast<svtkStringArray>(idArr);
      svtkIntArray* idArrInt = svtkArrayDownCast<svtkIntArray>(idArr);

      svtkIntArray* ghostLevelsArr = svtkArrayDownCast<svtkIntArray>(
        wholePointData->GetAbstractArray(svtkDataSetAttributes::GhostArrayName()));

      // Add new vertices
      localIdVec.clear();
      svtkIdType numVerts = curGraph->GetNumberOfVertices();
      for (svtkIdType v = 0; v < numVerts; v++)
      {
        svtkStdString globalIdStr = idArrStr ? idArrStr->GetValue(v) : svtkStdString("");
        int globalIdInt = idArrInt ? idArrInt->GetValue(v) : 0;

        double pt[3];
        if ((idArrInt && globalIdMapInt.count(globalIdInt) == 0) ||
          (idArrStr && globalIdMapStr.count(globalIdStr) == 0))
        {
          curGraph->GetPoint(v, pt);
          wholePoints->InsertNextPoint(pt);
          svtkIdType id = -1;
          if (directed)
          {
            id = dirBuilder->AddVertex();
          }
          else
          {
            id = undirBuilder->AddVertex();
          }

          // Cannot use CopyData because the arrays may switch order during network transfer.
          // Instead, look up the array name.  This assumes unique array names.
          // wholePointData->CopyData(curGraph->GetPointData(), v, id);
          for (svtkIdType arrIndex = 0; arrIndex < wholePointData->GetNumberOfArrays(); arrIndex++)
          {
            svtkAbstractArray* arr = wholePointData->GetAbstractArray(arrIndex);
            svtkAbstractArray* curArr = curGraph->GetVertexData()->GetAbstractArray(arr->GetName());

            // Always set the ghost levels array to zero.
            if (arr == ghostLevelsArr)
            {
              ghostLevelsArr->InsertNextValue(0);
            }
            else
            {
              arr->InsertNextTuple(v, curArr);
            }
          }

          if (idArrInt)
          {
            globalIdMapInt[globalIdInt] = id;
          }
          else
          {
            globalIdMapStr[globalIdStr] = id;
          }
          localIdVec.push_back(id);
        }
        else
        {
          if (idArrInt)
          {
            localIdVec.push_back(globalIdMapInt[globalIdInt]);
          }
          else
          {
            localIdVec.push_back(globalIdMapStr[globalIdStr]);
          }
        }
      }

      // Add non-ghost edges
      svtkIntArray* edgeGhostLevelsArr = svtkArrayDownCast<svtkIntArray>(
        curGraph->GetEdgeData()->GetAbstractArray(svtkDataSetAttributes::GhostArrayName()));
      curGraph->GetEdges(edges);
      while (edges->HasNext())
      {
        svtkEdgeType e = edges->Next();
        if (edgeGhostLevelsArr == nullptr || edgeGhostLevelsArr->GetValue(e.Id) == 0)
        {
          if (directed)
          {
            dirBuilder->AddEdge(localIdVec[e.Source], localIdVec[e.Target]);
          }
          else
          {
            undirBuilder->AddEdge(localIdVec[e.Source], localIdVec[e.Target]);
          }
        }
      }

      if (idx != 0)
      {
        curGraph->Delete();
      }
    }
    undirBuilder->Squeeze();
    dirBuilder->Squeeze();

    if (this->SocketController)
    { // Send collected data onto client.
      this->SocketController->Send(builder, 1, 121767);
      // output will be empty.
    }
    else
    { // No client. Keep the output here.
      output->ShallowCopy(builder);
    }
  }
  else
  {
    this->Controller->Send(input, 0, 121767);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkCollectGraph::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PassThough: " << this->PassThrough << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "SocketController: (" << this->SocketController << ")\n";
  os << indent << "OutputType: " << this->OutputType << endl;
}
