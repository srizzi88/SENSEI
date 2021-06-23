/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToGraph.cxx

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

#include "svtkTableToGraph.h"

#include "svtkBitArray.h"
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkDataArray.h"
#include "svtkEdgeListIterator.h"
#include "svtkExecutive.h"
#include "svtkExtractSelectedGraph.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkStandardNewMacro(svtkTableToGraph);
svtkCxxSetObjectMacro(svtkTableToGraph, LinkGraph, svtkMutableDirectedGraph);
//---------------------------------------------------------------------------
svtkTableToGraph::svtkTableToGraph()
{
  this->Directed = 0;
  this->LinkGraph = svtkMutableDirectedGraph::New();
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//---------------------------------------------------------------------------
svtkTableToGraph::~svtkTableToGraph()
{
  this->SetLinkGraph(nullptr);
}

//---------------------------------------------------------------------------
int svtkTableToGraph::ValidateLinkGraph()
{
  if (!this->LinkGraph)
  {
    this->LinkGraph = svtkMutableDirectedGraph::New();
  }
  if (!svtkArrayDownCast<svtkStringArray>(
        this->LinkGraph->GetVertexData()->GetAbstractArray("column")))
  {
    if (this->LinkGraph->GetNumberOfVertices() == 0)
    {
      svtkStringArray* column = svtkStringArray::New();
      column->SetName("column");
      this->LinkGraph->GetVertexData()->AddArray(column);
      column->Delete();
      this->Modified();
    }
    else
    {
      svtkErrorMacro("The link graph must contain a string array named \"column\".");
      return 0;
    }
  }
  if (!svtkArrayDownCast<svtkStringArray>(
        this->LinkGraph->GetVertexData()->GetAbstractArray("domain")))
  {
    svtkStringArray* domain = svtkStringArray::New();
    domain->SetName("domain");
    domain->SetNumberOfTuples(this->LinkGraph->GetNumberOfVertices());
    for (svtkIdType i = 0; i < this->LinkGraph->GetNumberOfVertices(); i++)
    {
      domain->SetValue(i, "");
    }
    this->LinkGraph->GetVertexData()->AddArray(domain);
    domain->Delete();
    this->Modified();
  }
  if (!svtkArrayDownCast<svtkBitArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("hidden")))
  {
    svtkBitArray* hidden = svtkBitArray::New();
    hidden->SetName("hidden");
    hidden->SetNumberOfTuples(this->LinkGraph->GetNumberOfVertices());
    this->LinkGraph->GetVertexData()->AddArray(hidden);
    hidden->Delete();
    this->Modified();
  }
  if (!svtkArrayDownCast<svtkIntArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("active")))
  {
    svtkIntArray* active = svtkIntArray::New();
    active->SetName("active");
    active->SetNumberOfTuples(this->LinkGraph->GetNumberOfVertices());
    for (svtkIdType i = 0; i < this->LinkGraph->GetNumberOfVertices(); ++i)
    {
      active->SetValue(i, 1);
    }
    this->LinkGraph->GetVertexData()->AddArray(active);
    active->Delete();
    this->Modified();
  }
  return 1;
}

//---------------------------------------------------------------------------
void svtkTableToGraph::AddLinkVertex(const char* column, const char* domain, int hidden)
{
  if (!column)
  {
    svtkErrorMacro("Column name cannot be null");
    return;
  }

  svtkStdString domainStr = "";
  if (domain)
  {
    domainStr = domain;
  }

  if (!this->ValidateLinkGraph())
  {
    return;
  }

  svtkStringArray* columnArr =
    svtkArrayDownCast<svtkStringArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("column"));
  svtkStringArray* domainArr =
    svtkArrayDownCast<svtkStringArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("domain"));
  svtkBitArray* hiddenArr =
    svtkArrayDownCast<svtkBitArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("hidden"));
  svtkIntArray* activeArr =
    svtkArrayDownCast<svtkIntArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("active"));

  svtkIdType index = -1;
  for (svtkIdType i = 0; i < this->LinkGraph->GetNumberOfVertices(); i++)
  {
    if (!strcmp(column, columnArr->GetValue(i)))
    {
      index = i;
      break;
    }
  }
  if (index >= 0)
  {
    domainArr->SetValue(index, domainStr);
    hiddenArr->SetValue(index, hidden);
    activeArr->SetValue(index, 1);
  }
  else
  {
    this->LinkGraph->AddVertex();
    columnArr->InsertNextValue(column);
    domainArr->InsertNextValue(domainStr);
    hiddenArr->InsertNextValue(hidden);
    activeArr->InsertNextValue(1);
  }
  this->Modified();
}

