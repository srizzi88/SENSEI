/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollapseVerticesByArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCollapseVerticesByArray.h"

#include "svtkAbstractArray.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkEdgeListIterator.h"
#include "svtkGraphEdge.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkOutEdgeIterator.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkVertexListIterator.h"

#include <map>    // Using STL.
#include <string> // Using STL.
#include <vector> // Using STL.

svtkStandardNewMacro(svtkCollapseVerticesByArray);

//------------------------------------------------------------------------------
class svtkCollapseVerticesByArrayInternal
{
public:
  std::vector<std::string> AggregateEdgeArrays;
};

//------------------------------------------------------------------------------
svtkCollapseVerticesByArray::svtkCollapseVerticesByArray()
  : svtkGraphAlgorithm()
  , AllowSelfLoops(false)
  , VertexArray(nullptr)
  , CountEdgesCollapsed(0)
  , EdgesCollapsedArray(nullptr)
  , CountVerticesCollapsed(0)
  , VerticesCollapsedArray(nullptr)
{
  // Setting default names.
  this->SetVerticesCollapsedArray("VerticesCollapsedCountArray");
  this->SetEdgesCollapsedArray("EdgesCollapsedCountArray");

  this->Internal = new svtkCollapseVerticesByArrayInternal();
}

// ----------------------------------------------------------------------------
svtkCollapseVerticesByArray::~svtkCollapseVerticesByArray()
{
  delete this->Internal;
  delete[] this->VertexArray;
  delete[] this->VerticesCollapsedArray;
  delete[] this->EdgesCollapsedArray;
}

// ----------------------------------------------------------------------------
void svtkCollapseVerticesByArray::PrintSelf(ostream& os, svtkIndent indent)
{
  // Base class print.
  svtkGraphAlgorithm::PrintSelf(os, indent);

  os << indent << "AllowSelfLoops: " << this->AllowSelfLoops << endl;
  os << indent << "VertexArray: " << (this->VertexArray ? this->VertexArray : "nullptr") << endl;

  os << indent << "CountEdgesCollapsed: " << this->CountEdgesCollapsed << endl;
  os << indent << "EdgesCollapsedArray: "
     << (this->EdgesCollapsedArray ? this->EdgesCollapsedArray : "nullptr") << endl;

  os << indent << "CountVerticesCollapsed: " << this->CountVerticesCollapsed << endl;
  os << indent << "VerticesCollapsedArray: "
     << (this->VerticesCollapsedArray ? this->VerticesCollapsedArray : "nullptr") << endl;
}

//------------------------------------------------------------------------------
void svtkCollapseVerticesByArray::AddAggregateEdgeArray(const char* arrName)
{
  this->Internal->AggregateEdgeArrays.push_back(std::string(arrName));
}

//------------------------------------------------------------------------------
void svtkCollapseVerticesByArray::ClearAggregateEdgeArray()
{
  this->Internal->AggregateEdgeArrays.clear();
}

//------------------------------------------------------------------------------
int svtkCollapseVerticesByArray::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
  {
    svtkErrorMacro("Error: nullptr input svtkInformation");
    return 0;
  }

  svtkDataObject* inObj = inInfo->Get(svtkDataObject::DATA_OBJECT());

  if (!inObj)
  {
    svtkErrorMacro(<< "Error: nullptr svtkDataObject");
    return 0;
  }

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    svtkErrorMacro("Error: nullptr output svtkInformation");
    return 0;
  }

  svtkDataObject* outObj = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (!outObj)
  {
    svtkErrorMacro("Error: nullptr output svtkDataObject");
    return 0;
  }

  svtkGraph* outGraph = this->Create(svtkGraph::SafeDownCast(inObj));
  if (outGraph)
  {
    svtkDirectedGraph::SafeDownCast(outObj)->ShallowCopy(outGraph);
    outGraph->Delete();
  }
  else
  {
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
int svtkCollapseVerticesByArray::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDirectedGraph");
  return 1;
}

