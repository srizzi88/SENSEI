/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTulipReaderProperties.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkGraph.h"
#include "svtkIntArray.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTestUtilities.h"
#include "svtkTulipReader.h"
#include "svtkVariantArray.h"

template <typename value_t>
void TestValue(const value_t& Value, const value_t& ExpectedValue,
  const svtkStdString& ValueDescription, int& ErrorCount)
{
  if (Value == ExpectedValue)
  {
    return;
  }

  cerr << ValueDescription << " is [" << Value << "] - expected [" << ExpectedValue << "]" << endl;

  ++ErrorCount;
}

int TestTulipReaderProperties(int argc, char* argv[])
{
  char* file = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/clustered-graph.tlp");

  cerr << "file: " << file << endl;

  svtkSmartPointer<svtkTulipReader> reader = svtkSmartPointer<svtkTulipReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  svtkGraph* const graph = reader->GetOutput();

  int error_count = 0;

  // Test a sample of the node pedigree id property
  svtkVariantArray* nodePedigree =
    svtkArrayDownCast<svtkVariantArray>(graph->GetVertexData()->GetPedigreeIds());
  if (nodePedigree)
  {
    TestValue(nodePedigree->GetValue(0), svtkVariant(0), "Node 0 pedigree id property", error_count);
    TestValue(nodePedigree->GetValue(5), svtkVariant(5), "Node 5 pedigree id property", error_count);
    TestValue(
      nodePedigree->GetValue(11), svtkVariant(11), "Node 11 pedigree id property", error_count);
  }
  else
  {
    cerr << "Node pedigree id property not found." << endl;
    ++error_count;
  }

  // Test a sample of the node string property
  svtkStringArray* nodeProperty1 =
    svtkArrayDownCast<svtkStringArray>(graph->GetVertexData()->GetAbstractArray("Node Name"));
  if (nodeProperty1)
  {
    TestValue(
      nodeProperty1->GetValue(0), svtkStdString("Node A"), "Node 0 string property", error_count);
    TestValue(
      nodeProperty1->GetValue(5), svtkStdString("Node F"), "Node 5 string property", error_count);
    TestValue(
      nodeProperty1->GetValue(11), svtkStdString("Node L"), "Node 11 string property", error_count);
  }
  else
  {
    cerr << "Node string property 'Node Name' not found." << endl;
    ++error_count;
  }

  // Test a sample of the node int property
  svtkIntArray* nodeProperty2 =
    svtkArrayDownCast<svtkIntArray>(graph->GetVertexData()->GetAbstractArray("Weight"));
  if (nodeProperty2)
  {
    TestValue(nodeProperty2->GetValue(0), 100, "Node 0 int property", error_count);
    TestValue(nodeProperty2->GetValue(5), 105, "Node 5 int property", error_count);
    TestValue(nodeProperty2->GetValue(11), 111, "Node 11 int property", error_count);
  }
  else
  {
    cerr << "Node int property 'Weight' not found." << endl;
    ++error_count;
  }

  // Test a sample of the node double property
  svtkDoubleArray* nodeProperty3 = svtkArrayDownCast<svtkDoubleArray>(
    graph->GetVertexData()->GetAbstractArray("Betweenness Centrality"));
  if (nodeProperty3)
  {
    TestValue(nodeProperty3->GetValue(0), 0.0306061, "Node 0 double property", error_count);
    TestValue(nodeProperty3->GetValue(5), 0.309697, "Node 5 double property", error_count);
    TestValue(nodeProperty3->GetValue(11), 0.0306061, "Node 11 double property", error_count);
  }
  else
  {
    cerr << "Node double property 'Betweenness Centrality' not found." << endl;
    ++error_count;
  }

  // Test a sample of the edge string property
  svtkStringArray* edgeProperty1 =
    svtkArrayDownCast<svtkStringArray>(graph->GetEdgeData()->GetAbstractArray("Edge Name"));
  if (edgeProperty1)
  {
    TestValue(
      edgeProperty1->GetValue(0), svtkStdString("Edge A"), "Edge 0 string property", error_count);
    TestValue(
      edgeProperty1->GetValue(7), svtkStdString("Edge H"), "Edge 7 string property", error_count);
    TestValue(
      edgeProperty1->GetValue(16), svtkStdString("Edge Q"), "Edge 16 string property", error_count);
  }
  else
  {
    cerr << "Edge string property 'Edge Name' not found." << endl;
    ++error_count;
  }

  // Test a sample of the edge int property
  svtkIntArray* edgeProperty2 =
    svtkArrayDownCast<svtkIntArray>(graph->GetEdgeData()->GetAbstractArray("Weight"));
  if (edgeProperty2)
  {
    TestValue(edgeProperty2->GetValue(0), 100, "Edge 0 int property", error_count);
    TestValue(edgeProperty2->GetValue(7), 107, "Edge 7 int property", error_count);
    TestValue(edgeProperty2->GetValue(16), 116, "Edge 16 int property", error_count);
  }
  else
  {
    cerr << "Edge int property 'Weight' not found." << endl;
    ++error_count;
  }

  // Test a sample of the edge pedigree id property
  svtkVariantArray* edgePedigree =
    svtkArrayDownCast<svtkVariantArray>(graph->GetEdgeData()->GetPedigreeIds());
  if (edgePedigree)
  {
    TestValue(edgePedigree->GetValue(0), svtkVariant(0), "Edge 0 pedigree id property", error_count);
    TestValue(edgePedigree->GetValue(7), svtkVariant(7), "Edge 7 pedigree id property", error_count);
    TestValue(
      edgePedigree->GetValue(16), svtkVariant(16), "Edge 16 pedigree id property", error_count);
  }
  else
  {
    cerr << "Edge pedigree id property not found." << endl;
    ++error_count;
  }

  cerr << error_count << " errors" << endl;
  return error_count;
}
