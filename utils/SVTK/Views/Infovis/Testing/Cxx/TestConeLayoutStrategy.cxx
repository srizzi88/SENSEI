/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConeLayoutStrategy.cxx

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
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkConeLayoutStrategy.h"
#include "svtkDataRepresentation.h"
#include "svtkGraphLayoutView.h"
#include "svtkIdTypeArray.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringArray.h"
#include "svtkStringToNumeric.h"
#include "svtkTestUtilities.h"
#include "svtkXMLTreeReader.h"

using std::string;

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestConeLayoutStrategy(int argc, char* argv[])
{
  SVTK_CREATE(svtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string file = dataRoot + "/Data/Infovis/XML/svtkclasses.xml";

  SVTK_CREATE(svtkXMLTreeReader, reader);
  reader->SetFileName(file.c_str());
  reader->SetMaskArrays(true);
  reader->Update();
  svtkTree* t = reader->GetOutput();
  SVTK_CREATE(svtkStringArray, label);
  label->SetName("edge label");
  SVTK_CREATE(svtkIdTypeArray, dist);
  dist->SetName("distance");
  for (svtkIdType i = 0; i < t->GetNumberOfEdges(); i++)
  {
    dist->InsertNextValue(i);
    switch (i % 3)
    {
      case 0:
        label->InsertNextValue("a");
        break;
      case 1:
        label->InsertNextValue("b");
        break;
      case 2:
        label->InsertNextValue("c");
        break;
    }
  }
  t->GetEdgeData()->AddArray(dist);
  t->GetEdgeData()->AddArray(label);

  SVTK_CREATE(svtkStringToNumeric, numeric);
  numeric->SetInputData(t);

  // Graph layout view
  SVTK_CREATE(svtkGraphLayoutView, view);
  view->DisplayHoverTextOff();
  SVTK_CREATE(svtkConeLayoutStrategy, strategy);
  strategy->SetSpacing(0.3);
  view->SetLayoutStrategy(strategy);
  view->SetVertexLabelArrayName("id");
  view->VertexLabelVisibilityOn();
  view->SetEdgeColorArrayName("distance");
  view->ColorEdgesOn();
  view->SetEdgeLabelArrayName("edge label");
  view->EdgeLabelVisibilityOn();
  view->SetRepresentationFromInputConnection(numeric->GetOutputPort());

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