//---------------------------------------------------------------------------
void svtkTableToGraph::ClearLinkVertices()
{
  this->ValidateLinkGraph();
  svtkIntArray* activeArr =
    svtkArrayDownCast<svtkIntArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("active"));
  for (svtkIdType i = 0; i < this->LinkGraph->GetNumberOfVertices(); ++i)
  {
    activeArr->SetValue(i, 0);
  }
  this->Modified();
}

//---------------------------------------------------------------------------
void svtkTableToGraph::AddLinkEdge(const char* column1, const char* column2)
{
  if (!column1 || !column2)
  {
    svtkErrorMacro("Column names may not be null.");
  }
  this->ValidateLinkGraph();
  svtkStringArray* columnArr =
    svtkArrayDownCast<svtkStringArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("column"));
  svtkIdType source = -1;
  svtkIdType target = -1;
  for (svtkIdType i = 0; i < this->LinkGraph->GetNumberOfVertices(); i++)
  {
    if (!strcmp(column1, columnArr->GetValue(i)))
    {
      source = i;
    }
    if (!strcmp(column2, columnArr->GetValue(i)))
    {
      target = i;
    }
  }
  if (source < 0)
  {
    this->AddLinkVertex(column1);
    source = this->LinkGraph->GetNumberOfVertices() - 1;
  }
  if (target < 0)
  {
    this->AddLinkVertex(column2);
    target = this->LinkGraph->GetNumberOfVertices() - 1;
  }
  this->LinkGraph->AddEdge(source, target);
  this->Modified();
}

//---------------------------------------------------------------------------
void svtkTableToGraph::ClearLinkEdges()
{
  SVTK_CREATE(svtkMutableDirectedGraph, newLinkGraph);
  for (svtkIdType i = 0; i < this->LinkGraph->GetNumberOfVertices(); ++i)
  {
    newLinkGraph->AddVertex();
  }
  newLinkGraph->GetVertexData()->ShallowCopy(this->LinkGraph->GetVertexData());
  this->SetLinkGraph(newLinkGraph);
}

//---------------------------------------------------------------------------
void svtkTableToGraph::LinkColumnPath(
  svtkStringArray* column, svtkStringArray* domain, svtkBitArray* hidden)
{
  svtkMutableDirectedGraph* g = svtkMutableDirectedGraph::New();
  for (svtkIdType i = 0; i < column->GetNumberOfTuples(); i++)
  {
    g->AddVertex();
  }
  for (svtkIdType i = 1; i < column->GetNumberOfTuples(); i++)
  {
    g->AddEdge(i - 1, i);
  }
  column->SetName("column");
  g->GetVertexData()->AddArray(column);
  if (domain)
  {
    domain->SetName("domain");
    g->GetVertexData()->AddArray(domain);
  }
  if (hidden)
  {
    hidden->SetName("hidden");
    g->GetVertexData()->AddArray(hidden);
  }
  this->SetLinkGraph(g);
  g->Delete();
}

//---------------------------------------------------------------------------
int svtkTableToGraph::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
class svtkTableToGraphCompare
{
public:
  bool operator()(const std::pair<svtkStdString, svtkVariant>& a,
    const std::pair<svtkStdString, svtkVariant>& b) const
  {
    if (a.first != b.first)
    {
      return a.first < b.first;
    }
    return svtkVariantLessThan()(a.second, b.second);
  }
};

//---------------------------------------------------------------------------
template <typename T>
svtkVariant svtkTableToGraphGetValue(T* arr, svtkIdType index)
{
  return svtkVariant(arr[index]);
}

