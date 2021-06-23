/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNetworkViews.cxx

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

#include "svtkNetworkHierarchy.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSQLDatabaseTableSource.h"
#include "svtkTableToGraph.h"
#include "svtkTextProperty.h"
#include "svtkTreeRingView.h"
#include "svtkVertexDegree.h"
#include "svtkViewTheme.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

using std::string;

int TestNetworkViews(int argc, char* argv[])
{
  SVTK_CREATE(svtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string file = dataRoot + "/Data/Infovis/SQLite/ports_protocols.db";

  // Pull the table (that represents relationships/edges) from the database
  SVTK_CREATE(svtkSQLDatabaseTableSource, databaseToEdgeTable);
  databaseToEdgeTable->SetURL("sqlite://" + file);
  databaseToEdgeTable->SetQuery("select src, dst, dport, protocol, port_protocol from tcp");

  // Pull the table (that represents entities/vertices) from the database
  SVTK_CREATE(svtkSQLDatabaseTableSource, databaseToVertexTable);
  databaseToVertexTable->SetURL("sqlite://" + file);
  databaseToVertexTable->SetQuery("select ip, hostname from dnsnames");

  // Make a graph
  SVTK_CREATE(svtkTableToGraph, graph);
  graph->AddInputConnection(0, databaseToEdgeTable->GetOutputPort());
  graph->AddInputConnection(1, databaseToVertexTable->GetOutputPort());
  graph->AddLinkVertex("src", "ip", false);
  graph->AddLinkVertex("dst", "ip", false);
  graph->AddLinkEdge("src", "dst");

  // Make a tree out of ip addresses
  SVTK_CREATE(svtkNetworkHierarchy, ip_tree);
  ip_tree->AddInputConnection(graph->GetOutputPort());

  SVTK_CREATE(svtkTreeRingView, dummy);

  // Create a view on city/region/country
  SVTK_CREATE(svtkTreeRingView, view1);
  view1->DisplayHoverTextOff();
  view1->SetTreeFromInputConnection(ip_tree->GetOutputPort());
  view1->SetGraphFromInputConnection(graph->GetOutputPort());
  view1->Update();
  view1->SetLabelPriorityArrayName("VertexDegree");
  view1->SetAreaColorArrayName("VertexDegree");
  view1->SetColorAreas(true);
  view1->SetAreaLabelArrayName("ip");
  view1->SetAreaHoverArrayName("ip");
  view1->SetAreaLabelVisibility(true);
  view1->SetEdgeColorArrayName("dport");
  view1->SetColorEdges(true);
  view1->SetInteriorLogSpacingValue(5.);
  view1->SetBundlingStrength(.5);

  // Apply a theme to the views
  svtkViewTheme* const theme = svtkViewTheme::CreateMellowTheme();
  theme->GetPointTextProperty()->ShadowOn();
  view1->ApplyViewTheme(theme);
  theme->Delete();

  view1->GetRenderWindow()->SetMultiSamples(0);
  view1->GetRenderWindow()->SetSize(600, 600);

  view1->ResetCamera();
  view1->Render();

  int retVal = svtkRegressionTestImage(view1->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view1->GetInteractor()->Initialize();
    view1->GetInteractor()->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
