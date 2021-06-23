/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConvertSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkConvertSelection.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSortDataArray.h"
#include "svtkStringArray.h"
#include "svtkVariant.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include <map>

template <typename T>
int CompareArrays(T* a, T* b, svtkIdType n)
{
  int errors = 0;
  for (svtkIdType i = 0; i < n; i++)
  {
    if (a[i] != b[i])
    {
      cerr << "ERROR: Arrays do not match at index " << i << " (" << a[i] << "!=" << b[i] << ")"
           << endl;
      errors++;
    }
  }
  return errors;
}

const char* SelectionTypeToString(int type)
{
  switch (type)
  {
    case svtkSelectionNode::SELECTIONS:
      return "Selections";
    case svtkSelectionNode::GLOBALIDS:
      return "Global IDs";
    case svtkSelectionNode::PEDIGREEIDS:
      return "Pedigree IDs";
    case svtkSelectionNode::VALUES:
      return "Values";
    case svtkSelectionNode::INDICES:
      return "Indices";
    case svtkSelectionNode::FRUSTUM:
      return "Frustum";
    case svtkSelectionNode::THRESHOLDS:
      return "Thresholds";
    case svtkSelectionNode::LOCATIONS:
      return "Locations";
    default:
      return "Unknown";
  }
}

int CompareSelections(svtkSelectionNode* a, svtkSelectionNode* b)
{
  int errors = 0;
  if (!a || !b)
  {
    cerr << "ERROR: Empty Selection Node(s)" << endl;
    errors++;
    return errors;
  }
  if (a->GetContentType() != b->GetContentType())
  {
    cerr << "ERROR: Content type " << SelectionTypeToString(a->GetContentType())
         << " does not match " << SelectionTypeToString(b->GetContentType()) << endl;
    errors++;
  }
  if (a->GetFieldType() != b->GetFieldType())
  {
    cerr << "ERROR: Field type " << a->GetFieldType() << " does not match " << b->GetFieldType()
         << endl;
    errors++;
  }
  svtkAbstractArray* arra = a->GetSelectionList();
  svtkAbstractArray* arrb = b->GetSelectionList();
  if (arra->GetName() && !arrb->GetName())
  {
    cerr << "ERROR: Array name a is not null but b is" << endl;
    errors++;
  }
  else if (!arra->GetName() && arrb->GetName())
  {
    cerr << "ERROR: Array name a is null but b is not" << endl;
    errors++;
  }
  else if (arra->GetName() && strcmp(arra->GetName(), arrb->GetName()))
  {
    cerr << "ERROR: Array name " << arra->GetName() << " does not match " << arrb->GetName()
         << endl;
    errors++;
  }
  if (arra->GetDataType() != arrb->GetDataType())
  {
    cerr << "ERROR: Array type " << arra->GetDataType() << " does not match " << arrb->GetDataType()
         << endl;
    errors++;
  }
  else if (arra->GetNumberOfTuples() != arrb->GetNumberOfTuples())
  {
    cerr << "ERROR: Array tuples " << arra->GetNumberOfTuples() << " does not match "
         << arrb->GetNumberOfTuples() << endl;
    errors++;
  }
  else
  {
    svtkSortDataArray::Sort(arra);
    svtkSortDataArray::Sort(arrb);
    switch (arra->GetDataType())
    {
      svtkExtendedTemplateMacro(errors += CompareArrays((SVTK_TT*)arra->GetVoidPointer(0),
                                 (SVTK_TT*)arrb->GetVoidPointer(0), arra->GetNumberOfTuples()));
    }
  }
  return errors;
}

int TestConvertSelectionType(std::map<int, svtkSmartPointer<svtkSelection> >& selMap,
  svtkDataObject* data, int inputType, int outputType, svtkStringArray* arr = nullptr,
  bool allowMissingArray = false)
{
  cerr << "Testing conversion from type " << SelectionTypeToString(inputType) << " to "
       << SelectionTypeToString(outputType) << "..." << endl;
  svtkSelection* s = svtkConvertSelection::ToSelectionType(
    selMap[inputType], data, outputType, arr, -1, allowMissingArray);
  int errors = 0;
  if (!allowMissingArray)
  {
    errors = CompareSelections(selMap[outputType]->GetNode(0), s->GetNode(0));
  }
  s->Delete();
  cerr << "...done." << endl;
  return errors;
}

