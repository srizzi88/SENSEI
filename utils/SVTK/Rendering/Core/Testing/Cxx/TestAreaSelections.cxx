/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAreaSelections.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests svtkHardwareSelector, svtkExtractSelectedFrustum,
// svtkRenderedAreaPicker, and svtkInteractorStyleRubberBandPick.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkDataSetMapper.h"
#include "svtkExtractSelectedFrustum.h"
#include "svtkExtractSelectedPolyDataIds.h"
#include "svtkHardwareSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkImageActor.h"
#include "svtkImageMandelbrotSource.h"
#include "svtkInteractorStyleRubberBandPick.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedAreaPicker.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

#include "svtkDataSetReader.h"

static svtkSmartPointer<svtkRenderer> renderer;
static svtkSmartPointer<svtkSphereSource> SS1;
static svtkSmartPointer<svtkDataSetMapper> sMap;
static svtkSmartPointer<svtkPolyData> emptyPD;

#define MY_CREATE_NEW(class, variable)                                                             \
  svtkSmartPointer<class> variable = svtkSmartPointer<class>::New();

static void EndPick(svtkObject* svtkNotUsed(caller), unsigned long svtkNotUsed(eventId), void*, void*)
{
  MY_CREATE_NEW(svtkHardwareSelector, sel);
  sel->SetRenderer(renderer);

  double x0 = renderer->GetPickX1();
  double y0 = renderer->GetPickY1();
  double x1 = renderer->GetPickX2();
  double y1 = renderer->GetPickY2();

  sel->SetArea(
    static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1));
  svtkSmartPointer<svtkSelection> res;
  res.TakeReference(sel->Select());
  if (!res)
  {
    cerr << "Selection not supported." << endl;
    return;
  }

  /*
  cerr << "x0 " << x0 << " y0 " << y0 << "\t";
  cerr << "x1 " << x1 << " y1 " << y1 << endl;
  svtkIdTypeArray *a = svtkIdTypeArray::New();
  sel->GetSelectedIds(a);
  cerr << "numhits = " << a->GetNumberOfTuples() << endl;
  sel->PrintSelectedIds(a);
  a->Delete();
  */

  svtkSelectionNode* cellids = res->GetNode(0);
  MY_CREATE_NEW(svtkExtractSelectedPolyDataIds, extr);
  if (cellids)
  {
    extr->SetInputConnection(0, SS1->GetOutputPort());
    svtkSmartPointer<svtkSelection> temp = svtkSmartPointer<svtkSelection>::New();
    temp->AddNode(cellids);
    extr->SetInputData(1, temp);
    extr->Update();
    sMap->SetInputConnection(extr->GetOutputPort());
  }
  else
  {
    cerr << "Empty color buffer selection -" << endl;
    cerr << "Check display color depth. Must be at least 24 bit." << endl;
    sMap->SetInputData(emptyPD);
  }
}

int TestAreaSelections(int argc, char* argv[])
{
  // Standard rendering classes
  renderer = svtkSmartPointer<svtkRenderer>::New();
  MY_CREATE_NEW(svtkRenderWindow, renWin);
  renWin->AddRenderer(renderer);
  MY_CREATE_NEW(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->GetActiveCamera()->SetPosition(1.5, -0.75, 7);
  renderer->GetActiveCamera()->SetFocalPoint(1.5, -0.75, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(300, 300);

  // use the rubber band pick interactor style
  svtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  MY_CREATE_NEW(svtkInteractorStyleRubberBandPick, rbp);
  rwi->SetInteractorStyle(rbp);

  MY_CREATE_NEW(svtkRenderedAreaPicker, areaPicker);
  rwi->SetPicker(areaPicker);

  ////////////////////////////////////////////////////////////
  // Create a unstructured grid data source to test FrustumExtractor with.
  MY_CREATE_NEW(svtkDataSetReader, reader);
  char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SampleStructGrid.svtk");
  reader->SetFileName(cfname);
  delete[] cfname;

  MY_CREATE_NEW(svtkDataSetMapper, map1);
  map1->SetInputConnection(reader->GetOutputPort());

  MY_CREATE_NEW(svtkActor, act1);
  act1->SetMapper(map1);
  act1->PickableOff(); // prevents the visible cell selector from trying
  renderer->AddActor(act1);

  // frustum extractor works on geometry and doesn't care about pickability
  MY_CREATE_NEW(svtkExtractSelectedFrustum, extractor);
  extractor->SetInputConnection(reader->GetOutputPort());
  extractor->PreserveTopologyOff();
  extractor->SetFrustum(areaPicker->GetFrustum());

  MY_CREATE_NEW(svtkDataSetMapper, eMap);
  eMap->SetInputConnection(extractor->GetOutputPort());

  MY_CREATE_NEW(svtkActor, eAct);
  eAct->SetPosition(2, 0, 0);
  eAct->SetMapper(eMap);
  eAct->PickableOff();
  renderer->AddActor(eAct);

  ////////////////////////////////////////////////////////////
  emptyPD = svtkSmartPointer<svtkPolyData>::New();

  int res = 20;
  SS1 = svtkSmartPointer<svtkSphereSource>::New();
  SS1->SetThetaResolution(res);
  SS1->SetPhiResolution(res);
  SS1->SetRadius(0.5);
  SS1->SetCenter(0.5, -1.5, 0);
  MY_CREATE_NEW(svtkPolyDataMapper, map2);
  map2->SetInputConnection(SS1->GetOutputPort());

  MY_CREATE_NEW(svtkActor, act2);
  act2->SetMapper(map2);
  act2->PickableOn(); // lets the HardwareSelector select in it
  act2->GetProperty()->SetColor(0.2, 0.1, 0.5);
  act2->GetProperty()->SetOpacity(0.6);
  renderer->AddActor(act2);

  sMap = svtkSmartPointer<svtkDataSetMapper>::New();
  sMap->SetInputConnection(SS1->GetOutputPort());

  MY_CREATE_NEW(svtkActor, sAct);
  sAct->SetMapper(sMap);
  sAct->SetPosition(2, 0, 0);
  sAct->PickableOff();
  renderer->AddActor(sAct);

  // pass pick events to the HardwareSelector
  MY_CREATE_NEW(svtkCallbackCommand, cbc);
  cbc->SetCallback(EndPick);
  cbc->SetClientData(renderer);
  rwi->AddObserver(svtkCommand::EndPickEvent, cbc);

  ////////////////////////////////////////////////////////////

  // run the test

  renWin->Render();
  int rgba[4];
  renWin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
  {
    cout << "Color buffer depth must be at least 8 bit. Currently: " << rgba[0] << ", " << rgba[1]
         << ", " << rgba[2] << endl;
    return 0;
  }

  areaPicker->AreaPick(51, 78, 82, 273, renderer);
  EndPick(nullptr, 0, nullptr, nullptr);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  renderer = nullptr;
  SS1 = nullptr;
  sMap = nullptr;
  emptyPD = nullptr;

  // Cleanup
  return !retVal;
}
