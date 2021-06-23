/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIcicleView.cxx

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
#include "svtkIcicleView.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringToNumeric.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"
#include "svtkViewTheme.h"
#include "svtkXMLTreeReader.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()
using std::string;

int TestIcicleView(int argc, char* argv[])
{
  SVTK_CREATE(svtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string treeFileName = dataRoot + "/Data/Infovis/XML/smalltest.xml";

  // We need to put the graph and tree edges in different domains.
  SVTK_CREATE(svtkXMLTreeReader, reader);
  reader->SetFileName(treeFileName.c_str());

  SVTK_CREATE(svtkStringToNumeric, numeric);
  numeric->SetInputConnection(reader->GetOutputPort());

  SVTK_CREATE(svtkIcicleView, view);
  view->DisplayHoverTextOff();
  view->SetTreeFromInputConnection(numeric->GetOutputPort());

  view->SetAreaColorArrayName("size");
  view->ColorAreasOn();
  view->SetAreaLabelArrayName("label");
  view->AreaLabelVisibilityOn();
  view->SetAreaHoverArrayName("label");
  view->SetAreaSizeArrayName("size");

  // Apply a theme to the views
  svtkViewTheme* const theme = svtkViewTheme::CreateMellowTheme();
  theme->GetPointTextProperty()->ShadowOn();
  view->ApplyViewTheme(theme);
  theme->Delete();

  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
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