void GraphConvertSelections(int& errors, int size)
{
  // Create the test data
  SVTK_CREATE(svtkMutableUndirectedGraph, g);
  SVTK_CREATE(svtkIdTypeArray, pedIdVertArr);
  pedIdVertArr->SetName("PedId");
  g->GetVertexData()->AddArray(pedIdVertArr);
  g->GetVertexData()->SetPedigreeIds(pedIdVertArr);
  SVTK_CREATE(svtkIdTypeArray, globalIdVertArr);
  globalIdVertArr->SetName("GlobalId");
  g->GetVertexData()->AddArray(globalIdVertArr);
  g->GetVertexData()->SetGlobalIds(globalIdVertArr);
  SVTK_CREATE(svtkDoubleArray, doubleVertArr);
  doubleVertArr->SetName("Double");
  g->GetVertexData()->AddArray(doubleVertArr);
  SVTK_CREATE(svtkStringArray, stringVertArr);
  stringVertArr->SetName("String");
  g->GetVertexData()->AddArray(stringVertArr);
  SVTK_CREATE(svtkPoints, pts);
  for (int i = 0; i < size; i++)
  {
    g->AddVertex();
    doubleVertArr->InsertNextValue(i % 2);
    stringVertArr->InsertNextValue(svtkVariant(i).ToString());
    pedIdVertArr->InsertNextValue(i);
    globalIdVertArr->InsertNextValue(i);
    pts->InsertNextPoint(i, i % 2, 0);
  }
  g->SetPoints(pts);

  g->GetEdgeData()->AddArray(pedIdVertArr);
  g->GetEdgeData()->SetPedigreeIds(pedIdVertArr);
  g->GetEdgeData()->AddArray(globalIdVertArr);
  g->GetEdgeData()->SetGlobalIds(globalIdVertArr);
  g->GetEdgeData()->AddArray(doubleVertArr);
  g->GetEdgeData()->AddArray(stringVertArr);
  for (int i = 0; i < size; i++)
  {
    g->AddEdge(i, i);
  }

  std::map<int, svtkSmartPointer<svtkSelection> > selMap;

  SVTK_CREATE(svtkSelection, globalIdsSelection);
  SVTK_CREATE(svtkSelectionNode, globalIdsSelectionNode);
  globalIdsSelection->AddNode(globalIdsSelectionNode);
  globalIdsSelectionNode->SetContentType(svtkSelectionNode::GLOBALIDS);
  globalIdsSelectionNode->SetFieldType(svtkSelectionNode::VERTEX);
  SVTK_CREATE(svtkIdTypeArray, globalIdsArr);
  globalIdsArr->SetName("GlobalId");
  globalIdsSelectionNode->SetSelectionList(globalIdsArr);
  for (int i = 0; i < size; i += 2)
  {
    globalIdsArr->InsertNextValue(i);
  }
  selMap[svtkSelectionNode::GLOBALIDS] = globalIdsSelection;

  SVTK_CREATE(svtkSelection, pedigreeIdsSelection);
  SVTK_CREATE(svtkSelectionNode, pedigreeIdsSelectionNode);
  pedigreeIdsSelection->AddNode(pedigreeIdsSelectionNode);
  pedigreeIdsSelectionNode->SetContentType(svtkSelectionNode::PEDIGREEIDS);
  pedigreeIdsSelectionNode->SetFieldType(svtkSelectionNode::VERTEX);
  SVTK_CREATE(svtkIdTypeArray, pedigreeIdsArr);
  pedigreeIdsArr->SetName("PedId");
  pedigreeIdsSelectionNode->SetSelectionList(pedigreeIdsArr);
  for (int i = 0; i < size; i += 2)
  {
    pedigreeIdsArr->InsertNextValue(i);
  }
  selMap[svtkSelectionNode::PEDIGREEIDS] = pedigreeIdsSelection;

  SVTK_CREATE(svtkSelection, valuesSelection);
  SVTK_CREATE(svtkSelectionNode, valuesSelectionNode);
  valuesSelection->AddNode(valuesSelectionNode);
  valuesSelectionNode->SetContentType(svtkSelectionNode::VALUES);
  valuesSelectionNode->SetFieldType(svtkSelectionNode::VERTEX);
  SVTK_CREATE(svtkStringArray, valuesArr);
  valuesArr->SetName("String");
  valuesSelectionNode->SetSelectionList(valuesArr);
  for (int i = 0; i < size; i += 2)
  {
    valuesArr->InsertNextValue(svtkVariant(i).ToString());
  }
  selMap[svtkSelectionNode::VALUES] = valuesSelection;

  SVTK_CREATE(svtkSelection, indicesSelection);
  SVTK_CREATE(svtkSelectionNode, indicesSelectionNode);
  indicesSelection->AddNode(indicesSelectionNode);
  indicesSelectionNode->SetContentType(svtkSelectionNode::INDICES);
  indicesSelectionNode->SetFieldType(svtkSelectionNode::VERTEX);
  SVTK_CREATE(svtkIdTypeArray, indicesArr);
  indicesSelectionNode->SetSelectionList(indicesArr);
  for (int i = 0; i < size; i += 2)
  {
    indicesArr->InsertNextValue(i);
  }
  selMap[svtkSelectionNode::INDICES] = indicesSelection;

  SVTK_CREATE(svtkSelection, frustumSelection);
  SVTK_CREATE(svtkSelectionNode, frustumSelectionNode);
  frustumSelection->AddNode(frustumSelectionNode);
  frustumSelectionNode->SetContentType(svtkSelectionNode::FRUSTUM);
  frustumSelectionNode->SetFieldType(svtkSelectionNode::VERTEX);
  // near lower left,
  // far lower left,
  // near upper left,
  // far upper left,
  // near lower right,
  // far lower right,
  // near upper right,
  // far upper right,
  double corners[] = { -1.0, -0.5, 1.0, 1.0, -1.0, -0.5, -1.0, 1.0, -1.0, 0.5, 1.0, 1.0, -1.0, 0.5,
    -1.0, 1.0, static_cast<double>(size), -0.5, 1.0, 1.0, static_cast<double>(size), -0.5, -1.0,
    1.0, static_cast<double>(size), 0.5, 1.0, 1.0, static_cast<double>(size), 0.5, -1.0, 1.0 };
  SVTK_CREATE(svtkDoubleArray, frustumArr);
  for (svtkIdType i = 0; i < 32; i++)
  {
    frustumArr->InsertNextValue(corners[i]);
  }
  frustumSelectionNode->SetSelectionList(frustumArr);
  selMap[svtkSelectionNode::FRUSTUM] = frustumSelection;

  SVTK_CREATE(svtkSelection, locationsSelection);
  SVTK_CREATE(svtkSelectionNode, locationsSelectionNode);
  locationsSelection->AddNode(locationsSelectionNode);
  locationsSelectionNode->SetContentType(svtkSelectionNode::INDICES);
  locationsSelectionNode->SetFieldType(svtkSelectionNode::VERTEX);
  SVTK_CREATE(svtkFloatArray, locationsArr);
  locationsArr->SetNumberOfComponents(3);
  locationsSelectionNode->SetSelectionList(locationsArr);
  for (int i = 0; i < size; i += 2)
  {
    locationsArr->InsertNextTuple3(i, 0, 0);
  }
  selMap[svtkSelectionNode::LOCATIONS] = locationsSelection;

  SVTK_CREATE(svtkSelection, thresholdsSelection);
  SVTK_CREATE(svtkSelectionNode, thresholdsSelectionNode);
  thresholdsSelection->AddNode(thresholdsSelectionNode);
  thresholdsSelectionNode->SetContentType(svtkSelectionNode::THRESHOLDS);
  thresholdsSelectionNode->SetFieldType(svtkSelectionNode::VERTEX);
  SVTK_CREATE(svtkDoubleArray, thresholdsArr);
  thresholdsArr->SetName("Double");
  thresholdsArr->InsertNextValue(-0.5);
  thresholdsArr->InsertNextValue(0.5);
  thresholdsSelectionNode->SetSelectionList(thresholdsArr);
  selMap[svtkSelectionNode::THRESHOLDS] = thresholdsSelection;

  SVTK_CREATE(svtkStringArray, arrNames);
  arrNames->InsertNextValue("String");

  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::PEDIGREEIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::VALUES, arrNames);

  //
  // Test cell selections
  //

  selMap[svtkSelectionNode::GLOBALIDS]->GetNode(0)->SetFieldType(svtkSelectionNode::EDGE);
  selMap[svtkSelectionNode::PEDIGREEIDS]->GetNode(0)->SetFieldType(svtkSelectionNode::EDGE);
  selMap[svtkSelectionNode::VALUES]->GetNode(0)->SetFieldType(svtkSelectionNode::EDGE);
  selMap[svtkSelectionNode::INDICES]->GetNode(0)->SetFieldType(svtkSelectionNode::EDGE);
  selMap[svtkSelectionNode::THRESHOLDS]->GetNode(0)->SetFieldType(svtkSelectionNode::EDGE);
  selMap[svtkSelectionNode::FRUSTUM]->GetNode(0)->SetFieldType(svtkSelectionNode::EDGE);
  selMap[svtkSelectionNode::LOCATIONS]->GetNode(0)->SetFieldType(svtkSelectionNode::EDGE);

  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::PEDIGREEIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::VALUES, arrNames);
}

