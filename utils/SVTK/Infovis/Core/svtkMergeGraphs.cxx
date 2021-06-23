/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeGraphs.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkMergeGraphs.h"

#include "svtkDataSetAttributes.h"
#include "svtkEdgeListIterator.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergeTables.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableGraphHelper.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#include <map>

svtkStandardNewMacro(svtkMergeGraphs);
//---------------------------------------------------------------------------
svtkMergeGraphs::svtkMergeGraphs()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
  this->UseEdgeWindow = false;
  this->EdgeWindowArrayName = nullptr;
  this->SetEdgeWindowArrayName("time");
  this->EdgeWindow = 10000;
}

//---------------------------------------------------------------------------
svtkMergeGraphs::~svtkMergeGraphs()
{
  this->SetEdgeWindowArrayName(nullptr);
}

//---------------------------------------------------------------------------
int svtkMergeGraphs::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//---------------------------------------------------------------------------
// Fills array_map with matching arrays from data1 to data2
static void svtkMergeGraphsCreateArrayMapping(
  std::map<svtkAbstractArray*, svtkAbstractArray*>& array_map, svtkDataSetAttributes* data1,
  svtkDataSetAttributes* data2)
{
  svtkIdType narr1 = data1->GetNumberOfArrays();
  for (int arr1 = 0; arr1 < narr1; ++arr1)
  {
    svtkAbstractArray* a1 = data1->GetAbstractArray(arr1);
    svtkAbstractArray* a2 = data2->GetAbstractArray(a1->GetName());
    if (a2 && a1->GetDataType() == a2->GetDataType() &&
      a1->GetNumberOfComponents() == a2->GetNumberOfComponents())
    {
      array_map[a1] = a2;
    }
  }
  // Force pedigree id array to match
  if (data1->GetPedigreeIds() && data2->GetPedigreeIds())
    array_map[data1->GetPedigreeIds()] = data2->GetPedigreeIds();
}

//---------------------------------------------------------------------------
// Uses array_map to append a row to data1 corresponding to
// row index2 of mapped arrays (which came from data2)
static void svtkMergeGraphsAddRow(svtkDataSetAttributes* data1, svtkIdType index2,
  std::map<svtkAbstractArray*, svtkAbstractArray*>& array_map)
{
  int narr1 = data1->GetNumberOfArrays();
  for (int arr1 = 0; arr1 < narr1; ++arr1)
  {
    svtkAbstractArray* a1 = data1->GetAbstractArray(arr1);
    if (array_map.find(a1) != array_map.end())
    {
      svtkAbstractArray* a2 = array_map[a1];
      a1->InsertNextTuple(index2, a2);
    }
    else
    {
      svtkIdType num_values = a1->GetNumberOfTuples() * a1->GetNumberOfComponents();
      for (svtkIdType i = 0; i < a1->GetNumberOfComponents(); ++i)
      {
        a1->InsertVariantValue(num_values + i, svtkVariant());
      }
    }
  }
}

//---------------------------------------------------------------------------
int svtkMergeGraphs::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* graph1_info = inputVector[0]->GetInformationObject(0);
  svtkGraph* graph1 = svtkGraph::SafeDownCast(graph1_info->Get(svtkDataObject::DATA_OBJECT()));

  // Copy structure into output graph.
  svtkInformation* outputInfo = outputVector->GetInformationObject(0);
  svtkGraph* output = svtkGraph::SafeDownCast(outputInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkInformation* graph2_info = inputVector[1]->GetInformationObject(0);
  if (!graph2_info)
  {
    // If no second graph provided, we're done
    output->CheckedShallowCopy(graph1);
    return 1;
  }

  svtkGraph* graph2 = svtkGraph::SafeDownCast(graph2_info->Get(svtkDataObject::DATA_OBJECT()));

  // Make a copy of the graph
  svtkSmartPointer<svtkMutableGraphHelper> builder = svtkSmartPointer<svtkMutableGraphHelper>::New();
  if (svtkDirectedGraph::SafeDownCast(output))
  {
    svtkSmartPointer<svtkMutableDirectedGraph> g = svtkSmartPointer<svtkMutableDirectedGraph>::New();
    builder->SetGraph(g);
  }
  else
  {
    svtkSmartPointer<svtkMutableUndirectedGraph> g =
      svtkSmartPointer<svtkMutableUndirectedGraph>::New();
    builder->SetGraph(g);
  }
  builder->GetGraph()->DeepCopy(graph1);

  if (!this->ExtendGraph(builder, graph2))
  {
    return 0;
  }

  if (!output->CheckedShallowCopy(builder->GetGraph()))
  {
    svtkErrorMacro("Output graph format invalid.");
    return 0;
  }

  return 1;
}

