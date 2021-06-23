/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAssignAttribute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests svtkAssignAttribute.

#include "svtkAssignAttribute.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMutableUndirectedGraph.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#include <cstring>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestAssignAttribute(int, char*[])
{
  int errors = 0;

  SVTK_CREATE(svtkMutableUndirectedGraph, graph);
  SVTK_CREATE(svtkPolyData, poly);
  SVTK_CREATE(svtkPoints, pts);
  SVTK_CREATE(svtkCellArray, verts);

  SVTK_CREATE(svtkDoubleArray, scalars);
  scalars->SetName("scalars");
  scalars->SetNumberOfComponents(3);

  SVTK_CREATE(svtkDoubleArray, tensors);
  tensors->SetName(nullptr); // no name.
  tensors->SetNumberOfComponents(9);
  for (svtkIdType i = 0; i < 10; ++i)
  {
    pts->InsertNextPoint(i, 0, 0);
    verts->InsertNextCell(1, &i);
    graph->AddVertex();
    scalars->InsertNextTuple3(i, 0.5 * i, 0.1 * i);
    tensors->InsertNextTuple9(1., 0., 0., 0., 1., 0., 0., 0., 1.);
  }
  for (svtkIdType i = 0; i < 10; ++i)
  {
    graph->AddEdge(i, (i + 1) % 10);
  }
  graph->GetVertexData()->AddArray(scalars);
  graph->GetEdgeData()->AddArray(scalars);
  graph->GetVertexData()->SetTensors(tensors);
  graph->GetEdgeData()->SetTensors(tensors);

  poly->SetPoints(pts);
  poly->SetVerts(verts);
  poly->GetPointData()->AddArray(scalars);
  poly->GetCellData()->AddArray(scalars);
  poly->GetPointData()->SetTensors(tensors);
  poly->GetCellData()->SetTensors(tensors);

  SVTK_CREATE(svtkAssignAttribute, assign);

  assign->SetInputData(graph);
  assign->Assign("scalars", svtkDataSetAttributes::SCALARS, svtkAssignAttribute::VERTEX_DATA);
  assign->Update();
  svtkGraph* output = svtkGraph::SafeDownCast(assign->GetOutput());
  if (output->GetVertexData()->GetScalars() != scalars.GetPointer())
  {
    cerr << "Vertex scalars not set properly" << endl;
    ++errors;
  }
  assign->Assign("scalars", svtkDataSetAttributes::SCALARS, svtkAssignAttribute::EDGE_DATA);
  assign->Update();
  output = svtkGraph::SafeDownCast(assign->GetOutput());
  if (output->GetEdgeData()->GetScalars() != scalars.GetPointer())
  {
    cerr << "Edge scalars not set properly" << endl;
    ++errors;
  }

  assign->SetInputData(poly);
  assign->Assign("scalars", svtkDataSetAttributes::SCALARS, svtkAssignAttribute::POINT_DATA);
  assign->Update();
  svtkPolyData* outputPoly = svtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetPointData()->GetScalars() != scalars.GetPointer())
  {
    cerr << "Point scalars not set properly" << endl;
    ++errors;
  }
  assign->Assign("scalars", svtkDataSetAttributes::SCALARS, svtkAssignAttribute::CELL_DATA);
  assign->Update();
  outputPoly = svtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetCellData()->GetScalars() != scalars.GetPointer())
  {
    cerr << "Cell scalars not set properly" << endl;
    ++errors;
  }

  assign->Assign(
    svtkDataSetAttributes::TENSORS, svtkDataSetAttributes::SCALARS, svtkAssignAttribute::POINT_DATA);
  assign->Update();
  outputPoly = svtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetPointData()->GetTensors() != tensors.GetPointer())
  {
    cerr << "Point scalar not set when name is empty" << endl;
    ++errors;
  }
  assign->Assign(
    svtkDataSetAttributes::TENSORS, svtkDataSetAttributes::SCALARS, svtkAssignAttribute::CELL_DATA);
  assign->Update();
  outputPoly = svtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetCellData()->GetTensors() != tensors.GetPointer())
  {
    cerr << "Cell scalar not set when name is empty" << endl;
    ++errors;
  }
  svtkInformation* inInfo =
    assign->GetExecutive()->GetInputInformation()[0]->GetInformationObject(0);
  svtkInformation* outInfo = assign->GetExecutive()->GetOutputInformation()->GetInformationObject(0);
  outInfo->Clear();
  svtkDataObject::SetActiveAttribute(inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS,
    scalars->GetName(), svtkDataSetAttributes::SCALARS);
  svtkDataObject::SetActiveAttributeInfo(inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS,
    svtkDataSetAttributes::SCALARS, scalars->GetName(), scalars->GetDataType(),
    scalars->GetNumberOfComponents(), scalars->GetNumberOfTuples());
  assign->Assign(scalars->GetName(), svtkDataSetAttributes::VECTORS, svtkAssignAttribute::POINT_DATA);
  assign->UpdateInformation();
  svtkInformation* outFieldInfo = svtkDataObject::GetActiveFieldInformation(
    outInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);
  if (!outFieldInfo || !outFieldInfo->Has(svtkDataObject::FIELD_NAME()) ||
    std::strcmp(outFieldInfo->Get(svtkDataObject::FIELD_NAME()), scalars->GetName()) ||
    outFieldInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()) !=
      scalars->GetNumberOfComponents() ||
    outFieldInfo->Get(svtkDataObject::FIELD_NUMBER_OF_TUPLES()) != scalars->GetNumberOfTuples() ||
    outFieldInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE()) != scalars->GetDataType())
  {
    cerr << "Scalar information not passed when attribute is assigned by name." << endl;
    ++errors;
  }
  outInfo->Clear();
  inInfo = assign->GetExecutive()->GetInputInformation()[0]->GetInformationObject(0);
  svtkDataObject::SetActiveAttribute(inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS,
    scalars->GetName(), svtkDataSetAttributes::SCALARS);
  svtkDataObject::SetActiveAttributeInfo(inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS,
    svtkDataSetAttributes::SCALARS, scalars->GetName(), scalars->GetDataType(),
    scalars->GetNumberOfComponents(), scalars->GetNumberOfTuples());
  assign->Assign(
    svtkDataSetAttributes::SCALARS, svtkDataSetAttributes::VECTORS, svtkAssignAttribute::POINT_DATA);
  assign->UpdateInformation();
  outInfo = assign->GetExecutive()->GetOutputInformation()->GetInformationObject(0);
  outFieldInfo = svtkDataObject::GetActiveFieldInformation(
    outInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);
  if (!outFieldInfo || !outFieldInfo->Has(svtkDataObject::FIELD_NAME()) ||
    std::strcmp(outFieldInfo->Get(svtkDataObject::FIELD_NAME()), scalars->GetName()) ||
    outFieldInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()) !=
      scalars->GetNumberOfComponents() ||
    outFieldInfo->Get(svtkDataObject::FIELD_NUMBER_OF_TUPLES()) != scalars->GetNumberOfTuples() ||
    outFieldInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE()) != scalars->GetDataType())
  {
    cerr << "Scalar information not passed when attribute is assigned by type." << endl;
    ++errors;
  }
  outInfo->Clear();
  assign->SetInputData(graph);
  tensors->SetName("tensors");
  inInfo = assign->GetExecutive()->GetInputInformation()[0]->GetInformationObject(0);
  svtkDataObject::SetActiveAttribute(inInfo, svtkDataObject::FIELD_ASSOCIATION_EDGES,
    tensors->GetName(), svtkDataSetAttributes::TENSORS);
  svtkDataObject::SetActiveAttributeInfo(inInfo, svtkDataObject::FIELD_ASSOCIATION_EDGES,
    svtkDataSetAttributes::TENSORS, tensors->GetName(), tensors->GetDataType(),
    tensors->GetNumberOfComponents(), tensors->GetNumberOfTuples());
  assign->Assign(tensors->GetName(), svtkDataSetAttributes::SCALARS, svtkAssignAttribute::EDGE_DATA);
  assign->UpdateInformation();
  outInfo = assign->GetExecutive()->GetOutputInformation()->GetInformationObject(0);
  outFieldInfo = svtkDataObject::GetActiveFieldInformation(
    outInfo, svtkDataObject::FIELD_ASSOCIATION_EDGES, svtkDataSetAttributes::SCALARS);
  if (!outFieldInfo || !outFieldInfo->Has(svtkDataObject::FIELD_NAME()) ||
    std::strcmp(outFieldInfo->Get(svtkDataObject::FIELD_NAME()), tensors->GetName()) ||
    outFieldInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()) !=
      tensors->GetNumberOfComponents() ||
    outFieldInfo->Get(svtkDataObject::FIELD_NUMBER_OF_TUPLES()) != tensors->GetNumberOfTuples() ||
    outFieldInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE()) != tensors->GetDataType())
  {
    cerr << "Tensor information not passed when attribute is assigned by name." << endl;
    ++errors;
  }
  outInfo->Clear();
  inInfo = assign->GetExecutive()->GetInputInformation()[0]->GetInformationObject(0);
  svtkDataObject::SetActiveAttribute(inInfo, svtkDataObject::FIELD_ASSOCIATION_EDGES,
    tensors->GetName(), svtkDataSetAttributes::TENSORS);
  svtkDataObject::SetActiveAttributeInfo(inInfo, svtkDataObject::FIELD_ASSOCIATION_EDGES,
    svtkDataSetAttributes::TENSORS, tensors->GetName(), tensors->GetDataType(),
    tensors->GetNumberOfComponents(), tensors->GetNumberOfTuples());
  assign->Assign(
    svtkDataSetAttributes::TENSORS, svtkDataSetAttributes::SCALARS, svtkAssignAttribute::EDGE_DATA);
  assign->UpdateInformation();
  outInfo = assign->GetExecutive()->GetOutputInformation()->GetInformationObject(0);
  outFieldInfo = svtkDataObject::GetActiveFieldInformation(
    outInfo, svtkDataObject::FIELD_ASSOCIATION_EDGES, svtkDataSetAttributes::SCALARS);
  if (!outFieldInfo || !outFieldInfo->Has(svtkDataObject::FIELD_NAME()) ||
    std::strcmp(outFieldInfo->Get(svtkDataObject::FIELD_NAME()), tensors->GetName()) ||
    outFieldInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()) !=
      tensors->GetNumberOfComponents() ||
    outFieldInfo->Get(svtkDataObject::FIELD_NUMBER_OF_TUPLES()) != tensors->GetNumberOfTuples() ||
    outFieldInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE()) != tensors->GetDataType())
  {
    cerr << "Tensor information not passed when attribute is assigned by type." << endl;
    ++errors;
  }
  return 0;
}