void PolyDataConvertSelections(int& errors, int size)
{
  // Create the test data
  SVTK_CREATE(svtkPolyData, g);
  SVTK_CREATE(svtkIdTypeArray, pedIdVertArr);
  pedIdVertArr->SetName("PedId");
  g->GetPointData()->AddArray(pedIdVertArr);
  g->GetPointData()->SetPedigreeIds(pedIdVertArr);
  SVTK_CREATE(svtkIdTypeArray, globalIdVertArr);
  globalIdVertArr->SetName("GlobalId");
  g->GetPointData()->AddArray(globalIdVertArr);
  g->GetPointData()->SetGlobalIds(globalIdVertArr);
  SVTK_CREATE(svtkDoubleArray, doubleVertArr);
  doubleVertArr->SetName("Double");
  g->GetPointData()->AddArray(doubleVertArr);
  SVTK_CREATE(svtkStringArray, stringVertArr);
  stringVertArr->SetName("String");
  g->GetPointData()->AddArray(stringVertArr);
  SVTK_CREATE(svtkPoints, pts);
  for (int i = 0; i < size; i++)
  {
    doubleVertArr->InsertNextValue(i % 2);
    stringVertArr->InsertNextValue(svtkVariant(i).ToString());
    pedIdVertArr->InsertNextValue(i);
    globalIdVertArr->InsertNextValue(i);
    pts->InsertNextPoint(i, i % 2, 0);
  }
  g->SetPoints(pts);

  g->GetCellData()->AddArray(pedIdVertArr);
  g->GetCellData()->SetPedigreeIds(pedIdVertArr);
  g->GetCellData()->AddArray(globalIdVertArr);
  g->GetCellData()->SetGlobalIds(globalIdVertArr);
  g->GetCellData()->AddArray(doubleVertArr);
  g->GetCellData()->AddArray(stringVertArr);

  SVTK_CREATE(svtkCellArray, newLines);
  newLines->AllocateEstimate(size, 2);
  svtkIdType cellPts[2];
  for (int i = 0; i < size; i++)
  {
    cellPts[0] = i;
    cellPts[1] = i;
    newLines->InsertNextCell(2, cellPts);
  }
  g->SetLines(newLines);

  std::map<int, svtkSmartPointer<svtkSelection> > selMap;

  SVTK_CREATE(svtkSelection, globalIdsSelection);
  SVTK_CREATE(svtkSelectionNode, globalIdsSelectionNode);
  globalIdsSelection->AddNode(globalIdsSelectionNode);
  globalIdsSelectionNode->SetContentType(svtkSelectionNode::GLOBALIDS);
  globalIdsSelectionNode->SetFieldType(svtkSelectionNode::POINT);
  SVTK_CREATE(svtkIdTypeArray, globalIdsArr);
  globalIdsArr->SetName("GlobalId");
  globalIdsSelectionNode->SetSelectionList(globalIdsArr);
  for (int i = 0; i < size; i += 2)
  {
    globalIdsArr->InsertNextValue(i);
  }
  selMap[svtkSelectionNode::GLOBALIDS] = globalIdsSelection;

  SVTK_CREATE(svtkSelection, pedigreeIdsSelection);
  SVTK_CREATE(svtkSelectionNode, pedigreeIdsSelectionNode);
  pedigreeIdsSelection->AddNode(pedigreeIdsSelectionNode);
  pedigreeIdsSelectionNode->SetContentType(svtkSelectionNode::PEDIGREEIDS);
  pedigreeIdsSelectionNode->SetFieldType(svtkSelectionNode::POINT);
  SVTK_CREATE(svtkIdTypeArray, pedigreeIdsArr);
  pedigreeIdsArr->SetName("PedId");
  pedigreeIdsSelectionNode->SetSelectionList(pedigreeIdsArr);
  for (int i = 0; i < size; i += 2)
  {
    pedigreeIdsArr->InsertNextValue(i);
  }
  selMap[svtkSelectionNode::PEDIGREEIDS] = pedigreeIdsSelection;

  SVTK_CREATE(svtkSelection, valuesSelection);
  SVTK_CREATE(svtkSelectionNode, valuesSelectionNode);
  valuesSelection->AddNode(valuesSelectionNode);
  valuesSelectionNode->SetContentType(svtkSelectionNode::VALUES);
  valuesSelectionNode->SetFieldType(svtkSelectionNode::POINT);
  SVTK_CREATE(svtkStringArray, valuesArr);
  valuesArr->SetName("String");
  valuesSelectionNode->SetSelectionList(valuesArr);
  for (int i = 0; i < size; i += 2)
  {
    valuesArr->InsertNextValue(svtkVariant(i).ToString());
  }
  selMap[svtkSelectionNode::VALUES] = valuesSelection;

  SVTK_CREATE(svtkSelection, indicesSelection);
  SVTK_CREATE(svtkSelectionNode, indicesSelectionNode);
  indicesSelection->AddNode(indicesSelectionNode);
  indicesSelectionNode->SetContentType(svtkSelectionNode::INDICES);
  indicesSelectionNode->SetFieldType(svtkSelectionNode::POINT);
  SVTK_CREATE(svtkIdTypeArray, indicesArr);
  indicesSelectionNode->SetSelectionList(indicesArr);
  for (int i = 0; i < size; i += 2)
  {
    indicesArr->InsertNextValue(i);
  }
  selMap[svtkSelectionNode::INDICES] = indicesSelection;

  SVTK_CREATE(svtkSelection, frustumSelection);
  SVTK_CREATE(svtkSelectionNode, frustumSelectionNode);
  frustumSelection->AddNode(frustumSelectionNode);
  frustumSelectionNode->SetContentType(svtkSelectionNode::FRUSTUM);
  frustumSelectionNode->SetFieldType(svtkSelectionNode::POINT);
  // near lower left, far lower left
  // near upper left, far upper left
  // near lower right, far lower right
  // near upper right, far upper right
  double corners[] = { -1.0, -0.5, 1.0, 1.0, -1.0, -0.5, -1.0, 1.0, -1.0, 0.5, 1.0, 1.0, -1.0, 0.5,
    -1.0, 1.0, static_cast<double>(size), -0.5, 1.0, 1.0, static_cast<double>(size), -0.5, -1.0,
    1.0, static_cast<double>(size), 0.5, 1.0, 1.0, static_cast<double>(size), 0.5, -1.0, 1.0 };
  SVTK_CREATE(svtkDoubleArray, frustumArr);
  for (svtkIdType i = 0; i < 32; i++)
  {
    frustumArr->InsertNextValue(corners[i]);
  }
  frustumSelectionNode->SetSelectionList(frustumArr);
  selMap[svtkSelectionNode::FRUSTUM] = frustumSelection;

  SVTK_CREATE(svtkSelection, locationsSelection);
  SVTK_CREATE(svtkSelectionNode, locationsSelectionNode);
  locationsSelection->AddNode(locationsSelectionNode);
  locationsSelectionNode->SetContentType(svtkSelectionNode::INDICES);
  locationsSelectionNode->SetFieldType(svtkSelectionNode::POINT);
  SVTK_CREATE(svtkFloatArray, locationsArr);
  locationsArr->SetNumberOfComponents(3);
  locationsSelectionNode->SetSelectionList(locationsArr);
  for (int i = 0; i < size; i += 2)
  {
    locationsArr->InsertNextTuple3(i, 0, 0);
  }
  selMap[svtkSelectionNode::LOCATIONS] = locationsSelection;

  SVTK_CREATE(svtkSelection, thresholdsSelection);
  SVTK_CREATE(svtkSelectionNode, thresholdsSelectionNode);
  thresholdsSelection->AddNode(thresholdsSelectionNode);
  thresholdsSelectionNode->SetContentType(svtkSelectionNode::THRESHOLDS);
  thresholdsSelectionNode->SetFieldType(svtkSelectionNode::POINT);
  SVTK_CREATE(svtkDoubleArray, thresholdsArr);
  thresholdsArr->SetName("Double");
  thresholdsArr->InsertNextValue(-0.5);
  thresholdsArr->InsertNextValue(0.5);
  thresholdsSelectionNode->SetSelectionList(thresholdsArr);
  selMap[svtkSelectionNode::THRESHOLDS] = thresholdsSelection;

  SVTK_CREATE(svtkStringArray, arrNames);
  arrNames->InsertNextValue("String");

  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::PEDIGREEIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::FRUSTUM, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::FRUSTUM, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::FRUSTUM, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::FRUSTUM, svtkSelectionNode::INDICES);
  // errors += TestConvertSelectionType(selMap, g, svtkSelectionNode::LOCATIONS,
  // svtkSelectionNode::GLOBALIDS); errors += TestConvertSelectionType(selMap, g,
  // svtkSelectionNode::LOCATIONS, svtkSelectionNode::PEDIGREEIDS); errors +=
  // TestConvertSelectionType(selMap, g, svtkSelectionNode::LOCATIONS, svtkSelectionNode::VALUES,
  // arrNames); errors += TestConvertSelectionType(selMap, g, svtkSelectionNode::LOCATIONS,
  // svtkSelectionNode::INDICES);

  // Test Quiet Error
  thresholdsArr->SetName("DoubleTmp");
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::GLOBALIDS, nullptr, true);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::PEDIGREEIDS, nullptr, true);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::VALUES, arrNames, true);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::INDICES, nullptr, true);
  thresholdsArr->SetName("Double");

  //
  // Test cell selections
  //

  selMap[svtkSelectionNode::GLOBALIDS]->GetNode(0)->SetFieldType(svtkSelectionNode::CELL);
  selMap[svtkSelectionNode::PEDIGREEIDS]->GetNode(0)->SetFieldType(svtkSelectionNode::CELL);
  selMap[svtkSelectionNode::VALUES]->GetNode(0)->SetFieldType(svtkSelectionNode::CELL);
  selMap[svtkSelectionNode::INDICES]->GetNode(0)->SetFieldType(svtkSelectionNode::CELL);
  selMap[svtkSelectionNode::THRESHOLDS]->GetNode(0)->SetFieldType(svtkSelectionNode::CELL);
  selMap[svtkSelectionNode::FRUSTUM]->GetNode(0)->SetFieldType(svtkSelectionNode::CELL);
  selMap[svtkSelectionNode::LOCATIONS]->GetNode(0)->SetFieldType(svtkSelectionNode::CELL);

  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::GLOBALIDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::PEDIGREEIDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::PEDIGREEIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::VALUES, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::INDICES, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::GLOBALIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::THRESHOLDS, svtkSelectionNode::INDICES);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::FRUSTUM, svtkSelectionNode::GLOBALIDS);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::FRUSTUM, svtkSelectionNode::PEDIGREEIDS);
  errors += TestConvertSelectionType(
    selMap, g, svtkSelectionNode::FRUSTUM, svtkSelectionNode::VALUES, arrNames);
  errors +=
    TestConvertSelectionType(selMap, g, svtkSelectionNode::FRUSTUM, svtkSelectionNode::INDICES);
  // errors += TestConvertSelectionType(selMap, g, svtkSelectionNode::LOCATIONS,
  // svtkSelectionNode::GLOBALIDS); errors += TestConvertSelectionType(selMap, g,
  // svtkSelectionNode::LOCATIONS, svtkSelectionNode::PEDIGREEIDS); errors +=
  // TestConvertSelectionType(selMap, g, svtkSelectionNode::LOCATIONS, svtkSelectionNode::VALUES,
  // arrNames); errors += TestConvertSelectionType(selMap, g, svtkSelectionNode::LOCATIONS,
  // svtkSelectionNode::INDICES);
}

int TestConvertSelection(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  int errors = 0;
  int size = 10;

  GraphConvertSelections(errors, size);
  PolyDataConvertSelections(errors, size);

  return errors;
}
