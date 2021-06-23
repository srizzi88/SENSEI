/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeRingView.cxx

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

#include "svtkDataRepresentation.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedTreeAreaRepresentation.h"
#include "svtkRenderer.h"
#include "svtkSplineGraphEdges.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"
#include "svtkTreeRingView.h"
#include "svtkViewTheme.h"
#include "svtkXMLTreeReader.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()
using std::string;

int TestTreeRingView(int argc, char* argv[])
{
  SVTK_CREATE(svtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string treeFileName = dataRoot + "/Data/Infovis/XML/svtkclasses.xml";
  string graphFileName = dataRoot + "/Data/Infovis/XML/svtklibrary.xml";

  // We need to put the graph and tree edges in different domains.
  SVTK_CREATE(svtkXMLTreeReader, reader1);
  reader1->SetFileName(treeFileName.c_str());
  reader1->SetEdgePedigreeIdArrayName("graph edge");
  reader1->GenerateVertexPedigreeIdsOff();
  reader1->SetVertexPedigreeIdArrayName("id");

  SVTK_CREATE(svtkXMLTreeReader, reader2);
  reader2->SetFileName(graphFileName.c_str());
  reader2->SetEdgePedigreeIdArrayName("tree edge");
  reader2->GenerateVertexPedigreeIdsOff();
  reader2->SetVertexPedigreeIdArrayName("id");

  reader1->Update();
  reader2->Update();

  SVTK_CREATE(svtkTreeRingView, view);
  view->DisplayHoverTextOn();
  view->SetTreeFromInputConnection(reader2->GetOutputPort());
  view->SetGraphFromInputConnection(reader1->GetOutputPort());
  view->Update();

  view->SetAreaColorArrayName("VertexDegree");

  // Uncomment for edge colors
  // view->SetEdgeColorArrayName("graph edge");
  // view->SetColorEdges(true);

  // Uncomment for edge labels
  // view->SetEdgeLabelArrayName("graph edge");
  // view->SetEdgeLabelVisibility(true);

  view->SetAreaLabelArrayName("id");
  view->SetAreaLabelVisibility(true);
  view->SetAreaHoverArrayName("id");
  view->SetAreaSizeArrayName("VertexDegree");
  svtkRenderedTreeAreaRepresentation::SafeDownCast(view->GetRepresentation())
    ->SetGraphHoverArrayName("graph edge");
  svtkRenderedTreeAreaRepresentation::SafeDownCast(view->GetRepresentation())
    ->SetGraphSplineType(svtkSplineGraphEdges::CUSTOM, 0);

  // Apply a theme to the views
  svtkViewTheme* const theme = svtkViewTheme::CreateMellowTheme();
  theme->SetLineWidth(1);
  theme->GetPointTextProperty()->ShadowOn();
  view->ApplyViewTheme(theme);
  theme->Delete();

  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->ResetCamera();
  view->Render();

  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
