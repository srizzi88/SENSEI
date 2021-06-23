/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSpanTreeLayoutStrategy.cxx

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

#include "svtkGraphLayoutView.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSpanTreeLayoutStrategy.h"
#include "svtkTestUtilities.h"
#include "svtkXGMLReader.h"

using std::string;

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestSpanTreeLayoutStrategy(int argc, char* argv[])
{
  SVTK_CREATE(svtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string file = dataRoot + "/Data/Infovis/fsm.gml";

  SVTK_CREATE(svtkXGMLReader, reader);
  reader->SetFileName(file.c_str());
  reader->Update();

  // Graph layout view
  SVTK_CREATE(svtkGraphLayoutView, view);
  view->DisplayHoverTextOff();
  view->SetLayoutStrategyToSpanTree();
  view->SetVertexLabelArrayName("vertex id");
  view->VertexLabelVisibilityOn();
  view->SetVertexColorArrayName("vertex id");
  view->SetColorVertices(true);
  view->SetRepresentationFromInputConnection(reader->GetOutputPort());

  view->ResetCamera();
  view->GetRenderWindow()->SetSize(600, 600);
  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->SetInteractionModeTo3D();
  view->SetLabelPlacementModeToNoOverlap();

  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
