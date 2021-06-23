/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtTreeRingLabeler.cxx

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
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"
#include "svtkTreeRingView.h"
#include "svtkViewTheme.h"
#include "svtkXMLTreeReader.h"

#include <QApplication>
#include <QFontDatabase>

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()
using std::string;

int TestQtTreeRingLabeler(int argc, char* argv[])
{
  SVTK_CREATE(svtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string treeFileName = dataRoot + "/Data/Infovis/XML/svtklibrary.xml";

  SVTK_CREATE(svtkXMLTreeReader, reader);
  reader->SetFileName(treeFileName.c_str());
  reader->SetEdgePedigreeIdArrayName("graph edge");
  reader->GenerateVertexPedigreeIdsOff();
  reader->SetVertexPedigreeIdArrayName("id");

  reader->Update();

  QApplication app(argc, argv);

  QString fontFileName = testHelper->GetDataRoot();
  fontFileName.append("/Data/Infovis/martyb_-_Ridiculous.ttf");
  QFontDatabase::addApplicationFont(fontFileName);

  SVTK_CREATE(svtkTreeRingView, view);
  view->SetTreeFromInputConnection(reader->GetOutputPort());
  view->Update();
  view->SetLabelRenderModeToQt();
  view->SetAreaColorArrayName("VertexDegree");
  view->SetEdgeColorToSplineFraction();
  view->SetColorEdges(true);
  view->SetAreaLabelArrayName("id");
  view->SetAreaHoverArrayName("id");
  view->SetAreaLabelVisibility(true);
  view->SetAreaSizeArrayName("VertexDegree");

  // Apply a theme to the views
  svtkViewTheme* const theme = svtkViewTheme::CreateMellowTheme();
  //  theme->GetPointTextProperty()->SetColor(0, 0, 0);
  theme->GetPointTextProperty()->SetFontFamilyAsString("Ridiculous");
  theme->GetPointTextProperty()->BoldOn();
  theme->GetPointTextProperty()->SetFontSize(16);
  theme->GetPointTextProperty()->ShadowOn();
  view->ApplyViewTheme(theme);
  theme->Delete();

  view->GetRenderWindow()->SetSize(600, 600);
  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->ResetCamera();
  view->Render();

  // using image-test threshold of 200 since this test tends to render slightly
  // differently on different platforms.
  int retVal = svtkRegressionTestImageThreshold(view->GetRenderWindow(), 200);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  QFontDatabase::removeAllApplicationFonts();

  return !retVal;
}
