/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTableToGraph.cxx

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

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkBitArray.h"
#include "svtkCircularLayoutStrategy.h"
#include "svtkDataObjectToTable.h"
#include "svtkDataRepresentation.h"
#include "svtkDelimitedTextReader.h"
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkGraph.h"
#include "svtkGraphLayout.h"
#include "svtkGraphToPolyData.h"
#include "svtkIntArray.h"
#include "svtkLabeledDataMapper.h"
#include "svtkMergeTables.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSimple2DLayoutStrategy.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkStringToCategory.h"
#include "svtkTable.h"
#include "svtkTableToGraph.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"
#include "svtkTransform.h"
#include "svtkUndirectedGraph.h"
#include "svtkVariant.h"

// Uncomment the following line to show Qt tables of
// the vertex and edge tables.
//#define SHOW_QT_DATA_TABLES 1

#if SHOW_QT_DATA_TABLES
#include "svtkQtTableView.h"
#include <QApplication>
#endif

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

void TestTableToGraphRender(svtkRenderer* ren, svtkGraphAlgorithm* alg, int test, int cols,
  const char* labelArray, bool circular)
{
  double distance = circular ? 2.5 : 100.0;
  double xoffset = (test % cols) * distance;
  double yoffset = -(test / cols) * distance;

  SVTK_CREATE(svtkStringToCategory, cat);
  cat->SetInputConnection(alg->GetOutputPort());
  cat->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, "domain");

  cat->Update();
  svtkUndirectedGraph* output = svtkUndirectedGraph::SafeDownCast(cat->GetOutput());
  SVTK_CREATE(svtkUndirectedGraph, graph);
  graph->DeepCopy(output);

  SVTK_CREATE(svtkGraphLayout, layout);
  layout->SetInputData(graph);
  if (circular)
  {
    SVTK_CREATE(svtkCircularLayoutStrategy, strategy);
    layout->SetLayoutStrategy(strategy);
  }
  else
  {
    SVTK_CREATE(svtkSimple2DLayoutStrategy, strategy);
    strategy->SetMaxNumberOfIterations(10);
    layout->SetLayoutStrategy(strategy);
  }

  SVTK_CREATE(svtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(layout->GetOutputPort());

  SVTK_CREATE(svtkGlyphSource2D, glyph);
  glyph->SetGlyphTypeToVertex();
  SVTK_CREATE(svtkGlyph3D, vertexGlyph);
  vertexGlyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  vertexGlyph->SetInputConnection(1, glyph->GetOutputPort());
  SVTK_CREATE(svtkPolyDataMapper, vertexMapper);
  vertexMapper->SetInputConnection(vertexGlyph->GetOutputPort());
  vertexMapper->SetScalarModeToUsePointFieldData();
  vertexMapper->SelectColorArray("category");
  double rng[2] = { 0, 0 };
  graph->GetVertexData()->GetArray("category")->GetRange(rng);
  cerr << rng[0] << "," << rng[1] << endl;
  vertexMapper->SetScalarRange(rng[0], rng[1]);
  SVTK_CREATE(svtkActor, vertexActor);
  vertexActor->SetMapper(vertexMapper);
  vertexActor->GetProperty()->SetPointSize(7.0);
  vertexActor->GetProperty()->SetColor(0.7, 0.7, 0.7);
  vertexActor->SetPosition(xoffset, yoffset, 0.001);

  SVTK_CREATE(svtkPolyDataMapper, edgeMapper);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  edgeMapper->ScalarVisibilityOff();
  SVTK_CREATE(svtkActor, edgeActor);
  edgeActor->SetMapper(edgeMapper);
  edgeActor->GetProperty()->SetColor(0.6, 0.6, 0.6);
  edgeActor->SetPosition(xoffset, yoffset, 0);

  if (labelArray)
  {
    SVTK_CREATE(svtkLabeledDataMapper, labelMapper);
    labelMapper->SetInputConnection(graphToPoly->GetOutputPort());
    labelMapper->SetLabelModeToLabelFieldData();
    labelMapper->SetFieldDataName(labelArray);
    labelMapper->GetLabelTextProperty()->SetColor(0, 0, 0);
    labelMapper->GetLabelTextProperty()->SetShadow(0);
    SVTK_CREATE(svtkTransform, translate);
    translate->Translate(xoffset, yoffset, 0);
    labelMapper->SetTransform(translate);
    SVTK_CREATE(svtkActor2D, labelActor);
    labelActor->SetMapper(labelMapper);
    ren->AddActor(labelActor);
  }

  ren->AddActor(vertexActor);
  ren->AddActor(edgeActor);
}