//---------------------------------------------------------------------------
template <typename T>
void svtkTableToGraphFindVertices(T* arr, // The raw edge table column
  svtkIdType size,                        // The size of the edge table column
  std::map<std::pair<svtkStdString, svtkVariant>, svtkIdType, svtkTableToGraphCompare>& vertexMap,
  // A map of domain-value pairs to graph id
  svtkStringArray* domainArr, // The domain of each vertex
  svtkStringArray* labelArr,  // The label of each vertex
  svtkVariantArray* idArr,    // The pedigree id of each vertex
  svtkIdType& curVertex,      // The current vertex id
  svtkTable* vertexTable,     // An array that holds the actual value of each vertex
  svtkStdString domain)       // The domain of the array
{
  for (svtkIdType i = 0; i < size; i++)
  {
    T v = arr[i];
    svtkVariant val(v);
    std::pair<svtkStdString, svtkVariant> value(domain, val);
    if (vertexMap.count(value) == 0)
    {
      svtkIdType row = vertexTable->InsertNextBlankRow();
      vertexTable->SetValueByName(row, domain, val);
      vertexMap[value] = row;
      domainArr->InsertNextValue(domain);
      labelArr->InsertNextValue(val.ToString());
      idArr->InsertNextValue(val);
      curVertex = row;
    }
  }
}

//---------------------------------------------------------------------------
template <typename T>
void svtkTableToGraphFindHiddenVertices(T* arr, // The raw edge table column
  svtkIdType size,                              // The size of the edge table column
  std::map<std::pair<svtkStdString, svtkVariant>, svtkIdType, svtkTableToGraphCompare>& hiddenMap,
  // A map of domain-value pairs to hidden vertex id
  svtkIdType& curHiddenVertex, // The current hidden vertex id
  svtkStdString domain)        // The domain of the array
{
  for (svtkIdType i = 0; i < size; i++)
  {
    T v = arr[i];
    svtkVariant val(v);
    std::pair<svtkStdString, svtkVariant> value(domain, val);
    if (hiddenMap.count(value) == 0)
    {
      hiddenMap[value] = curHiddenVertex;
      ++curHiddenVertex;
    }
  }
}