//------------------------------------------------------------------------------
svtkGraph* svtkCollapseVerticesByArray::Create(svtkGraph* inGraph)
{
  if (!inGraph)
  {
    return nullptr;
  }

  if (!this->VertexArray)
  {
    return nullptr;
  }

  typedef svtkSmartPointer<svtkMutableDirectedGraph> svtkMutableDirectedGraphRefPtr;
  typedef svtkSmartPointer<svtkEdgeListIterator> svtkEdgeListIteratorRefPtr;
  typedef svtkSmartPointer<svtkVertexListIterator> svtkVertexListIteratorRefPtr;
  typedef svtkSmartPointer<svtkIntArray> svtkIntArrayRefPtr;
  typedef std::pair<svtkVariant, svtkIdType> NameIdPair;

  // Create a new merged graph.
  svtkMutableDirectedGraphRefPtr outGraph(svtkMutableDirectedGraphRefPtr::New());

  svtkVertexListIteratorRefPtr itr(svtkVertexListIteratorRefPtr::New());
  itr->SetGraph(inGraph);

  // Copy the input vertex data and edge data arrays to the
  // output graph vertex and edge data.
  outGraph->GetVertexData()->CopyAllocate(inGraph->GetVertexData());
  outGraph->GetEdgeData()->CopyAllocate(inGraph->GetEdgeData());

  svtkDataSetAttributes* inVtxDsAttrs = inGraph->GetVertexData();
  svtkDataSetAttributes* inEgeDsAttrs = inGraph->GetEdgeData();

  if (!inVtxDsAttrs)
  {
    svtkErrorMacro("Error: No vertex data found on the graph.");
    return nullptr;
  }

  // Find the vertex array of interest.
  svtkAbstractArray* inVertexAOI = inVtxDsAttrs->GetAbstractArray(this->VertexArray);

  // Cannot proceed if vertex array of interest is not found.
  if (!inVertexAOI)
  {
    svtkErrorMacro("Error: Could not find the key vertex array.");
    return nullptr;
  }

  // Optional.
  svtkIntArrayRefPtr countEdgesCollapsedArray(nullptr);
  if (this->CountEdgesCollapsed)
  {
    countEdgesCollapsedArray = svtkIntArrayRefPtr::New();
    countEdgesCollapsedArray->SetName(this->EdgesCollapsedArray);
    countEdgesCollapsedArray->SetNumberOfComponents(1);
    outGraph->GetEdgeData()->AddArray(countEdgesCollapsedArray);
  }

  // Optional.
  svtkIntArrayRefPtr countVerticesCollapsedArray(nullptr);
  if (this->CountVerticesCollapsed)
  {
    countVerticesCollapsedArray = svtkIntArrayRefPtr::New();
    countVerticesCollapsedArray->SetName(this->VerticesCollapsedArray);
    countVerticesCollapsedArray->SetNumberOfComponents(1);
    outGraph->GetVertexData()->AddArray(countVerticesCollapsedArray);
  }

  // Arrays of interest.
  std::vector<svtkDataArray*> inEdgeDataArraysOI;
  std::vector<svtkDataArray*> outEdgeDataArraysOI;

  // All other arrays.
  std::vector<svtkAbstractArray*> inEdgeDataArraysAO;
  std::vector<svtkAbstractArray*> outEdgeDataArraysAO;

  std::vector<svtkAbstractArray*> inVertexDataArraysAO;
  std::vector<svtkAbstractArray*> outVertexDataArraysAO;

  //++
  // Find all the input vertex data arrays except the one set as key.
  for (int i = 0; i < inVtxDsAttrs->GetNumberOfArrays(); ++i)
  {
    svtkAbstractArray* absArray = inVtxDsAttrs->GetAbstractArray(i);
    if (strcmp(absArray->GetName(), inVertexAOI->GetName()) == 0)
    {
      continue;
    }

    inVertexDataArraysAO.push_back(absArray);
  }

  for (size_t i = 0; i < inVertexDataArraysAO.size(); ++i)
  {
    if (!inVertexDataArraysAO[i]->GetName())
    {
      svtkErrorMacro("Error: Name on the array is nullptr or not set.");
      return nullptr;
    }

    outVertexDataArraysAO.push_back(
      outGraph->GetVertexData()->GetAbstractArray(inVertexDataArraysAO[i]->GetName()));
    outVertexDataArraysAO.back()->SetNumberOfTuples(inVertexDataArraysAO[i]->GetNumberOfTuples());
  }
  //--

  //++
  // Find all the input edge data arrays of interest or not.
  for (int i = 0; i < inEgeDsAttrs->GetNumberOfArrays(); ++i)
  {
    svtkAbstractArray* absArray = inEgeDsAttrs->GetAbstractArray(i);

    bool alreadyAdded(false);
    for (size_t j = 0; j < this->Internal->AggregateEdgeArrays.size(); ++j)
    {
      if (strcmp(absArray->GetName(), this->Internal->AggregateEdgeArrays[j].c_str()) == 0)
      {
        svtkDataArray* inDataArray = svtkArrayDownCast<svtkDataArray>(absArray);
        if (inDataArray)
        {
          inEdgeDataArraysOI.push_back(inDataArray);
        }
        else
        {
          inEdgeDataArraysAO.push_back(absArray);
        }
        alreadyAdded = true;
        break;
      }
      else
      {
        continue;
      }
    } // End inner for.

    if (!alreadyAdded)
    {
      inEdgeDataArraysAO.push_back(absArray);
    }
  }

  // Find the corresponding empty arrays in output graph.
  svtkAbstractArray* outVertexAOI(outGraph->GetVertexData()->GetAbstractArray(this->VertexArray));

  // Arrays of interest.
  if (!inEdgeDataArraysOI.empty())
  {
    for (size_t i = 0; i < inEdgeDataArraysOI.size(); ++i)
    {
      if (!inEdgeDataArraysOI[i]->GetName())
      {
        svtkErrorMacro("Error: Name on the array is nullptr or not set.");
        return nullptr;
      }

      svtkDataArray* outDataArray = svtkArrayDownCast<svtkDataArray>(
        outGraph->GetEdgeData()->GetAbstractArray(inEdgeDataArraysOI[i]->GetName()));

      outEdgeDataArraysOI.push_back(outDataArray);
      outDataArray->SetNumberOfTuples(inEdgeDataArraysOI[i]->GetNumberOfTuples());
    }
  }

  // All others.
  if (!inEdgeDataArraysAO.empty())
  {
    for (size_t i = 0; i < inEdgeDataArraysAO.size(); ++i)
    {
      if (!inEdgeDataArraysAO[i]->GetName())
      {
        svtkErrorMacro("Error: Name on the array is nullptr or not set.");
        return nullptr;
      }

      svtkAbstractArray* outAbsArray =
        outGraph->GetEdgeData()->GetAbstractArray(inEdgeDataArraysAO[i]->GetName());

      outEdgeDataArraysAO.push_back(outAbsArray);
      outAbsArray->SetNumberOfTuples(inEdgeDataArraysAO[i]->GetNumberOfTuples());
    }
  }
  //--

  std::map<svtkVariant, svtkIdType> myMap;
  std::map<svtkVariant, svtkIdType>::iterator myItr;

  svtkIdType inSourceId;
  svtkIdType inTargetId;
  svtkIdType outSourceId;
  svtkIdType outTargetId;
  svtkIdType outEdgeId;

  // Iterate over all the vertices.
  while (itr->HasNext())
  {
    inSourceId = itr->Next();
    svtkVariant source = inVertexAOI->GetVariantValue(inSourceId);

    myItr = myMap.find(source);

    if (myItr != myMap.end())
    {
      // If we already have a vertex for this "source" get its id.
      outSourceId = myItr->second;
      if (this->CountVerticesCollapsed)
      {
        countVerticesCollapsedArray->SetValue(
          outSourceId, countVerticesCollapsedArray->GetValue(outSourceId) + 1);
      }
    }
    else
    {
      // If not then add a new vertex to the output graph.
      outSourceId = outGraph->AddVertex();
      outVertexAOI->InsertVariantValue(outSourceId, source);
      myMap.insert(NameIdPair(source, outSourceId));

      if (this->CountVerticesCollapsed)
      {
        countVerticesCollapsedArray->InsertValue(outSourceId, 1);
      }
    }

    for (size_t i = 0; i < inVertexDataArraysAO.size(); ++i)
    {
      outVertexDataArraysAO[i]->SetTuple(outSourceId, inSourceId, inVertexDataArraysAO[i]);
    }
  }

  // Now itereate over all the edges in the graph.
  // Result vary dependeing on whether the input graph is
  // directed or not.
  svtkEdgeListIteratorRefPtr elItr(svtkEdgeListIteratorRefPtr::New());
  inGraph->GetEdges(elItr);

  while (elItr->HasNext())
  {
    svtkGraphEdge* edge = elItr->NextGraphEdge();
    inSourceId = edge->GetSource();
    inTargetId = edge->GetTarget();

    svtkVariant source = inVertexAOI->GetVariantValue(inSourceId);
    svtkVariant target = inVertexAOI->GetVariantValue(inTargetId);

    // Again find if there is associated vertex with this name.
    myItr = myMap.find(source);
    outSourceId = myItr->second;

    myItr = myMap.find(target);
    outTargetId = myItr->second;

    // Find if there is an edge between the out source and target.
    FindEdge(outGraph, outSourceId, outTargetId, outEdgeId);

    if ((outSourceId == outTargetId) && !this->AllowSelfLoops)
    {
      continue;
    }

    //++
    if (outEdgeId == -1)
    {
      outEdgeId = outGraph->AddEdge(outSourceId, outTargetId).Id;

      // Edge does not exist. Add a new one.
      if (inEdgeDataArraysOI.empty() && inEdgeDataArraysAO.empty())
      {
        continue;
      }

      // Arrays of interest.
      for (size_t i = 0; i < inEdgeDataArraysOI.size(); ++i)
      {

        outEdgeDataArraysOI[i]->SetTuple(outEdgeId, edge->GetId(), inEdgeDataArraysOI[i]);
      }

      // All others. Last entered will override previous ones.
      for (size_t i = 0; i < inEdgeDataArraysAO.size(); ++i)
      {
        outEdgeDataArraysAO[i]->SetTuple(outEdgeId, edge->GetId(), inEdgeDataArraysAO[i]);
      }

      if (this->CountEdgesCollapsed)
      {
        countEdgesCollapsedArray->InsertValue(outEdgeId, 1);
      }
    }
    else
    {
      if (inEdgeDataArraysOI.empty() && inEdgeDataArraysAO.empty())
      {
        continue;
      }

      // Find the data on the out edge and add the data from in edge
      // and set it on the out edge.
      for (size_t i = 0; i < inEdgeDataArraysOI.size(); ++i)
      {
        double* outEdgeData = outEdgeDataArraysOI[i]->GetTuple(outEdgeId);
        double* inEdgeData = inEdgeDataArraysOI[i]->GetTuple(edge->GetId());

        if (!outEdgeData && !inEdgeData)
        {
          continue;
        }
        for (int j = 0; j < inEdgeDataArraysOI[i]->GetNumberOfComponents(); ++j)
        {
          outEdgeData[j] = outEdgeData[j] + inEdgeData[j];
        }

        outEdgeDataArraysOI[i]->SetTuple(outEdgeId, outEdgeData);
      }

      // All others. Last entered will override previous ones.
      for (size_t i = 0; i < inEdgeDataArraysAO.size(); ++i)
      {
        outEdgeDataArraysAO[i]->SetTuple(outEdgeId, edge->GetId(), inEdgeDataArraysAO[i]);
      }

      if (this->CountEdgesCollapsed)
      {
        countEdgesCollapsedArray->SetValue(
          outEdgeId, static_cast<int>(countEdgesCollapsedArray->GetValue(outEdgeId) + 1));
      }
    }
    //--

  } // while(elItr->HasNext())

  outGraph->Register(nullptr);
  return outGraph;
}

//------------------------------------------------------------------------------
void svtkCollapseVerticesByArray::FindEdge(
  svtkGraph* outGraph, svtkIdType source, svtkIdType target, svtkIdType& edgeId)
{
  typedef svtkSmartPointer<svtkOutEdgeIterator> svtkOutEdgeIteratorRefPtr;
  edgeId = -1;

  if (!outGraph)
  {
    return;
  }

  svtkOutEdgeIteratorRefPtr itr(svtkOutEdgeIteratorRefPtr::New());

  outGraph->GetOutEdges(source, itr);
  while (itr->HasNext())
  {
    svtkGraphEdge* edge = itr->NextGraphEdge();
    if (edge->GetTarget() == target)
    {
      edgeId = edge->GetId();
      break;
    }
  }
}