int TestTableToGraph(int argc, char* argv[])
{
#if SHOW_QT_DATA_TABLES
  QApplication app(argc, argv);
#endif

  const char* label = 0;
  bool circular = true;
  for (int a = 1; a < argc; a++)
  {
    if (!strcmp(argv[a], "-L"))
    {
      label = "label";
    }
    if (!strcmp(argv[a], "-F"))
    {
      circular = false;
    }
  }

  // Read edge table from a file.
  char* file =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/authors-tabletographtest.csv");

  SVTK_CREATE(svtkDelimitedTextReader, reader);
  reader->SetFileName(file);
  delete[] file;
  reader->SetHaveHeaders(true);

  // Create a simple person table.
  SVTK_CREATE(svtkTable, personTable);
  SVTK_CREATE(svtkStringArray, nameArr);
  nameArr->SetName("name");
  SVTK_CREATE(svtkStringArray, petArr);
  petArr->SetName("pet");
  nameArr->InsertNextValue("Biff");
  petArr->InsertNextValue("cat");
  nameArr->InsertNextValue("Bob");
  petArr->InsertNextValue("bird");
  nameArr->InsertNextValue("Baz");
  petArr->InsertNextValue("dog");
  nameArr->InsertNextValue("Bippity");
  petArr->InsertNextValue("lizard");
  nameArr->InsertNextValue("Boppity");
  petArr->InsertNextValue("chinchilla");
  nameArr->InsertNextValue("Boo");
  petArr->InsertNextValue("rabbit");
  personTable->AddColumn(nameArr);
  personTable->AddColumn(petArr);

  // Insert rows for organizations
  SVTK_CREATE(svtkTable, orgTable);
  SVTK_CREATE(svtkStringArray, orgNameArr);
  orgNameArr->SetName("name");
  SVTK_CREATE(svtkIntArray, sizeArr);
  sizeArr->SetName("size");
  orgNameArr->InsertNextValue("NASA");
  sizeArr->InsertNextValue(10000);
  orgNameArr->InsertNextValue("Bob's Supermarket");
  sizeArr->InsertNextValue(100);
  orgNameArr->InsertNextValue("Oil Changes 'R' Us");
  sizeArr->InsertNextValue(20);
  orgTable->AddColumn(orgNameArr);
  orgTable->AddColumn(sizeArr);

  // Merge the two tables
  SVTK_CREATE(svtkMergeTables, merge);
  merge->SetInputData(0, personTable);
  merge->SetFirstTablePrefix("person.");
  merge->SetInputData(1, orgTable);
  merge->SetSecondTablePrefix("organization.");
  merge->MergeColumnsByNameOff();
  merge->PrefixAllButMergedOn();

  // Create the renderer.
  SVTK_CREATE(svtkRenderer, ren);

  // Create table to graph filter with edge and vertex table inputs
  SVTK_CREATE(svtkTableToGraph, tableToGraph);
  tableToGraph->SetInputConnection(0, reader->GetOutputPort());

  int cols = 3;
  int test = 0;

  // Path
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Boss", "person");
  tableToGraph->AddLinkVertex("Affiliation", "organization");
  tableToGraph->AddLinkVertex("Alma Mater", "school");
  tableToGraph->AddLinkVertex("Categories", "interest");
  tableToGraph->AddLinkEdge("Author", "Boss");
  tableToGraph->AddLinkEdge("Boss", "Affiliation");
  tableToGraph->AddLinkEdge("Affiliation", "Alma Mater");
  tableToGraph->AddLinkEdge("Alma Mater", "Categories");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Star
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Boss", "person");
  tableToGraph->AddLinkVertex("Affiliation", "organization");
  tableToGraph->AddLinkVertex("Alma Mater", "school");
  tableToGraph->AddLinkVertex("Categories", "interest");
  tableToGraph->AddLinkEdge("Author", "Boss");
  tableToGraph->AddLinkEdge("Author", "Affiliation");
  tableToGraph->AddLinkEdge("Author", "Alma Mater");
  tableToGraph->AddLinkEdge("Author", "Categories");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Affiliation
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Affiliation", "organization");
  tableToGraph->AddLinkEdge("Author", "Affiliation");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Group by affiliation (hide affiliation)
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person", 0);
  tableToGraph->AddLinkVertex("Affiliation", "organization", 1);
  tableToGraph->AddLinkEdge("Author", "Affiliation");
  tableToGraph->AddLinkEdge("Affiliation", "Author");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Boss
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Boss", "person");
  tableToGraph->AddLinkEdge("Author", "Boss");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Boss in different domain
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Boss", "boss");
  tableToGraph->AddLinkEdge("Author", "Boss");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Use simple linking of column path
  tableToGraph->ClearLinkVertices();
  SVTK_CREATE(svtkStringArray, pathColumn);
  SVTK_CREATE(svtkStringArray, pathDomain);
  SVTK_CREATE(svtkBitArray, pathHidden);
  pathColumn->InsertNextValue("Author");
  pathHidden->InsertNextValue(0);
  pathColumn->InsertNextValue("Boss");
  pathHidden->InsertNextValue(0);
  pathColumn->InsertNextValue("Affiliation");
  pathHidden->InsertNextValue(0);
  pathColumn->InsertNextValue("Alma Mater");
  pathHidden->InsertNextValue(0);
  pathColumn->InsertNextValue("Categories");
  pathHidden->InsertNextValue(0);
  // Set domains to equal column names, except put Author and Boss
  // in the same domain.
  pathDomain->DeepCopy(pathColumn);
  pathDomain->SetValue(0, "person");
  pathDomain->SetValue(1, "person");
  tableToGraph->LinkColumnPath(pathColumn, pathDomain, pathHidden);
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Use a vertex table.
  tableToGraph->SetInputConnection(1, merge->GetOutputPort());
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person.name", 0);
  tableToGraph->AddLinkVertex("Affiliation", "organization.name", 0);
  tableToGraph->AddLinkEdge("Author", "Affiliation");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  SVTK_CREATE(svtkRenderWindow, win);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  win->AddRenderer(ren);
  ren->SetBackground(1, 1, 1);

  //  SVTK_CREATE(svtkGraphLayoutView, view);
  //  view->SetupRenderWindow(win);
  //  view->SetRepresentationFromInputConnection(tableToGraph->GetOutputPort());
  //  view->SetVertexLabelArrayName("label");
  //  view->VertexLabelVisibilityOn();
  //  view->SetLayoutStrategyToCircular();
  //  view->Update();
  //  view->GetRenderer()->ResetCamera();
  //  view->Update();

#if SHOW_QT_DATA_TABLES
  SVTK_CREATE(svtkQtTableView, mergeView);
  mergeView->SetRepresentationFromInputConnection(merge->GetOutputPort());
  mergeView->GetWidget()->show();

  SVTK_CREATE(svtkDataObjectToTable, vertToTable);
  vertToTable->SetInputConnection(tableToGraph->GetOutputPort());
  vertToTable->SetFieldType(svtkDataObjectToTable::POINT_DATA);
  SVTK_CREATE(svtkQtTableView, vertView);
  vertView->SetRepresentationFromInputConnection(vertToTable->GetOutputPort());
  vertView->GetWidget()->show();
  vertView->Update();

  SVTK_CREATE(svtkDataObjectToTable, edgeToTable);
  edgeToTable->SetInputConnection(tableToGraph->GetOutputPort());
  edgeToTable->SetFieldType(svtkDataObjectToTable::CELL_DATA);
  SVTK_CREATE(svtkQtTableView, edgeView);
  edgeView->SetRepresentationFromInputConnection(edgeToTable->GetOutputPort());
  edgeView->GetWidget()->show();
#endif

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
#if SHOW_QT_DATA_TABLES
    QApplication::exec();
#else
    iren->Initialize();
    iren->Start();
#endif

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