//---------------------------------------------------------------------------
int svtkTableToGraph::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Check that the link graph is valid
  if (!this->ValidateLinkGraph())
  {
    return 0;
  }

  // Extract edge table
  svtkInformation* edgeTableInfo = inputVector[0]->GetInformationObject(0);
  svtkTable* edgeTable = svtkTable::SafeDownCast(edgeTableInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Extract vertex table
  svtkInformation* vertexTableInfo = inputVector[1]->GetInformationObject(0);
  svtkTable* vertexTable = nullptr;
  if (vertexTableInfo)
  {
    vertexTable = svtkTable::SafeDownCast(vertexTableInfo->Get(svtkDataObject::DATA_OBJECT()));
  }

  if (svtkArrayDownCast<svtkIntArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("active")))
  {
    // Extract only the active link graph.
    svtkSelection* activeSel = svtkSelection::New();
    svtkSelectionNode* activeSelNode = svtkSelectionNode::New();
    activeSel->AddNode(activeSelNode);
    activeSelNode->SetContentType(svtkSelectionNode::VALUES);
    activeSelNode->SetFieldType(svtkSelectionNode::VERTEX);
    svtkIntArray* list = svtkIntArray::New();
    list->SetName("active");
    list->InsertNextValue(1);
    activeSelNode->SetSelectionList(list);
    svtkExtractSelectedGraph* extract = svtkExtractSelectedGraph::New();
    extract->SetInputData(0, this->LinkGraph);
    extract->SetInputData(1, activeSel);
    extract->Update();
    svtkGraph* g = extract->GetOutput();
    this->LinkGraph->ShallowCopy(g);
    list->Delete();
    activeSel->Delete();
    activeSelNode->Delete();
    extract->Delete();
  }

  svtkStringArray* linkColumn =
    svtkArrayDownCast<svtkStringArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("column"));

  if (!linkColumn)
  {
    svtkErrorMacro("The link graph must have a string array named \"column\".");
    return 0;
  }

  svtkStringArray* linkDomain =
    svtkArrayDownCast<svtkStringArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("domain"));
  svtkBitArray* linkHidden =
    svtkArrayDownCast<svtkBitArray>(this->LinkGraph->GetVertexData()->GetAbstractArray("hidden"));

  // Find all the hidden types
  std::set<svtkStdString> hiddenTypes;
  if (linkHidden)
  {
    for (svtkIdType h = 0; h < linkHidden->GetNumberOfTuples(); h++)
    {
      svtkStdString type;
      if (linkDomain)
      {
        type = linkDomain->GetValue(h);
      }
      if (linkHidden->GetValue(h))
      {
        hiddenTypes.insert(type);
      }
    }
  }

  // Calculate the percent time based on whether there are hidden types
  double createVertexTime = 0.25;
  double createEdgeTime = 0.75;
  double hiddenTime = 0.0;
  if (!hiddenTypes.empty())
  {
    createVertexTime = 0.1;
    createEdgeTime = 0.3;
    hiddenTime = 0.6;
  }

  // Create the auxiliary arrays.  These arrays summarize the
  // meaning of each row in the vertex table.
  // domainArr contains the domain string of the vertex.
  // labelArr contains the string value of each vertex (appropriate for labeling).
  // idArr contains the raw value of the vertex as a variant.
  SVTK_CREATE(svtkStringArray, domainArr);
  domainArr->SetName("domain");
  SVTK_CREATE(svtkStringArray, labelArr);
  labelArr->SetName("label");
  SVTK_CREATE(svtkVariantArray, idArr);
  idArr->SetName("ids");

  // Create the lookup maps for vertices and hidden vertices.
  // When edges are added later, we need to be able to lookup the
  // vertex ID for any domain-value pair.
  std::map<std::pair<svtkStdString, svtkVariant>, svtkIdType, svtkTableToGraphCompare> vertexMap;
  std::map<std::pair<svtkStdString, svtkVariant>, svtkIdType, svtkTableToGraphCompare> hiddenMap;

  // Set up the vertex table.  If we have one, just populate the
  // auxiliary arrays and vertex maps.
  // If we are not provided one, create one using values found in
  // the edge table.
  if (!vertexTable)
  {
    // If we don't have a vertex table,
    // create one by going through edge table columns.
    vertexTable = svtkTable::New();
    svtkIdType curVertex = 0;
    svtkIdType curHiddenVertex = 0;
    for (svtkIdType c = 0; c < linkColumn->GetNumberOfTuples(); c++)
    {
      svtkStdString domain = "globaldomain";
      if (linkDomain)
      {
        domain = linkDomain->GetValue(c);
      }
      int hidden = 0;
      if (linkHidden)
      {
        hidden = linkHidden->GetValue(c);
      }
      svtkStdString column = linkColumn->GetValue(c);
      svtkAbstractArray* arr = edgeTable->GetColumnByName(column);
      if (!arr)
      {
        svtkErrorMacro("svtkTableToGraph cannot find edge array: " << column.c_str());
        vertexTable->Delete();
        return 0;
      }
      // For each new domain, add an array for that domain
      // containing the values for only that domain.
      svtkAbstractArray* domainValuesArr = vertexTable->GetColumnByName(domain);
      if (!domainValuesArr && !hidden)
      {
        domainValuesArr = svtkAbstractArray::CreateArray(arr->GetDataType());
        domainValuesArr->SetName(domain);
        domainValuesArr->SetNumberOfTuples(vertexTable->GetNumberOfRows());
        vertexTable->AddColumn(domainValuesArr);
        domainValuesArr->Delete();
        for (svtkIdType r = 0; r < vertexTable->GetNumberOfRows(); ++r)
        {
          if (svtkArrayDownCast<svtkStringArray>(domainValuesArr))
          {
            vertexTable->SetValueByName(r, domain, "");
          }
          else
          {
            vertexTable->SetValueByName(r, domain, 0);
          }
        }
      }
      if (hidden)
      {
        // If these vertices will be hidden, add vertices to the hiddenMap
        // but don't update the vertex table.
        switch (arr->GetDataType())
        {
          svtkSuperExtraExtendedTemplateMacro(
            svtkTableToGraphFindHiddenVertices(static_cast<SVTK_TT*>(arr->GetVoidPointer(0)),
              arr->GetNumberOfTuples(), hiddenMap, curHiddenVertex, domain));
        }
      }
      else
      {
        // If the vertices are not hidden, add vertices to the vertexMap,
        // auxiliary arrays, and add rows to the vertex table.
        switch (arr->GetDataType())
        {
          svtkSuperExtraExtendedTemplateMacro(svtkTableToGraphFindVertices(
            static_cast<SVTK_TT*>(arr->GetVoidPointer(0)), arr->GetNumberOfTuples(), vertexMap,
            domainArr, labelArr, idArr, curVertex, vertexTable, domain));
        }
      }
      double progress = createVertexTime * ((c + 1.0) / linkColumn->GetNumberOfTuples());
      this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
    }
  }
  else
  {
    // The domain is what links the edge and vertex table,
    // so error if we don't have it.
    if (!linkDomain)
    {
      svtkErrorMacro("Domain is required when you have a vertex table");
      return 0;
    }

    // We know the number of vertices, so set the auxiliary array sizes.
    svtkIdType numRows = vertexTable->GetNumberOfRows();
    domainArr->SetNumberOfTuples(numRows);
    labelArr->SetNumberOfTuples(numRows);
    idArr->SetNumberOfTuples(numRows);

    // Keep track of the current hidden vertex id.
    svtkIdType curHiddenVertex = 0;

    // For each new domain encountered, iterate through the values
    // of that column, adding vertices for each new value encountered.
    std::set<svtkStdString> domainSet;
    for (svtkIdType c = 0; c < linkDomain->GetNumberOfTuples(); c++)
    {
      svtkStdString domain = linkDomain->GetValue(c);
      if (domainSet.count(domain) > 0)
      {
        continue;
      }
      domainSet.insert(domain);
      int hidden = 0;
      if (linkHidden)
      {
        hidden = linkHidden->GetValue(c);
      }

      if (!hidden)
      {
        // If the domain is not hidden, find unique values in the vertex table
        // column.  If there are multiple matches in the column, only the
        // first vertex with that value will be used.
        svtkAbstractArray* arr = vertexTable->GetColumnByName(domain);
        if (!arr)
        {
          svtkErrorMacro("svtkTableToGraph cannot find vertex array: " << domain.c_str());
          return 0;
        }
        for (svtkIdType i = 0; i < arr->GetNumberOfTuples(); ++i)
        {
          svtkVariant val = vertexTable->GetValueByName(i, domain);
          std::pair<svtkStdString, svtkVariant> value(domain, val);
          // Fancy check for whether we have a valid value.
          // 1. It must not exist yet in the vertex map.
          // 2. The variant value must be valid.
          //    This allows invalid variants to indicate null entries.
          // 3. It's string equivalent must be at least 1 character long.
          //    This is to allow the empty string to indicate null entries.
          // 4. If it is numeric, it's value must be at least 0.
          //    This is to allow a negative value to indicate null entries.
          if (vertexMap.count(value) == 0 && val.IsValid() && val.ToString().length() > 0 &&
            (!val.IsNumeric() || val.ToDouble() >= 0.0))
          {
            vertexMap[value] = i;
            domainArr->InsertValue(i, domain);
            labelArr->InsertValue(i, val.ToString());
            idArr->InsertValue(i, val);
          }
        }
      }
      else
      {
        // If the domain is hidden, we look through the edge table to
        // find new hidden vertices which will not be correllated to the
        // vertex table.
        svtkStdString column = linkColumn->GetValue(c);
        svtkAbstractArray* edgeArr = edgeTable->GetColumnByName(column);
        if (!edgeArr)
        {
          svtkErrorMacro("svtkTableToGraph cannot find edge array: " << column.c_str());
          return 0;
        }
        switch (edgeArr->GetDataType())
        {
          svtkSuperExtraExtendedTemplateMacro(
            svtkTableToGraphFindHiddenVertices(static_cast<SVTK_TT*>(edgeArr->GetVoidPointer(0)),
              edgeArr->GetNumberOfTuples(), hiddenMap, curHiddenVertex, domain));
        } // end switch
      }   // end else !hidden
      double progress = createVertexTime * ((c + 1.0) / linkDomain->GetNumberOfTuples());
      this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
    } // end for each domain
  }   // end else !vertexTable

  // Create builder for the graph
  SVTK_CREATE(svtkMutableDirectedGraph, dirBuilder);
  SVTK_CREATE(svtkMutableUndirectedGraph, undirBuilder);
  svtkGraph* builder = nullptr;
  if (this->Directed)
  {
    builder = dirBuilder;
  }
  else
  {
    builder = undirBuilder;
  }

  // Add the correct number of vertices to the graph based on the number of
  // rows in the vertex table.
  builder->GetVertexData()->PassData(vertexTable->GetRowData());
  for (svtkIdType i = 0; i < vertexTable->GetNumberOfRows(); ++i)
  {
    if (this->Directed)
    {
      dirBuilder->AddVertex();
    }
    else
    {
      undirBuilder->AddVertex();
    }
  }

  // Add the auxiliary arrays to the vertex table.
  builder->GetVertexData()->AddArray(labelArr);
  builder->GetVertexData()->AddArray(domainArr);

  // Check if the vertex table already has pedigree ids.
  // If it does we're not going to add the generated
  // array.
  if (vertexTable->GetRowData()->GetPedigreeIds() == nullptr)
  {
    builder->GetVertexData()->SetPedigreeIds(idArr);
  }
  else
  {
    builder->GetVertexData()->SetPedigreeIds(vertexTable->GetRowData()->GetPedigreeIds());
  }

  // Now go through the edge table, adding edges.
  // For each row in the edge table, add one edge to the
  // output graph for each edge in the link graph.
  SVTK_CREATE(svtkDataSetAttributes, edgeTableData);
  edgeTableData->ShallowCopy(edgeTable->GetRowData());
  builder->GetEdgeData()->CopyAllocate(edgeTableData);
  std::map<svtkIdType, std::vector<std::pair<svtkIdType, svtkIdType> > > hiddenInEdges;
  std::map<svtkIdType, std::vector<svtkIdType> > hiddenOutEdges;
  int numHiddenToHiddenEdges = 0;
  SVTK_CREATE(svtkEdgeListIterator, edges);
  for (svtkIdType r = 0; r < edgeTable->GetNumberOfRows(); r++)
  {
    this->LinkGraph->GetEdges(edges);
    while (edges->HasNext())
    {
      svtkEdgeType e = edges->Next();
      svtkIdType linkSource = e.Source;
      svtkIdType linkTarget = e.Target;
      svtkStdString columnNameSource = linkColumn->GetValue(linkSource);
      svtkStdString columnNameTarget = linkColumn->GetValue(linkTarget);
      svtkStdString typeSource;
      svtkStdString typeTarget;
      if (linkDomain)
      {
        typeSource = linkDomain->GetValue(linkSource);
        typeTarget = linkDomain->GetValue(linkTarget);
      }
      int hiddenSource = 0;
      int hiddenTarget = 0;
      if (linkHidden)
      {
        hiddenSource = linkHidden->GetValue(linkSource);
        hiddenTarget = linkHidden->GetValue(linkTarget);
      }
      svtkAbstractArray* columnSource = edgeTable->GetColumnByName(columnNameSource);
      svtkAbstractArray* columnTarget = edgeTable->GetColumnByName(columnNameTarget);
      svtkVariant valueSource;
      if (!columnSource)
      {
        svtkErrorMacro("svtkTableToGraph cannot find array: " << columnNameSource.c_str());
        return 0;
      }
      switch (columnSource->GetDataType())
      {
        svtkSuperExtraExtendedTemplateMacro(
          valueSource =
            svtkTableToGraphGetValue(static_cast<SVTK_TT*>(columnSource->GetVoidPointer(0)), r));
      }
      svtkVariant valueTarget;
      if (!columnTarget)
      {
        svtkErrorMacro("svtkTableToGraph cannot find array: " << columnNameTarget.c_str());
        return 0;
      }
      switch (columnTarget->GetDataType())
      {
        svtkSuperExtraExtendedTemplateMacro(
          valueTarget =
            svtkTableToGraphGetValue(static_cast<SVTK_TT*>(columnTarget->GetVoidPointer(0)), r));
      }
      std::pair<svtkStdString, svtkVariant> lookupSource(typeSource, svtkVariant(valueSource));
      std::pair<svtkStdString, svtkVariant> lookupTarget(typeTarget, svtkVariant(valueTarget));
      svtkIdType source = -1;
      svtkIdType target = -1;
      if (!hiddenSource && vertexMap.count(lookupSource) > 0)
      {
        source = vertexMap[lookupSource];
      }
      else if (hiddenSource && hiddenMap.count(lookupSource) > 0)
      {
        source = hiddenMap[lookupSource];
      }
      if (!hiddenTarget && vertexMap.count(lookupTarget) > 0)
      {
        target = vertexMap[lookupTarget];
      }
      else if (hiddenTarget && hiddenMap.count(lookupTarget) > 0)
      {
        target = hiddenMap[lookupTarget];
      }

      if (!hiddenSource && !hiddenTarget)
      {
        if (source >= 0 && target >= 0)
        {
          svtkEdgeType newEdge;
          if (this->Directed)
          {
            newEdge = dirBuilder->AddEdge(source, target);
          }
          else
          {
            newEdge = undirBuilder->AddEdge(source, target);
          }
          builder->GetEdgeData()->CopyData(edgeTableData, r, newEdge.Id);
        }
      }
      else if (hiddenSource && !hiddenTarget)
      {
        hiddenOutEdges[source].push_back(target);
      }
      else if (!hiddenSource && hiddenTarget)
      {
        hiddenInEdges[target].push_back(std::make_pair(source, r));
      }
      else
      {
        // Cannot currently handle edges between hidden vertices.
        ++numHiddenToHiddenEdges;
      }
    }
    if (r % 100 == 0)
    {
      double progress = createVertexTime + createEdgeTime * r / edgeTable->GetNumberOfRows();
      this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
    }
  }
  if (numHiddenToHiddenEdges > 0)
  {
    svtkWarningMacro(<< "TableToGraph does not currently support edges between hidden vertices.");
  }

  // Now add hidden edges.
  std::map<svtkIdType, std::vector<svtkIdType> >::iterator out, outEnd;
  out = hiddenOutEdges.begin();
  outEnd = hiddenOutEdges.end();
  svtkIdType curHidden = 0;
  svtkIdType numHidden = static_cast<svtkIdType>(hiddenOutEdges.size());
  for (; out != outEnd; ++out)
  {
    std::vector<svtkIdType> outVerts = out->second;
    std::vector<std::pair<svtkIdType, svtkIdType> > inVerts = hiddenInEdges[out->first];
    std::vector<svtkIdType>::size_type i, j;
    for (i = 0; i < inVerts.size(); ++i)
    {
      svtkIdType inVertId = inVerts[i].first;
      svtkIdType inEdgeId = inVerts[i].second;
      for (j = 0; j < outVerts.size(); ++j)
      {
        svtkEdgeType newEdge;
        if (this->Directed)
        {
          newEdge = dirBuilder->AddEdge(inVertId, outVerts[j]);
        }
        else
        {
          newEdge = undirBuilder->AddEdge(inVertId, outVerts[j]);
        }
        builder->GetEdgeData()->CopyData(edgeTableData, inEdgeId, newEdge.Id);
      }
    }
    if (curHidden % 100 == 0)
    {
      double progress = createVertexTime + createEdgeTime + hiddenTime * curHidden / numHidden;
      this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
    }
    ++curHidden;
  }

  // Check if pedigree ids are in the input edge data
  if (edgeTable->GetRowData()->GetPedigreeIds() == nullptr)
  {
    // Add pedigree ids to the edges of the graph.
    svtkIdType numEdges = builder->GetNumberOfEdges();
    SVTK_CREATE(svtkIdTypeArray, edgeIds);
    edgeIds->SetNumberOfTuples(numEdges);
    edgeIds->SetName("edge");
    for (svtkIdType i = 0; i < numEdges; ++i)
    {
      edgeIds->SetValue(i, i);
    }
    builder->GetEdgeData()->SetPedigreeIds(edgeIds);
  }
  else
  {
    builder->GetEdgeData()->SetPedigreeIds(edgeTable->GetRowData()->GetPedigreeIds());
  }

  // Copy structure into output graph.
  svtkInformation* outputInfo = outputVector->GetInformationObject(0);
  svtkGraph* output = svtkGraph::SafeDownCast(outputInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!output->CheckedShallowCopy(builder))
  {
    svtkErrorMacro(<< "Invalid graph structure");
    return 0;
  }

  // Clean up
  if (!vertexTableInfo)
  {
    vertexTable->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkTableToGraph::RequestDataObject(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  svtkGraph* output = nullptr;
  if (this->Directed)
  {
    output = svtkDirectedGraph::New();
  }
  else
  {
    output = svtkUndirectedGraph::New();
  }
  this->GetExecutive()->SetOutputData(0, output);
  output->Delete();

  return 1;
}

//---------------------------------------------------------------------------
svtkMTimeType svtkTableToGraph::GetMTime()
{
  svtkMTimeType time = this->Superclass::GetMTime();
  svtkMTimeType linkGraphTime = this->LinkGraph->GetMTime();
  time = (linkGraphTime > time ? linkGraphTime : time);
  return time;
}

//---------------------------------------------------------------------------
void svtkTableToGraph::SetVertexTableConnection(svtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

//---------------------------------------------------------------------------
void svtkTableToGraph::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "LinkGraph: " << (this->LinkGraph ? "" : "(null)") << endl;
  if (this->LinkGraph)
  {
    this->LinkGraph->PrintSelf(os, indent.GetNextIndent());
  }
}
