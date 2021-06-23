/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHierarchicalGraphView.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCosmicTreeLayoutStrategy.h"
#include "svtkDataRepresentation.h"
#include "svtkHierarchicalGraphView.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedHierarchyRepresentation.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSplineGraphEdges.h"
#include "svtkTestUtilities.h"
#include "svtkViewTheme.h"
#include "svtkXMLTreeReader.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

using std::string;

int TestHierarchicalGraphView(int argc, char* argv[])
{
  SVTK_CREATE(svtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string treeFileName = dataRoot + "/Data/Infovis/XML/svtklibrary.xml";
  string graphFileName = dataRoot + "/Data/Infovis/XML/svtkclasses.xml";

  // We need to put the graph and tree edges in different domains.
  SVTK_CREATE(svtkXMLTreeReader, reader1);
  reader1->SetFileName(treeFileName.c_str());
  reader1->SetEdgePedigreeIdArrayName("tree edge");
  reader1->GenerateVertexPedigreeIdsOff();
  reader1->SetVertexPedigreeIdArrayName("id");

  SVTK_CREATE(svtkXMLTreeReader, reader2);
  reader2->SetFileName(graphFileName.c_str());
  reader2->SetEdgePedigreeIdArrayName("graph edge");
  reader2->GenerateVertexPedigreeIdsOff();
  reader2->SetVertexPedigreeIdArrayName("id");

  reader1->Update();
  reader2->Update();

  SVTK_CREATE(svtkHierarchicalGraphView, view);
  view->DisplayHoverTextOff();
  view->GetRenderWindow()->SetMultiSamples(0);
  view->SetHierarchyFromInputConnection(reader1->GetOutputPort());
  view->SetGraphFromInputConnection(reader2->GetOutputPort());
  view->SetVertexColorArrayName("VertexDegree");
  view->SetColorVertices(true);
  view->SetVertexLabelArrayName("id");
  view->SetVertexLabelVisibility(true);
  view->SetScalingArrayName("TreeRadius");

  view->Update(); // Needed for now
  view->SetGraphEdgeColorArrayName("graph edge");
  view->SetColorGraphEdgesByArray(true);
  svtkRenderedHierarchyRepresentation::SafeDownCast(view->GetRepresentation())
    ->SetGraphSplineType(svtkSplineGraphEdges::CUSTOM, 0);

  SVTK_CREATE(svtkCosmicTreeLayoutStrategy, ct);
  ct->SetNodeSizeArrayName("VertexDegree");
  ct->SetSizeLeafNodesOnly(true);
  view->SetLayoutStrategy(ct);

  // Apply a theme to the views
  svtkViewTheme* const theme = svtkViewTheme::CreateMellowTheme();
  theme->SetLineWidth(1);
  view->ApplyViewTheme(theme);
  theme->Delete();

  view->ResetCamera();

  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