//---------------------------------------------------------------------------
int svtkMergeGraphs::ExtendGraph(svtkMutableGraphHelper* builder, svtkGraph* graph2)
{
  svtkAbstractArray* ped_ids1 = builder->GetGraph()->GetVertexData()->GetPedigreeIds();
  if (!ped_ids1)
  {
    svtkErrorMacro("First graph must have pedigree ids");
    return 0;
  }

  svtkAbstractArray* ped_ids2 = graph2->GetVertexData()->GetPedigreeIds();
  if (!ped_ids1)
  {
    svtkErrorMacro("Second graph must have pedigree ids");
    return 0;
  }

  // Find matching vertex arrays
  std::map<svtkAbstractArray*, svtkAbstractArray*> vert_array_map;
  svtkDataSetAttributes* vert_data1 = builder->GetGraph()->GetVertexData();
  svtkMergeGraphsCreateArrayMapping(vert_array_map, vert_data1, graph2->GetVertexData());

  // Find graph1 vertices matching graph2's pedigree ids
  svtkIdType n2 = graph2->GetNumberOfVertices();
  std::vector<svtkIdType> graph2_to_graph1(n2);
  for (svtkIdType vert2 = 0; vert2 < n2; ++vert2)
  {
    svtkIdType vert1 = ped_ids1->LookupValue(ped_ids2->GetVariantValue(vert2));
    if (vert1 == -1)
    {
      vert1 = builder->AddVertex();
      svtkMergeGraphsAddRow(vert_data1, vert2, vert_array_map);
    }
    graph2_to_graph1[vert2] = vert1;
  }

  // Find matching edge arrays
  std::map<svtkAbstractArray*, svtkAbstractArray*> edge_array_map;
  svtkDataSetAttributes* edge_data1 = builder->GetGraph()->GetEdgeData();
  svtkMergeGraphsCreateArrayMapping(edge_array_map, edge_data1, graph2->GetEdgeData());

  // For each edge in graph2, add it to the output
  svtkSmartPointer<svtkEdgeListIterator> it = svtkSmartPointer<svtkEdgeListIterator>::New();
  graph2->GetEdges(it);
  while (it->HasNext())
  {
    svtkEdgeType e = it->Next();
    svtkIdType source = graph2_to_graph1[e.Source];
    svtkIdType target = graph2_to_graph1[e.Target];
    if (source != -1 && target != -1)
    {
      builder->AddEdge(source, target);
      svtkMergeGraphsAddRow(edge_data1, e.Id, edge_array_map);
    }
  }

  // Remove edges if using an edge window.
  if (this->UseEdgeWindow)
  {
    if (!this->EdgeWindowArrayName)
    {
      svtkErrorMacro("EdgeWindowArrayName must not be null if using edge window.");
      return 0;
    }
    svtkDataArray* windowArr = svtkArrayDownCast<svtkDataArray>(
      builder->GetGraph()->GetEdgeData()->GetAbstractArray(this->EdgeWindowArrayName));
    if (!windowArr)
    {
      svtkErrorMacro("EdgeWindowArrayName not found or not a numeric array.");
      return 0;
    }
    double range[2];
    range[0] = SVTK_DOUBLE_MAX;
    range[1] = SVTK_DOUBLE_MIN;
    svtkIdType numEdges = builder->GetGraph()->GetNumberOfEdges();
    for (svtkIdType i = 0; i < numEdges; ++i)
    {
      double val = windowArr->GetTuple1(i);
      if (val < range[0])
      {
        range[0] = val;
      }
      if (val > range[1])
      {
        range[1] = val;
      }
    }
    double cutoff = range[1] - this->EdgeWindow;
    if (range[0] < cutoff)
    {
      svtkSmartPointer<svtkIdTypeArray> edgesToRemove = svtkSmartPointer<svtkIdTypeArray>::New();
      for (svtkIdType i = 0; i < numEdges; ++i)
      {
        if (windowArr->GetTuple1(i) < cutoff)
        {
          edgesToRemove->InsertNextValue(i);
        }
      }
      builder->RemoveEdges(edgesToRemove);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
void svtkMergeGraphs::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseEdgeWindow: " << this->UseEdgeWindow << endl;
  os << indent << "EdgeWindowArrayName: "
     << (this->EdgeWindowArrayName ? this->EdgeWindowArrayName : "(none)") << endl;
  os << indent << "EdgeWindow: " << this->EdgeWindow << endl;
}
