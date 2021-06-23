/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtLabelStrategy.cxx

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
#include "svtkActor2D.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkDataRepresentation.h"
#include "svtkDoubleArray.h"
#include "svtkGraphLayoutView.h"
#include "svtkIdTypeArray.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkLabelPlacementMapper.h"
#include "svtkPointData.h"
#include "svtkPointSetToLabelHierarchy.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkQtLabelRenderStrategy.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringArray.h"
#include "svtkStringToNumeric.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"
#include "svtkUnicodeString.h"
#include "svtkUnicodeStringArray.h"
#include "svtkXMLTreeReader.h"

#include <sstream>
#include <time.h>

#include <QApplication>
#include <QFontDatabase>

using std::string;

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestQtLabelStrategy(int argc, char* argv[])
{
  int n = 1000;

  SVTK_CREATE(svtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  QString fontFileName = testHelper->GetDataRoot();
  fontFileName.append("/Data/Infovis/martyb_-_Ridiculous.ttf");

  QApplication app(argc, argv);

  QFontDatabase::addApplicationFont(fontFileName);

  SVTK_CREATE(svtkPolyData, pd);
  SVTK_CREATE(svtkPoints, pts);
  SVTK_CREATE(svtkCellArray, verts);
  SVTK_CREATE(svtkDoubleArray, orient);
  orient->SetName("orientation");
  SVTK_CREATE(svtkStringArray, label);
  label->SetName("label");

  srand(time(nullptr));

  for (int i = 0; i < n; i++)
  {
    pts->InsertNextPoint((double)(rand() % 100), (double)(rand() % 100), (double)(rand() % 100));
    verts->InsertNextCell(1);
    verts->InsertCellPoint(i);
    orient->InsertNextValue((double)(rand() % 100) * 3.60);
    svtkStdString s;
    std::stringstream out;
    out << i;
    s = out.str();
    label->InsertNextValue(s);
  }

  pd->SetPoints(pts);
  pd->SetVerts(verts);
  pd->GetPointData()->AddArray(label);
  pd->GetPointData()->AddArray(orient);

  SVTK_CREATE(svtkPointSetToLabelHierarchy, hier);
  hier->SetInputData(pd);
  hier->SetOrientationArrayName("orientation");
  hier->SetLabelArrayName("label");
  hier->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
  //  hier->GetTextProperty()->SetFontFamilyAsString("Talvez assim");
  hier->GetTextProperty()->SetFontFamilyAsString("Ridiculous");
  //  hier->GetTextProperty()->SetFontFamilyAsString("Sketchy");
  hier->GetTextProperty()->SetFontSize(72);

  SVTK_CREATE(svtkLabelPlacementMapper, lmapper);
  lmapper->SetInputConnection(hier->GetOutputPort());
  lmapper->SetShapeToRoundedRect();
  lmapper->SetBackgroundColor(1.0, 1.0, 0.7);
  lmapper->SetBackgroundOpacity(0.8);
  lmapper->SetMargin(3);

  SVTK_CREATE(svtkQtLabelRenderStrategy, strategy);
  lmapper->SetRenderStrategy(strategy);

  SVTK_CREATE(svtkActor2D, lactor);
  lactor->SetMapper(lmapper);

  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputData(pd);
  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);

  SVTK_CREATE(svtkRenderer, ren);
  ren->AddActor(lactor);
  ren->AddActor(actor);
  ren->ResetCamera();

  SVTK_CREATE(svtkRenderWindow, win);
  win->SetSize(600, 600);
  win->AddRenderer(ren);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);

  int retVal = svtkRegressionTestImageThreshold(win, 200);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();

    retVal = svtkRegressionTester::PASSED;
  }

  return !retVal;
}
